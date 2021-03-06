void	accessdir(Iobuf*, Dentry*, int, int);
void	addfree(Device*, long, Superb*);
Alarm*	alarm(int, void(*)(Alarm*, void*), void*);
void	alarminit(void);
void	arpstart(void);
void	arginit(void);
char*	authaname(Auth*);
void	authinit(void);
void	authfree(Auth*);
Auth*	authnew(char*, char*);
int	authread(File*, uchar*, int);
int	authuid(Auth*);
char*	authuname(Auth*);
int	authwrite(File*, uchar*, int);
void	cdiag(char*, int);
int	cnumb(void);
Device*	config(void);
int	rawchar(int);
long	bufalloc(Device*, int, long, int);
void	buffree(Device*, long, int);
int	byuid(void*, void*);
void	cancel(Alarm*);
int	canlock(Lock*);
int	canqlock(QLock*);
void	cfsdump(Filsys*);
Chan*	chaninit(int, int, int);
void	cmd_check(int, char*[]);
void	cmd_users(int, char*[]);
void	cmd_newuser(int, char*[]);
void	cmd_netdb(int, char*[]);
void	cmd_passwd(int, char*[]);
void	cmd_printconf(int, char*[]);
int	checkname(char*);
int	checktag(Iobuf*, int, long);
int	cksum(void*, int, int);
int	cksum0(int, int);
void	clock(ulong, ulong);
void	clockinit(void);
void	clockreload(ulong);
void	cyclstart(void);
void	dotrace(int);
int	conschar(void);
void	consinit(void (*)(char*, int));
void	consreset(void);
void	consstart(int);
int	(*consgetc)(void);
void	(*consputc)(int);
void	(*consputs)(char*, int);
void	consserve(void);
int	conslock(void);
int	con_attach(int, char*, char*);
int	con_clone(int, int);
int	con_create(int, char*, int, int, long, int);
int	con_clri(int);
int	con_fstat(int);
int	con_open(int, int);
int	con_read(int, char*, long, int);
int	con_remove(int);
void	con_rintr(int);
int	con_session(void);
int	con_walk(int, char*);
int	con_write(int, char*, long, int);
int	cwgrow(Device*, Superb*, int);
int	cwfree(Device*, long);
void	cwinit(Device*);
long	cwraddr(Device*);
int	cwread(Device*, long, void*);
void	cwream(Device*);
void	cwrecover(Device*);
long	cwsaddr(Device*);
long	cwsize(Device*);
long	dbufread(Iobuf*, Dentry*, long, long, int);
int	cwwrite(Device*, long, void*);
void	datestr(char*, ulong);
void	delay(int);
int	devcmpr(Device*, Device*);
void	devream(Device*, int);
void	devrecover(Device*);
void	devinit(Device*);
int	devread(Device*, long, void*);
long	devsize(Device*);
int	devwrite(Device*, long, void*);
Iobuf*	dnodebuf(Iobuf*, Dentry*, long, int, int);
Iobuf*	dnodebuf1(Iobuf*, Dentry*, long, int, int);
void	dofilter(Filter*, int, int, int);
int	doremove(File*, int);
void	dtrunc(Iobuf*, Dentry*, int);
int	dumpblock(Device*);
void	dumpregs(Ureg*);
void	dumpstack(User*);
void	exit(void);
Float	famd(Float, int, int, int);
ulong	fdf(Float, int);
void	fileinit(Chan*);
File*	filep(Chan*, ulong, int);
void	firmware(void);
int	fname(char*);
int	fpair(char*, char*);
void	formatinit(void);
int	fread(void*, int);
void	freealarm(Alarm*);
void	freefp(File*);
void	freewp(Wpath*);
Filsys*	fsstr(char*);
long	fwormsize(Device*);
void	fwormream(Device*);
void	fworminit(Device*);
int	fwormread(Device*, long, void*);
int	fwormwrite(Device*, long, void*);
char*	getauthlist(void);
Iobuf*	getbuf(Device*, long, int);
void*	getarg(void);
char*	getwd(char*, char*);
int	getc(void);
ulong	getcallerpc(void*);
Dentry*	getdir(Iobuf*, int);
Chan*	getlcp(uchar*, long);
long	getraddr(Device*);
void	getstring(char*, int, int);
void	gotolabel(Label*);
void	hexdump(void*, int);
int	iaccess(File*, Dentry*, int);
void*	ialloc(ulong, int);
void	ilock(Lock*);
void	iunlock(Lock*);
long	indfetch(Device*, long, long, long , int, int, int);
int	ingroup(int, int);
int	inh(int, uchar*);
void	init0(void);
void	iobufinit(void);
void*	iobufmap(Iobuf*);
void	iobufunmap(Iobuf*);
int	iobufql(QLock*);
int	jukeread(Device*, long, void*);
int	jukewrite(Device*, long, void*);
void	jukeinit(Device*);
void	jukeream(Device*);
void	jukerecover(Device*);
long	jukesaddr(Device*);
long	jukesize(Device*);
void	kbdchar(int);
void	lights(int, int);
void	launchinit(void);
void	localconfinit(void);
int	leadgroup(int, int);
void	lock(Lock*);
void	lockinit(void);
void	machinit(void);
Msgbuf*	mballoc(int, Chan*, int);
void	mbinit(void);
void	mbfree(Msgbuf*);
ulong	meminit(void);
Iobuf*	movebuf(Iobuf*);
void	mcatinit(Device*);
int	mcatread(Device*, long, void*);
long	mcatsize(Device*);
int	mcatwrite(Device*, long, void*);
void	mkqid(Qid*, Dentry*, int);
int	mkqidcmp(Qid*, Dentry*);
void	mkqid9p1(Qid9p1*, Qid*);
void	mkqid9p2(Qid*, Qid9p1*, int);
void	mlevinit(Device*);
int	mlevread(Device*, long, void*);
long	mlevsize(Device*);
int	mlevwrite(Device*, long, void*);
int	nametokey(char*, char*);
Alarm*	newalarm(void);
File*	newfp(void);
User*	newproc(void);
Queue*	newqueue(int);
void	newstart(void);
Wpath*	newwp(void);
Auth*	newauth(void);
int	nvrcheck(void);
int	nvread(int, void*, int);
char*	nvrgetconfig(void);
int	nvrsetconfig(char*);
int	nvwrite(int, void*, int);
int	walkto(char*);
int	no(void*);
long	number(char*, int, int);
void	online(void);
void	otherinit(void);
void	panic(char*, ...);
void	partinit(Device*);
int	partread(Device*, long, void*);
long	partsize(Device*);
int	partwrite(Device*, long, void*);
void	prdate(void);
void	preread(Device*, long);
void	prflush(void);
int	prime(long);
void	printinit(void);
void	procinit(void);
void	putbuf(Iobuf*);
long	qidpathgen(Device*);
void	qlock(QLock*);
void	qunlock(QLock*);
void	rahead(void);
void	ready(User*);
void	ream(Filsys*);
void*	recv(Queue*, int);
void	restartprint(Alarm*);
void	rlock(RWlock*);
void	rootream(Device*, long);
int	roread(Device*, long, void*);
void	rstate(Chan*, int);
ulong	rtc2sec(Rtc *);
void	runlock(RWlock*);
User*	runproc(void);
void	sched(void);
void	schedinit(void);
int	scsiio(Device*, int, uchar*, int, void*, int);
void	sec2rtc(ulong, Rtc *);
void	send(Queue*, void*);
void	serve(void);
int	serve9p1(Msgbuf*);
int	serve9p2(Msgbuf*);
int	setlabel(Label*);
void	settag(Iobuf*, int, long);
void	settime(ulong);
void	sleep(Rendez*, int(*)(void*), void*);
void	sntpinit(void);
int	splhi(void);
int	spllo(void);
void	splx(int);
void	startprint(void);
int	strtouid(char*);
long	superaddr(Device*);
void	superream(Device*, long);
void	swab(void*, int);
void	sync(char*);
int	syncblock(void);
long	syscall(Ureg*);
void	sysinit(void);
int	Tfmt(Fmt*);
ulong	time(void);
ulong	nextime(ulong, int, int);
Tlock*	tlocked(Iobuf*, Dentry*);
void	tsleep(Rendez*, int(*)(void*), void*, int);
void	touser(void);
ulong	toytime(void);
ulong	rtctime(void);
void	setrtc(ulong);
void	uidtostr(char*, int, int);
Uid*	uidpstr(char*);
void	unlock(Lock*);
void	userinit(void(*)(void), void*, char*);
void	vecinit(void);
void	wakeup(Rendez*);
void	wbflush(void);
void	wlock(RWlock*);
void	wormcopy(void);
void	wormprobe(void);
int	dowcp(void);
int	dowcmp(void);
void	synccopy(void);
void	waitsec(int);
long	wormsearch(Device*, int, long, long);
int	wormread(Device*, long, void*);
long	wormsize(Device*);
int	wormwrite(Device*, long, void*);
void	wreninit(Device*);
int	wrenread(Device*, long, void*);
long	wrensize(Device*);
int	wrenwrite(Device*, long, void*);
void	wunlock(RWlock*);
void 	cmd_exec(char*);
void	cmd_install(char*, char*, void (*)(int, char*[]));
ulong	flag_install(char*, char*);

int	chartoip(uchar *, char *);
int	isvalidip(uchar*);

int	nhgets(uchar*);
long	nhgetl(uchar*);
void	hnputs(uchar*, int);
void	hnputl(uchar*, long);

void	pokewcp(void);
