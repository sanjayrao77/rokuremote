/*
 * action.c - send commands to device
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
#if 0
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <termios.h>
#include <poll.h>
#endif
#define DEBUG
#include "common/conventions.h"
#include "common/someutils.h"
#include "common/blockmem.h"
#include "common/blockspool.h"
#include "common/block_hget.h"
#include "roku.h"
#include "discover.h"
#include "automute.h"
#include "joystick.h"
#include "userstate.h"

#include "action.h"

char *reboot_global[]={"Home","sleep","sleep","Home","sleep","Up","sleep","Right","sleep","Up","sleep","Right","sleep","Up","sleep","Up","sleep","Right","sleep","Select",NULL};
char *reboot2_global[]={"Up","sleep","Right","sleep","Up","sleep","Right","sleep","Up","sleep","Up","sleep","Right","sleep","Select",NULL};
char *right40_global[]={ "Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right",
		"Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right",
		"Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right",
		"Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right", "Right",NULL};
char *left40_global[]={ "Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left",
		"Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left",
		"Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left",
		"Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left", "Left",NULL};

int sendkeypress_action(int *issent_out, struct userstate *userstate, struct discover *d, char *keyname) {
struct blockspool blockspool;
char uri[80];
char *extra="Content-type: application/x-www-form-urlencoded\r\n";
int issent=0;

if (!d->found.ipv4) {
	fprintf(stderr,"Unable to send keypress %s, no roku discovered yet.\n",keyname);
	*issent_out=issent;
	return 0;
}

clear_blockspool(&blockspool);

snprintf(uri,80,"/keypress/%s",keyname);

if (init_blockspool(&blockspool)) GOTOERROR;

{
	uint32_t ipv4;
	ipv4=d->found.ipv4;
	if (userstate->terminal.isverbose) {
		(void)reset_printstate(userstate);
		fprintf(stdout,"Sending %u.%u.%u.%u:%u POST %s\n",
				(ipv4>>0)&0xff, (ipv4>>8)&0xff, (ipv4>>16)&0xff, (ipv4>>24)&0xff,d->found.port, uri);
	}
	if (ipv4_hget(&blockspool,NULL,ipv4,8060,uri,(unsigned char *)"",0,extra,time(NULL)+5)) GOTOERROR;
}


#if 0
fprintf(stderr,"reply is %u\n",sizeof_blockspool(&blockspool));
fwrite_blockspool(&blockspool,stderr);
fputc('\n',stderr);
#endif

if (!sizeof_blockspool(&blockspool)) issent=1;

*issent_out=issent;
deinit_blockspool(&blockspool);
return 0;
error:
	deinit_blockspool(&blockspool);
	return -1;
}

int sendmute_action(int *isnotsent_out, struct userstate *userstate, struct discover *d, int isup) {
int isnotsent=0;
if (!userstate->options.isnomute) {
	int issent;
	if (sendkeypress_action(&issent,userstate,d,"VolumeMute")) {
		isnotsent=1;
		fprintf(stderr,"Error sending http request\n");
	} else {
		if (!issent) isnotsent=1;
	}
} else {
	unsigned int step;
	step=userstate->volume.full;
	(void)setdirty_volume(userstate);
	if (isup) {
		userstate->volume.target+=step;
		if (userstate->volume.target > userstate->volume.full) userstate->volume.target=userstate->volume.full;
	} else {
		if (userstate->volume.target <= step) userstate->volume.target=0;
		else userstate->volume.target-=step;
	}
}
*isnotsent_out=isnotsent;
return 0;
}

int changemute_action(struct userstate *userstate, struct discover *discover, int mutestep, char *finalkeypress) {
if (!userstate->automute.isactive && (mutestep>0)) {
	int isnotsent;
	(void)reset_automute(&userstate->automute,mutestep);
	if (sendmute_action(&isnotsent,userstate,discover,0)) GOTOERROR;
	if (isnotsent) {
		userstate->automute.isactive=0;
	} else {
		userstate->volume.finalkeypress=finalkeypress;
	}
} else {
	if (userstate->volume.finalkeypress && finalkeypress) { // toggle off
		userstate->volume.finalkeypress=NULL;
	} else if (!userstate->volume.finalkeypress && finalkeypress) { // toggle on
		time_t now;
		now=time(NULL);
		userstate->volume.finalkeypress=finalkeypress;
		if (now <= userstate->automute.start+2) { // if we hit youtube within 2 seconds, go +5 seconds from start
			userstate->automute.stop=userstate->automute.start+mutestep;
		} else {
			userstate->automute.stop=now+mutestep;
		}
	} else {
		userstate->automute.stop+=mutestep;
	}
}
return 0;
error:
	return -1;
}

int sendkeypresses_action(struct userstate *userstate, struct discover *d, char **keypresses) {
for (;*keypresses;keypresses++) {
	char *key=*keypresses;
	int issent;
	if (!strcmp(key,"sleep")) { usleep(750*1000); continue; }
	if (sendkeypress_action(&issent,userstate,d,key)) GOTOERROR;
	if (!issent) return -1;
//	usleep(750*1000);
}
return 0;
error:
	return -1;
}

void volumeup_action(struct userstate *userstate) {
#if 0
fprintf(stderr,"%s:%d volumeup full:%u current:%u target:%u\n",__FILE__,__LINE__,
userstate->volume.full,
userstate->volume.current,
userstate->volume.target);
#endif
if (userstate->automute.isactive) {
	userstate->automute.stop=0;
} else {
	if (userstate->volume.full==userstate->volume.target) userstate->volume.target+=1;
	userstate->volume.full+=1;
	(void)setdirty_volume(userstate);
	if (!userstate->terminal.issilent) {
		if (userstate->options.isnomute) {
			(void)set_printstate(userstate,NEWVOLUME_PRINTSTATE_TERMINAL);
			fprintf(stdout,"\r   m/M                     :   Volume down/up %u steps ",userstate->volume.full);
			fflush(stdout);
		}
	}
}
}

void volumedown_action(struct userstate *userstate) {
#if 0
fprintf(stderr,"%s:%d volumedown full:%u current:%u target:%u\n",__FILE__,__LINE__,
userstate->volume.full,
userstate->volume.current,
userstate->volume.target);
#endif
if (userstate->volume.full) {
	if (!userstate->volume.target && (userstate->volume.target==userstate->volume.current)) {
		// we think we're at rock-bottom but user is lowering volume so we may not be
		userstate->volume.current+=1;
	} else {
		if (userstate->volume.full==userstate->volume.target) userstate->volume.target-=1;
		userstate->volume.full-=1;
		if (!userstate->terminal.issilent) {
			if (userstate->options.isnomute) {
				(void)set_printstate(userstate,NEWVOLUME_PRINTSTATE_TERMINAL);
				fprintf(stdout,"\r   m/M                     :   Volume down/up %u steps ",userstate->volume.full);
				fflush(stdout);
			}
		}
	}
	(void)setdirty_volume(userstate);
} else {
	// current,full,target are all at 0, but user says volumedown
	userstate->volume.current=1;
	(void)setdirty_volume(userstate);
}
}


static inline int printfilter(FILE *fout, char *str, char *filter) {
// modifies str
while (1) {
	char *temp;
	temp=strchr(str,'\n');
	if (temp) *temp=0;
	if (strstr(str,filter)) {
		(ignore)fputs(str,fout);
		(ignore)fputc('\n',fout);
	}
	if (!temp) break;
	*temp='\n';
	str=temp+1;
}
return 0;
}

int querydeviceinfo_action(FILE *fout, uint32_t ipv4, unsigned short port, char *filter) {
struct blockspool blockspool;
char *flat=NULL;
unsigned int flatlen;

clear_blockspool(&blockspool);
if (init_blockspool(&blockspool)) GOTOERROR;
if (ipv4_hget(&blockspool,NULL,ipv4,port,"/query/device-info",NULL,0,NULL,time(NULL)+10)) GOTOERROR;
if (exportz_blockspool(&flat,&flatlen,&blockspool)) GOTOERROR;

if (!filter) {
	fprintf(fout,"Device info (%u.%u.%u.%u):\n", ipv4&0xff, (ipv4>>8)&0xff, (ipv4>>16)&0xff, (ipv4>>24)&0xff);
	fwrite(flat,flatlen,1,fout);
	fputc('\n',fout);
} else {
	(ignore)printfilter(fout,flat,filter);
}

free(flat);
return 0;
error:
	iffree(flat);
	return -1;
}
