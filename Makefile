#
# Copyright 2004-2012 poc@pocnet.net
#
# This file is part of dpsctrl.
#
# Dpsctrl is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# Dpsctrl is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# Dpsctrl; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA or get it at
# http://www.gnu.org/licenses/gpl.html
#
# $Id: Makefile,v 1.13 2018/12/23 23:45:55 poc Exp $
#
# Our needed Programs
CC=gcc

# Debian packaging tools need this
DESTDIR=/
BINDIR=$(DESTDIR)/usr/bin
MANDIR=$(DESTDIR)/usr/share/man/man1

# Use Curses for some interactivity? (Comment out if not wanted)
CURSESCFLAGS=-DUSE_CURSES -I/usr/include/ncurses -g
CURSESLDFLAGS=-lncurses -g

.PHONY: clean distclean

dpsctrl: dpsctrl.o
	$(CC) -O -Wall -pedantic $(CURSESLDFLAGS) -o $@ $<

dpsctrl.o: dpsctrl.c dpsctrl.h Makefile
	$(CC) -O -Wall -pedantic $(CURSESCFLAGS) -o $@ -c $<

clean:
	rm -f dpsctrl *.o build-stamp configure-stamp

install: dpsctrl dpsctrl.1
	install -m 755 -o root -g root -s dpsctrl $(BINDIR)
	install -m 644 -o man -g root dpsctrl.1 $(MANDIR)

