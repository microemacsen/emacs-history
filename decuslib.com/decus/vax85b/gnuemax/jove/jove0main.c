/*R
   Jonathan Payne at Lincoln-Sudbury Regional High School 4-19-83 
 
   jove_main.c 
 
   Contains the main loop, initializations, getch routine... */ 
 
#include "jove.h" 
 
#include "termcap.h" 
 
#ifndef BRLUNIX 
#include <sys/ioctl.h>		/* This is not found on BRL pdp-11 */ 
#endif 
 
#include <signal.h> 
#include <sgtty.h> 
#include <errno.h> 
 
#ifdef TIOCSLTC 
struct ltchars	ls1, 
		ls2; 
#endif 
 
#ifdef VENIX 
struct tchars { 
	char t_intrc; 
	char t_quitc; 
	char t_startc; 
	char t_stopc; 
	char t_eofc; 
	char t_brkc; 
}; 
#endif 
 
struct tchars	tc1, 
		tc2; 
 
#ifdef BRLUNIX 
struct sg_brl	sg1, 
		sg2; 
#else 
struct sgttyb	sg1, 
		sg2; 
#endif 
 
int	errormsg; 
 
int	iniargc; 
char	**iniargv; 
 
extern char	*tfname; 
extern int	errno; 
 
 
char	NullStr[] = ""; 
 
/* Push process bindings */ 
 
#define NPUSHED	3 
 
struct proc_bind { 
	data_obj	*p_push; 
	int	p_key; 
} ProcBinds[NPUSHED]; 
 
PopPBs() 
{ 
	register int	i; 
 
	for (i = 0; i < NPUSHED; i++) 
		mainmap[ProcBinds[i].p_key] = ProcBinds[i].p_push; 
} 
 
PushPBs() 
{ 
	extern int	ProcInt(), 
			ProcQuit(), 
			ProcNewline(); 
 
	ProcBinds[0].p_key = tc1.t_intrc; 
	ProcBinds[1].p_key = CTL(M); 
	ProcBinds[2].p_key = tc1.t_quitc; 
 
	ProcBinds[0].p_push = mainmap[tc1.t_intrc]; 
	ProcBinds[1].p_push = mainmap[CTL(M)]; 
	ProcBinds[2].p_push = mainmap[tc1.t_quitc]; 
 
	BindFunc(mainmap, tc1.t_intrc, ProcInt); 
	BindFunc(mainmap, CTL(M), ProcNewline); 
	BindFunc(mainmap, tc1.t_quitc, ProcQuit); 
} 
 
finish(code) 
{ 
	int	Crashit = code; 
 
#ifdef LSRHS_KLUDGERY 
	if (Crashit) 
		setdump(1); 
#endif 
	if (code == SIGINT) { 
		char	c; 
 
#ifndef MENLO_JCL 
		ignorf(signal(code, finish)); 
#endif 
		f_mess("Quit (Type 'n' if you're not sure)? "); 
		ignore(read(0, &c, 1)); 
		message(NullStr); 
		if ((c & 0377) != 'y') { 
			redisplay(); 
			return; 
		} 
	} 
	if (Crashit) { 
		if (!Crashing) { 
			putstr("Writing modified JOVE buffers..."); 
			Crashing++; 
			exp_p = 0; 
			DoWtModBuf("_", NullStr); 
		} else 
			putstr("Complete lossage!"); 
	} 
	ttyset(0); 
	if (VE) 
		putpad(VE, 1); 
	Placur(LI - 1, 0); 
	putpad(CE, 1); 
	flusho(); 
#ifdef LSRHS_KLUDGERY 
	if (CS) 
		deal_with_scroll(); 
#endif 
	ignore(unlink(tfname)); 
	if (Crashit) 
		abort(); 
	ASdel_saves();		/* Delete saved buffers if we aren't crashing */ 
	exit(code); 
} 
 
#ifdef VENIX 
#define NTOBUF	1 
#else 
#define NTOBUF	20		/* Should never get that far ahead */ 
#endif 
 
static char	smbuf[NTOBUF], 
		*bp = smbuf; 
static int	nchars = 0; 
 
Ungetc(c) 
{ 
	if (c == EOF || nchars >= NTOBUF) 
		return EOF; 
	*--bp = c; 
	nchars++; 
 
	return c; 
} 
 
char	*Inputp = 0; 
 
getchar() 
{ 
	if (nchars <= 0) { 
		/* Get a character from the keyboard, first checking for 
 		   any input from a process.  Handle that first, and then 
		   deal with the terminal input. */ 
 
		do 
			nchars = read(0, smbuf, sizeof smbuf); 
		while (nchars < 0 && errno == EINTR); 
 
		if (nchars <= 0) 
			finish(0); 
		bp = smbuf; 
		InputPending = nchars > 1; 
	} 
	nchars--; 
	return *bp++ & 0377; 
} 
 
/* Returns non-zero if a character waiting */ 
 
int	InputPending = 0; 
 
/* Returns non-zero if a character waiting */ 
 
charp() 
{ 
	extern int	InJoverc; 
 
	if (InJoverc) 
		return 0; 
	if (nchars > 0) 
		return 1;			/* Quick check */ 
	else { 
#ifdef BRLUNIX 
		static struct sg_brl gttyBuf; 
 
		gtty(0, (char *) &gttyBuf); 
		if (gttyBuf.sg_xflags & INWAIT) 
			return 1; 
		else 
			return 0; 
#else 
#ifdef FIONREAD 
		long c; 
 
		if (ioctl(0, FIONREAD, (char *) &c) == -1) 
#else 
#ifdef VENIX 
		struct sgttyb sg; 
		if (ioctl(0, TIOCQCNT, &sg) != -1) 
			c = sg.sg_ispeed; 
		else 
#else 
		int c; 
 
		if (ioctl(0, TIOCEMPTY, (char *) &c) == -1) 
#endif venix 
#endif fionread 
			c = 0; 
		return (c > 0); 
#endif brlunix 
	} 
} 
 
ResetTerm() 
{ 
	if (IS) 
		putpad(IS, 1); 
	if (VS) 
		putpad(VS, 1); 
	ttyset(1); 
} 
 
UnsetTerm() 
{ 
	ttyset(0); 
#ifdef LSRHS_KLUDGERY 
	if (CS) 
		deal_with_scroll(); 
#endif 
	if (VE) 
		putpad(VE, 1); 
	Placur(LI - 1, 0); 
	outchar('\n'); 
	flusho(); 
} 
 
#ifdef JOB_CONTROL 
PauseJove() 
{ 
	if (ModBufs()) 
		f_mess("There are modified buffers."); 
	UnsetTerm(); 
	ignore(kill(0, SIGTSTP)); 
	ResetTerm(); 
	ClAndRedraw(); 
} 
#endif 
 
Push() 
{ 
	int	pid; 
 
	switch (pid = fork()) { 
	case -1: 
		complain("Fork failed"); 
 
	case 0: 
		UnsetTerm(); 
		signal(SIGTERM, SIG_DFL); 
		signal(SIGINT, SIG_DFL); 
		execl(DefShell, "jove_sh", 0); 
		message("Execl failed"); 
		_exit(1); 
 
	default: 
	    { 
	    	int	(*old_int)() = signal(SIGINT, SIG_IGN); 
 
#ifdef PROCS 
		sighold(SIGCHLD); 
#endif 
	    	dowait(pid, (int *) 0); 
#ifdef PROCS 
		sigrelse(SIGCHLD); 
#endif 
	    	ResetTerm(); 
	    	ClAndRedraw(); 
	    	signal(SIGINT, old_int); 
	    } 
	} 
} 
 
int	OKXonXoff = 1;		/* fhsu - ^S and ^Q initially work */ 
 
static int	Intrc = DFLT_INTRC,	/* User settable interrupt character */ 
		DoneTTinit = 0; 
 
SetIntrc() 
{ 
	int	c; 
 
	message(FuncName()); 
	c = getch(); 
	if (DoneTTinit) { 
		tc2.t_intrc = c; 
		ttyset(1); 
	} else 
		Intrc = c; 
} 
 
ReInitTTY() 
{ 
	if (DoneTTinit) 
		ttyset(0);	/* Back to original settings */ 
	ttinit(); 
} 
 
#ifdef VENIX			/*  use raw mode for venix; eats too many chars*/ 
#define META_CHAR 
#endif 
 
#ifdef META_CHAR 
#define CHR_AT_A_TIME	RAW 
#else 
 
#ifdef  EUNICE 
#define CHR_AT_A_TIME	RAW 
#else 
#define CHR_AT_A_TIME	CBREAK 
#endif 
 
#endif META_CHAR 
 
ttinit() 
{ 
	DoneTTinit++; 
#ifdef TIOCSLTC 
	ignore(ioctl(0, TIOCGLTC, (char *) &ls1)); 
	ls2 = ls1; 
	ls2.t_suspc = (char) -1; 
	ls2.t_dsuspc = (char) -1; 
	ls2.t_flushc = (char) -1; 
	ls2.t_lnextc = (char) -1; 
#endif 
 
	/* Change interupt and quit. */ 
#ifndef VENIX 
	ignore(ioctl(0, TIOCGETC, (char *) &tc1)); 
#endif 
	tc2 = tc1; 
	tc2.t_intrc = Intrc; 
	tc2.t_quitc = (char) -1; 
	if (OKXonXoff) { 
		tc2.t_stopc = (char) -1; 
		tc2.t_startc = (char) -1; 
	} 
 
	ignore(gtty(0, &sg1)); 
	sg2 = sg1; 
 
	sg2.sg_flags &= ~(ECHO | CRMOD | RAW); 
	sg2.sg_flags |= CHR_AT_A_TIME; 
#ifdef BRLUNIX 
	sg2.sg_xflags &= ~(STALL | PAGE); 
#endif 
 
	ttyset(1); 
 
	ignorf(signal(SIGHUP, finish)); 
	ignorf(signal(SIGINT, finish)); 
	ignorf(signal(SIGQUIT, SIG_IGN)); 
	ignorf(signal(SIGBUS, finish)); 
	ignorf(signal(SIGSEGV, finish)); 
	ignorf(signal(SIGPIPE, finish)); 
	ignorf(signal(SIGTERM, SIG_IGN)); 
} 
 
/* If n is 0 reset to original modes */ 
 
ttyset(n) 
{ 
	static int	HaveSetup = 0; 
 
	if (HaveSetup == 0 && n == 0)	/* Try to reset before we've set! */ 
		return; 
#ifdef BRLUNIX 
#ifdef VENIX 
	ioctl(0, TIOCSETN, n == 0 ? (char *) &sg1 : (char *) &sg2); 
#else 
	ignore(stty(0, n == 0 ? (char *) &sg1 : (char *) &sg2)); 
#endif 
#else 
	ioctl(0, TIOCSETN, n == 0 ? (char *) &sg1 : (char *) &sg2); 
#endif 
 
	ioctl(0, TIOCSETC, n == 0 ? (char *) &tc1 : (char *) &tc2); 
 
#ifdef TIOCSLTC 
	ioctl(0, TIOCSLTC, n == 0 ? (char *) &ls1 : (char *) &ls2); 
#endif 
	HaveSetup = 1; 
} 
 
#ifdef LSRHS_KLUDGERY 
deal_with_scroll() 
{ 
	char	*pp; 
 
	pp = getenv("SCROLL"); 
	if (!pp || !strcmp(pp, "smooth")) 
		putstr("\033[?4h");	/* Put in smooth scroll. */ 
} 
#endif 
 
int	this_cmd, 
	last_cmd; 
 
dispatch(c) 
register int	c; 
{ 
	data_obj	*fp; 
 
	this_cmd = 0; 
	fp = mainmap[c]; 
 
	if (fp == 0) { 
		rbell(); 
		exp = 1; 
		exp_p = errormsg = 0; 
		message(NullStr); 
		return; 
	} 
	add_stroke(c); 
	ExecFunc(fp, 0); 
} 
 
int	LastKeyStruck, 
	MetaKey = 0, 
	NoMacPeekc = -1; 
 
PushBack(c) 
int	c; 
{ 
	return NoMacPeekc = c; 
} 
 
int	ASInterval = 300, 
	AScount = 0; 
 
getch() 
{ 
	int	c, 
		peeked = 0; 
	extern int	NumProcs; 
 
	if (MinorMode(Save) && ++AScount >= ASInterval) { 
		AScount = 0; 
		ASbuffers(); 
	} 
	if (Inputp) { 
		int	c; 
 
		if ((c = *Inputp++) != 0) 
			return c; 
		Inputp = 0; 
	} 
 
	if (stackp >= 0 && macstack[stackp]->Flags & EXECUTE) 
		c = MacGetc(); 
	else { 
		/* So messages that aren't error messages don't 
		   hang around forever */ 
		if (!UpdMesg && !Asking) {	/* Don't erase if we are asking */ 
			if (mesgbuf[0] && !errormsg) 
				message(NullStr); 
		} 
		redisplay(); 
#ifdef PROCS 
		if (NumProcs > 0) { 
			sigrelse(INPUT_SIG); 
			sigrelse(SIGCHLD); 
		} 
#endif 
		if ((c = NoMacPeekc) == -1) { 
			if ((c = getchar()) == EOF) 
				finish(SIGHUP); 
		} else { 
			peeked = 1; 
			NoMacPeekc = -1; 
		} 
#ifdef PROCS 
		if (NumProcs > 0) { 
			sighold(INPUT_SIG); 
			sighold(SIGCHLD); 
		} 
#endif 
		c &= 0377; 
		if (!MetaKey) 
			c &= 0177; 
		else if (c & 0200) { 
			ignore(Ungetc(c & 0177)); 
			c = '\033'; 
		} 
		if (!peeked) 
			if (KeyMacro.Flags & DEFINE) 
				MacPutc(c); 
	} 
	LastKeyStruck = c; 
	return c; 
} 
 
char	**argvp; 
 
UNIX_cmdline(argc, argv) 
char	*argv[]; 
{ 
	Buffer	*firstbuf = 0; 
	int	lineno = 0; 
 
	if (OKXonXoff) 
		message("Jonathan's Own Version of Emacs"); 
	else 
		message("Jonathan's Own Version of Emacs [XON/XOFF mode enabled]"); /*fhsu */ 
 
	*argv = (char *) 0; 
	argvp = argv + 1; 
 
	while (argc > 1) { 
		if (argv[1][0] != '-' && argv[1][0] != '+') { 
			SetBuf(do_find(curwind, argv[1])); 
			SetLine(next_line(curbuf->b_first, lineno)); 
			if (!firstbuf) 
				firstbuf = curbuf; 
			lineno = 0; 
		} else	switch (argv[1][1]) { 
			case 't': 
				++argv; 
				--argc; 
				exp_p = 1; 
				find_tag(argv[1]); 
				if (!firstbuf) 
					firstbuf = curbuf; 
				break; 
 
			case 'd': 
				++argv; 
				--argc; 
				break; 
 
			case 'l': 
				*(argv + 1) = (char *) 0; 
				argc = 1;	/* So we'll break */ 
				argvp = argv + 2; 
				break;			 
 
			case '0': 
			case '1': 
			case '2': 
			case '3': 
			case '4': 
			case '5': 
			case '6': 
			case '7': 
			case '8': 
			case '9': 
				lineno = atoi(&argv[1][1]) - 1; 
				break; 
 
			case 'j':	/* Ignore .joverc in HOME */ 
				break; 
		} 
		++argv; 
		--argc; 
	} 
 
	if (firstbuf) 
		SetBuf(do_select(curwind, firstbuf->b_name)); 
} 
 
#ifdef lint 
Ignore(a) 
	char *a; 
{ 
 
	a = a; 
} 
 
Ignorf(a) 
	int (*a)(); 
{ 
 
	a = a; 
} 
#endif 
 
/* VARARGS1 */ 
 
error(fmt, args) 
char	*fmt; 
{ 
	if (fmt) { 
		format(mesgbuf, fmt, &args); 
		UpdMesg++; 
	} 
	rbell(); 
	longjmp(mainjmp, ERROR); 
} 
 
/* VARARGS1 */ 
 
complain(fmt, args) 
char	*fmt; 
{ 
	if (fmt) { 
		format(mesgbuf, fmt, &args); 
		UpdMesg++; 
	} 
	rbell();	/* Always ring the bell now */ 
	longjmp(mainjmp, COMPLAIN); 
} 
 
/* VARARGS1 */ 
 
confirm(fmt, args) 
char	*fmt; 
{ 
	char	*yorn; 
 
	format(mesgbuf, fmt, &args); 
	yorn = ask((char *)0, mesgbuf); 
	if (*yorn != 'Y' && *yorn != 'y') 
		longjmp(mainjmp, COMPLAIN); 
} 
 
exit(status) 
{ 
	flusho(); 
	_exit(status); 
} 
 
int	RecDepth = 0; 
 
Recur() 
{ 
	RecDepth++; 
	UpdModLine++; 
	DoKeys(1);	/* 1 means not first time */ 
	UpdModLine++; 
	RecDepth--; 
} 
 
jmp_buf	mainjmp; 
 
DoKeys(nocmdline) 
{ 
	int	c; 
	jmp_buf	savejmp; 
 
	copynchar((char *) savejmp, (char *) mainjmp, sizeof savejmp); 
 
	switch (setjmp(mainjmp)) { 
	case 0: 
		if (!nocmdline) 
			UNIX_cmdline(iniargc, iniargv); 
		break; 
 
	case QUIT: 
		if (RecDepth == 0) { 
#ifdef PROCS 
			NoProcs(); 
#endif 
			if (ModBufs() && *ask("No", "Modified buffers exist; leave anyway? ") != 'y') 
				break; 
		} 
		copynchar((char *) mainjmp, (char *) savejmp, sizeof mainjmp); 
		return; 
 
	case ERROR: 
		getDOT();	/* God knows what state linebuf was in */ 
 
	case COMPLAIN: 
		IOclose(); 
		Getchar = getch; 
		errormsg++; 
		FixMacros(); 
		Asking = 0;		/* Not anymore we ain't */ 
		curwind->w_bufp = curbuf; 
		redisplay(); 
		break; 
	} 
 
	this_cmd = last_cmd = 0; 
 
	for (;;) { 
		exp = 1; 
		exp_p = 0; 
		last_cmd = this_cmd; 
		key_strokes[0] = 0; 
cont: 
		c = getch(); 
		if (c == -1) 
			continue; 
	 	dispatch(c); 
		if (this_cmd == ARG_CMD) 
			goto cont; 
	} 
} 
 
int	Crashing = 0; 
 
int	(*Getchar)() = getch; 
 
char ** 
scanvec(args, str) 
register char	**args, 
		*str; 
{ 
	while (*args) { 
		if (strcmp(*args, str) == 0) 
			return args; 
		args++; 
	} 
	return 0; 
} 
 
main(argc, argv) 
char	*argv[]; 
{ 
	extern char searchbuf[]; 
	extern int CreatMode; 
	char	*home; 
 
	errormsg = 0; 
 
	iniargc = argc; 
	iniargv = argv; 
 
	searchbuf[0] = '\0'; 
 
	if (setjmp(mainjmp)) { 
		printf("Pre-error: \"%s\"; tell fhsu@uw-june\n", mesgbuf); 
		finish(0); 
	} 
 
#ifdef PROCS 
	{ 
		int	proc_child(); 
 
		ignorf(signal(SIGCHLD, proc_child)); 
	} 
#endif 
 
	getTERM(); 
	InitCM(); 
	settout(); 
	make_scr();	/* Do this before making zero */ 
	tmpinit();	/* Init temp file */ 
	MacInit();	/* Initialize Macros */ 
	winit();	/* Initialize Window */ 
 
#ifdef PROCS 
	pinit();	/* Pipes/process initialization */ 
#endif 
 
	curbuf = do_select(curwind, Mainbuf); 
	FundMode(); 
 
	ignore(joverc(JOVERC)); 
	if (!scanvec(argv, "-j") && (home = getenv("HOME"))) { 
		char	tmpbuf[100]; 
 
		ignore(joverc(sprintf(tmpbuf, "%s/.joverc", home))); 
	} 
 
	DefMinor = curbuf->b_minor;	/* Inherit from buffer Main. */ 
 
	ttinit();	/* Initialize terminal (after ~/.joverc) */ 
 
	putpad(CL, 1); 
	if (IS) 
		putpad(IS, 1); 
	if (VS) 
		putpad(VS, 1); 
 
	/* All new buffers will have these flags when created. */ 
#ifdef CHDIR 
	{ 
		char	**argp; 
 
		if ((argp = scanvec(argv, "-d")) && (argp[1][0] == '/')) 
			setCWD(argp[1]); 
		else 
			getCWD();	/* After we setup curbuf in case we have to getwd() */ 
	} 
#endif 
 
	RedrawDisplay();	/* Start the redisplay process */ 
	if ((DefShell = getenv("SHELL")) == 0) 
		DefShell = DFLT_SH; 
#ifdef JOVE4.2 
	CreatMode = umask((int)022);	/* get old mask */ 
	(void) umask(CreatMode);	/* restore it... */ 
	CreatMode = (~CreatMode)&0666;	/* new mask for files */ 
#endif 
	DoKeys(0); 
	finish(0); 
} 
 
