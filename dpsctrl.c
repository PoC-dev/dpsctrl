/*
 * vi: tabstop=4
 *
 * Copyright 2004-2012 poc@pocnet.net
 *
 * This file is part of dpsctrl.
 *
 * Dpsctrl is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Dpsctrl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dpsctrl; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * or get it at http://www.gnu.org/licenses/gpl.html
 *
 * Many thanks to Tooltime for a little bitshifting-aid!
 *
 * $Id: dpsctrl.c,v 1.17 2018/12/23 23:45:55 poc Exp $
 *
 * $Log: dpsctrl.c,v $
 * Revision 1.17  2018/12/23 23:45:55  poc
 * Date strings
 *
 * Revision 1.16  2011-11-13 16:43:59  poc
 * Resolved most FIXMEs
 *
 * Revision 1.15  2011-11-01 20:18:29  poc
 * Reorganized manpage,
 * synced manpage (section BUGS) with BUGS-file,
 * random doc fixes,
 * marked FIXME in dpsctrl.c for fixing not-exact-values-bug.
 *
 * Revision 1.14  2011-10-27 20:26:26  poc
 * Fixed SIGSEGV when trying to open nonexistant serial port
 * We also prefer DPSDEVICE env var before the hardcoded default
 *
 * Revision 1.13  2011-10-27 19:23:40  poc
 * Cleanup,
 * dpkg-buildpackage is now compatible with debian squeeze
 *
 * Revision 1.12  2009-09-16 19:49:22  poc
 * Added debian files,
 * language corrections,
 * cosmetics
 *
 * Revision 1.11  2006/09/17 18:08:12  poc
 * Fixed long standing bug: dpsctrl crashed when compiled with curses support but called from command line
 * Be more verbose with certain errors
 *
 * Revision 1.10  2006/08/10 14:42:25  poc
 * Cosmetics
 *
 * Revision 1.9  2005/01/26 00:05:15  poc
 * Bugfix: Kleinere DPS können natürlich auch mehr als 2,5A
 *
 * Revision 1.8  2004/10/04 19:31:55  poc
 * Kompilieren ohne Curses geht jetzt auch wieder
 * Vorbereitungen für Leistungskram
 * Leistung wird jetzt auch ausgegeben (wenn auch berechnet)
 *
 * Revision 1.7  2004/06/22 19:52:36  poc
 * okbox()
 * Fenstertitel Zentrierungsberechnung gefixt
 * Nur Zahlen werden jetzt in den Dialogfeldern akzeptiert
 * Check auf vernünftige Werte im Cursesmode
 *
 * Revision 1.6  2004/06/21 21:19:47  poc
 * Cursesmode weiter verbessert
 *
 * Revision 1.5  2004/06/21 20:37:21  poc
 * serase()
 * Debugmode entfernt
 * Pakete in Statuszeile
 * Kommentare vereinheitlicht
 * Rahmen um Hauptfenster
 * getval()
 * oncemode ergänzt
 * Eingabe von Werten aus Curses heraus
 *
 * Revision 1.4  2004/06/18 09:40:22  poc
 * Quit eingebaut
 * Statusline gibt nun den Zustand des Remotebuttons aus
 *
 * Revision 1.3  2004/06/18 00:18:46  poc
 * Cursesmode eingebaut (und diesmal richtig)
 * Kleinere Cleanups
 *
 * Revision 1.2  2004/06/16 14:15:33  poc
 * Kleinere Typos gefixt
 * Defineumbenennung zwecks Kollisionen mit Curses
 * Cursessupport begonnen
 *
 * Revision 1.1  2004/05/18 09:21:44  poc
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "dpsctrl.h"

#ifdef USE_CURSES
#include <ncurses.h>
#endif

/* Prototypes */
void help (char *progname);
int opendev (char *file);
unsigned char getbyte (void);
int fill_inbuf (struct dpsinbuf *inbuf);
int sendpkt(int type, unsigned int optval);
int check_overtemp(struct dpsinbuf *inbuf);
struct splitfloat adjval(float have, float want, char lim);
int jog(struct splitfloat *intfloat);
void cleanup();
#ifdef USE_CURSES
int init_curses();
int okbox(char *winstring);
void serase();
float getval(char *winstring);
#endif

/* Globals. Ugly but sending all things
 * to all functions would be way more ugly.
 */
int					myerrno, fd, cursesmode=0;
struct parsedargs	args;
char				*versionstring="Dpsctrl 2.5 by poc@pocnet.net";
#ifdef USE_CURSES
WINDOW				*mainWin=NULL, *statusLine=NULL;
#endif

/*---------------------------------------------------------------------------*/

/* What shall we do if we exit cleanly? */

void cleanup() {
	#ifdef USE_CURSES
	if ( cursesmode == 1 ) {
		erase();
		endwin();
	}
	#endif
}

/*---------------------------------------------------------------------------*/

void help (char *progname) {
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "%s [-d] [-h] [-v] [-l] [-r] [-u<n>] [-U<n>] [-I<n>] [-o]\n\n", progname);
	fprintf(stderr, "Description:\n");
	fprintf(stderr, "-h                   This help.\n");
	fprintf(stderr, "-v                   Show version.\n");
	fprintf(stderr, "-r                   Read continuously from DPS.\n");
	fprintf(stderr, "-1                   Read once from DPS and exit.\n");
	fprintf(stderr, "-u<n>                Set output voltage to n Volts.\n");
	fprintf(stderr, "-U<n>                Limit voltage (U) to n V.\n");
	fprintf(stderr, "-I<n>                Limit current (I) to n mA.\n");
	fprintf(stderr, "-o                   Toggle output relay.\n\n");
}

/*---------------------------------------------------------------------------*/

/* Open serial device and initialize it with the right parameters:
 * 1200-N-8-1 according to the docs. To be safe, we also enable both RTS and
 * DTR lines. Ah, and we ignore DCD, even if the DPS sometimes makes similar
 * noises (like a modem connecting). :-)
 */

int opendev (char *file) {
	struct termios	settings;
	int				tmp;

    /* Open Device */
	fd = open(file, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd == -1) {
		perror("opendev()");
		return(-1);
	}

    /* Adjust parameters */
	settings.c_iflag = IGNBRK | IGNPAR;
	settings.c_oflag = 0;
	settings.c_cflag = CS8 | CREAD | CLOCAL | B1200 | HUPCL;
	settings.c_lflag = 0;
	settings.c_cc[VTIME]=20;	/* Read Timeout 2s */
	settings.c_cc[VMIN]=0;		/* Passthru all chars immediately */

	if ( tcsetattr(fd, TCSANOW, &settings) == -1 ) {
		perror("tcsetattr");
		return(-1);
	}

    /* Pull RTS high. */
	if ( ioctl(fd, TIOCMGET, &tmp) == -1 ) {
		perror("TIOCMGET");
		return(-1);
	}

	/* What the heck is this? Oops, now you know that I copied this example
	 * from somwhere. And no Darl, surely not from SCO Unix Sources, which I
	 * don't have nor want.
	 */
	tmp &= ~TIOCM_RTS | ~TIOCM_DTR;
	if ( ioctl(fd, TIOCMSET, &tmp) == -1 ) {
		perror("TIOCMSET");
		return(-1);
	}

	return(fd);
}
/*---------------------------------------------------------------------------*/

/* Read one char out of fd. A test which only detected the starting sequence
 * byte by byte while reading the rest as a bigger block failed. My serial
 * port never delivered the needed 15 Bytes, but 14 or 13. Perhaps this may
 * be related to a buggy serial port or a buggy serial driver (my box in
 * the working room runs linux 1.1.59 - it's small!)
 */

unsigned char getbyte (void) {
	unsigned char	inbuf;

	if ( read(fd, &inbuf, 1) == 0 ) {
		if ( cursesmode == 0 )
			fprintf(stderr, "Timeout while reading serial port.\n");
#ifdef USE_CURSES
		else {
			serase();
			mvwprintw(statusLine, 0, 1, "Timeout while reading serial port.");
			wrefresh(statusLine);
			sleep(1);
#endif
		}
		myerrno = -1;
		return(0);
	} else {
		myerrno = 0;
		return(inbuf);
	}
}

/*---------------------------------------------------------------------------*/

/* We read until we've found the byte sequence which indicates a packet start.
 * We must read byte by byte since the DPS sends an even byte count. If we
 * would read words instead of bytes, we'll wait forever for a packet start.
 * When we've found the starting sequence, fill our inbuf-struct.
 *
 */

int fill_inbuf (struct dpsinbuf *inbuf) {
	int				retval=-1;

	while ( retval == -1 ) {
		if ( getbyte() == PKT_STRT1 ) {
			if ( getbyte() == PKT_STRT2 ) {
				retval = 0;
				inbuf->u = (getbyte() << 8) | getbyte();
				inbuf->u = inbuf->u / 100;
				inbuf->i = (getbyte() << 8) | getbyte();
				inbuf->p = (getbyte() << 8) | getbyte();
				inbuf->lim_u = (getbyte() << 8) | getbyte();
				inbuf->lim_u = inbuf->lim_u / 100;
				inbuf->lim_i = (getbyte() << 8) | getbyte();
				inbuf->lim_p = (getbyte() << 8) | getbyte();
				inbuf->flags = getbyte();
			}
		}
	}
	return(retval);
}

/*---------------------------------------------------------------------------*/

/* Assemble a packet and send it on it's journey thru the wire */

int sendpkt(int type, unsigned int optval) {
	unsigned char		outbuf[4];
	int					retval = 0, written;

	outbuf[0] = PKT_STRT1;
	outbuf[1] = PKT_STRT2;
	outbuf[2] = type;
	outbuf[3] = optval;

	#ifdef USE_CURSES
	if ( cursesmode == 1 ) {
		/* Display a nice representation of sent packets in the statusline */
		serase();
		mvwprintw(statusLine, 0, 1, "sending: %#2x %#2x %#2x %#2x",
			outbuf[0], outbuf[1], outbuf[2], outbuf[3]);
		wrefresh(statusLine);
	}
	#endif

	written = write(fd, outbuf, 4);

	if ( cursesmode == 0 && written < 0 )
		perror("write");

	if ( written != 4 )
		retval = -1;

	/* Poor DPS gets confused if packets arrive too fast. (Or my serial port
	 * is buggy and swallows random bytes). So, we sleep a bit after saying
	 * byebye to our packet.
	 */
	usleep(333333);

	return(retval);
}

/*---------------------------------------------------------------------------*/

/* Checks for Overheating and switches the relay off */

int check_overtemp(struct dpsinbuf *inbuf) {
	int					retval = 0;

	if ( inbuf->flags & DPS_FLG_OVERTEMP ) {
		if ( cursesmode == 0 )
			fprintf(stderr, "Overtemp! ");
		retval = 1;

		if ( inbuf->flags & DPS_FLG_IO ) {
			if ( cursesmode == 0 )
				fprintf(stderr, "Switching off relais.");
			sendpkt(PKT_KEYPRESS, DPS_KEY_IO);
		}

		if ( inbuf->flags & DPS_FLG_REMOTE && cursesmode == 0 )
			fprintf(stderr, "Please switch off relais.");


	if ( cursesmode == 0 )
		fprintf(stderr, "\n");
	}

	return(retval);
}

/*---------------------------------------------------------------------------*/

/* We split a float value into coarse- and fine steps. Theoretically this
 * could be done with rint() and a little bit subtracting. To keep us
 * small, we do that by hand and avoid linking to m.
 *
 * Accordig to the docs, the functions have these stepwidths:
 * 
 *  Button | Coarse | Fine
 * --------+--------+-------
 *    u    |  1  V  | 0,01V
 *    U    |  1  V  | 1   V
 *    I    |  0,1A  | 0,01A
 *    P    |  1  W  | 1   W
 */

struct splitfloat adjval(float have, float want, char lim) {
	float				diff;
	struct splitfloat	intfloat;
	char				buf[8];
	struct {	char	*pre;
				char	*post;
			}			bufptr;
	
	diff = want - have;

	/* DPS wants current-limit in 100mA steps and not in 1A */
	if ( lim == 'I' )
		diff = diff / 100;

	/* Initialize buffer */
	buf[0] = 0;
	sprintf(buf, "%.2f", diff);
	
	/* Set pointers */
	bufptr.pre = buf;
	bufptr.post = strchr(buf, '.');
	bufptr.post[0] = 0;
	bufptr.post++;

	/* Convert to integers */
	intfloat.pre = atoi(bufptr.pre);
	intfloat.post = atoi(bufptr.post);

	/* Negative values at whole should give negative post-decimal values */
	if ( diff < 0 )
		intfloat.post = intfloat.post * -1;

	/* Sorry, only integers here */
	if ( lim == 'U' )
		intfloat.post = 0;

	/* Correct the stepwidth, fine steps change current in 10mA steps. */
	if ( lim == 'I' )
		intfloat.post = intfloat.post / 10;

	return(intfloat);
}

/*---------------------------------------------------------------------------*/

/* Send packets emulating jog-shuttling. Let's spin the wheel! */

int jog(struct splitfloat *intfloat) {
	int					retval = 0;

	/* No coarse steps, no work */
	if ( intfloat->pre != 0 ) {
		retval += sendpkt(PKT_KEYPRESS, DPS_KEY_N);

		/* Negative values -> jog left */
		if ( intfloat->pre < 0 )
			retval += sendpkt(PKT_JOG_DOWN, intfloat->pre * -1);

		/* Positive values -> jog right */
		if ( intfloat->pre > 0 )
			retval += sendpkt(PKT_JOG_UP, intfloat->pre);
	}

	/* No fine steps, no work */
	if ( intfloat->post != 0 ) {
		retval += sendpkt(PKT_KEYPRESS, DPS_KEY_F);

		/* Negative values -> jog left */
		if ( intfloat->post < 0 )
			retval += sendpkt(PKT_JOG_DOWN, intfloat->post * -1);

		/* Positive values -> jog right */
		if ( intfloat->post > 0 )
			retval += sendpkt(PKT_JOG_UP, intfloat->post);

		retval += sendpkt(PKT_KEYPRESS, DPS_KEY_N);
	}

	return(retval);
}

#ifdef USE_CURSES
/*---------------------------------------------------------------------------*/

/* Initialize Curses und Windowing environment */

int init_curses() {

	/* Init Curses */
	if ( initscr() == NULL)
		return(-1);

	if ( cbreak() == ERR )
		return(-1);
		
	if ( noecho() == ERR )
		return(-1);


	/* Create main window */
	mainWin = newwin(LINES - 1, 0, 0, 0);
	if ( mainWin == NULL )
		return(-1);

	box(mainWin, 0, 0);
	mvwprintw(mainWin, 0, (COLS - strlen(versionstring) - 2) / 2, " %s ",
		versionstring);
	nodelay(mainWin, TRUE);
	wrefresh(mainWin);

	/* Create status line */
	statusLine = newwin(1, 0, LINES - 1, 0);
	if ( statusLine == NULL )
		return(-1);

	wattron(statusLine, A_REVERSE);
	wrefresh(statusLine);

	return(0);
}

/*---------------------------------------------------------------------------*/

/* Draw a new window and ask for value */

float getval(char *winstring) {
	WINDOW				*tempWin;
	char				buf[8], *msg="Please enter new value: ";
	float				retval=0;
	int					key='#', count=0, winwid=35;

	buf[0] = '\0';

	tempWin = newwin(3, winwid, (LINES - 3) / 2, (COLS - winwid) / 2);
	if ( tempWin == NULL )
		return(-1);
	
	nodelay(tempWin, FALSE);
	box(tempWin, 0, 0);
	if ( winstring != NULL )
		mvwprintw(tempWin, 0, (winwid - strlen(winstring) - 2) / 2, " %s ",
			winstring);
	mvwprintw(tempWin, 1, 2, "%s", msg);

	while ( key != '\n' || count > 7 ) {
		wrefresh(tempWin);
		key = wgetch(tempWin);
		if ( isdigit(key) != 0 || key == '.' ) {
			waddch(tempWin, key);
			strncat(buf, (char *)&key, 1);
			count++;
		}
	}

	retval = atof(buf);
	
	werase(tempWin);
	wrefresh(tempWin);
	delwin(tempWin);

	return(retval);
}

/*---------------------------------------------------------------------------*/

/* Draw a new window and complain */

int okbox(char *winstring) {
	WINDOW				*tempWin;
	char				*msg="Error", *press="Press any key to continue...";
	int					winwid=45;

	tempWin = newwin(7, winwid, (LINES - 7) / 2, (COLS - winwid) / 2);
	if ( tempWin == NULL )
		return(-1);
	
	nodelay(tempWin, FALSE);
	box(tempWin, 0, 0);
	mvwprintw(tempWin, 0, (winwid - strlen(msg) - 2) / 2, " %s ", msg);
	mvwprintw(tempWin, 5, winwid - strlen(press) - 2, "%s", press);
	if ( winstring != NULL )
		mvwprintw(tempWin, 2, 2, "%s", winstring);
	wrefresh(tempWin);

	wgetch(tempWin);

	werase(tempWin);
	wrefresh(tempWin);
	delwin(tempWin);

	return(0);
}

/*---------------------------------------------------------------------------*/

/* Erases the status line with true spaces
 *  - can't remember how to accomplish that properly
 */

void serase() {
	int					i;

	mvwprintw(statusLine, 0, 0, " ");
	for (i = 0; i < COLS; i++)
		waddch(statusLine, ' ');
}

#endif

/*---------------------------------------------------------------------------*/

int main (int argc, char *argv[]) {
	struct dpsinbuf		inbuf;
	struct optioncount	optcount;
	struct splitfloat	intfloat;
	int					retval=0;
	char				*ser_dev;
	struct stat			statbuf;
	#ifdef USE_CURSES
	char				key, *statustext="u - Set Ouput Voltage | U, I - Set Limit | o - Toggle Relais | q - Quit         ";
	#endif

	
	/* Fill structs with reasonable defaults */
	args.help = args.version = args.read = args.io = args.once = cursesmode =\
		args.val_u = args.val_u_lim = args.val_i_lim = args.loop =\
		optcount.u = optcount.u_lim = optcount.i_lim = 0;

	/* What to do if we must exit */
	atexit(cleanup);

	/* Do something if no parameters given */
	if ( argc < 2 )
		#ifndef USE_CURSES
		args.read = 1;
		#else
		cursesmode = 1;
		#endif

    /* Parse arguments. We set variables instead of directly calling functions
	 * in the getopt loop to avoid multiple calls to functions and to allow
	 * to set multiple parameters concurrently.
	 */
    while ( retval != -1 ) {
        retval = getopt(argc, argv, "hvlr1u:U:I:o");

        switch (retval) {
		case 'h':
			args.help = 1;
            break;

        case 'v':
			args.version = 1;
            break;

		case 'r':
			args.read = 1;
			break;

		case 'l':
			args.loop = 1;
			break;

		case '1':
			args.read = 1;
			args.once = 1;
			break;

		case 'u':
			optcount.u++;
			if ( optcount.u > 1 )
				fprintf(stderr, "Can set voltage only once, using first value.\n");
			else
				args.val_u = atof(optarg);
			break;

		case 'U':
			optcount.u_lim++;
			if ( optcount.u_lim > 1 )
				fprintf(stderr, "Can limit voltage only once, using first value.\n");
			else
				args.val_u_lim = atof(optarg);
			break;

		case 'I':
			optcount.i_lim++;
			if ( optcount.i_lim > 1 )
				fprintf(stderr, "Can limit current only once, using first value.\n");
			else
				args.val_i_lim = atof(optarg);
			break;

		case 'o':
			args.io = 1;
			break;
        }
    }

	retval = 0;

	/* Show some limitations */
	if ( args.val_u < 0 ) {
		fprintf(stderr, "DPS cannot provide negative voltages.\n");
		exit(1);
	}
	if ( args.val_u_lim < 0  || args.val_i_lim < 0 ) {
		fprintf(stderr, "Please explain to me: How should I set negative limits?\n");
		exit(1);
	}
	if ( args.val_u > 81 || args.val_u_lim > 81 ) {
		fprintf(stderr, "No known DPS can supply significantly more than 80V.\n");
		exit(1);
	}
	if ( args.val_i_lim > 10000 ) {
		fprintf(stderr, "No known DPS can supply more than 10A.\n");
		exit(1);
	}

	if ( args.help == 1 ) {
		help(argv[0]);
		exit(0);
	}

	if ( args.version == 1 )  {
		printf("%s\n", versionstring);
		exit(0);
	}

	/* Find out proper serial device */
	if (!(ser_dev = getenv("DPSDEVICE"))) {
			ser_dev = (SER_DEV);
	}

	if (stat(ser_dev, &statbuf) != 0) {
		fprintf(stderr, "Error opening serial port %s", ser_dev);
		perror(" ");
		exit(1);
	}

	if (S_ISCHR(statbuf.st_mode) == 0) {
		fprintf(stderr, "Error opening serial port %s: Not a character-device.\n", ser_dev);
		exit(1);
	}

	/* And open */
	fd = opendev(ser_dev);
	if ( fd < 0 ) {
		perror("Error opening serial port");
		exit(1);
	}

	/* Read once, so we have the current DPS readings */
	retval = fill_inbuf(&inbuf);
	retval = check_overtemp(&inbuf);

	/* Check if we must write something */
	if ( optcount.u_lim == 1  || optcount.i_lim == 1 ||\
		optcount.u == 1 || args.io == 1 ) {
		if ( inbuf.flags & DPS_FLG_REMOTE ) {
			/* Set voltage */
			/* FIXME: Here is the right place to fix bug #1 */
			if ( optcount.u == 1 ) {
				retval = sendpkt(PKT_KEYPRESS, DPS_KEY_U);
				intfloat = adjval(inbuf.u, args.val_u, '0');
				retval = jog(&intfloat);
			}

			/* Limit to a limit if limiting is wanted */
			if ( optcount.u_lim == 1 ) {
				retval = sendpkt(PKT_KEYPRESS, DPS_KEY_LIM_U);
				intfloat = adjval(inbuf.lim_u, args.val_u_lim, 'U');
				retval = jog(&intfloat);
				retval = sendpkt(PKT_KEYPRESS, DPS_KEY_LIM_ENT);
			}
			if ( optcount.i_lim == 1 ) {
				retval = sendpkt(PKT_KEYPRESS, DPS_KEY_LIM_I);
				intfloat = adjval(inbuf.lim_i, args.val_i_lim, 'I');
				retval = jog(&intfloat);
				retval = sendpkt(PKT_KEYPRESS, DPS_KEY_LIM_ENT);
			}

			/* Let the relais click */
			if ( args.io == 1 )
				retval = sendpkt(PKT_KEYPRESS, DPS_KEY_IO);

			/* Report failure to set voltage to user input */
			retval = fill_inbuf(&inbuf);
			if ( args.val_u >= 0 && inbuf.u != args.val_u )
				retval = -1;

		} else {
			fprintf(stderr, "DPS isn't accepting, press REM button.\n");
		}

		/* Flush all pending input so far */
		tcflush(fd, TCIOFLUSH);
	}

	/* Read... */
	if ( args.read == 1 ) {
		/* Read once - good for batches! */
		if ( args.once == 1 ) {
			printf("%.2fV\t%.0fmA\n", inbuf.u, inbuf.i);
		} else {
			while ( 1 ) {
				retval = fill_inbuf(&inbuf);
				if ( check_overtemp(&inbuf) == 1 )
					break;

				printf("%.2fV\t%.0fmA\t(%.0fV\t%.0fmA)\n",
					inbuf.u, inbuf.i, inbuf.lim_u, inbuf.lim_i);
				
				sleep(1);
			}
		}
	}

	#ifdef USE_CURSES
	/* Interactive mode - used only when started without parameters */
	if ( cursesmode == 1 ) {

		if ( init_curses() != 0 ) {
			fprintf(stderr, "Error initializing curses environment.\n");
			exit(1);
		}

		/* Basic Screen contents */
		mvwprintw(mainWin, 5, 5, "Current Values:");
		mvwprintw(mainWin, 7, 5, "Voltage:");
		mvwprintw(mainWin, 7, 30, "Current:");
		mvwprintw(mainWin, 7, 55, "Power:");

		mvwprintw(mainWin, 12, 5, "Current Limits:");
		mvwprintw(mainWin, 14, 5, "Voltage:");
		mvwprintw(mainWin, 14, 30, "Current:");
		mvwprintw(mainWin, 14, 55, "Power:");

		mvwprintw(mainWin, 19, 5, "DPS Flags:");

		while ( 1 ) {
			retval = fill_inbuf(&inbuf);

			/* Fight redraw errors */
			mvwprintw(mainWin, 7, 14, "        ");
			mvwprintw(mainWin, 7, 38, "        ");
			mvwprintw(mainWin, 7, 62, "        ");
			mvwprintw(mainWin, 14, 14,  "     ");
			mvwprintw(mainWin, 14, 38, "       ");
			mvwprintw(mainWin, 14, 62, "       ");
			mvwprintw(mainWin, 19, 33, "                          ");

			/* Barf out values */
			mvwprintw(mainWin, 7, 14, "%.2fV", inbuf.u);
			mvwprintw(mainWin, 7, 38, "%.0fmA", inbuf.i);
			mvwprintw(mainWin, 7, 62, "%.1fW", inbuf.u * inbuf.i / 1000);
			mvwprintw(mainWin, 14, 14, "%.0fV", inbuf.lim_u);
			mvwprintw(mainWin, 14, 38, "%.0fmA", inbuf.lim_i);
			mvwprintw(mainWin, 14, 62, "%.0fW", inbuf.lim_u * inbuf.lim_i / 1000);

			/* Add flags as desired */
			mvwprintw(mainWin, 19, 30, "");
			if ( inbuf.flags & DPS_FLG_REMOTE )
				waddstr(mainWin, "REM ");
			if ( inbuf.flags & DPS_FLG_IO )
				waddstr(mainWin, "IO ");
			if ( inbuf.flags & DPS_FLG_OVERTEMP )
				waddstr(mainWin, "OVERTEMP ");
			if ( inbuf.flags & DPS_FLG_FINEADJ )
				waddstr(mainWin, "FINEADJ ");

			/* Check for excessive Heat */
			check_overtemp(&inbuf);

			/* A mini-event-loop (if you want to say so) */
			key = wgetch(mainWin);
			if ( key == 'q' )
				break;

			/* We can only set values when REM has been pressed */
			if ( inbuf.flags & DPS_FLG_REMOTE ) {
				serase();
				mvwprintw(statusLine, 0, 1, "%s", statustext);
				if ( key == 'o') {
					retval = sendpkt(PKT_KEYPRESS, DPS_KEY_IO);
				} else if ( key == 'u' ) {
					args.val_u = getval("Set Voltage");
					if ( args.val_u > 0 && args.val_u <= 80 ) {
						/* FIXME: Here is the right place to fix bug #1 */
						retval = sendpkt(PKT_KEYPRESS, DPS_KEY_U);
						intfloat = adjval(inbuf.u, args.val_u, '0');
						retval = jog(&intfloat);
					} else {
						okbox("Value out of range.");
					}
				} else if ( key == 'U' ) {
					args.val_u_lim = getval("Set Voltage LIMIT");
					if ( args.val_u_lim > 0 && args.val_u_lim <= 80 ) {
						retval = sendpkt(PKT_KEYPRESS, DPS_KEY_LIM_U);
						intfloat = adjval(inbuf.lim_u, args.val_u_lim, 'U');
						retval = jog(&intfloat);
						retval = sendpkt(PKT_KEYPRESS, DPS_KEY_LIM_ENT);
					} else {
						okbox("Value out of range.");
					}
				} else if ( key == 'I' ) {
					args.val_i_lim = getval("Set Current LIMIT");
					if ( args.val_i_lim > 0 && args.val_i_lim <= 10000 ) {
						retval = sendpkt(PKT_KEYPRESS, DPS_KEY_LIM_I);
						intfloat = adjval(inbuf.lim_i, args.val_i_lim, 'I');
						retval = jog(&intfloat);
						retval = sendpkt(PKT_KEYPRESS, DPS_KEY_LIM_ENT);
					} else {
						okbox("Value out of range.");
					}
				}
			} else {
				serase();
				mvwprintw(statusLine, 0, 1, "Press REM button on DPS to allow changes. | q - Quit.                          ");
			}

			wrefresh(mainWin);
			wrefresh(statusLine);
		}
	}
	#endif
	close(fd);
	return(retval);
}

/*---------------------------------------------------------------------------*/
