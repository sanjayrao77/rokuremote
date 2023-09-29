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

#define MAIN_MENUTYPE	1
#define KEYBOARD_MENUTYPE	2
#define POWEROFF_MENUTYPE	3
#define REBOOT_MENUTYPE	4
#define SETTINGS_MENUTYPE 5
#define UINTENTRY_MENUTYPE 6
// #define BOOLENTRY_MENUTYPE 7 // works but unused
// #define AUTOMUTE_MENUTYPE 8 // works but unused

// #define MUTESTEP_FIELDINDEX	1
// #define DEF_AUTOMUTE_FIELDINDEX	2
// #define SET_AUTOMUTE_FIELDINDEX	3
// #define TIEMUTESTEP_FIELDINDEX	4
#define FULLVOLUME_FIELDINDEX	5

struct userstate {
	int menutype;
	int lastmenuprinted;
	int isnomute; // if we don't have VolumeMute, we can similuate mute
#if 0
	unsigned int def_automute; // initial seconds for automute
#endif
#define NONE_PRINTSTATE 0
#define AUTOMUTE_PRINTSTATE	1
	int printstate;
	union {
		struct {
			int fieldindex;
			unsigned int value;
			int isedited;
		} uintentry;
		struct {
			int fieldindex;
			int value; // 0,1
			int isedited;
		} boolentry;
	};
	struct automute automute;
	struct {
		int _isdirty;
		unsigned int full;
		unsigned int current;
		unsigned int target;
		int errorfuse;
	} volume;
};
void clear_userstate(struct userstate *g) {
static struct userstate blank={.menutype=MAIN_MENUTYPE,.volume.full=15,.volume.current=15,.volume.target=15};
// .def_automute=30
*g=blank;
}

static int isverbose_global;

static inline void voidinit_uintentry(struct userstate *u, int fieldindex, unsigned int defvalue) {
u->uintentry.isedited=0;
u->menutype=UINTENTRY_MENUTYPE;
u->uintentry.fieldindex=fieldindex;
u->uintentry.value=defvalue;
}
#if 0
static inline void voidinit_boolentry(struct userstate *u, int fieldindex) {
u->boolentry.isedited=0;
u->menutype=BOOLENTRY_MENUTYPE;
u->boolentry.fieldindex=fieldindex;
u->boolentry.value=0;
}
#endif

static int sendkeypress(int *issent_out, struct discover *d, char *keyname) {
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

if (!sizeof_blockspool(&blockspool)) issent=1;

*issent_out=issent;
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
fputs("\tset a settings value: --Dname=value\n",fout);
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

static inline void reset_printstate(struct userstate *uo) {
if (uo->printstate!=NONE_PRINTSTATE) {
	fputc('\n',stdout);
	uo->printstate=NONE_PRINTSTATE;
}
}

static int printmenu(struct userstate *uo) {
if (uo->lastmenuprinted==uo->menutype) return 0;
uo->lastmenuprinted=uo->menutype;

(void)reset_printstate(uo);

switch (uo->menutype) {
#if 0
	case AUTOMUTE_MENUTYPE:
		fprintf(stdout,"Roku remote, automute settings:\n");
		fprintf(stdout,"\t Right                   :   Add a second\n");
		fprintf(stdout,"\t Up                      :   Add 5 seconds\n");
		fprintf(stdout,"\t z,Z                     :   Add %u/%u seconds\n",uo->def_automute,2*uo->def_automute);
		fprintf(stdout,"\t Left                    :   Subtract a second\n");
		fprintf(stdout,"\t Down                    :   Subtract 5 seconds\n");
		fprintf(stdout,"\t Enter                   :   Specify remaining seconds\n");
		fprintf(stdout,"\t Backspace               :   Unmute now\n");
		fprintf(stdout,"\t -,_                     :   Volume down \n");
		fprintf(stdout,"\t +,=                     :   Volume up \n");
		fprintf(stdout,"\t anything else           :   Main menu\n");
		break;
#endif
	case SETTINGS_MENUTYPE:
		fprintf(stdout,"Roku remote, settings menu:\n");
		fprintf(stdout,"\t v                       :   Set tv volume (%u)\n",uo->volume.full);
#if 0
		fprintf(stdout,"\t t                       :   Tie volume change to tv volume (%s)\n",
				(uo->istievolume)?"True":"False");
#endif
#if 0
		fprintf(stdout,"\t z                       :   Change automute seconds (%u)\n",uo->def_automute);
#endif
		fprintf(stdout,"\t anything else           :   Main menu\n");
		break;
	case REBOOT_MENUTYPE:
		fprintf(stdout,"Roku remote, confirm reboot sequence:\n");
		fprintf(stdout,"\t y                       :   Send reboot keypresses\n");
		fprintf(stdout,"\t anything else           :   Cancel\n");
		break;
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
			fprintf(stdout,"\t m,M                     :   Volume down/up %u steps\n",uo->volume.full);
		} else {
			fprintf(stdout,"\t m                       :   Mute toggle\n");
		}
		fprintf(stdout,"\t p                       :   Power off \n");
		fprintf(stdout,"\t q                       :   Quit\n");
		fprintf(stdout,"\t Q                       :   Reboot\n");
		fprintf(stdout,"\t r                       :   Instant Replay\n");
		fprintf(stdout,"\t s                       :   Settings menu\n");
		fprintf(stdout,"\t z,x,c,v                 :   Auto-mute (%d/%d/%d/%d seconds)\n",60,30,10,5);
		fprintf(stdout,"\t Z,X,C,V                 :   Reduce or cancel auto-mute (%d/%d/%d/%d seconds)\n",60,30,10,5);
		break;
}
fflush(stdout);
return 0;
}

static void printprompt_uintentry(struct userstate *userstate, int which) {
if (which<0) {
	if (userstate->uintentry.value) {
		(ignore)fprintf(stdout,"Value: %u",userstate->uintentry.value); // this is odd, unused
	} else {
		(ignore)fputs("Value: ",stdout);
	}
	(ignore)fflush(stdout);
} else if (!which) {
	if (userstate->uintentry.value) {
		char buff20[20];
		snprintf(buff20,20,"%u",userstate->uintentry.value);
		(ignore)fprintf(stdout,"\rValue: %s \rValue: %s",buff20,buff20);
	} else {
		(ignore)fputs("\rValue:  \rValue: ",stdout);
	}
	(ignore)fflush(stdout);
} else {
	(ignore)fputc('\n',stdout);
}
}

#if 0
static void printprompt_boolentry(struct userstate *userstate, int which) {
if (which<0) {
	(ignore)fputs("Value (y/n): ",stdout);
	(ignore)fflush(stdout);
} else if (!which) {
	if (userstate->boolentry.value) (ignore)fputs("True",stdout);
	else (ignore)fputs("False",stdout);
	(ignore)fputc('\n',stdout);
} else {
	(ignore)fputc('\n',stdout);
}
}
#endif

static inline void setdirty_volume(struct userstate *u) {
if (u->volume._isdirty) return;
u->volume._isdirty=1;
u->volume.errorfuse=5;
}

static int sendmute(int *isnotsent_out, struct userstate *userstate, struct discover *d, int isup) {
int isnotsent=0;
if (!userstate->isnomute) {
	int issent;
	if (sendkeypress(&issent,d,"VolumeMute")) {
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

static int sendkeypresses(struct discover *d, char **keypresses) {
for (;*keypresses;keypresses++) {
	char *key=*keypresses;
	int issent;
	if (!strcmp(key,"sleep")) { sleep(1); continue; }
	if (sendkeypress(&issent,d,key)) GOTOERROR;
	if (!issent) return -1;
	usleep(750*1000);
}
return 0;
error:
	return -1;
}

static int handlekey(int *isquit_out, int *nextkey_out, struct discover *d,
		struct userstate *userstate, int ch) {
static char *reboot[]={"Home","sleep","sleep","Home","Up","Right","Up","Right","Up","Up","Right","Select",NULL};
int nextkey=0;
int isquit=0;
int ign;
char *keypress=NULL;
char **keypresses=NULL;
char buff8[8];

switch (userstate->menutype) {
#if 0
	case AUTOMUTE_MENUTYPE:
		switch (ch) {
			case KEY_RIGHT:
				userstate->automute.stop+=1;
				break;
			case KEY_UP:
				userstate->automute.stop+=5;
				break;
			case KEY_LEFT:
				userstate->automute.stop-=1;
				break;
			case KEY_DOWN:
				userstate->automute.stop-=5;
				break;
			case 'z':
				userstate->automute.stop+=userstate->def_automute;
				break;
			case 'Z':
				userstate->automute.stop+=2*userstate->def_automute;
				break;
			case '-': case '_':
				if (userstate->volume.full) {
					if (userstate->volume.full==userstate->volume.target) userstate->volume.target-=1;
					userstate->volume.full-=1;
					(void)setdirty_volume(userstate);
				}
				break;
			case '+': case '=':
				if (userstate->volume.full==userstate->volume.target) userstate->volume.target+=1;
				userstate->volume.full+=1;
				(void)setdirty_volume(userstate);
				break;
			case '\r': case '\n':
				userstate->automute.isinentry=1;
				fprintf(stdout,"Enter the number of seconds for automute:\n");
				(void)voidinit_uintentry(userstate,SET_AUTOMUTE_FIELDINDEX,0);
				(void)printprompt_uintentry(userstate,-1);
			break;
			case 8: case 127:
				userstate->automute.stop=userstate->automute.start-1;
				break;
			default:
				userstate->menutype=MAIN_MENUTYPE;
				userstate->automute.isactive=0;
				break;
		}
		break;
#endif
#if 0
	case BOOLENTRY_MENUTYPE:
		{
			int value=0;
			switch (ch) {
				case 'y': case 'Y': case '1': case 't': case 'T':
					value=1;
				// no break
				case 'n': case 'N': case '0': case 'f': case 'F':
					userstate->boolentry.value=value;
					userstate->boolentry.isedited=1;
					(void)printprompt_boolentry(userstate,0);
					if (userstate->boolentry.isedited) {
						switch (userstate->boolentry.fieldindex) {
							case TIEMUTESTEP_FIELDINDEX: userstate->istievolume=userstate->boolentry.value; break;
						}
					}
					userstate->menutype=SETTINGS_MENUTYPE;
					break;
				default:
					(void)printprompt_boolentry(userstate,1);
					userstate->menutype=SETTINGS_MENUTYPE;
					break;
			}
		}
		break;
#endif
	case UINTENTRY_MENUTYPE:
		switch (ch) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				userstate->uintentry.value=userstate->uintentry.value*10+ch-48;
				userstate->uintentry.isedited=1;
				(void)printprompt_uintentry(userstate,0);
				break;
			case 8: case 127:
				userstate->uintentry.value/=10;
				(void)printprompt_uintentry(userstate,0);
				break;
			case '\n': case '\r':
				if (userstate->uintentry.isedited) {
					switch (userstate->uintentry.fieldindex) {
#if 0
						case DEF_AUTOMUTE_FIELDINDEX: userstate->def_automute=userstate->uintentry.value; break;
#endif
#if 0
						case SET_AUTOMUTE_FIELDINDEX: userstate->automute.stop=time(NULL)-userstate->automute.muteseconds+userstate->uintentry.value; break;
#endif
						case FULLVOLUME_FIELDINDEX:
							userstate->volume.full=userstate->uintentry.value;
							userstate->volume.target=userstate->volume.full;
							userstate->volume.current=userstate->volume.full;
							break;
					}
				}
				// no break
			default:
				(void)printprompt_uintentry(userstate,1);
				switch (userstate->uintentry.fieldindex) {
#if 0
					case SET_AUTOMUTE_FIELDINDEX:
						userstate->menutype=AUTOMUTE_MENUTYPE;
						userstate->automute.isinentry=0;
						break;
#endif
					default:
						userstate->menutype=SETTINGS_MENUTYPE;
						break;
				}
				break;
		}
		break;
	case SETTINGS_MENUTYPE:
		userstate->menutype=MAIN_MENUTYPE;
		switch (ch) {
			case 'v': case 'V':
				fprintf(stdout,"Enter the normal tv volume, for --nomute simulation:\n");
				(void)voidinit_uintentry(userstate,FULLVOLUME_FIELDINDEX,0);
				(void)printprompt_uintentry(userstate,-1);
				break;
#if 0
			case 't': case 'T':
				fprintf(stdout,"Tie volume changes to --nomute mute simulation steps:\n");
				(void)voidinit_boolentry(userstate,TIEMUTESTEP_FIELDINDEX);
				(void)printprompt_boolentry(userstate,-1);
				break;
#endif
#if 0
			case 'z': case 'Z':
				fprintf(stdout,"Enter the number of default seconds for automute:\n");
				(void)voidinit_uintentry(userstate,DEF_AUTOMUTE_FIELDINDEX,0);
				(void)printprompt_uintentry(userstate,-1);
				break;
#endif
		}
		break;
	case REBOOT_MENUTYPE:
		userstate->menutype=MAIN_MENUTYPE;
		if (ch=='y') {
			keypresses=reboot;
		}
		break;
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
				if (isverbose_global) fprintf(stderr,"Got unknown ch: %d\r\n",ch);
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
			case '-': case '_':
				if (userstate->volume.full) {
					if (userstate->volume.full==userstate->volume.target) userstate->volume.target-=1;
					userstate->volume.full-=1;
					(void)setdirty_volume(userstate);
					fprintf(stdout,"\t m,M                     :   Volume down/up %u steps\n",userstate->volume.full);
				}
				break;
			case '+': case '=':
				if (userstate->volume.full==userstate->volume.target) userstate->volume.target+=1;
				userstate->volume.full+=1;
				(void)setdirty_volume(userstate);
				fprintf(stdout,"\t m,M                     :   Volume down/up %u steps\n",userstate->volume.full);
				break;
			case 'm': if (sendmute(&ign,userstate,d,0)) GOTOERROR; break;
			case 'M': if (sendmute(&ign,userstate,d,1)) GOTOERROR; break;
//			case 'p': case 'P': keypress="PowerOff"; break;
			case 'p': userstate->menutype=POWEROFF_MENUTYPE; break;
			case 'i': case 'I': keypress="Info"; break;
			case 'q': isquit=1; break;
			case 'Q': userstate->menutype=REBOOT_MENUTYPE; break;
			case 'r': case 'R': keypress="InstantReplay"; break;
			case 's': case 'S': userstate->menutype=SETTINGS_MENUTYPE; break;
			case 'z': case 'x': case 'c': case 'v':
				{
					int step=0;
					switch (ch) {
						case 'z': step=60; break; case 'x': step=30; break;
						case 'c': step=10; break; case 'v': step=5; break;
					}
					if (!userstate->automute.isactive) {
						int isnotsent;
						(void)reset_automute(&userstate->automute,step);
						if (sendmute(&isnotsent,userstate,d,0)) GOTOERROR;
						if (isnotsent) {
							userstate->automute.isactive=0;
						}
					} else {
						userstate->automute.stop+=step;
					}
				}
				break;
			case 'Z': case 'X': case 'C': case 'V':
				{
					int step=0;
					switch (ch) {
						case 'Z': step=60; break; case 'X': step=30; break;
						case 'C': step=10; break; case 'V': step=5; break;
					}
					if (userstate->automute.isactive) {
						userstate->automute.stop-=step;
					}
				}
				break;
#if 0
			case 'z': case 'Z':
				userstate->menutype=AUTOMUTE_MENUTYPE;
				(ignore)printmenu(userstate);
				if (!userstate->automute.isactive) {
					int isnotsent;
					if (ch=='Z') (void)reset_automute(&userstate->automute,2*userstate->def_automute);
					else (void)reset_automute(&userstate->automute,userstate->def_automute);
					if (sendmute(&isnotsent,userstate,d,0)) GOTOERROR;
					if (isnotsent) {
						userstate->menutype=MAIN_MENUTYPE;
						userstate->automute.isactive=0;
					}
				}
				break;
#endif
			case '\n': case '\r': keypress="Select"; break; // enter
			case 27: keypress="Home"; break; // esc
			case ' ': keypress="Play"; break;
			case KEY_UP: keypress="Up"; break;
			case KEY_DOWN: keypress="Down"; break;
			case KEY_LEFT: keypress="Left"; break;
			case KEY_RIGHT: keypress="Right"; break;
			default:
				if (isverbose_global) fprintf(stderr,"Got unknown ch: %d\r\n",ch);
				break;
		}
		break;
}

if (keypress) {
	int issent;
	if (sendkeypress(&issent,d,keypress)) {
		fprintf(stderr,"Error sending http request\n");
	}
}
if (keypresses) {
	if (sendkeypresses(d,keypresses)) {
		fprintf(stderr,"Error sending http request\n");
	}
}

*isquit_out=isquit;
*nextkey_out=nextkey;
return 0;
error:
	return -1;
}

static int setstring_userstate(struct userstate *u, char *str) {
char *temp;

if (!strncmp(str,"fullvolume",10)) {
	temp=str+10;
	if (*temp!='=') GOTOERROR;
	u->volume.full=slowtou(temp+1);
#if 0
} else if (!strncmp(str,"automute",8)) {
	temp=str+8;
	if (*temp!='=') GOTOERROR;
	u->def_automute=slowtou(temp+1);
#endif
#if 0
} else if (!strncmp(str,"tievolume",9)) {
	temp=str+9;
	if (*temp!='=') GOTOERROR;
	switch (temp[1]) {
		case 'y': case 'Y': case '1': case 't': case 'T': u->istievolume=1; break;
		case 'n': case 'N': case '0': case 'f': case 'F': u->istievolume=0; break;
		default: GOTOERROR;
	}
#endif
} else GOTOERROR;

return 0;
error:
	return -1;
}

static int step_getch(int *isquit_inout, struct userstate *userstate, struct discover *discover, struct getch *getch) {
int ch;

ch=getch_getch(getch);
if (isverbose_global) {
	fprintf(stderr,"Got ch: %d\n",ch);
}
{
	int isquit,nextch;
	if (handlekey(&isquit,&nextch,discover,userstate,ch)) GOTOERROR;
	if (isquit) *isquit_inout=1;
	if (nextch) {
		if (handlekey(&isquit,&nextch,discover,userstate,nextch)) GOTOERROR;
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

char *keypress=NULL;
int delta=0;
if (userstate->volume.target < userstate->volume.current) {
	keypress="VolumeDown";
	delta=-1;
} else if (userstate->volume.target > userstate->volume.current) {
	keypress="VolumeUp";
	delta=1;
}
if (delta) {
	int issent;
	if (sendkeypress(&issent,discover,keypress)) {
		fprintf(stderr,"Error sending http request\n");
		if (!userstate->volume.errorfuse) {
			userstate->volume._isdirty=0;
		}
		sleep(1); // without a sleep we might burn through errorfuse before device wakes
		userstate->volume.errorfuse--;
	} else {
		if (issent) userstate->volume.current+=delta;
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
		(void)reset_printstate(userstate);
		fprintf(stdout,"%s:%d selected roku at %u.%u.%u.%u:%u sn:%s\n",__FILE__,__LINE__,
				(reply_discover.ipv4>>0)&0xff, (reply_discover.ipv4>>8)&0xff, (reply_discover.ipv4>>16)&0xff, (reply_discover.ipv4>>24)&0xff,
				reply_discover.port,reply_discover.id_roku.usn);
		if (userstate->volume.current!=userstate->volume.target) (void)setdirty_volume(userstate);
	} else {
		(void)reset_printstate(userstate);
		fprintf(stdout,"%s:%d another roku is at %u.%u.%u.%u:%u sn:%s\n",__FILE__,__LINE__,
				(reply_discover.ipv4>>0)&0xff, (reply_discover.ipv4>>8)&0xff, (reply_discover.ipv4>>16)&0xff, (reply_discover.ipv4>>24)&0xff,
				reply_discover.port,reply_discover.id_roku.usn);
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

if (userstate->printstate!=AUTOMUTE_PRINTSTATE) {
	fputs("Countdown to unmute:",stdout);
	userstate->printstate=AUTOMUTE_PRINTSTATE;
}

if (secleft<=0) {
	int ign;
	userstate->automute.isactive=0;
	fputs("\rCountdown to unmute: 0 ... unmuting now\n",stdout);
#if 0
	if (userstate.menutype==AUTOMUTE_MENUTYPE) {
		userstate.menutype=MAIN_MENUTYPE;
		(ignore)printmenu(&userstate);
	}
#endif
	if (sendmute(&ign,userstate,discover,1)) GOTOERROR;
} else {
	fprintf(stdout,"\rCountdown to unmute: %d ",secleft);
	fflush(stdout);
}

return 0;
error:
	return -1;
}

int main(int argc, char **argv) {
struct userstate userstate;
struct discover discover;
struct getch getch;
uint32_t ipv4=0;
unsigned short port;
char *sn=NULL;
int discovertimeout=30;
int isprecmd=0;
int isinteractive=1;


clear_userstate(&userstate);
clear_discover(&discover);
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
		} else if (!strncmp(arg,"--D",3)) {
			if (setstring_userstate(&userstate,arg+3)) {
				fprintf(stderr,"%s:%d invalid name or value: %s\n",__FILE__,__LINE__,arg);
			}
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
	struct reply_discover reply_discover;
	int ign;
	clear_reply_discover(&reply_discover);
	if (check_discover(&ign,&reply_discover,&discover,-1)) GOTOERROR;
	if (!discover.found.ipv4) {
		fprintf(stderr,"%s:%d no roku device discovered\n",__FILE__,__LINE__);
		isinteractive=0;
	} else {
		int i;
		if (isinteractive) {
			fprintf(stdout,"%s:%d selected roku at %u.%u.%u.%u:%u sn:%s\n",__FILE__,__LINE__,
					(reply_discover.ipv4>>0)&0xff, (reply_discover.ipv4>>8)&0xff, (reply_discover.ipv4>>16)&0xff, (reply_discover.ipv4>>24)&0xff,
					reply_discover.port,reply_discover.id_roku.usn);
		}
		for (i=1;i<argc;i++) {
			char *arg=argv[i];
			if (!strncmp(arg,"--keypress_",11)) {
				int issent;
				if (sendkeypress(&issent,&discover,arg+11)) GOTOERROR;
				if (!issent) break;
			}
		}
	}
}
if (isinteractive) {
#define STDIN_POLLFDS	0
#define DISCOVER_POLLFDS	1
	struct pollfd pollfds[2];
	int isquit=0;

	pollfds[0].fd=STDIN_FILENO;
	pollfds[0].events=POLLIN;
	pollfds[0].revents=0; // might need to clear this on use
	pollfds[1].fd=-1;
	pollfds[1].events=POLLIN;
	pollfds[1].revents=0; // be sure to clear this on use

	if (init_getch(&getch,STDIN_FILENO)) GOTOERROR;

	while (!isquit) {
		int ms_poll,num_poll;

		if (getch.isnextchar) {
			if (step_getch(&isquit,&userstate,&discover,&getch)) GOTOERROR;
			continue;
		}

		(ignore)printmenu(&userstate);

		ms_poll=-1;
		num_poll=1;

		if (userstate.volume._isdirty) {
			ms_poll=0;
		} else if (userstate.automute.isactive) {
			ms_poll=1000;
		}
		if (discover.udp.fd>=0) {
			num_poll=2;
			pollfds[DISCOVER_POLLFDS].fd=discover.udp.fd;
		}
		switch (poll(pollfds,num_poll,ms_poll)) {
			case -1: if (errno==EAGAIN) continue; GOTOERROR;
			case 1: case 2:
				if (pollfds[DISCOVER_POLLFDS].revents) {
					pollfds[DISCOVER_POLLFDS].revents=0;
					if (step_discover(&userstate,&discover)) GOTOERROR;
				}
				if (pollfds[STDIN_POLLFDS].revents) {
					if (step_getch(&isquit,&userstate,&discover,&getch)) GOTOERROR;
				}
				break;
		}

		if (step_volume(&userstate,&discover)) GOTOERROR;
		if (step_automute(&userstate,&discover)) GOTOERROR;



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
