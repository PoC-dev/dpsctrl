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
 * $Id: dpsctrl.h,v 1.6 2011/11/13 16:43:59 poc Exp $
 *
 * $Log: dpsctrl.h,v $
 * Revision 1.6  2011/11/13 16:43:59  poc
 * Resolved most FIXMEs
 *
 * Revision 1.5  2011-10-27 19:23:40  poc
 * Cleanup,
 * dpkg-buildpackage is now compatible with debian squeeze
 *
 * Revision 1.4  2009-09-16 19:49:22  poc
 * Added debian files,
 * language corrections,
 * cosmetics
 *
 * Revision 1.3  2004/06/21 20:33:07  poc
 * Oncemode ergänzt
 * Debugmode entfernt
 *
 * Revision 1.2  2004/06/16 14:15:33  poc
 * Defineumbenennung zwecks Kollisionen mit Curses
 *
 * Revision 1.1  2004/05/18 09:21:44  poc
 * Initial revision
 *
 *
 */

/* Serial Device */
#define SER_DEV "/dev/dps"

/* Buffer for incoming data.
 */
struct dpsinbuf {
	float			u;
	float			i;
	float			p;
	float			lim_u;
	float			lim_i;
	float			lim_p;
	unsigned short	flags;
};

/* Cmdline-args */
struct parsedargs {
	unsigned short	help;
	unsigned short	version;
    unsigned short	read;
    unsigned short	once;
	unsigned short	io;
	unsigned short	loop;
	float			val_u;
	float			val_u_lim;
	float			val_i_lim;
};

/* Options shall be called only once */
struct optioncount {
	unsigned short	u;
	unsigned short	u_lim;
	unsigned short	i_lim;
};

/* Split values into pre- and postdecimal values. Obviously these are
 * the Normal- and Fine- byte values for the DPS.
 */
struct splitfloat {
	int				pre;
	int				post;
};


/* Defines make us more readable */
/* How's a packet beginning, then? */
#define PKT_STRT1 0xeb
#define PKT_STRT2 0x90

/* Which packet types do exist? */
#define PKT_KEYPRESS 0xaa
#define PKT_JOG_DOWN 0xcc
#define PKT_JOG_UP 0x55

/* Option values PKT_KEYPRESS */
#define DPS_KEY_LIM_U 0
#define DPS_KEY_LIM_I 4
#define DPS_KEY_LIM_P 8
#define DPS_KEY_LIM_ENT 5
#define DPS_KEY_LIM_CE 9
#define DPS_KEY_U 1
#define DPS_KEY_F 6
#define DPS_KEY_N 2
#define DPS_KEY_IO 12

/* Flags (Bit 7) */
#define DPS_FLG_REMOTE 2
#define DPS_FLG_IO 4
#define DPS_FLG_OVERTEMP 8
#define DPS_FLG_SET_LIM_P 16
#define DPS_FLG_SET_LIM_I 32
#define DPS_FLG_SET_LIM_U 64
#define DPS_FLG_FINEADJ 128

/* EOF */
