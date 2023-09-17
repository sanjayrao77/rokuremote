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

#define MAIN_MENUTYPE	0
#define KEYBOARD_MENUTYPE	1
#define POWEROFF_MENUTYPE	2

struct userstate {
	int isnomute; // if we don't have VolumeMute, we can similuate mute
	int menutype;
};
SCLEARFUNC(userstate);

static int isverbose_global;

static int sendkeypress(struct discover *d, char *keyname) {
struct blockspool blockspool;
char uri[80];
char *extra="Content-type: application/x-www-form-urlencoded\r\n";

if (!d->found.ipv4) {
	fprintf(stderr,"Unable to send keypress %s, no roku discovered yet.\n",keyname);
	return 0;
}

clear_blockspool(&blockspool);

snprintf(uri,80,"/keypress/%s",keyname);

if (init_blockspool(&blockspool)) GOTOERROR;

{
	uint32_t ipv4;
	ipv4=d->found.ipv4;
	if (isverbose_global) {
		fprintf(stdout,"Sending %u.%u.%u.%u:%u POST %s\n",
				(ipv4>>0)&0xff, (ipv4>>8)&0xff, (ipv4>>16)&0xff, (ipv4>>24)&0xff,d->found.port, uri);
	}
	if (ipv4_post_hget(&blockspool,NULL,ipv4,8060,uri,NULL,0,extra,time(NULL)+5)) GOTOERROR;
}


#if 0
fprintf(stderr,"reply is %u\n",sizeof_blockspool(&blockspool));
fwrite_blockspool(&blockspool,stderr);
fputc('\n',stderr);
#endif

deinit_blockspool(&blockspool);
return 0;
error:
	deinit_blockspool(&blockspool);
	return -1;
}

static int printusage(FILE *fout) {
fputs("Usage:\n",fout);
fputs("\tautodetect: rokuremote\n",fout);
fputs("\tconnect to serial number: rokuremote sn:SERIALNUMBER\n",fout);
fputs("\tconnect to ip: rokuremote ip:IPADDRESS[:port]\n",fout);
fputs("\tsend keypress: rokuremote --keypress_XXX --keypress_YYY\n",fout);
fputs("\tdon't run interactively: rokuremote --keypress_XXX --quit\n",fout);
fputs("\tsimulate mute for streaming devices: --nomute\n",fout);
fputs("\tprint operations: --verbose\n",fout);
return 0;
}

static int parseipv4(uint32_t *ipv4_out, unsigned short *port_out, char *str, unsigned short defport) {
unsigned int a,b,c,d,p=defport;
char *temp,*next;
temp=str;
a=slowtou(temp);
next=strchr(temp,'.');
if (!next) {
	b=c=d=0;
	next=temp;
} else {
	temp=next+1;
	b=slowtou(temp);
	temp=strchr(temp,'.');
	if (!temp) GOTOERROR;
	temp++;
	c=slowtou(temp);
	temp=strchr(temp,'.');
	if (!temp) GOTOERROR;
	temp++;
	d=slowtou(temp);
	next=temp;
}
temp=strchr(next,':');
if (temp) {
	p=slowtou(temp+1);
}

*ipv4_out=(a)|(b<<8)|(c<<16)|(d<<24);
*port_out=p;
return 0;
error:
	return -1;
}

static int printmenu(struct userstate *uo) {
switch (uo->menutype) {
	case POWEROFF_MENUTYPE:
		fprintf(stdout,"Roku remote, confirm power-off:\n");
		fprintf(stdout,"\t Y                       :   Send PowerOff keypress\n");
		fprintf(stdout,"\t anything else           :   Cancel\n");
		break;
	case KEYBOARD_MENUTYPE:
		fprintf(stdout,"Roku remote, keyboard mode:\n");
		fprintf(stdout,"\t a-z,0-9,Backspace       :   send key\n");
		fprintf(stdout,"\t Left,Right,Up,Down      :   send key and exit keyboard mode\n");
		fprintf(stdout,"\t Tab,Esc                 :   exit keyboard mode\n");
		break;
	case MAIN_MENUTYPE:
		fprintf(stdout,"Roku remote, commands:\n");
		fprintf(stdout,"\t a,1                     :   Enter keyboard mode\n");
		fprintf(stdout,"\t Left,Right,Up,Down      :   Send key\n");
		fprintf(stdout,"\t Backspace               :   Back\n");
		fprintf(stdout,"\t Enter                   :   Select\n");
		fprintf(stdout,"\t Escape                  :   Home\n");
		fprintf(stdout,"\t Space                   :   Play/Pause\n");
		fprintf(stdout,"\t <                       :   Reverse\n");
		fprintf(stdout,"\t >                       :   Forward\n");
		fprintf(stdout,"\t -,_                     :   Volume down \n");
		fprintf(stdout,"\t +,=                     :   Volume up \n");
		fprintf(stdout,"\t ?                       :   Search\n");
		fprintf(stdout,"\t d                       :   Discover roku devices\n");
		fprintf(stdout,"\t f                       :   Find remote\n");
		fprintf(stdout,"\t i                       :   Information\n");
		if (uo->isnomute) {
			fprintf(stdout,"\t m,M                     :   Volume down/up 5 steps\n");
		} else {
			fprintf(stdout,"\t m                       :   Mute toggle\n");
		}
		fprintf(stdout,"\t p                       :   Power off \n");
		fprintf(stdout,"\t q,x                     :   Quit\n");
		fprintf(stdout,"\t r                       :   Instant Replay\n");
		break;
}
return 0;
}

static int handlekey(int *isquit_out, int *nextkey_out, struct discover *d,
		struct userstate *userstate, int ch) {
int nextkey=0;
int isquit=0;
char *keypress=NULL;
char buff8[8];

switch (userstate->menutype) {
	case POWEROFF_MENUTYPE:
		userstate->menutype=MAIN_MENUTYPE;
		if (ch=='Y') {
			keypress="PowerOff";
		}
		break;
	case KEYBOARD_MENUTYPE:
		if ((ch>=32)&&(ch<127)) {
			static char hexchars[]="0123456789ABCDEF";
			unsigned char uc=ch;
			memcpy(buff8,"LIT_%",5);
			buff8[5]=hexchars[uc>>4];
			buff8[6]=hexchars[uc&0xf];
			buff8[7]=0;
			keypress=buff8;
		} else switch (ch) {
			case 3: case 4: isquit=1; break;
			case KEY_UP:
			case KEY_DOWN:
			case KEY_LEFT:
			case KEY_RIGHT:
				userstate->menutype=MAIN_MENUTYPE;
				nextkey=ch;
				break;
			case '\t':
			case 27:
				userstate->menutype=MAIN_MENUTYPE;
				break;
			case '\n': case '\r': keypress="Enter"; break;
			case 8: case 127: keypress="Backspace"; break;
			default:
				fprintf(stderr,"Got unknown ch: %d\r\n",ch);
				break;
		}
		break;
	case MAIN_MENUTYPE:
		switch (ch) {
			case 3: case 4: isquit=1; break;
			case 'a': case 'A': case '1': userstate->menutype=KEYBOARD_MENUTYPE; break;
			case ',': case '<': case '[': case '{': keypress="Rev"; break;
			case '.': case '>': case ']': case '}': keypress="Fwd"; break;
			case '/': case '?': keypress="Search"; break;
			case 8: case 127: keypress="Back"; break;
			case 'd': case 'D': if (start_discover(d)) GOTOERROR; break;
			case 'f': case 'F': keypress="FindRemote"; break;
			case '-': case '_': keypress="VolumeDown"; break;
			case '+': case '=': keypress="VolumeUp"; break;
			case 'm':
				if (!userstate->isnomute) keypress="VolumeMute";
				else {
					int i;
					for (i=0;i<5;i++) {
						if (sendkeypress(d,"VolumeDown")) {
							fprintf(stderr,"Error sending http request\n");
							break;
						}
					}
				}
				break;
			case 'M':
				if (!userstate->isnomute) keypress="VolumeMute";
				else {
					int i;
					for (i=0;i<5;i++) {
						if (sendkeypress(d,"VolumeUp")) {
							fprintf(stderr,"Error sending http request\n");
							break;
						}
					}
				}
				break;
//			case 'p': case 'P': keypress="PowerOff"; break;
			case 'p': userstate->menutype=POWEROFF_MENUTYPE; break;
			case 'i': case 'I': keypress="Info"; break;
			case 'q': case 'x': case 'Q': case 'X': isquit=1; break;
			case 'r': case 'R': keypress="InstantReplay"; break;
			case '\n': case '\r': keypress="Select"; break; // enter
			case 27: keypress="Home"; break; // esc
			case ' ': keypress="Play"; break;
			case KEY_UP: keypress="Up"; break;
			case KEY_DOWN: keypress="Down"; break;
			case KEY_LEFT: keypress="Left"; break;
			case KEY_RIGHT: keypress="Right"; break;
			default:
				fprintf(stderr,"Got unknown ch: %d\r\n",ch);
				break;
		}
		break;
}

if (keypress) {
	if (sendkeypress(d,keypress)) {
		fprintf(stderr,"Error sending http request\n");
	}
}

*isquit_out=isquit;
*nextkey_out=nextkey;
return 0;
error:
	return -1;
}

int main(int argc, char **argv) {
struct userstate userstate;
struct discover discover;
struct reply_discover reply_discover;
struct getch getch;
uint32_t ipv4=0;
unsigned short port;
char *sn=NULL;
int discovertimeout=30;
int isprecmd=0;
int isinteractive=1;


clear_userstate(&userstate);
clear_discover(&discover);
clear_reply_discover(&reply_discover);
clear_getch(&getch);

{
	int i;
	for (i=1;i<argc;i++) {
		char *arg=argv[i];
		if (!strncmp(arg,"sn:",3)) {
			sn=arg+3;
			if (strlen(sn)>MAX_USN_ROKU) {
				fprintf(stderr,"%s:%d invalid serial number: \"%s\"\n",__FILE__,__LINE__,arg);
				return 0;
			}
		} else if (!strncmp(arg,"ip:",3)) {
			if (parseipv4(&ipv4,&port,arg+3,8060)) {
				fprintf(stderr,"%s:%d unparsed ip: \"%s\"\n",__FILE__,__LINE__,arg);
				return 0;
			}
		} else if (!strcmp(arg,"--verbose") || !strcmp(arg,"-v")) {
			isverbose_global=1;
		} else if (!strcmp(arg,"--nomute")) {
			userstate.isnomute=1;
		} else if (!strcmp(arg,"--quit")) {
			isinteractive=0;
			discovertimeout=5;
		} else if (!strncmp(arg,"--keypress_",11)) {
			isprecmd=1;
		} else {
			if (strcmp(arg,"--help") && strcmp(arg,"-h"))
					fprintf(stderr,"%s:%d unrecognized argument: \"%s\"\n",__FILE__,__LINE__,arg);
			(ignore)printusage(stdout);
			return 0;
		}
	}
}

uint32_t multicastipv4;
#ifdef MULTICASTIP
multicastipv4=inet_addr(MULTICASTIP);
if (multicastipv4==-1) GOTOERROR;
#else
{
	struct interfaces interfaces;
	clear_interfaces(&interfaces);
	if (init_interfaces(&interfaces)) GOTOERROR;
	if (getipv4multicastip_interfaces(&multicastipv4,&interfaces)) {
		deinit_interfaces(&interfaces);
		GOTOERROR;
	}
	deinit_interfaces(&interfaces);
}
#endif

voidinit_discover(&discover,discovertimeout,multicastipv4);

if (ipv4) {
	discover.found.ipv4=ipv4;
	discover.found.port=port;
}
if (sn) {
	(void)setfilter_discover(&discover,sn);
}

if (!discover.found.ipv4) {
	if (start_discover(&discover)) GOTOERROR;
}

if (isprecmd) {
	int ign;
	if (check_discover(&ign,&reply_discover,&discover,-1)) GOTOERROR;
	if (!discover.found.ipv4) {
		fprintf(stderr,"%s:%d no roku device discovered\n",__FILE__,__LINE__);
		isinteractive=0;
	} else {
		int i;
		for (i=1;i<argc;i++) {
			char *arg=argv[i];
			if (!strncmp(arg,"--keypress_",11)) {
				if (sendkeypress(&discover,arg+11)) GOTOERROR;
			}
		}
		clear_reply_discover(&reply_discover);
	}
}
if (isinteractive) {
	if (init_getch(&getch,STDIN_FILENO)) GOTOERROR;
	(ignore)printmenu(&userstate);

	while (1) {
		int ch;

		if ((discover.udp.fd>=0) && !getch.isnextchar) {
			int isalt;
			uint32_t oldip;
			oldip=discover.found.ipv4;
			if (check_discover(&isalt,&reply_discover,&discover,STDIN_FILENO)) GOTOERROR;
			if (reply_discover.ipv4) {
				if (discover.found.ipv4!=oldip) {
					fprintf(stdout,"%s:%d selected roku at %u.%u.%u.%u:%u sn:%s\n",__FILE__,__LINE__,
							(reply_discover.ipv4>>0)&0xff, (reply_discover.ipv4>>8)&0xff, (reply_discover.ipv4>>16)&0xff, (reply_discover.ipv4>>24)&0xff,
						reply_discover.port,reply_discover.id_roku.usn);
				} else {
					fprintf(stdout,"%s:%d another roku is at %u.%u.%u.%u:%u sn:%s\n",__FILE__,__LINE__,
							(reply_discover.ipv4>>0)&0xff, (reply_discover.ipv4>>8)&0xff, (reply_discover.ipv4>>16)&0xff, (reply_discover.ipv4>>24)&0xff,
						reply_discover.port,reply_discover.id_roku.usn);
				}
				clear_reply_discover(&reply_discover);
			}
			if (!isalt) continue;
		}
		
		ch=getch_getch(&getch);
		if (isverbose_global) {
			fprintf(stderr,"Got ch: %d\n",ch);
		}
		{
			int old_menutype,isquit,nextch;
			old_menutype=userstate.menutype;
			if (handlekey(&isquit,&nextch,&discover,&userstate,ch)) GOTOERROR;
			if (isquit) break;
			if (nextch) {
				if (handlekey(&isquit,&nextch,&discover,&userstate,nextch)) GOTOERROR;
			}
			if (userstate.menutype!=old_menutype) {
				(ignore)printmenu(&userstate);
			}
		}
	}
} // isinteractive


deinit_getch(&getch);
deinit_discover(&discover);
return 0;
error:
	deinit_getch(&getch);
	deinit_discover(&discover);
	return -1;
}
