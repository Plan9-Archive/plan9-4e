#include "../port/portfns.h"

void	addclock0link(void (*)(void));
void	audiopower(int);
void	audioamppower(int);
void	audioicpower(int);
void	cacheflush(void);
void	cachewb(void);
void	cachewbaddr(void*);
void	cachewbregion(ulong, int);
void	clockintrsched(void);	/* to be deleted */
void	coma(ulong);
void	dcacheinvalidate(void);
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
void	clockinit(void);
void	clockpower(int);
#define	coherence()
#define	dcflush(a, b)
void	delay(int);
void	µdelay(ulong);
void	µcpower(int);
void	dmainit(void);
void	_doze(void);
void	(*doze)(void);
void	egpiobits(ulong, int);
void	evenaddr(ulong);
void	exppackpower(int);
ulong findva(ulong, ulong, ulong);
void	flashprogpower(int);
void	flushmmu(void);
int	fpiarm(Ureg *ur);
char*	getconf(char*);
ulong	getcpuid(void);
ulong	getfar(void);
ulong	getfsr(void);
ulong	getcontrol(void);
ulong	getdac(void);
ulong	getttb(void);
void*	getlink(void);
#define	getpgcolor(a)	0
ulong	getsp(void);
void	gpiointrenable(ulong, int, void (*)(Ureg*, void*), void*, char*);
void	h3650uartsetup(void);
int	havetimer(void);
void	icacheinvalidate(void);
void	idle(void);
void	idlehands(void);
uchar	inb(ulong);
ushort	ins(ulong);
void	inss(ulong, void*, int);
ulong	inl(ulong);
void	intrenable(int, int, void (*)(Ureg*, void*), void*, char*);
void	irpower(int);
#define	kmapinval()
void	lcdpower(int);
void	links(void);
void*	mapmem(ulong, int, int);
void	mappedIvecEnable(void);
void	mappedIvecDisable(void);
void*	mapspecial(ulong, int);
void	meminit(void);
void	mmuinit(void);
void	mmuenable(void);
void	mmudisable(void);
void	mmuinvalidate(void);
void	mmuinvalidateaddr(ulong);
void	mmurestart(void);
ulong	mmu_kaddr(ulong);
ulong	mmu_paddr(ulong);
int	µcputc(Queue*, int);
void	noted(Ureg*, ulong);
int	notify(Ureg*);
void	outb(ulong, uchar);
void	outs(ulong, ushort);
void	outss(ulong, void*, int);
void	outl(ulong, ulong);
void	pcmcisread(PCMslot*);
int	pcmcistuple(int, int, int, void*, int);
PCMmap*	pcmmap(int, ulong, int, int);
void	pcmunmap(int, PCMmap*);
void	penbutton(int, int);
void	pentrackxy(int x, int y);
void	power_down(void);
void	powerinit(void);
void	powerkproc(void*);
#define	procrestore(p)
void	procsave(Proc*);
void	procsetup(Proc*);
void	putdac(ulong);
void	putttb(ulong);
void	putpid(ulong);
void	qpanic(char *, ...);
void	rs232power(int);
void	uartspecial(Uart*, int, Queue**, Queue**, int (*)(Queue*, int));
void	sa1110_uartsetup(int);
void	screeninit(void);
void	screenpower(int);
int	screenprint(char*, ...);			/* debugging */
void	serialµcputs(uchar *str, int n);
void	setr13(int, ulong*);
uchar*	tarlookup(uchar*, char*, int*);
void	timersinit(void);
void	timeradd(Timer*);
void	timerdel(Timer*);
void	timerintr(Ureg *, uvlong);
void	timerset(uvlong);
void	touser(void*);
void	trapdump(char *tag);
void	trapinit(void);
void	trapresume(void);
int	tas(void*);
void	uartpower(int);
int	uartstageoutput(Uart*);
void	uartkick(void*);
void	uartrecv(Uart*, char);
int	unsac(uchar*, uchar*, int, int);
#define	userureg(ur)	(((ur)->psr & PsrMask) == PsrMusr)
void	vectors(void);
void	vtable(void);
void	wbflush(void);
ulong	TK2MS(ulong);				/* ticks to milliseconds */
#define KADDR(a) (void*)mmu_kaddr((ulong)(a))
#define PADDR(a) mmu_paddr((ulong)(a))

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
