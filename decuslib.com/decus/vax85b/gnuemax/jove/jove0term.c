/*R
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
 
   jove_term.c 
 
   Gets the termcap information and complains if there are not enough 
   of the basic features on the particular terminal. */ 
 
#include "jove.h" 
#include <sgtty.h> 
#include <errno.h> 
 
#ifdef PROCS 
#include <signal.h> 
#endif 
 
/* Termcap definitions */ 
 
char	*UP, 
	*CS, 
	*SO, 
	*SE, 
	*CM, 
	*CL, 
	*CE, 
	*HO, 
	*AL, 
	*DL, 
	*IS, 
	*VS, 
	*VE, 
	*IC, 
	*DC, 
	*IM, 
	*EI, 
	*LL, 
	*BC, 
	*SR, 
	*VB; 
 
int	LI, 
	CO, 
 
	UL, 
	MI, 
 
	TABS, 
	UpLen, 
	HomeLen, 
	LowerLen; 
 
 
int ospeed; 
 
char	tspace[128]; 
 
char **meas[] = { 
	&VS, &VE, &IS, &AL, &DL, &CS, &SO, &SE, 
	&CM, &CL, &CE, &HO, &UP, &BC, &IC, &IM, 
	&DC, &EI, &LL, &SR, &VB, 0 
}; 
 
gets(buf) 
char	*buf; 
{ 
	buf[read(0, buf, 12) - 1] = 0; 
}	 
 
char	*sprint(); 
 
TermError(str) 
char	*str; 
{ 
	char	*cp; 
 
	cp = sprint("Termcap error: %s\n", str); 
	if (write(1, cp, strlen(cp))); 
	exit(1); 
} 
 
getTERM() 
{ 
	char	*getenv(); 
	struct sgttyb tty; 
	char	*ts="vsveisaldlcssosecmclcehoupbcicimdceillsrvb"; 
	char	termbuf[13], 
		*termname = 0, 
		*termp = tspace, 
		tbuff[1024]; 
	int	i; 
 
	if (gtty(1, &tty)) { 
		TABS = 0; 
		ospeed = B1200; 
	} else { 
		TABS = !(tty.sg_flags & XTABS); 
		ospeed = tty.sg_ospeed; 
	} 
 
	termname = getenv("TERM"); 
	if (termname == 0) { 
		putstr("Enter terminal name: "); 
		gets(termbuf); 
		if (termbuf[0] == 0) 
			TermError(NullStr); 
 
		termname = termbuf; 
	} 
 
	/* fhsu: GIGIs and VT100s can't keep up with anything */ 
	if ((strcmp(termname, "vk100") == 0) || 
	    (strcmp(termname, "vt100") == 0) || 
	    (strcmp(termname, "vt220") == 0) || 
	    (strcmp(termname, "vt125") == 0)) 
	   	OKXonXoff = 0; 
 
	if (tgetent(tbuff, termname) < 1) 
		TermError("terminal type?"); 
 
	if ((CO = tgetnum("co")) == -1) 
		TermError("columns?"); 
 
	if ((LI = tgetnum("li")) == -1) 
		TermError("lines?"); 
 
	for (i = 0; meas[i]; i++) { 
		*(meas[i]) = (char *) tgetstr(ts, &termp); 
		ts += 2; 
	} 
	if (IM && (*IM == 0)) 
		IM = 0; 
	else 
		MI = tgetflag("mi"); 
	UL = tgetflag("ul"); 
/* You can decide whether you want this ... */ 
	if (!CE || !UP) 
		TermError("I need CE and UP\n"); 
	if (CanScroll = ((AL && DL) || CS)) 
		IDline_setup(termname); 
} 
 
/* 
   Deals with output to the terminal, setting up the amount of characters 
   to be buffered depending on the output baud rate.  Why it's in a  
   separate file I don't know ... */ 
 
IOBUF	termout; 
 
outc(c) 
register int	c; 
{ 
	outchar(c); 
} 
 
/* Put a string with padding */ 
 
putpad(str, lines) 
char	*str; 
{ 
	tputs(str, lines, outc); 
} 
 
#ifdef PROCS 
dowrite(fd, buf, n) 
char	*buf; 
{ 
	int	nbytes = n, 
		justsent; 
	extern int	errno; 
 
	do { 
		justsent = write(fd, buf, nbytes); 
		if (justsent == -1) { 
			printf("\r\007Write failed (%d)", errno); 
			justsent = 0; 
		} 
		nbytes -= justsent; 
		buf += justsent; 
	} while (nbytes > 0); 
} 
#else 
#define	dowrite write 
#endif	 
 
/* Flush the output, and check for more characters.  If there are 
   some, then return to main, to process them, aborting redisplay. */ 
 
flushout(x, p) 
register IOBUF	*p; 
{ 
	register int	n; 
	extern int	errno; 
 
	if ((n = p->io_ptr - p->io_base) <= 0) 
		goto skip; 
	CheckTime = 1; 
	if (n > 0) { 
		dowrite(p->io_fd, p->io_base, n); 
		if (p == &termout) { 
			CheckTime = BufSize; 
			p->io_cnt = BufSize; 
		} else 
			p->io_cnt = BUFSIZ; 
		p->io_ptr = p->io_base; 
	} 
skip: 
	if (x >= 0) 
		Putc(x, p); 
} 
 
/* Determine the number of characters to buffer at each 
   baud rate.  The lower the number, the quicker the 
   response when new input arrives.  Of course the lower 
   the number, the more prone the program is to stop in 
   output.  Decide what matters most to you. 
   This sets the int BufSize to the right number or chars, 
   allocates the buffer, and initiaizes `termout'.  */ 
 
int	BufSize; 
 
settout() 
{ 
	static int speeds[] = { 
		1,	/* 0	*/ 
		1,	/* 50	*/ 
		1,	/* 75	*/ 
		1,	/* 110	*/ 
		1,	/* 134	*/ 
		1,	/* 150	*/ 
		1,	/* 200	*/ 
		1,	/* 300	*/ 
		1,	/* 600	*/ 
		5,	/* 1200 */ 
		15,	/* 1800	*/ 
		30,	/* 2400	*/ 
		60,	/* 4800	*/ 
		120,	/* 9600	*/ 
		180,	/* EXTA	*/ 
		240,	/* EXT	*/ 
	}; 
 
	termout.io_cnt = BufSize = CheckTime = speeds[ospeed] * max(LI / 24, 1); 
	termout.io_base = termout.io_ptr = emalloc(BufSize); 
	termout.io_fd = 1;	/* Standard output */ 
} 
 
SitFor(delay) 
int	delay; 
{ 
	static float cps[] = { 
		0.0, 
		5.0, 
		7.5, 
		11.0, 
		13.4, 
		15.0, 
		20.0, 
		30.0, 
		60.0, 
		120.0, 
		180.0, 
		240.0, 
		480.0, 
		960.0, 
		1920.0, 
		1920.0, 
	}; 
	float	nsecs = (float) delay / 10; 
	register int	nchars = (int) (nsecs * cps[ospeed]); 
 
	redisplay(); 
	while ((--nchars > 0) && !InputPending) { 
		outchar(0); 
		if (CheckTime) { 
			flusho(); 
			CheckTime = 0; 
		} 
		InputPending = charp(); 
	} 
} 
 
