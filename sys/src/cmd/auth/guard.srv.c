#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <bio.h>
#include <ndb.h>
#include <authsrv.h>
#include "authcmdlib.h"

/*
 * c -> a	client
 * a -> c	challenge prompt
 * c -> a	KC'{challenge}
 * a -> c	OK or NO
 */

void	catchalarm(void*, char*);
void	getraddr(char*);
int	secureidcheck(char*, char*);

char	user[ANAMELEN];
char	raddr[128];
int	debug;
Ndb	*db;

void
main(int argc, char *argv[])
{
	char ukey[DESKEYLEN], resp[32], buf[NETCHLEN];
	long chal;
	int n;
	Ndb *db2;

	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	}ARGEND;

	db = ndbopen("/lib/ndb/auth");
	if(db == 0)
		syslog(0, AUTHLOG, "no /lib/ndb/auth");
	db2 = ndbopen(0);
	if(db2 == 0)
		syslog(0, AUTHLOG, "no /lib/ndb/local");
	db = ndbcat(db, db2);

	strcpy(raddr, "unknown");
	if(argc >= 1)
		getraddr(argv[argc-1]);

	argv0 = "guard";
	srand((getpid()*1103515245)^time(0));
	notify(catchalarm);

	/*
	 * read the host and client and get their keys
	 */
	if(readarg(0, user, sizeof user) < 0)
		fail(0);

	/*
	 * challenge-response
	 */
	chal = lnrand(MAXNETCHAL);
	sprint(buf, "challenge: %lud\nresponse: ", chal);
	n = strlen(buf) + 1;
	if(write(1, buf, n) != n){
		if(debug)
			syslog(0, AUTHLOG, "g-fail %s@%s :%r sending chal",
				user, raddr);
		exits("replying to server");
	}
	alarm(3*60*1000);
	if(readarg(0, resp, sizeof resp) < 0){
		if(debug)
			syslog(0, AUTHLOG, "g-fail %s@%s :%r reading resp",
				user, raddr);
		fail(0);
	}
	alarm(0);

	if(!findkey(NETKEYDB, user, ukey) || !netcheck(ukey, chal, resp))
	if(!findkey(KEYDB, user, ukey) || !netcheck(ukey, chal, resp))
	if(!secureidcheck(user, resp)){
		write(1, "NO", 2);
		if(debug)
			syslog(0, AUTHLOG, "g-fail %s@%s: bad response %s to %lud",
				user, raddr, resp, chal);
		fail(user);
	}
	write(1, "OK", 2);
	if(debug)
		syslog(0, AUTHLOG, "g-ok %s@%s", user, raddr);
	succeed(user);
	exits(0);
}

void
catchalarm(void *x, char *msg)
{
	USED(x, msg);
	if(debug)
		syslog(0, AUTHLOG, "g-timed out %s", raddr);
	fail(0);
}

void
getraddr(char *dir)
{
	int n, fd;
	char *cp;
	char file[128];

	snprint(file, sizeof(file), "%s/remote", dir);
	fd = open(file, OREAD);
	if(fd < 0)
		return;
	n = read(fd, raddr, sizeof(raddr)-1);
	close(fd);
	if(n <= 0)
		return;
	raddr[n] = 0;
	cp = strchr(raddr, '\n');
	if(cp)
		*cp = 0;
	cp = strchr(raddr, '!');
	if(cp)
		*cp = 0;
}
