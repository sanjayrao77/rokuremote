/*
 * joystick.c - handle /dev/input gamepad input
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
#include <fcntl.h>
#ifdef __linux__
# include <linux/input.h>
#endif
#define DEBUG
#include "common/conventions.h"
#include "common/someutils.h"

#include "joystick.h"

// #define DEBUGONLY

#ifndef _INPUT_H
#warning joystick support disabled
#endif

#ifdef DEBUGONLY
static char *codetostring(int code) {
switch (code) {
	case 0: return "None";
	case LEFT_DPAD_JOYSTICK: return "LEFT_DPAD";
	case UP_DPAD_JOYSTICK: return "UP_DPAD";
	case RIGHT_DPAD_JOYSTICK: return "RIGHT_DPAD";
	case DOWN_DPAD_JOYSTICK: return "DOWN_DPAD";
	case LEFT_SHOULDER_JOYSTICK: return "LEFT_SHOULDER";
	case RIGHT_SHOULDER_JOYSTICK: return "RIGHT_SHOULDER";
	case A_JOYSTICK: return "A";
	case B_JOYSTICK: return "B";
	case X_JOYSTICK: return "X";
	case Y_JOYSTICK: return "Y";
	case START_JOYSTICK: return "START";
	case SELECT_JOYSTICK: return "SELECT";
	case xHOME_JOYSTICK: return "xHOME";
	case xSTART_JOYSTICK: return "xSTART";
	case xSELECT_JOYSTICK: return "xSELECT";
	case nLEFT_DPAD_JOYSTICK: return "nLEFT_DPAD";
	case nUP_DPAD_JOYSTICK: return "nUP_DPAD";
	case nRIGHT_DPAD_JOYSTICK: return "nRIGHT_DPAD";
	case nDOWN_DPAD_JOYSTICK: return "nDOWN_DPAD";
	case nA_JOYSTICK: return "nA";
	case nB_JOYSTICK: return "nB";
	case nX_JOYSTICK: return "nX";
	case nY_JOYSTICK: return "nY";
	case nLEFT_SHOULDER_JOYSTICK: return "nLEFT_SHOULDER";
	case nRIGHT_SHOULDER_JOYSTICK: return "nRIGHT_SHOULDER";
	case vLEFT_DPAD_JOYSTICK: return "vLEFT_DPAD";
	case vUP_DPAD_JOYSTICK: return "vUP_DPAD";
	case vRIGHT_DPAD_JOYSTICK: return "vRIGHT_DPAD";
	case vDOWN_DPAD_JOYSTICK: return "vDOWN_DPAD";
	case vA_JOYSTICK: return "vA";
	case vB_JOYSTICK: return "vB";
	case vX_JOYSTICK: return "vX";
	case vY_JOYSTICK: return "vY";
	case vLEFT_SHOULDER_JOYSTICK: return "vLEFT_SHOULDER";
	case vRIGHT_SHOULDER_JOYSTICK: return "vRIGHT_SHOULDER";
}
return "Unknown";
}
#endif

#ifdef _INPUT_H

#if 1
#define VILROS_SNESPAD
static int snespad_vilros_getbutton(int *code_out, struct joystick *js) {
struct input_event ev;
int code=0;

if (readn(js->fd,(unsigned char *)&ev,sizeof(ev))) GOTOERROR;
#define DOONE(a,b) do { if (ev.value) { code=a; js->repeat.code=0; js->bitmask|=b; } else { js->bitmask&=~b; goto nocode; } } while (0)
#define DPAD(a,b) do { code=a; js->repeat.code=a; js->repeat.isshortdelay=0; js->bitmask|=b; } while (0)

switch (ev.type) {
	case 0: case 4: goto nocode;
	case 1:
		switch (ev.code) {
			case 292: DOONE(LEFT_SHOULDER_JOYSTICK,BIT_LEFT_SHOULDER_JOYSTICK); break;
			case 293: DOONE(RIGHT_SHOULDER_JOYSTICK,BIT_RIGHT_SHOULDER_JOYSTICK); break;
			case 296: DOONE(SELECT_JOYSTICK,BIT_SELECT_JOYSTICK); break;
			case 297: DOONE(START_JOYSTICK,BIT_START_JOYSTICK); break;
			case 288: DOONE(X_JOYSTICK,BIT_X_JOYSTICK); break;
			case 289: DOONE(A_JOYSTICK,BIT_A_JOYSTICK); break;
			case 290: DOONE(B_JOYSTICK,BIT_B_JOYSTICK); break;
			case 291: DOONE(Y_JOYSTICK,BIT_Y_JOYSTICK); break;
		}
		break;
	case 3:
		switch (ev.code) {
			case 0:
				switch (ev.value) {
					case 0: DPAD(LEFT_DPAD_JOYSTICK,BIT_LEFT_DPAD_JOYSTICK); break;
					case 127:
						js->repeat.code=0;
						js->bitmask&=~(BIT_LEFT_DPAD_JOYSTICK|BIT_RIGHT_DPAD_JOYSTICK);
						goto nocode; break;
					case 255: DPAD(RIGHT_DPAD_JOYSTICK,BIT_RIGHT_DPAD_JOYSTICK); break;
				}
				break;
			case 1:
				switch (ev.value) {
					case 0: DPAD(UP_DPAD_JOYSTICK,BIT_UP_DPAD_JOYSTICK); break;
					case 127:
						js->repeat.code=0;
						js->bitmask&=~(BIT_UP_DPAD_JOYSTICK|BIT_DOWN_DPAD_JOYSTICK);
						goto nocode; break;
					case 255: DPAD(DOWN_DPAD_JOYSTICK,BIT_DOWN_DPAD_JOYSTICK); break;
				}
				break;
		}
		break;
}
#undef DOONE
#undef DPAD
if (!code) {
	fprintf(stderr,"%s:%d unknown number %u.%u.%u\n",__FILE__,__LINE__,ev.type,ev.code,ev.value);
}

*code_out=code;
return 0;
nocode:
	*code_out=0;
	return 0;
error:
	return -1;
}
#endif // VILROS_SNESPAD

#if 1
#define x8BITDO_LITE
static int x8bitdo_lite_bluetooth_getbutton(int *code_out, struct joystick *js) {
struct input_event ev;
int code=0;

if (readn(js->fd,(unsigned char *)&ev,sizeof(ev))) GOTOERROR;
#define DOONE(a,b) do { if (ev.value) { code=a; js->repeat.code=0; js->bitmask|=b; } else { js->bitmask&=~b; goto nocode; } } while (0)
#define DPAD(a,b) do { code=a; js->repeat.code=a; js->repeat.isshortdelay=0; js->bitmask|=b; } while (0)

switch (ev.type) {
	case 0: case 4: goto nocode;
	case 1:
		switch (ev.code) {
			case 308: DOONE(vLEFT_SHOULDER_JOYSTICK,BIT_LEFT_SHOULDER_JOYSTICK); break;
			case 309: DOONE(vRIGHT_SHOULDER_JOYSTICK,BIT_RIGHT_SHOULDER_JOYSTICK); break;
			case 310: DOONE(xSELECT_JOYSTICK,BIT_SELECT_JOYSTICK); break;
			case 311: DOONE(xSTART_JOYSTICK,BIT_START_JOYSTICK); break;
			case 307: DOONE(vX_JOYSTICK,BIT_X_JOYSTICK); break;
			case 305: DOONE(vA_JOYSTICK,BIT_A_JOYSTICK); break;
			case 304: DOONE(vB_JOYSTICK,BIT_B_JOYSTICK); break;
			case 306: DOONE(vY_JOYSTICK,BIT_Y_JOYSTICK); break;
			case 139: DOONE(xHOME_JOYSTICK,BIT_HOME_JOYSTICK); break;
//			case 312: DOONE(HOME_JOYSTICK,BIT_HOME_JOYSTICK); break; // pressing down on the left dpad
//			case 313: DOONE(HOME_JOYSTICK,BIT_HOME_JOYSTICK); break; // pressing down on the right dpad
		}
		break;
	case 3:
		switch (ev.code) {
			case 0: // left dpad, left/right
				switch (ev.value) {
					case 0: DPAD(vLEFT_DPAD_JOYSTICK,BIT_LEFT_DPAD_JOYSTICK); break;
					case 32768:
						js->repeat.code=0;
						js->bitmask&=~(BIT_LEFT_DPAD_JOYSTICK|BIT_RIGHT_DPAD_JOYSTICK);
						goto nocode; break;
					case 65535: DPAD(vRIGHT_DPAD_JOYSTICK,BIT_RIGHT_DPAD_JOYSTICK); break;
				}
				break;
			case 1: // left dpad, up/down
				switch (ev.value) {
					case 0: DPAD(vUP_DPAD_JOYSTICK,BIT_UP_DPAD_JOYSTICK); break;
					case 32768:
						js->repeat.code=0;
						js->bitmask&=~(BIT_UP_DPAD_JOYSTICK|BIT_DOWN_DPAD_JOYSTICK);
						goto nocode; break;
					case 65535: DPAD(vDOWN_DPAD_JOYSTICK,BIT_DOWN_DPAD_JOYSTICK); break;
				}
				break;
			case 2: // left shoulder, 2nd set
				switch (ev.value) {
					case 1023: DPAD(nLEFT_SHOULDER_JOYSTICK,BIT_LEFT_SHOULDER_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_LEFT_SHOULDER_JOYSTICK|BIT_RIGHT_SHOULDER_JOYSTICK);
						goto nocode; break;
				}
				break;
			case 3: // right dpad, left/right
				switch (ev.value) {
					case 0: DPAD(nLEFT_DPAD_JOYSTICK,BIT_LEFT_DPAD_JOYSTICK); break;
					case 32768:
						js->repeat.code=0;
						js->bitmask&=~(BIT_LEFT_DPAD_JOYSTICK|BIT_RIGHT_DPAD_JOYSTICK);
						goto nocode; break;
					case 65535: DPAD(nRIGHT_DPAD_JOYSTICK,BIT_RIGHT_DPAD_JOYSTICK); break;
				}
				break;
			case 4: // left dpad, up/down
				switch (ev.value) {
					case 0: DPAD(nUP_DPAD_JOYSTICK,BIT_UP_DPAD_JOYSTICK); break;
					case 32768:
						js->repeat.code=0;
						js->bitmask&=~(BIT_UP_DPAD_JOYSTICK|BIT_DOWN_DPAD_JOYSTICK);
						goto nocode; break;
					case 65535: DPAD(nDOWN_DPAD_JOYSTICK,BIT_DOWN_DPAD_JOYSTICK); break;
				}
				break;
			case 5: // right shoulder, 2nd set
				switch (ev.value) {
					case 1023: DPAD(nRIGHT_SHOULDER_JOYSTICK,BIT_RIGHT_SHOULDER_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_LEFT_SHOULDER_JOYSTICK|BIT_RIGHT_SHOULDER_JOYSTICK);
						goto nocode; break;
				}
				break;
			case 16: // left abxy
				switch (ev.value) {
					case 4294967295: DPAD(nY_JOYSTICK,BIT_Y_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_Y_JOYSTICK|BIT_A_JOYSTICK);
						goto nocode; break;
					case 1: DPAD(nA_JOYSTICK,BIT_A_JOYSTICK); break;
				}
				break;
			case 17: // left abxy
				switch (ev.value) {
					case 4294967295: DPAD(nX_JOYSTICK,BIT_X_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_X_JOYSTICK|BIT_B_JOYSTICK);
						goto nocode; break;
					case 1: DPAD(nB_JOYSTICK,BIT_B_JOYSTICK); break;
				}
				break;
		}
		break;
}
#undef DOONE
#undef DPAD
if (!code) {
	fprintf(stderr,"%s:%d unknown number %u.%u.%u\n",__FILE__,__LINE__,ev.type,ev.code,ev.value);
}
#ifdef DEBUGONLY
fprintf(stderr,"%s:%d DEBUG: code %d(%s), %u.%u.%u, bitmask:%x\n",__FILE__,__LINE__,code,codetostring(code),ev.type,ev.code,ev.value,
		js->bitmask);
code=0;
#endif

*code_out=code;
return 0;
nocode:
	*code_out=0;
	return 0;
error:
	return -1;
}
static int x8bitdo_lite_wired_getbutton(int *code_out, struct joystick *js) {
struct input_event ev;
int code=0;

if (readn(js->fd,(unsigned char *)&ev,sizeof(ev))) GOTOERROR;
#define DOONE(a,b) do { if (ev.value) { code=a; js->repeat.code=0; js->bitmask|=b; } else { js->bitmask&=~b; goto nocode; } } while (0)
#define DPAD(a,b) do { code=a; js->repeat.code=a; js->repeat.isshortdelay=0; js->bitmask|=b; } while (0)

switch (ev.type) {
	case 0: case 4: goto nocode;
	case 1:
		switch (ev.code) {
			case 310: DOONE(vLEFT_SHOULDER_JOYSTICK,BIT_LEFT_SHOULDER_JOYSTICK); break;
			case 311: DOONE(vRIGHT_SHOULDER_JOYSTICK,BIT_RIGHT_SHOULDER_JOYSTICK); break;
			case 314: DOONE(xSELECT_JOYSTICK,BIT_SELECT_JOYSTICK); break;
			case 315: DOONE(xSTART_JOYSTICK,BIT_START_JOYSTICK); break;
			case 308: DOONE(vX_JOYSTICK,BIT_X_JOYSTICK); break;
			case 305: DOONE(vA_JOYSTICK,BIT_A_JOYSTICK); break;
			case 304: DOONE(vB_JOYSTICK,BIT_B_JOYSTICK); break;
			case 307: DOONE(vY_JOYSTICK,BIT_Y_JOYSTICK); break;
			case 316: DOONE(xHOME_JOYSTICK,BIT_HOME_JOYSTICK); break;
//			case 317: DOONE(HOME_JOYSTICK,BIT_HOME_JOYSTICK); break; // pressing down on the left dpad
//			case 318: DOONE(HOME_JOYSTICK,BIT_HOME_JOYSTICK); break; // pressing down on the right dpad
		}
		break;
	case 3:
		switch (ev.code) {
			case 0: // left dpad, left/right
				switch (ev.value) {
					case 4294934528: DPAD(vLEFT_DPAD_JOYSTICK,BIT_LEFT_DPAD_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_LEFT_DPAD_JOYSTICK|BIT_RIGHT_DPAD_JOYSTICK);
						goto nocode; break;
					case 32512: DPAD(vRIGHT_DPAD_JOYSTICK,BIT_RIGHT_DPAD_JOYSTICK); break;
				}
				break;
			case 1: // left dpad, up/down
				switch (ev.value) {
					case 4294934783: DPAD(vUP_DPAD_JOYSTICK,BIT_UP_DPAD_JOYSTICK); break;
					case 4294967295:
						js->repeat.code=0;
						js->bitmask&=~(BIT_UP_DPAD_JOYSTICK|BIT_DOWN_DPAD_JOYSTICK);
						goto nocode; break;
					case 32767: DPAD(vDOWN_DPAD_JOYSTICK,BIT_DOWN_DPAD_JOYSTICK); break;
				}
				break;
			case 2: // left shoulder, 2nd set
				switch (ev.value) {
					case 255: DPAD(nLEFT_SHOULDER_JOYSTICK,BIT_LEFT_SHOULDER_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_LEFT_SHOULDER_JOYSTICK);
						goto nocode; break;
				}
				break;
			case 3: // right dpad, left/right
				switch (ev.value) {
					case 4294934528: DPAD(nLEFT_DPAD_JOYSTICK,BIT_LEFT_DPAD_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_LEFT_DPAD_JOYSTICK|BIT_RIGHT_DPAD_JOYSTICK);
						goto nocode; break;
					case 32512: DPAD(nRIGHT_DPAD_JOYSTICK,BIT_RIGHT_DPAD_JOYSTICK); break;
				}
				break;
			case 4: // left dpad, up/down
				switch (ev.value) {
					case 4294934783: DPAD(nUP_DPAD_JOYSTICK,BIT_UP_DPAD_JOYSTICK); break;
					case 4294967295:
						js->repeat.code=0;
						js->bitmask&=~(BIT_UP_DPAD_JOYSTICK|BIT_DOWN_DPAD_JOYSTICK);
						goto nocode; break;
					case 32767: DPAD(nDOWN_DPAD_JOYSTICK,BIT_DOWN_DPAD_JOYSTICK); break;
				}
				break;
			case 5: // right shoulder, 2nd set
				switch (ev.value) {
					case 255: DPAD(nRIGHT_SHOULDER_JOYSTICK,BIT_RIGHT_SHOULDER_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_RIGHT_SHOULDER_JOYSTICK);
						goto nocode; break;
				}
				break;
			case 16: // left abxy
				switch (ev.value) {
					case 4294967295: DPAD(nY_JOYSTICK,BIT_Y_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_Y_JOYSTICK|BIT_A_JOYSTICK);
						goto nocode; break;
					case 1: DPAD(nA_JOYSTICK,BIT_A_JOYSTICK); break;
				}
				break;
			case 17: // left abxy
				switch (ev.value) {
					case 4294967295: DPAD(nX_JOYSTICK,BIT_X_JOYSTICK); break;
					case 0:
						js->repeat.code=0;
						js->bitmask&=~(BIT_X_JOYSTICK|BIT_B_JOYSTICK);
						goto nocode; break;
					case 1: DPAD(nB_JOYSTICK,BIT_B_JOYSTICK); break;
				}
				break;
		}
		break;
}
#undef DOONE
#undef DPAD
if (!code) {
	fprintf(stderr,"%s:%d unknown number %u.%u.%u\n",__FILE__,__LINE__,ev.type,ev.code,ev.value);
}
#ifdef DEBUGONLY
fprintf(stderr,"%s:%d DEBUG: code %d(%s), %u.%u.%u, bitmask:%x\n",__FILE__,__LINE__,code,codetostring(code),ev.type,ev.code,ev.value,
		js->bitmask);
code=0;
#endif

*code_out=code;
return 0;
nocode:
	*code_out=0;
	return 0;
error:
	return -1;
}
#endif // 8BITDO_LITE


#endif // _INPUT_H

void clear_joystick(struct joystick *js) {
static struct joystick blank={.fd=-1};
*js=blank;
}

#if 0
static int noop_getbutton(int *code_out, struct joystick *js) {
*code_out=0;
return 0;
}
#endif

static void reopen_joystick(struct joystick *js) {
if (!js->device.filename[0]) return;
if (access(js->device.filename,R_OK)) return;
ifclose(js->fd);
js->fd=open(js->device.filename,O_RDONLY);
#if 0
WHEREAMI;
printstatus_joystick(js);
#endif
}

static int setmapping(struct joystick *js, char *mappingname) {
if (!mappingname) {
	js->mapping.func=NULL;
	js->device.type=0;
}
#ifdef VILROS_SNESPAD
else if (!strcmp(mappingname,"vilros_snespad")) {
	js->mapping.func=snespad_vilros_getbutton;
	js->device.type=SNES_TYPE_DEVICE_JOYSTICK;
}
#endif
#ifdef x8BITDO_LITE
else if (!strcmp(mappingname,"8bitdo_lite_bluetooth")) {
	js->mapping.func=x8bitdo_lite_bluetooth_getbutton;
	js->device.type=FULL_TYPE_DEVICE_JOYSTICK;
}
else if (!strcmp(mappingname,"8bitdo_lite_wired")) {
	js->mapping.func=x8bitdo_lite_wired_getbutton;
	js->device.type=FULL_TYPE_DEVICE_JOYSTICK;
}
#endif
else {
	fprintf(stderr,"%s:%d joystickname \"%s\" is unknown\n",__FILE__,__LINE__,mappingname);
	GOTOERROR;
}
js->mapping.name=mappingname;
return 0;
error:
	return -1;
}

int detect_joystick(struct joystick *js) {
char eventname[16];
switch (js->detect.mode) {
	case NONE_MODE_DETECT_JOYSTICK:
		break;
	case MANUAL_MODE_DETECT_JOYSTICK:
		(void)reopen_joystick(js);
		break;
	case AUTO_MODE_DETECT_JOYSTICK:
		{
			char *mapping=NULL;
			if (findanydevice_joystick(&mapping,eventname,16)) GOTOERROR;
			if (*eventname) snprintf(js->device.filename,MAX_FILENAME_DEVICE_JOYSTICK+1,"/dev/input/%s",eventname);
			else js->device.filename[0]=0;
			if (setmapping(js,mapping)) GOTOERROR;
			(void)reopen_joystick(js);
		}
		break;
	case MODELNAME_MODE_DETECT_JOYSTICK:
		{
			if (finddevice_joystick(eventname,16,js->detect.modelname)) GOTOERROR;
			if (*eventname) snprintf(js->device.filename,MAX_FILENAME_DEVICE_JOYSTICK+1,"/dev/input/%s",eventname);
			else js->device.filename[0]=0;
			(void)reopen_joystick(js);
		}
		break;
	default: GOTOERROR;
}
return 0;
error:
	return -1;
}

int manual_set_joystick(struct joystick *js, char *devname, char *mappingname) {
if (!mappingname) GOTOERROR;
if (!devname) GOTOERROR;

if (setmapping(js,mappingname)) GOTOERROR;

if (strlen(devname)>MAX_FILENAME_DEVICE_JOYSTICK) GOTOERROR;
strcpy(js->device.filename,devname);

js->detect.mode=MANUAL_MODE_DETECT_JOYSTICK;

(ignore)detect_joystick(js);
return 0;
error:
	return -1;
}

int modelname_set_joystick(struct joystick *js, char *modelname, char *mappingname) {
if (!mappingname) GOTOERROR;

if (setmapping(js,mappingname)) GOTOERROR;

js->detect.modelname=modelname;
js->detect.mode=MODELNAME_MODE_DETECT_JOYSTICK;
(ignore)detect_joystick(js);
return 0;
error:
	return -1;
}

int auto_set_joystick(struct joystick *js, char *modelname) {
js->detect.mode=AUTO_MODE_DETECT_JOYSTICK;
(ignore)detect_joystick(js);
return 0;
}

void deinit_joystick(struct joystick *js) {
ifclose(js->fd);
}

int getbutton_joystick(int *code_out, struct joystick *js) {
int r,code;
r=js->mapping.func(&code,js);
if (r) return r;
if (code) {
	if (code==js->lastcode) {
		js->inarow+=1;
	} else {
		js->inarow=1;
		js->lastcode=code;
	}
}
*code_out=code;
return 0;
}

int scan_joystick(FILE *fout) {
FILE *fin=NULL;
char oneline[256];
char name[128];
char handlers[128];
unsigned int vendor,product;

name[127]=0; handlers[127]=0;
name[0]=0; handlers[0]=0; 
vendor=0; product=0;

if (!(fin=fopen("/proc/bus/input/devices","r"))) GOTOERROR;
while (1) {
	char *temp;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
	if (!fgets(oneline,256,fin)) break;
	if (oneline[0]=='\n') {
		if (*name && *handlers) {
			fprintf(fout,"%s \"%s\" Vendor:%x Product:%x -> %s\n",strstr(handlers,"js")?">>":"..",name,vendor,product,handlers);
		}
		name[0]=0; handlers[0]=0;
		vendor=0; product=0;
	} else if (!strncmp(oneline,"I: ",3)) {
		temp=strstr(oneline,"Vendor=");
		if (temp) vendor=hextou(temp+7);
		temp=strstr(oneline,"Product=");
		if (temp) product=hextou(temp+8);
	} else if (!strncmp(oneline,"N: Name=\"",9)) {
		strncpy(name,oneline+9,127);
		temp=strchr(name,'\"');
		if (!temp) GOTOERROR;
		*temp=0;
	} else if (!strncmp(oneline,"H: Handlers=",12)) {
		strncpy(handlers,oneline+12,127);
		temp=strchr(handlers,'\n');
		if (!temp) GOTOERROR;
		*temp=0;
	}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

fclose(fin);
return 0;
error:
	iffclose(fin);
	return -1;
}
int finddevice_joystick(char *dest, unsigned int destlen, char *modelname) {
FILE *fin=NULL;
char oneline[256];
char name[128];
char handlers[128];

name[127]=0; handlers[127]=0;
name[0]=0; handlers[0]=0;

if (!(fin=fopen("/proc/bus/input/devices","r"))) GOTOERROR;
while (1) {
	char *temp;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
	if (!fgets(oneline,256,fin)) break;
	if (oneline[0]=='\n') {
		if (!strcmp(name,modelname)) {
			temp=strstr(handlers,"event");
			if (temp) {
				char *temp2;
				temp2=strchr(temp,' ');
				if (temp2) *temp2=0;
				if (strlen(temp)>=destlen) GOTOERROR;
				strcpy(dest,temp);
				return 0;
			}
		}
		name[0]=0; handlers[0]=0;
	} else if (!strncmp(oneline,"N: Name=\"",9)) {
		strncpy(name,oneline+9,127);
		temp=strchr(name,'\"');
		if (!temp) GOTOERROR;
		*temp=0;
	} else if (!strncmp(oneline,"H: Handlers=",12)) {
		strncpy(handlers,oneline+12,127);
		temp=strchr(handlers,'\n');
		if (!temp) GOTOERROR;
		*temp=0;
	}
#ifdef __GNUCC__
#pragma GCC diagnostic pop
#endif
}

fclose(fin);
dest[0]=0;
return 0;
error:
	iffclose(fin);
	return -1;
}

struct device_joystick {
	char *nickname; // this is not used, it's only for reference
	unsigned int vendor;
	unsigned int product;
	char *mappingname;
};

static struct device_joystick devices_global[]={
{"8BitDo Lite gamepad",0x45e,0x2e0,"8bitdo_lite_bluetooth"},
{"Microsoft X-Box 360 Pad",0x45e,0x28e,"8bitdo_lite_wired"},
{"USB Gamepad ",0x79,0x11,"vilros_snespad"},
{NULL,0,0,NULL}
};


int findanydevice_joystick(char **mapping_out, char *dest, unsigned int destlen) {
FILE *fin=NULL;
char oneline[256];
char name[128];
char handlers[128];
unsigned int vendor,product;

name[127]=0; handlers[127]=0;
name[0]=0; handlers[0]=0;
vendor=0; product=0;

if (!(fin=fopen("/proc/bus/input/devices","r"))) GOTOERROR;
while (1) {
	char *temp;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
	if (!fgets(oneline,256,fin)) break;
	if (oneline[0]=='\n') {
		struct device_joystick *dev;
		for (dev=devices_global;dev->nickname;dev+=1) {
			if ((dev->vendor==vendor)&&(dev->product==product)) {
				temp=strstr(handlers,"event");
				if (temp) {
					char *temp2;
					temp2=strchr(temp,' ');
					if (temp2) *temp2=0;
					if (strlen(temp)>=destlen) GOTOERROR;
					strcpy(dest,temp);
					*mapping_out=dev->mappingname;
					return 0;
				}
			}
		}
		name[0]=0; handlers[0]=0;
		vendor=0; product=0;
	} else if (!strncmp(oneline,"I: ",3)) {
		temp=strstr(oneline,"Vendor=");
		if (temp) vendor=hextou(temp+7);
		temp=strstr(oneline,"Product=");
		if (temp) product=hextou(temp+8);
	} else if (!strncmp(oneline,"N: Name=\"",9)) {
		strncpy(name,oneline+9,127);
		temp=strchr(name,'\"');
		if (!temp) GOTOERROR;
		*temp=0;
	} else if (!strncmp(oneline,"H: Handlers=",12)) {
		strncpy(handlers,oneline+12,127);
		temp=strchr(handlers,'\n');
		if (!temp) GOTOERROR;
		*temp=0;
	}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

fclose(fin);
*mapping_out=NULL;
dest[0]=0;
return 0;
error:
	iffclose(fin);
	return -1;
}

static char *tostring_devicetype(int type) {
switch (type) {
	case FULL_TYPE_DEVICE_JOYSTICK: return "full";
	case SNES_TYPE_DEVICE_JOYSTICK: return "snes";
}
return "Unknown";
}

static char *tostring_detection(int mode) {
switch (mode) {
	case NONE_MODE_DETECT_JOYSTICK: return "none";
	case MANUAL_MODE_DETECT_JOYSTICK: return "manual";
	case AUTO_MODE_DETECT_JOYSTICK: return "auto";
	case MODELNAME_MODE_DETECT_JOYSTICK: return "modelname";
}
return "Unknown";
}

int printstatus_joystick(struct joystick *js, FILE *fout) {
fprintf(fout,"Joystick status:%s, device:%s, mapping:%s, devicetype:%s detection:%s\n",
		(js->fd>=0)?"ON":"OFF",js->device.filename,js->mapping.name,
		tostring_devicetype(js->device.type),
		tostring_detection(js->detect.mode));
return 0;
}
