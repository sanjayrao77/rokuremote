/*
 * automute.c - keep a timer to auto-unmute
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
#include <time.h>
#include <poll.h>
#include <errno.h>
#define DEBUG
#include "common/conventions.h"

#include "automute.h"

void reset_automute(struct automute *a, unsigned int decdelay, unsigned int incdelay, unsigned int full, unsigned int seconds) {
// decdelay, incdelay in milliseconds
// full is total number of steps
a->isactive=1;
a->start=time(NULL);
a->stop=a->start+seconds;
a->muteseconds=-1;
a->adjust_muteseconds=((incdelay-decdelay)*full)/1000;
}

int check_automute(int *secleft_out, struct automute *a, int inputfd, int optfd) {
// assumes a->isactive
struct pollfd pollfds[2];
time_t now;
time_t secleft;

pollfds[0].fd=inputfd;
pollfds[0].events=POLLIN;
pollfds[1].fd=optfd;
pollfds[1].events=POLLIN;
now=time(NULL);

if (a->muteseconds<0) { // assume time between reset_ and first check_ is same as time to unmute
	a->muteseconds=now-a->start+1+a->adjust_muteseconds;
	if (a->muteseconds>0) a->stop-=a->muteseconds;
}

secleft=a->stop-now;
if (secleft<=0) {
	secleft=0;
} else {
	while (1) {
		switch (poll(pollfds,2,1)) {
			case -1: if (errno==EAGAIN) continue; GOTOERROR; break;
			case 0: break;
			default:
				if (pollfds[0].revents || pollfds[1].revents) {
					secleft=-1;
				}
				break;
		}
		break;
	}
}

*secleft_out=(int)secleft;
return 0;
error:
	return -1;
}
