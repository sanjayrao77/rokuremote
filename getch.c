/*
 * getch.c - replace ncurses getch
 * Copyright (C) 2023 Sanjay Rao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <termios.h>
#define DEBUG
#include "common/conventions.h"

#include "getch.h"

void clear_getch(struct getch *g) {
static struct getch blank={.nofree.fd=-1};
*g=blank;
}

int init_getch(struct getch *g, int fd) {
struct termios t;

if (tcgetattr(fd,&t)) GOTOERROR;
g->nofree.orig_termios_fd=t;
t.c_lflag &= ~(ECHO|ICANON);
if (tcsetattr(fd,TCSANOW,&t)) GOTOERROR;

g->nofree.fd=fd;
return 0;
error:
	return -1;
}

void deinit_getch(struct getch *g) {
if (g->nofree.fd>=0) {
	if (tcsetattr(g->nofree.fd,TCSANOW,&g->nofree.orig_termios_fd)) {
		fprintf(stderr,"%s:%d error resetting termios: %s\n",__FILE__,__LINE__,strerror(errno));
	}
}
}

static int escapelookup(struct getch *g) {
static char hexchars[]="0123456789ABCDEF";
switch (g->escbufferlen) {
	case 1:
		switch (g->escbuffer[0]) {
			case 0x41: return KEY_UP;
			case 0x42: return KEY_DOWN;
			case 0x43: return KEY_RIGHT;
			case 0x44: return KEY_LEFT;
		}
		break;
}
int i;
fprintf(stderr,"%s:%d unknown escape sequence: ",__FILE__,__LINE__);
for (i=0;i<g->escbufferlen;i++) {
	unsigned char uc;
	uc=g->escbuffer[i];
	fputc('%',stderr);
	fputc(hexchars[uc>>4],stderr);
	fputc(hexchars[uc&0xf],stderr);
}
fputs("\n",stderr);
return 0;
}

int getch_getch(struct getch *g) {
unsigned char uc;
fd_set rset;
int fd;
int k;
fd=g->nofree.fd;
while (1) {
	if (g->isnextchar) {
		g->isnextchar=0;
		uc=g->nextchar;
	} else {
		while (1) {
			FD_ZERO(&rset);
			FD_SET(fd,&rset);
			switch (select(fd+1,&rset,NULL,NULL,NULL)) {
				case 0: continue;
				case -1:
					if (errno==EAGAIN) continue;
					GOTOERROR;
			}
			k=read(fd,&uc,1);
			if (k<=0) {
				if (!k) continue;
				if (errno==EAGAIN) continue;
				GOTOERROR;
			}
			break;
		}
	}
	if (g->iscompleting) {
		if (g->escbufferlen==MAX_ESCBUFFER_GETCH) GOTOERROR;
		g->escbuffer[g->escbufferlen]=uc;
		g->escbufferlen+=1;
		if (isalpha(uc)) {
			g->iscompleting=0;
			return escapelookup(g);
		}
	}
	if (uc!=27) return uc;
	struct timeval tv;
	tv.tv_sec=0;
	tv.tv_usec=1000*50; // 50ms
	FD_ZERO(&rset);
	FD_SET(fd,&rset);
	while (1) {
		switch (select(fd+1,&rset,NULL,NULL,&tv)) {
			case 0:	return 27;
			case -1: if (errno==EAGAIN) continue; GOTOERROR;
		}
		k=read(fd,&uc,1);
		if (k<=0) {
			if (!k) continue;
			if (errno==EAGAIN) continue;
			GOTOERROR;
		}
		break;
	}
	if (uc!='[') {
		g->isnextchar=1;
		g->nextchar=uc;
		return 27;
	}
	g->iscompleting=1;
	g->escbufferlen=0;
}
// unreachable
return 0;
error:
	return -1;
}
