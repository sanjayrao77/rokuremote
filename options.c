/*
 * options.c - manage command line arguments
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
#include <arpa/inet.h>
#define DEBUG
#include "common/conventions.h"
#include "common/someutils.h"
#include "roku.h"
#include "discover.h"
#include "automute.h"
#include "joystick.h"
#include "userstate.h"

#include "options.h"

int printusage_options(FILE *fout) {
fputs("Usage:\n",fout);
fputs("\tautodetect: rokuremote\n",fout);
fputs("\tconnect to serial number: rokuremote sn:SERIALNUMBER\n",fout);
fputs("\tconnect to ip: rokuremote ip:IPADDRESS[:port]\n",fout);
fputs("\tsend keypress: rokuremote --keypress_XXX --keypress_YYY\n",fout);
fputs("\tdon't run interactively: rokuremote --keypress_XXX --quit\n",fout);
fputs("\tset a settings value: --Dname=value\n",fout);
fputs("\tprint operations: --verbose\n",fout);
fputs("\tdon't use stdin or stdout: --nostdin\n",fout);
fputs("\trun in background: --background\n",fout);
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

int setstring_options(struct userstate *u, char *str) {
if (!strncmp(str,"fullvolume=",11)) {
	u->volume.full=slowtou(str+11);
	u->volume.target=u->volume.full;
	u->volume.current=u->volume.full;
#if 0
} else if (!strncmp(str,"automute=",9)) {
	u->def_automute=slowtou(str+9);
#endif
} else if (!strncmp(str,"incvolumedelay=",15)) {
	u->delays.up_volumestep=1000*slowtou(str+15);
} else if (!strncmp(str,"decvolumedelay=",15)) {
	u->delays.down_volumestep=1000*slowtou(str+15);
} else if (!strncmp(str,"multicastipv4=",14)) {
	u->options.multicastipv4=inet_addr(str+14);
} else if (!strncmp(str,"powerquit=",10)) {
	switch (str[10]) {
		case 'y': case 'Y': case '1': case 't': case 'T': u->options.isquitonpoweroff=1; break;
		case 'n': case 'N': case '0': case 'f': case 'F': u->options.isquitonpoweroff=0; break;
		default: GOTOERROR;
	}
} else {
	fprintf(stderr,"%s:%d unknown command line option: \"%s\"\n",__FILE__,__LINE__,str);
	GOTOERROR;
}

return 0;
error:
	return -1;
}

int parseargs_options(struct mainoptions *main, struct userstate *userstate, int argc, char **argv) {
int i;

for (i=1;i<argc;i++) {
	char *arg=argv[i];
	if (!strncmp(arg,"sn:",3)) {
		main->sn=arg+3;
		if (strlen(main->sn)>MAX_USN_ROKU) {
			fprintf(stderr,"%s:%d invalid serial number: \"%s\"\n",__FILE__,__LINE__,arg);
			GOTOERROR;
		}
	} else if (!strncmp(arg,"ip:",3)) {
		if (parseipv4(&main->ipv4,&main->port,arg+3,8060)) {
			fprintf(stderr,"%s:%d unparsed ip: \"%s\"\n",__FILE__,__LINE__,arg);
			GOTOERROR;
		}
	} else if (!strcmp(arg,"--verbose") || !strcmp(arg,"-v")) {
		userstate->terminal.isverbose=1;
	} else if (!strcmp(arg,"--stepmute")) {
		userstate->options.isstepmute=1;
	} else if (!strncmp(arg,"--joystick_devfile=",19)) {
		main->isanyjoystick=1;
		main->joystick.devicefile=arg+19;
	} else if (!strncmp(arg,"--joystick_mapping=",19)) {
		main->isanyjoystick=1;
		main->joystick.mapping=arg+19;
	} else if (!strncmp(arg,"--joystick_modelname=",21)) {
		main->isanyjoystick=1;
		main->joystick.modelname=arg+21;
	} else if (!strcmp(arg,"--joystick")) {
		main->isanyjoystick=1;
		main->joystick.isauto=1;
	} else if (!strcmp(arg,"--joystick-scan")) {
		main->isanyjoystick=1;
		main->joystick.isscan=1;
	} else if (!strcmp(arg,"--nostdin")) {
		main->stdinfd=-1;
		userstate->terminal.issilent=1;
	} else if (!strcmp(arg,"--background")) {
		main->stdinfd=-1;
		userstate->terminal.issilent=1;
		main->isbackground=1;
	} else if (!strcmp(arg,"--quit")) {
		main->isinteractive=0;
		main->discovertimeout=5;
	} else if (!strncmp(arg,"--keypress_",11)) {
		main->isprecmd=1;
	} else if (!strncmp(arg,"--sleep_",8)) {
		main->isprecmd=1;
	} else if (!strncmp(arg,"--query",7)) {
		main->isprecmd=1;
		main->isinteractive=0;
		main->discovertimeout=5;
	} else if (!strncmp(arg,"-D",2)) {
		if (setstring_options(userstate,arg+2)) {
			fprintf(stderr,"%s:%d invalid name or value: %s\n",__FILE__,__LINE__,arg);
			GOTOERROR;
		}
	} else if (!strncmp(arg,"--D",3)) {
		if (setstring_options(userstate,arg+3)) {
			fprintf(stderr,"%s:%d invalid name or value: %s\n",__FILE__,__LINE__,arg);
			GOTOERROR;
		}
	} else if (!strcmp(arg,"--help") || !strcmp(arg,"-h")) {
		(ignore)printusage_options(stdout);
		main->isquit=1;
		return 0;
	} else {
		fprintf(stderr,"%s:%d unrecognized argument: \"%s\"\n",__FILE__,__LINE__,arg);
		GOTOERROR;
	}
}

return 0;
error:
	return -1;
}

void setdevice_options(struct userstate *userstate, struct discover *discover) {
if (!discover->found.device.ismute) userstate->options.isstepmute=1;
}
