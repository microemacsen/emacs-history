/* s-header for VMS
   Copyright (C) 1986 Free Software Foundation, Inc.

This file is part of GNU Emacs.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.  No author or distributor
accepts responsibility to anyone for the consequences of using it
or for whether it serves any particular purpose or works at all,
unless he says so in writing.  Refer to the GNU Emacs General Public
License for full details.

Everyone is granted permission to copy, modify and redistribute
GNU Emacs, but only under the conditions described in the
GNU Emacs General Public License.   A copy of this license is
supposed to have been given to you along with GNU Emacs so you
can know your rights and responsibilities.  It should be in a
file named COPYING.  Among other things, the copyright notice
and this notice must be preserved on all copies.  */

/*
 *	Define symbols to identify the version of Unix this is.
 *	Define all the symbols that apply correctly.
 */

/* #define UNIPLUS */
/* #define USG5 */
/* #define USG */
/* #define BSD4_1 */
/* #define BSD4_2 */
/* #define BSD4_3 */
/* #define BSD */
/* #define UMAX4_2 */
/* #define UMAX */
#ifndef VMS		    /* Decus cpp doesn't define this but VAX C does */
#define VMS
#endif /* VMS */
/* Note that this file is used indirectly via s-vms4-0.h, or some other
   such file.  These other files define a symbol VMS4_0, VMS4_2, etc.  */

/* SYSTEM_TYPE should indicate the kind of system you are using.
 It sets the Lisp variable system-type.  */

#define SYSTEM_TYPE "vax-vms"

/* NOMULTIPLEJOBS should be defined if your system's shell
 does not have "job control" (the ability to stop a program,
 run some other program, then continue the first one).  */

/* #define NOMULTIPLEJOBS */

/* Emacs can read input using SIGIO and buffering characters itself,
   or using CBREAK mode and making C-g cause SIGINT.
   The choice is controlled by the variable interrupt_input.
   Define INTERRUPT_INPUT to make interrupt_input = 1 the default (use SIGIO)

   SIGIO can be used only on systems that implement it (4.2 and 4.3).
   CBREAK mode has two disadvatages
     1) At least in 4.2, it is impossible to handle the Meta key properly.
        I hear that in system V this problem does not exist.
     2) Control-G causes output to be discarded.
        I do not know whether this can be fixed in system V.

   Another method of doing input is planned but not implemented.
   It would have Emacs fork off a separate process
   to read the input and send it to the true Emacs process
   through a pipe.
*/

/* #define INTERRUPT_INPUT */

/* Letter to use in finding device name of first pty,
  if system supports pty's.  'a' means it is /dev/ptya0  */

#define FIRST_PTY_LETTER 'a'

/*
 *	Define HAVE_TIMEVAL if the system supports the BSD style clock values.
 *	Look in <sys/time.h> for a timeval structure.
 */

/* #define HAVE_TIMEVAL */

/*
 *	Define HAVE_SELECT if the system supports the `select' system call.
 */

/* #define HAVE_SELECT */

/*
 *	Define HAVE_PTYS if the system supports pty devices.
 */

/* #define HAVE_PTYS */

/* Define HAVE_SOCKETS if system supports 4.2-compatible sockets.  */

/* #define HAVE_SOCKETS */

/*
 *	Define NONSYSTEM_DIR_LIBRARY to make Emacs emulate
 *      The 4.2 opendir, etc., library functions.
 */

#define NONSYSTEM_DIR_LIBRARY

/* Define this symbol if your system has the functions bcopy, etc. */

/* #define BSTRING */

/* subprocesses should be defined if you want to
   have code for asynchronous subprocesses
   (as used in M-x compile and M-x shell).
   This is generally OS dependent, and not supported
   under most USG systems. */

/* #define subprocesses */

/* If your system uses COFF (Common Object File Format) then define the
   preprocessor symbol "COFF". */

/* #define COFF */

/* define MAIL_USE_FLOCK if the mailer uses flock
   to interlock access to /usr/spool/mail/$USER.
   The alternative is that a lock file named
   /usr/spool/mail/$USER.lock.  */

/* #define MAIL_USE_FLOCK */

/* Define CLASH_DETECTION if you want lock files to be written
   so that Emacs can tell instantly when you try to modify
   a file that someone else has modified in his Emacs.  */

/* #define CLASH_DETECTION */

/* Define the maximum record length for print strings, if needed. */

#define MAX_PRINT_CHARS 300


/* Here, on a separate page, add any special hacks needed
   to make Emacs work on this system.  For example,
   you might define certain system call names that don't
   exist on your system, or that do different things on
   your system and must be used only through an encapsulation
   (Which you should place, by convention, in sysdep.c).  */

/* Do you have the shareable library bug?  If you link with a shareable
   library that contains psects with the NOSHR attribute and also refer to
   those psects in your program, the linker give you a private version of
   the psect which is not related to the version used by the shareable
   library.  The end result is that your references to variables in that
   psect have absolutely nothing to do with library references to what is
   supposed to be the same variable.  If you intend to link with the standard
   C library (NOT the shareable one) you don't need to define this.  (This
   is NOT fixed in V4.4...) */

#define SHAREABLE_LIB_BUG

/* Partially due to the above mentioned bug and also so that we don't need
   to require that people have a shareable C library, the default for Emacs
   is to link with the non-shared library.  If you wan tto link with the
   shared library, define this and remake xmakefile and fileio.c. This allows
   us to ship a guaranteed executable image. */

/* #define LINK_CRTL_SHARE */

/* Define this if you want to read the file SYS$SYSTEM:SYSUAF.DAT for user
   information.  If you do use this, you must either make SYSUAF.DAT world 
   readable or install Emacs with SYSPRV.  */

/* #define READ_SYSUAF */

/* On VMS these have a different name */

#define index strchr
#define rindex strrchr
#define unlink delete

/* Hide these names so that we don't get linker errors */
#define malloc sys_malloc
#define free sys_free
#define realloc sys_realloc

/* On VMS we want to avoid reading and writing very large amounts of
   data at once, so we redefine read and write here. */

#define read sys_read
#define write sys_write

/* sys_creat just calls the real creat with additional args of
   "rfm=var", "rat=cr" to get "normal" VMS files... */
#define creat sys_creat

/* fwrite forces an RMS PUT on every call.  This is abysmally slow, so
   we emulate fwrite with fputc, which forces buffering and is much
   faster! */
#define fwrite sys_fwrite

/* getuid only returns the member number, which is not unique on most VMS
   systems.  We emulate it with (getgid()<<16 | getuid()). */
#define getuid sys_getuid

/* If user asks for TERM, check first for EMACS_TERM.  */
#define getenv sys_getenv
  
/* Standard C abort is less useful than it should be. */
#define abort sys_abort

/* Case conflicts with C library fread. */
#define Fread F_read

/* Case conflicts with C library srandom. */
#define Srandom S_random

/* Cause initialization of vmsfns.c to be run.  */
#define SYMS_SYSTEM syms_of_vmsfns ()

/* VAXCRTL access doesn't deal with SYSPRV very well (among other oddites...)
   Here, we use $CHKPRO to really determine access. */
#define access sys_access

#define	PAGESIZE	512

#define	_longjmp	longjmp
#define	_setjmp		setjmp

globalref char sdata[];
#define DATA_START (((int) sdata + 512 + 511) & ~511)
#define TEXT_START 512

/* Baud-rate values from tty status are not standard.  */

#define BAUD_CONVERT  \
{ 0, 50, 75, 110, 134, 150, 300, 600, 1200, 1800, \
  2000, 2400, 3600, 4800, 7200, 9600, 19200 }

#define PURESIZE 130000