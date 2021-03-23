#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>
#include <mp.h>
#include <libsec.h>
#include <flate.h>

#include "paqfs.h"

enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	OffsetSize = 4,		/* size in bytes of an offset */
};

typedef struct Fid Fid;
typedef struct Paq Paq;
typedef struct Block Block;

struct Fid
{
	short	busy;
	short	open;
	int	fid;
	char	*user;
	ulong	offset;		/* for directory reading */

	Paq	*paq;
	Fid	*next;
};

struct Paq
{	
	int ref;
	Paq *up;
	PaqDir *dir;
	Qid qid;
};

struct Block
{
	int ref;
	ulong addr;	/* block byte address */
	ulong age;
	uchar *data;
};

enum
{
	Pexec =		1,
	Pwrite = 	2,
	Pread = 	4,
	Pother = 	1,
	Pgroup = 	8,
	Powner =	64,
};

Fid	*fids;
Fcall	thdr, rhdr;
int 	blocksize;
int 	cachesize = 20;
int	mesgsize = 8*1024 + IOHDRSZ;
Paq 	*root;
Block 	*cache;
ulong 	cacheage;
Biobuf	*bin;

Fid *	newfid(int);
void	paqstat(PaqDir*, char*);
void	io(int fd);
void	*erealloc(void*, ulong);
void	*emalloc(ulong);
void 	*emallocz(ulong n);
char 	*estrdup(char*);
void	usage(void);
ulong	getl(uchar *p);
int	gets(uchar *p);
char 	*getstr(uchar *p);
PaqDir	*getDir(uchar*);
void	getHeader(uchar *p, PaqHeader *b);
void	getBlock(uchar *p, PaqBlock *b);
void	getTrailer(uchar *p, PaqTrailer *b);
void	init(char*, int);
void	paqDirFree(PaqDir*);
Qid	paqDirQid(PaqDir *d);
Paq	*paqCpy(Paq *s);
Paq	*paqLookup(Paq *s, char *name);
void	paqFree(Paq*);
Paq	*paqWalk(Paq *s, char *name);
int	perm(PaqDir *s, char *user, int p);
int	dirRead(Fid*, uchar*, int);
Block	*blockLoad(ulong addr, int type);
void	blockFree(Block*);
int	checkDirSize(uchar *p, uchar *ep);
int	packDir(PaqDir*, uchar*, int);
int	blockRead(uchar *data, ulong addr, int type);
void	readHeader(PaqHeader *hdr, char *name, DigestState *ds);
void	readBlocks(char *name, DigestState *ds);
void	readTrailer(PaqTrailer *tlr, char *name, DigestState *ds);

char	*rflush(Fid*), *rversion(Fid*),
	*rauth(Fid*), *rattach(Fid*), *rwalk(Fid*),
	*ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*);

char 	*(*fcalls[])(Fid*) = {
	[Tflush]	rflush,
	[Tversion]	rversion,
	[Tattach]	rattach,
	[Tauth]		rauth,
	[Twalk]		rwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
};

char	Eperm[] =	"permission denied";
char	Enotdir[] =	"not a directory";
char	Enoauth[] =	"no authentication required";
char	Enotexist[] =	"file does not exist";
char	Einuse[] =	"file in use";
char	Eexist[] =	"file exists";
char	Enotowner[] =	"not owner";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 	"exclusive use file already open";
char	Ename[] = 	"illegal name";
char	Erdonly[] = 	"read only file system";
char	Ebadblock[] = 	"bad block";
char	Eversion[] = 	"bad version of P9";
char	Edirtoobig[] = 	"directory entry too big";

int debug;

void
main(int argc, char *argv[])
{
	char *defmnt, *p;
	int pfd[2];
	int fd;
	int stdio = 0;
	int verify = 0;

	defmnt = "/n/paq";
	ARGBEGIN{
	case 'c':
		p = ARGF();
		if(p == nil)
			usage();
		cachesize = atoi(p);
		break;
	case 'v':
		verify = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 'i':
		defmnt = nil;
		stdio = 1;
		pfd[0] = 0;
		pfd[1] = 1;
		break;
	case 's':
		defmnt = nil;
		break;
	case 'm':
		defmnt = ARGF();
		if(defmnt == nil)
			usage();
		break;
	case 'M':
		p = ARGF();
		if(p == nil)
			usage();
		mesgsize = atoi(p);
		if(mesgsize < 512)
			mesgsize = 512;
		if(mesgsize > 128*1024)
			mesgsize = 128*1024;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	init(argv[0], verify);
	
	if(!stdio){
		if(pipe(pfd) < 0)
			sysfatal("pipe failed");
		if(defmnt == 0){
			fd = create("#s/paqfs", OWRITE, 0666);
			if(fd < 0)
				sysfatal("create of /srv/paqfs failed");
			if(fprint(fd, "%d", pfd[0]) < 0)
				sysfatal("writing /srv/paqfs");
		}
	}

	if(debug)
		fmtinstall('F', fcallfmt);
	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		sysfatal("fork");
	case 0:
		close(pfd[0]);
		io(pfd[1]);
		break;
	default:
		close(pfd[1]);	/* don't deadlock if child fails */
		if(defmnt && mount(pfd[0], -1, defmnt, MREPL|MCREATE, "") < 0)
			sysfatal("mount failed");
	}
	exits(0);
}

char*
rversion(Fid*)
{
	Fid *f;

	for(f = fids; f; f = f->next)
		if(f->busy)
			rclunk(f);
	if(thdr.msize > mesgsize)
		rhdr.msize = mesgsize;
	else
		rhdr.msize = thdr.msize;
	if(strcmp(thdr.version, "9P2000") != 0)
		return Eversion;
	rhdr.version = "9P2000";
	return 0;
}

char*
rauth(Fid*)
{
	return Enoauth;
}

char*
rflush(Fid *f)
{
	USED(f);
	return 0;
}

char*
rattach(Fid *f)
{
	/* no authentication! */
	f->busy = 1;
	f->paq = paqCpy(root);
	rhdr.qid = f->paq->qid;
	if(thdr.uname[0])
		f->user = estrdup(thdr.uname);
	else
		f->user = "none";
	return 0;
}

char*
clone(Fid *f, Fid **res)
{
	Fid *nf;

	if(f->open)
		return Eisopen;
	if(f->busy == 0)
		return Enotexist;
	nf = newfid(thdr.newfid);
	nf->busy = 1;
	nf->open = 0;
	nf->paq = paqCpy(f->paq);
	nf->user = strdup(f->user);
	*res = nf;
	return 0;
}

char*
rwalk(Fid *f)
{
	Paq *paq, *npaq;
	Fid *nf;
	int nqid, nwname;
	Qid qid;
	char *err;

	if(f->busy == 0)
		return Enotexist;
	nf = nil;
	if(thdr.fid != thdr.newfid){
		err = clone(f, &nf);
		if(err)
			return err;
		f = nf;	/* walk the new fid */
	}

	nwname = thdr.nwname;

	/* easy case */
	if(nwname == 0) {
		rhdr.nwqid = 0;
		return 0;
	}

	paq = paqCpy(f->paq);
	qid = paq->qid;
	err = nil;

	for(nqid = 0; nqid < nwname; nqid++){
		if((qid.type & QTDIR) == 0){
			err = Enotdir;
			break;
		}
		if(!perm(paq->dir, f->user, Pexec)) {
			err = Eperm;
			break;
		}
		npaq = paqWalk(paq, thdr.wname[nqid]);
		if(npaq == nil) {
			err = Enotexist;
			break;
		}
		paqFree(paq);
		paq = npaq;
		qid = paq->qid;
		rhdr.wqid[nqid] = qid;
	}

	rhdr.nwqid = nqid;

	if(nqid == nwname){
		/* success */
		paqFree(f->paq);
		f->paq = paq;
		return 0;
	}

	paqFree(paq);
	if(nf != nil)
		rclunk(nf);

	/* only error on the first element */
	if(nqid == 0)
		return err;

	return 0;
}

char *
ropen(Fid *f)
{
	int mode, trunc;

	if(f->open)
		return Eisopen;
	if(f->busy == 0)
		return Enotexist;
	mode = thdr.mode;
	if(f->paq->qid.type & QTDIR){
		if(mode != OREAD)
			return Eperm;
		rhdr.qid = f->paq->qid;
		return 0;
	}
	if(mode & ORCLOSE)
		return Erdonly;
	trunc = mode & OTRUNC;
	mode &= OPERM;
	if(mode==OWRITE || mode==ORDWR || trunc)
		return Erdonly;
	if(mode==OREAD)
		if(!perm(f->paq->dir, f->user, Pread))
			return Eperm;
	if(mode==OEXEC)
		if(!perm(f->paq->dir, f->user, Pexec))
			return Eperm;
	rhdr.qid = f->paq->qid;
	f->open = 1;
	return 0;
}

char *
rcreate(Fid *f)
{
	if(f->open)
		return Eisopen;
	if(f->busy == 0)
		return Enotexist;
	return Erdonly;
}

char *
readdir(Fid *f)
{
	PaqDir *pd;
	uchar *p, *ep;
	ulong off;
	int n, cnt, i;
	uchar *buf;
	Block *ptr, *b;

	buf = (uchar*)rhdr.data;
	cnt = thdr.count;
	if(thdr.offset == 0)
		f->offset = 0;
	off = f->offset;

	ptr = blockLoad(f->paq->dir->offset, PointerBlock);
	if(ptr == nil)
		return Ebadblock;
	i = off/blocksize;
	off -= i*blocksize;

	rhdr.count = 0;
	b = blockLoad(getl(ptr->data + i*4), DirBlock);
	while(b != nil) {
		p = b->data + off;
		ep = b->data + blocksize;
		if(checkDirSize(p, ep)) {
			pd = getDir(p);
			n = packDir(pd, buf, cnt);
			paqDirFree(pd);
			if(n == 0) {
				blockFree(b);
				if(rhdr.count == 0) {
					blockFree(ptr);
					return Edirtoobig;
				}
				break;
			}
			off += gets(p);
			cnt -= n;
			buf += n;
			rhdr.count += n;
		} else {
			off = 0;
			i++;
			blockFree(b);
			b = blockLoad(getl(ptr->data + i*4), DataBlock);
		}
	}
	f->offset = i*blocksize + off;
	blockFree(ptr);

	return 0;
}

char*
rread(Fid *f)
{
	PaqDir *pd;
	uchar *buf;
	ulong off;
	int n, cnt, i;
	Block *ptr, *b;

	if(f->busy == 0)
		return Enotexist;
	if(f->paq->qid.type & QTDIR)
		return readdir(f);
	pd = f->paq->dir;
	off = thdr.offset;
	buf = (uchar*)rhdr.data;
	cnt = thdr.count;

	rhdr.count = 0;
	if(off >= pd->length || cnt == 0){
		return 0;
	}

	if(cnt > pd->length - off){
		cnt = pd->length - off;
	}
	ptr = blockLoad(pd->offset, PointerBlock);
	if(ptr == nil)
		return Ebadblock;

	i = off/blocksize;
	off -= i*blocksize;

	while(cnt > 0) {
		b = blockLoad(getl(ptr->data + i*4), DataBlock);
		if(b == nil) {
			blockFree(ptr);
			return Ebadblock;
		}
		n = blocksize - off;
		if(n > cnt)
			n = cnt;
		memmove(buf, b->data + off, n);
		cnt -= n;
		rhdr.count += n;
		buf += n;
		off = 0;
		i++;
		blockFree(b);
	}
	blockFree(ptr);
	return 0;
}

char*
rwrite(Fid *f)
{
	if(f->busy == 0)
		return Enotexist;
	return Erdonly;
}

char *
rclunk(Fid *f)
{
	f->busy = 0;
	f->open = 0;
	free(f->user);
	paqFree(f->paq);
	return 0;
}

char *
rremove(Fid *f)
{
	rclunk(f);
	return Erdonly;
}

char *
rstat(Fid *f)
{
	if(f->busy == 0)
		return Enotexist;
	rhdr.stat = (uchar*)rhdr.data;
	rhdr.nstat = packDir(f->paq->dir, rhdr.stat, mesgsize);
	if(rhdr.nstat == 0)
		return Edirtoobig;
	return 0;
}

char *
rwstat(Fid *f)
{
	if(f->busy == 0)
		return Enotexist;
	return Erdonly;
}

Paq*
paqCpy(Paq *s)
{
	s->ref++;
	return s;
}

void
paqFree(Paq *p)
{
	if(p == nil)
		return;
	p->ref--;
	if(p->ref > 0)
		return;
assert(p != root);
	paqFree(p->up);
	paqDirFree(p->dir);
	free(p);
}

void
paqDirFree(PaqDir *pd)
{
	if(pd == nil)
		return;
	free(pd->name);
	free(pd->uid);
	free(pd->gid);
	free(pd);
}

Qid
paqDirQid(PaqDir *d)
{
	Qid q;

	q.path = d->qid;
	q.vers = 0;
	q.type = d->mode >> 24;

	return q;
}

int
packDir(PaqDir *s, uchar *buf, int n)
{
	Dir dir;

	memset(&dir, 0, sizeof(dir));
	dir.qid = paqDirQid(s);
	dir.mode = s->mode;
	dir.atime = s->mtime;
	dir.mtime = s->mtime;
	dir.length = s->length;
	dir.name = s->name;
	dir.uid = s->uid;
	dir.gid = s->gid;
	dir.muid = s->uid;

	n = convD2M(&dir, buf, n);
	if(n < STATFIXLEN)
		return 0;
	return n;
}

Block *
blockLoad(ulong addr, int type)
{
	ulong age;
	int i, j;
	Block *b;

	cacheage++;

	/* age has wraped */
	if(cacheage == 0) {
		for(i=0; i<cachesize; i++)
			cache[i].age = 0;
	}

	j = -1;
	age = ~0;
	for(i=0; i<cachesize; i++) {
		b = &cache[i];
		if(b->age < age && b->ref == 0) {
			age = b->age;
			j = i;
		}
		if(b->addr != addr)
			continue;
		b->age = cacheage;
		b->ref++;
		return b;
	}
	if(j < 0)
		sysfatal("no empty spots in cache!\n");
	b = &cache[j];
	assert(b->ref == 0);

	if(!blockRead(b->data, addr, type)) {
		b->addr = 0;
		b->age = 0;
		return nil;
	}

	b->age = cacheage;
	b->addr = addr;
	b->ref = 1;
	
	return b;
}

void
blockFree(Block *b)
{
	if(b == nil)
		return;
	if(--b->ref > 0)
		return;
	assert(b->ref == 0);
}

Paq*
paqWalk(Paq *s, char *name)
{
	Block *ptr, *b;
	uchar *p, *ep;
	PaqDir *pd;
	int i, n;
	Paq *ss;

	if(strcmp(name, "..") == 0)
		return paqCpy(s->up);

	ptr = blockLoad(s->dir->offset, PointerBlock);
	if(ptr == nil)
		return nil;

	for(i=0; i<blocksize/4; i++) {
		b = blockLoad(getl(ptr->data+i*4), DirBlock);
		if(b == nil)
			break;
		p = b->data;
		ep = p + blocksize;
		while(checkDirSize(p, ep)) {
			n = gets(p);
			pd = getDir(p);
			if(strcmp(pd->name, name) == 0) {
				ss = emallocz(sizeof(Paq));
				ss->ref = 1;
				ss->up = paqCpy(s);
				ss->dir = pd;
				ss->qid = paqDirQid(pd);
				blockFree(b);
				blockFree(ptr);
				return ss;
			}
			paqDirFree(pd);
			p += n;
		}
		blockFree(b);
	}

	blockFree(ptr);
	return nil;
}

Fid *
newfid(int fid)
{
	Fid *f, *ff;

	ff = 0;
	for(f = fids; f; f = f->next)
		if(f->fid == fid)
			return f;
		else if(!ff && !f->busy)
			ff = f;
	if(ff){
		ff->fid = fid;
		return ff;
	}
	f = emallocz(sizeof *f);
	f->fid = fid;
	f->next = fids;
	fids = f;
	return f;
}

void
io(int fd)
{
	char *err;
	int n, pid;
	uchar *mdata;

	mdata = emalloc(mesgsize);

	pid = getpid();

	for(;;){
		n = read9pmsg(fd, mdata, mesgsize);
		if(n < 0)
			sysfatal("mount read");
		if(n == 0)
			break;
		if(convM2S(mdata, n, &thdr) == 0)
			continue;

		if(debug)
			fprint(2, "paqfs %d:<-%F\n", pid, &thdr);

		rhdr.data = (char*)mdata + IOHDRSZ;
		if(!fcalls[thdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[thdr.type])(newfid(thdr.fid));
		if(err){
			rhdr.type = Rerror;
			rhdr.ename = err;
		}else{
			rhdr.type = thdr.type + 1;
			rhdr.fid = thdr.fid;
		}
		rhdr.tag = thdr.tag;
		if(debug)
			fprint(2, "paqfs %d:->%F\n", pid, &rhdr);/**/
		n = convS2M(&rhdr, mdata, mesgsize);
		if(n == 0)
			sysfatal("convS2M sysfatal on write");
		if(write(fd, mdata, n) != n)
			sysfatal("mount write");
	}
}

int
perm(PaqDir *s, char *user, int p)
{
	ulong perm = s->mode;

	if((p*Pother) & perm)
		return 1;
	if(strcmp(user, s->gid)==0 && ((p*Pgroup) & perm))
		return 1;
	if(strcmp(user, s->uid)==0 && ((p*Powner) & perm))
		return 1;
	return 0;
}

void
init(char *file, int verify)
{
	PaqHeader hdr;
	PaqTrailer tlr;
	Dir *dir;
	int i;
	uchar *p;
	DigestState *ds = nil;
	PaqDir *r;
	Block *b;
	ulong offset;

	inflateinit();

	bin = Bopen(file, OREAD);
	if(bin == nil)
		sysfatal("could not open file: %s: %r", file);
	if(verify)
		ds = sha1(0, 0, 0, 0);
	
	readHeader(&hdr, file, ds);

	blocksize = hdr.blocksize;

	if(verify) {
		readBlocks(file, ds);
	} else {
		dir = dirstat(file);
		if(dir == nil)
			sysfatal("could not stat file: %s: %r", file);
		offset = dir->length - TrailerSize;
		free(dir);
		if(Bseek(bin, offset, 0) != offset)
			sysfatal("could not seek to trailer: %s", file);
	}

	readTrailer(&tlr, file, ds);

	/* asctime includes a newline - yuk */
	fprint(2, "%s: %s", hdr.label, asctime(gmtime(hdr.time)));
	fprint(2, "fingerprint: ");
	for(i=0; i<20; i++)
		fprint(2, "%.2x", tlr.sha1[i]);
	fprint(2, "\n");

	cache = emallocz(cachesize*sizeof(Block));
	p = emalloc(cachesize*blocksize);
	for(i=0; i<cachesize; i++) {
		cache[i].data = p;
		p += blocksize;
	}

	/* hand craft root */
	b = blockLoad(tlr.root, DirBlock);
	if(b == nil || !checkDirSize(b->data, b->data+blocksize))
		sysfatal("could not read root block: %s", file);
	r = getDir(b->data);
	blockFree(b);
	root = emallocz(sizeof(Paq));
	root->qid = paqDirQid(r);
	root->ref = 1;
	root->dir = r;
	root->up = root;	/* parent of root is root */
}

int
blockRead(uchar *data, ulong addr, int type)
{
	uchar buf[BlockSize];
	PaqBlock b;
	uchar *cdat;

	if(Bseek(bin, addr, 0) != addr)
		return 0;
	if(Bread(bin, buf, BlockSize) != BlockSize)
		return 0;
	getBlock(buf, &b);
	if(b.magic != BlockMagic || b.size > blocksize || b.type != type)
		return 0;

	switch(b.encoding) {
	default:
		return 0;
	case NoEnc:
		if(Bread(bin, data, blocksize) < blocksize)
			return 0;
		break;
	case DeflateEnc:
		cdat = emalloc(b.size);
		if(Bread(bin, cdat, b.size) < b.size) {
			free(cdat);
			return 0;
		}
		if(inflateblock(data, blocksize, cdat, b.size) < 0) {
			free(cdat);
			return 0;
		}
		free(cdat);
		break;
	}
	if(adler32(0, data, blocksize) != b.adler32)
		return 0;
	return 1;
}

void
readHeader(PaqHeader *hdr, char *name, DigestState *ds)
{
	uchar buf[HeaderSize];
	
	if(Bread(bin, buf, HeaderSize) < HeaderSize)
		sysfatal("could not read header: %s: %r", name);
	if(ds)
		sha1(buf, HeaderSize, 0, ds);
	getHeader(buf, hdr);
	if(hdr->magic != HeaderMagic)
		sysfatal("bad header magic 0x%lux: %s", hdr->magic, name);
	if(hdr->version != Version)
		sysfatal("unknown file version: %s", name);
}

void
readBlocks(char *name, DigestState *ds)
{
	uchar *buf;
	PaqBlock b;

	buf = emalloc(BlockSize+blocksize);

	for(;;) {
		if(Bread(bin, buf, 4) < 4)
			sysfatal("could not read block: %s: %r", name);
		Bseek(bin, -4, 1);
		/* check if it is a data block */

		if(getl(buf) != BlockMagic)
			break;

		if(Bread(bin, buf, BlockSize) < BlockSize)
			sysfatal("could not read block: %s: %r", name);
		if(ds)
			sha1(buf, BlockSize, 0, ds);
		getBlock(buf, &b);

		if(b.size > blocksize)
			sysfatal("bad block size: %d: %s", b.size, name);
		if(ds) {
			if(Bread(bin, buf, b.size) < b.size)
				sysfatal("sysfatal reading block: %s: %r", name);
			sha1(buf, b.size, 0, ds);
		} else
			Bseek(bin, b.size, 1);
	}

	free(buf);
}

void
readTrailer(PaqTrailer *tlr, char *name, DigestState *ds)
{
	uchar buf[TrailerSize];
	uchar digest[20];

	if(Bread(bin, buf, TrailerSize) < TrailerSize)
		sysfatal("could not read trailer: %s: %r", name);
	getTrailer(buf, tlr);
	if(tlr->magic != TrailerMagic)
		sysfatal("bad trailer magic: %s", name);
	if(ds) {
		sha1(buf, TrailerSize-20, digest, ds);
		if(memcmp(digest, tlr->sha1, 20) != 0)
			sysfatal("bad sha1 digest: %s", name);
	}
}

void *
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(!p)
		sysfatal("out of memory");
	return p;
}

void *
emallocz(ulong n)
{
	void *p;

	p = emalloc(n);
	memset(p, 0, n);

	return p;
}

void *
erealloc(void *p, ulong n)
{
	p = realloc(p, n);
	if(!p)
		sysfatal("out of memory");
	return p;
}

char *
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil)
		sysfatal("out of memory");
	return s;
}


ulong
getl(uchar *p)
{
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}


int
gets(uchar *p)
{
	return (p[0]<<8) | p[1];
}

int
checkDirSize(uchar *p, uchar *ep)
{
	int n;	
	int i;

	if(ep-p < 2)
		return 0;
	n = gets(p);
	if(p+n > ep)
		return 0;
	ep = p+n;
	p += 22;
	for(i=0; i<3; i++) {
		if(p+2 > ep)
			return 0;
		n = gets(p);
		if(p+n > ep)
			return 0;
		p += n;
	}
	return 1;
}

void
getHeader(uchar *p, PaqHeader *h)
{
	h->magic = getl(p);
	h->version = gets(p+4);
	h->blocksize = gets(p+6);
	h->time = getl(p+8);
	memmove(h->label, p+12, sizeof(h->label));
	h->label[sizeof(h->label)-1] = 0;
}

void
getTrailer(uchar *p, PaqTrailer *t)
{
	t->magic = getl(p);
	t->root = getl(p+4);
	memmove(t->sha1, p+8, 20);
}

void
getBlock(uchar *p, PaqBlock *b)
{
	b->magic = getl(p);
	b->size = gets(p+4);
	b->type = p[6];
	b->encoding = p[7];
	b->adler32 = getl(p+8);
}

PaqDir *
getDir(uchar *p)
{
	PaqDir *pd;

	pd = emallocz(sizeof(PaqDir));
	pd->qid = getl(p+2);
	pd->mode = getl(p+6);
	pd->mtime = getl(p+10);
	pd->length = getl(p+14);
	pd->offset = getl(p+18);
	p += 22;
	pd->name = getstr(p);
	p += gets(p);
	pd->uid = getstr(p);
	p += gets(p);
	pd->gid = getstr(p);

	return pd;
}


char *
getstr(uchar *p)
{
	char *s;
	int n;

	n = gets(p);
	s = emalloc(n+1);
	memmove(s, p+2, n);
	s[n] = 0;
	return s;
}

void
usage(void)
{
	fprint(2, "usage: %s [-disv] [-c cachesize] [-m mountpoint] [-M mesgsize] paqfile\n", argv0);
	exits("usage");
}
