/*
 * main.c - control roku devices via keyboard and udp/tcp
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
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <termios.h>
#include <poll.h>
#define DEBUG
#include "common/conventions.h"
#include "common/someutils.h"
#include "common/blockmem.h"
#include "common/blockspool.h"
#include "common/block_hget.h"
#include "getch.h"
#include "roku.h"
// #define MULTICASTIP "192.168.1.8"
#ifndef MULTICASTIP
#include "interfaces.h"
#endif
#include "discover.h"
#include "automute.h"
#include "joystick.h"
#include "notify.h"
#include "userstate.h"
#include "options.h"
#include "menus.h"
#include "action.h"

static int step_getch(int *isquit_inout, struct userstate *userstate, struct discover *discover, struct getch *getch) {
int ch;

ch=getch_getch(getch);
if (userstate->terminal.isverbose) {
	(void)reset_printstate(userstate);
	fprintf(stderr,"Got ch: %d\n",ch);
}
{
	int isquit,nextch;
	if (handlekey_menus(&isquit,&nextch,userstate,discover,ch)) GOTOERROR;
	if (isquit) *isquit_inout=1;
	if (nextch) {
		if (handlekey_menus(&isquit,&nextch,userstate,discover,nextch)) GOTOERROR;
		if (isquit) *isquit_inout=1;
	}
}

return 0;
error:
	return -1;
}

static inline int step_volume(struct userstate *userstate, struct discover *discover) {
if (!userstate->volume._isdirty) return 0;
if (!discover->found.ipv4) {
	userstate->volume._isdirty=0; // avoid inf loop, wait for device to be discovered
	return 0;
}

#if 0
fprintf(stderr,"%s:%d step_volume, current: %u, target: %u, full: %u\n",__FILE__,__LINE__,
		userstate->volume.current,
		userstate->volume.target,
		userstate->volume.full);
#endif

char *keypress=NULL;
int delta=0;
unsigned int delay=0;

if (userstate->volume.target < userstate->volume.current) {
	keypress="VolumeDown";
	delta=-1;
	delay=userstate->delays.down_volumestep;
} else if (userstate->volume.target > userstate->volume.current) {
	keypress="VolumeUp";
	delta=1;
	delay=userstate->delays.up_volumestep;
}
if (delta) {
	int issent;
	if (sendkeypress_action(&issent,userstate,discover,keypress)) {
		fprintf(stderr,"Error sending http request\n");
		if (!userstate->volume.errorfuse) {
			userstate->volume._isdirty=0;
		}
		sleep(1); // without a sleep we might burn through errorfuse before device wakes
		userstate->volume.errorfuse--;
	} else {
		if (issent) {
			userstate->volume.current+=delta;
			if (delay) usleep(delay);
		}
	}
}
if (userstate->volume.target==userstate->volume.current) userstate->volume._isdirty=0;
return 0;
}

static inline int step_discover(struct userstate *userstate, struct discover *discover) {
struct reply_discover reply_discover;
uint32_t oldip;
if (discover->udp.fd<0) return 0;
oldip=discover->found.ipv4;
if (readreply_discover(&reply_discover,discover)) GOTOERROR;
if (reply_discover.ipv4) {
	if (discover->found.ipv4!=oldip) {
		if (!userstate->terminal.issilent) {
			(void)reset_printstate(userstate);
			fprintf(stdout,"%s:%d selected roku at %u.%u.%u.%u:%u sn:%s\n",__FILE__,__LINE__,
					(reply_discover.ipv4>>0)&0xff, (reply_discover.ipv4>>8)&0xff, (reply_discover.ipv4>>16)&0xff, (reply_discover.ipv4>>24)&0xff,
					reply_discover.port,reply_discover.id_roku.usn);
		}
		if (userstate->volume.current!=userstate->volume.target) (void)setdirty_volume(userstate);
	} else {
		if (!userstate->terminal.issilent) {
			(void)reset_printstate(userstate);
			fprintf(stdout,"%s:%d another roku is at %u.%u.%u.%u:%u sn:%s\n",__FILE__,__LINE__,
					(reply_discover.ipv4>>0)&0xff, (reply_discover.ipv4>>8)&0xff, (reply_discover.ipv4>>16)&0xff, (reply_discover.ipv4>>24)&0xff,
					reply_discover.port,reply_discover.id_roku.usn);
		}
	}
	if (userstate->terminal.isverbose) {
		(ignore)querydeviceinfo_action(stderr,reply_discover.ipv4,reply_discover.port,"user-device");
	}
}
return 0;
error:
	return -1;
}

static inline int step_automute(struct userstate *userstate, struct discover *discover) {
if (!userstate->automute.isactive) return 0;
#if 0
if (!userstate->automute.isinentry) return 0; // not in use
#endif

int secleft;
secleft=secleft_automute(&userstate->automute);

if (!userstate->terminal.issilent) {
	if (userstate->terminal.printstate!=AUTOMUTE_PRINTSTATE_TERMINAL) {
		if (userstate->terminal.printstate==AUTOMUTE2_PRINTSTATE_TERMINAL) {
			fputs("\r                                       ",stdout);
		} else {
			if (userstate->terminal.printstate) fputc('\n',stdout);
		}
		userstate->terminal.printstate=AUTOMUTE_PRINTSTATE_TERMINAL;
	}
}

if (secleft<=0) {
	int ign;
	userstate->automute.isactive=0;
	if (!userstate->terminal.issilent) {
		userstate->terminal.printstate=AUTOMUTE2_PRINTSTATE_TERMINAL;
		fputs("\rCountdown to unmute: 0 ... unmuting now",stdout);
		fflush(stdout);
	}
#if 0
	if (userstate.menus.type==AUTOMUTE_MENUTYPE) {
		userstate.menus.type=MAIN_TYPE_MENU;
		(ignore)print_menus(&userstate);
	}
#endif
	if (userstate->volume.finalkeypress) {
		if (sendkeypress_action(&ign,userstate,discover,userstate->volume.finalkeypress)) GOTOERROR;
	}
	if (sendmute_action(&ign,userstate,discover,1)) GOTOERROR;
} else {
	if (!userstate->terminal.issilent) {
		fprintf(stdout,"\rCountdown to unmute: %d ",secleft);
		fflush(stdout);
	}
}

return 0;
error:
	return -1;
}

static int step_joystick(struct userstate *userstate, struct discover *discover, struct joystick *js) {
int code;
if (getbutton_joystick(&code,js)) {
	deinit_joystick(js);
	clear_joystick(js);
	return 0;
}
if (!code) return 0; // speedup
if (handlebutton_menus(userstate,discover,code,js)) GOTOERROR;
return 0;
error:
	return -1;
}
static inline int step_repeat_joystick(struct userstate *userstate, struct discover *discover, struct joystick *js) {
if (!js->repeat.ischecking) return 0;
js->repeat.isshortdelay=1; // future repeats use the short delay
if (repeat_handlebutton_menus(userstate,discover,js->repeat.code)) GOTOERROR;
return 0;
error:
	return -1;
}


static int step_notify(struct userstate *userstate, struct notify *notify, struct joystick *joystick, struct pollfd *joypfd) {
if (readrecord_notify(notify)) GOTOERROR;
(ignore)detect_joystick(joystick);
joypfd->fd=joystick->fd;

if (userstate->terminal.isverbose) (ignore)printstatus_joystick(joystick,stdout);

userstate->joystick.devicetype=joystick->device.type; // for menus
if (joystick->fd>=0) userstate->joystick.isjoystick=1;
voidinit_buttons_menus(NULL,userstate);
return 0;
error:
	return -1;
}

int main(int argc, char **argv) {
struct userstate userstate;
struct discover discover;
struct joystick joystick;
struct notify notify;
struct getch getch;
struct mainoptions mainoptions;

clear_mainoptions(&mainoptions);
clear_userstate(&userstate);
clear_discover(&discover);
clear_getch(&getch);
clear_joystick(&joystick);
clear_notify(&notify);

if (parseargs_options(&mainoptions,&userstate,argc,argv)) GOTOERROR;
if (mainoptions.isquit) return 0;

if (mainoptions.isbackground) {
	pid_t pid;
	pid=fork();
	if (pid) {
		if (pid<0) GOTOERROR;
		goto done;
	}
}

uint32_t multicastipv4;
multicastipv4=userstate.options.multicastipv4;
if (!multicastipv4) {
#ifdef MULTICASTIP
	multicastipv4=inet_addr(MULTICASTIP);
	if (multicastipv4==-1) GOTOERROR;
#else
	struct interfaces interfaces;
	clear_interfaces(&interfaces);
	if (init_interfaces(&interfaces)) GOTOERROR;
	if (getipv4multicastip_interfaces(&multicastipv4,&interfaces)) {
		deinit_interfaces(&interfaces);
		GOTOERROR;
	}
	deinit_interfaces(&interfaces);
#endif
}

if (userstate.terminal.isverbose) {
	fprintf(stderr,"%s:%d using multicastip: %u.%u.%u.%u\n",__FILE__,__LINE__,
			multicastipv4&0xff, (multicastipv4>>8)&0xff, (multicastipv4>>16)&0xff, (multicastipv4>>24)&0xff);
}
voidinit_discover(&discover,mainoptions.discovertimeout,multicastipv4);

if (mainoptions.ipv4) {
	discover.found.ipv4=mainoptions.ipv4;
	discover.found.port=mainoptions.port;
}
if (mainoptions.sn) {
	(void)setfilter_discover(&discover,mainoptions.sn);
}

if (!discover.found.ipv4) {
	if (start_discover(&discover)) GOTOERROR;
}

if (mainoptions.isprecmd) {
	struct reply_discover reply_discover;
	clear_reply_discover(&reply_discover);
	while (1) {
		int ign;
		if (check_discover(&ign,&reply_discover,&discover,-1)) GOTOERROR;
		if (discover.found.ipv4) {
			if (mainoptions.isinteractive) {
				fprintf(stdout,"%s:%d selected roku at %u.%u.%u.%u:%u sn:%s\n",__FILE__,__LINE__,
						(reply_discover.ipv4>>0)&0xff, (reply_discover.ipv4>>8)&0xff, (reply_discover.ipv4>>16)&0xff, (reply_discover.ipv4>>24)&0xff,
						reply_discover.port,reply_discover.id_roku.usn);
			}
			break;
		}
		if (discover.expires && (discover.expires <= time(NULL))) {
			(void)expire_discover(&discover);
			fprintf(stderr,"%s:%d no roku device discovered\n",__FILE__,__LINE__);
			mainoptions.isinteractive=0;
			break;
		}
	}
	if (discover.found.ipv4) {
		int i;
		for (i=1;i<argc;i++) {
			char *arg=argv[i];
			if (!strncmp(arg,"--keypress_",11)) {
				int issent;
				if (sendkeypress_action(&issent,&userstate,&discover,arg+11)) GOTOERROR;
				if (!issent) break;
			} else if (!strncmp(arg,"--sleep_",8)) {
				unsigned int ui;
				ui=slowtou(arg+8);
				usleep(1000*ui);
			}
		}
	}
}
if (mainoptions.isinteractive) {
#define STDIN_POLLFDS	0
#define JOYSTICK_POLLFDS	1
#define NOTIFY_POLLFDS	2
#define DISCOVER_POLLFDS	3
	struct pollfd pollfds[4];

	if (mainoptions.isanyjoystick) {
		voidinit_joystick(&joystick);
		if (mainoptions.joystick.isscan) {
			(ignore)scan_joystick(stdout);
			goto done;
		}
		if (init_notify(&notify,"/dev/input")) GOTOERROR;
		if (mainoptions.joystick.devicefile) {
			if (manual_set_joystick(&joystick,mainoptions.joystick.devicefile,mainoptions.joystick.mapping)) GOTOERROR;
		} else if (mainoptions.joystick.modelname) {
			if (modelname_set_joystick(&joystick,mainoptions.joystick.modelname,mainoptions.joystick.mapping)) GOTOERROR;
		} else if (mainoptions.joystick.isauto) {
			if (auto_set_joystick(&joystick,mainoptions.joystick.modelname)) GOTOERROR;
		}
		userstate.joystick.devicetype=joystick.device.type; // for menus
		if (joystick.fd>=0) userstate.joystick.isjoystick=1;
		voidinit_buttons_menus(NULL,&userstate);
		if (userstate.terminal.isverbose) (ignore)printstatus_joystick(&joystick,stdout);
	}

	pollfds[STDIN_POLLFDS].fd=mainoptions.stdinfd;
	pollfds[STDIN_POLLFDS].events=POLLIN;
	pollfds[STDIN_POLLFDS].revents=0;
	pollfds[JOYSTICK_POLLFDS].fd=joystick.fd;
	pollfds[JOYSTICK_POLLFDS].events=POLLIN;
	pollfds[JOYSTICK_POLLFDS].revents=0;
	pollfds[NOTIFY_POLLFDS].fd=notify.fd;
	pollfds[NOTIFY_POLLFDS].events=POLLIN;
	pollfds[NOTIFY_POLLFDS].revents=0;
	pollfds[DISCOVER_POLLFDS].fd=-1;
	pollfds[DISCOVER_POLLFDS].events=POLLIN;
	pollfds[DISCOVER_POLLFDS].revents=0; // be sure to clear this on use

	if (init_getch(&getch,mainoptions.stdinfd)) GOTOERROR;

	while (!mainoptions.isquit) {
		int ms_poll,num_poll;

		if (getch.isnextchar) {
			if (step_getch(&mainoptions.isquit,&userstate,&discover,&getch)) GOTOERROR;
			continue;
		}

		(ignore)print_menus(&userstate);

		ms_poll=-1;
		num_poll=3;

		if (userstate.volume._isdirty) {
			ms_poll=0;
			joystick.repeat.ischecking=0;
		} else if (joystick.repeat.code) {
			if (joystick.repeat.isshortdelay) {
				ms_poll=150;
			} else {
				ms_poll=750;
			}
			joystick.repeat.ischecking=1;
		} else if (userstate.automute.isactive) {
			ms_poll=1000;
			joystick.repeat.ischecking=0;
		}
		if (discover.udp.fd>=0) {
			time_t now;
			now=time(NULL);
			if (discover.expires && (discover.expires <= now)) {
				(void)expire_discover(&discover);
			} else {
				num_poll=4;
				pollfds[DISCOVER_POLLFDS].fd=discover.udp.fd;
				if (discover.nextpacket < now) {
					if (sendpacket_discover(&discover)) GOTOERROR;
					discover.nextpacket=now+1;
				}
			}
		}
		switch (poll(pollfds,num_poll,ms_poll)) {
			case -1: if (errno==EAGAIN) continue; GOTOERROR;
			case 1: case 2: case 3:
				if (pollfds[DISCOVER_POLLFDS].revents&POLLIN) {
					pollfds[DISCOVER_POLLFDS].revents=0;
					if (step_discover(&userstate,&discover)) GOTOERROR;
				}
				if (pollfds[JOYSTICK_POLLFDS].revents&POLLIN) {
					if (step_joystick(&userstate,&discover,&joystick)) GOTOERROR;
				}
				if (pollfds[NOTIFY_POLLFDS].revents&POLLIN) {
					if (step_notify(&userstate,&notify,&joystick,&pollfds[JOYSTICK_POLLFDS])) GOTOERROR;
				}
				if (pollfds[STDIN_POLLFDS].revents&POLLIN) {
					if (step_getch(&mainoptions.isquit,&userstate,&discover,&getch)) GOTOERROR;
				}
				break;
			case 0:
				if (step_repeat_joystick(&userstate,&discover,&joystick)) GOTOERROR;
				break;
		}

		if (step_volume(&userstate,&discover)) GOTOERROR;
		if (step_automute(&userstate,&discover)) GOTOERROR;
	}
	(void)reset_printstate(&userstate);
} // isinteractive

done:
deinit_notify(&notify);
deinit_joystick(&joystick);
deinit_getch(&getch);
deinit_discover(&discover);
return 0;
error:
	deinit_notify(&notify);
	deinit_joystick(&joystick);
	deinit_getch(&getch);
	deinit_discover(&discover);
	return -1;
}
