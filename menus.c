/*
 * menus.c - user interaction on the terminal
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
#include <termios.h>
#define DEBUG
#include "common/conventions.h"
#include "getch.h"
#include "roku.h"
#include "discover.h"
#include "automute.h"
#include "joystick.h"
#include "userstate.h"
#include "action.h"

#include "menus.h"

static inline void voidinit_uintentry(struct userstate *u, int fieldindex, unsigned int defvalue) {
u->menus.uintentry.isedited=0;
u->menus.type=UINTENTRY_TYPE_MENU;
u->menus.uintentry.fieldindex=fieldindex;
u->menus.uintentry.value=defvalue;
}
static inline void voidinit_boolentry(struct userstate *u, int fieldindex) {
u->menus.boolentry.isedited=0;
u->menus.type=BOOLENTRY_MENUTYPE;
u->menus.boolentry.fieldindex=fieldindex;
u->menus.boolentry.value=0;
}

int snes_joystick_printmenu(void) {
fprintf(stdout,"Roku remote, SNES joystick buttons:\n");
fprintf(stdout,"   Select            :   Enter navigation mode (default)\n");
fprintf(stdout,"   Start             :   Enter viewing mode\n");
fprintf(stdout,"   Select+Start      :   Power toggle\n");
fprintf(stdout,"   A                 :   Volume up\n");
fprintf(stdout,"   B                 :   Volume down\n");
fprintf(stdout,"   Select+A          :   Direct volume up\n");
fprintf(stdout,"   Select+B          :   Direct volume down\n");
fprintf(stdout,"   Y                 :   Info\n");
fprintf(stdout,"   Right Shoulder    :   Select\n");
fprintf(stdout,"   Navigation mode   :   (press Select button)\n");
fprintf(stdout,"     Left Shoulder   :   Back\n");
fprintf(stdout,"     Arrows          :   Move cursor\n");
fprintf(stdout,"     Select+Left     :   Move left 40 times\n");
fprintf(stdout,"     Select+Right    :   Move right 40 times\n");
fprintf(stdout,"     X               :   Home\n");
fprintf(stdout,"     X, X, X, X      :   Reboot\n");
fprintf(stdout,"   Viewing mode      :   (press Start button)\n");
fprintf(stdout,"     Left Shoulder   :   InstantReplay\n");
fprintf(stdout,"     Left            :   Automute +30 seconds\n");
fprintf(stdout,"     Select+Left     :   Rev\n");
fprintf(stdout,"     Up              :   Automute +10 seconds\n");
fprintf(stdout,"     Right           :   Automute +60 seconds\n");
fprintf(stdout,"     Select+Right    :   Fwd\n");
fprintf(stdout,"     Down            :   Automute +5 seconds\n");
fprintf(stdout,"     X               :   Play/Pause\n");
return 0;
}
int full_joystick_printmenu(void) {
fprintf(stdout,"Roku remote, full joystick buttons:\n");
fprintf(stdout,"   Select+Home     :   Power toggle\n");
fprintf(stdout,"   Home, Home, ... :   Home\n");
fprintf(stdout,"   Home, 5 times   :   Reboot\n");
fprintf(stdout,"   Shoulder L1     :   InstantReplay\n");
fprintf(stdout,"   Select+L1       :   Rev\n");
fprintf(stdout,"   Shoulder L2     :   Info\n");
fprintf(stdout,"   Shoulder R1     :   Select\n");
fprintf(stdout,"   Select+R1       :   Fwd\n");
fprintf(stdout,"   Shoulder R2     :   Back\n");
fprintf(stdout,"   Left D-Pad\n");
fprintf(stdout,"     Left          :   Automute +30 seconds\n");
fprintf(stdout,"     Up            :   Automute +10 seconds\n");
fprintf(stdout,"     Right         :   Automute +60 seconds\n");
fprintf(stdout,"     Down          :   Automute +5 seconds\n");
fprintf(stdout,"   Right D-Pad\n");
fprintf(stdout,"     Arrows        :   Move cursor\n");
fprintf(stdout,"     Select+Left   :   Move left 40 times\n");
fprintf(stdout,"     Select+Right  :   Move right 40 times\n");
fprintf(stdout,"   Either button set\n");
fprintf(stdout,"     X             :   Play/Pause\n");
fprintf(stdout,"     Y             :   5 second automute + Select\n");
fprintf(stdout,"     A             :   Volume up\n");
fprintf(stdout,"     B             :   Volume down\n");
fprintf(stdout,"     Select+A      :   Direct volume up\n");
fprintf(stdout,"     Select+B      :   Direct volume down\n");
return 0;
}

int joystick_printmenu(struct userstate *uo) {
if (!uo->joystick.isjoystick) return 0;
switch (uo->joystick.devicetype) {
	case SNES_TYPE_DEVICE_JOYSTICK: (ignore)snes_joystick_printmenu(); break;
	case FULL_TYPE_DEVICE_JOYSTICK: (ignore)full_joystick_printmenu(); break;
}
return 0;
}

int print_menus(struct userstate *uo) {
if (uo->terminal.issilent) return 0;
if (uo->menus.lastprinted==uo->menus.type) return 0;
uo->menus.lastprinted=uo->menus.type;

(void)reset_printstate(uo);

switch (uo->menus.type) {
#if 0
	case AUTOMUTE_MENUTYPE:
		fprintf(stdout,"Roku remote, automute settings:\n");
		fprintf(stdout,"   Right                :   Add a second\n");
		fprintf(stdout,"   Up                   :   Add 5 seconds\n");
		fprintf(stdout,"   z,Z                  :   Add %u/%u seconds\n",uo->def_automute,2*uo->def_automute);
		fprintf(stdout,"   Left                 :   Subtract a second\n");
		fprintf(stdout,"   Down                 :   Subtract 5 seconds\n");
		fprintf(stdout,"   Enter                :   Specify remaining seconds\n");
		fprintf(stdout,"   Backspace            :   Unmute now\n");
		fprintf(stdout,"   -,_                  :   Volume down \n");
		fprintf(stdout,"   +,=                  :   Volume up \n");
		fprintf(stdout,"   anything else        :   Main menu\n");
		break;
#endif
	case SETTINGS_TYPE_MENU:
		fprintf(stdout,"Roku remote, settings menu:\n");
		fprintf(stdout,"   v                    :   Set tv volume (%u)\n",uo->volume.full);
		fprintf(stdout,"   u                    :   Set up volume delay (%u)\n",uo->delays.up_volumestep);
		fprintf(stdout,"   d                    :   Set down volume delay (%u)\n",uo->delays.down_volumestep);
		fprintf(stdout,"   p                    :   Quit on PowerOff (%s)\n",uo->options.isquitonpoweroff?"True":"False");
#if 0
		fprintf(stdout,"   t                    :   Tie volume change to tv volume (%s)\n",
				(uo->istievolume)?"True":"False");
#endif
#if 0
		fprintf(stdout,"   z                    :   Change automute seconds (%u)\n",uo->def_automute);
#endif
		fprintf(stdout,"   anything else        :   Main menu\n");
		break;
	case REBOOT_TYPE_MENU:
		fprintf(stdout,"Roku remote, confirm reboot sequence:\n");
		fprintf(stdout,"   y                    :   Send reboot keypresses\n");
		fprintf(stdout,"   anything else        :   Cancel\n");
		break;
	case POWER_TYPE_MENU:
		fprintf(stdout,"Roku remote, confirm power-off:\n");
		fprintf(stdout,"   F                    :   Off; send PowerOff keypress\n");
		fprintf(stdout,"   T                    :   Toggle; send Power keypress\n");
		fprintf(stdout,"   anything else        :   Cancel\n");
		break;
	case KEYBOARD_TYPE_MENU:
		fprintf(stdout,"Roku remote, keyboard mode:\n");
		fprintf(stdout,"   a-z,0-9,Backspace    :   send key\n");
		fprintf(stdout,"   Left,Right,Up,Down   :   send key and exit keyboard mode\n");
		fprintf(stdout,"   Tab,Esc              :   exit keyboard mode\n");
		break;
	case MAIN_TYPE_MENU:
		(ignore)joystick_printmenu(uo);
		fprintf(stdout,"Roku remote, commands:\n");
		fprintf(stdout,"   a                    :   Enter keyboard mode\n");
		fprintf(stdout,"   Left/Right/Up/Down   :   Arrows\n");
		fprintf(stdout,"   Backspace            :   Back\n");
		fprintf(stdout,"   Enter                :   Select\n");
		fprintf(stdout,"   Escape               :   Home\n");
		fprintf(stdout,"   Space                :   Play/Pause\n");
		fprintf(stdout,"   < / >                :   Reverse / Forward\n");
		fprintf(stdout,"   -,_  / +,=           :   Volume down / up\n");
		fprintf(stdout,"   ?                    :   Search\n");
		fprintf(stdout,"   1/2/3/4              :   InputHDMI1/2/3/4\n");
		fprintf(stdout,"   0/5                  :   InputTuner/InputAV1\n");
		fprintf(stdout,"   d                    :   Discover roku devices\n");
		fprintf(stdout,"   f                    :   Find remote\n");
		fprintf(stdout,"   i                    :   Information\n");
		if (uo->options.isnomute) {
			fprintf(stdout,"   m/M                  :   Volume down/up %u steps\n",uo->volume.full);
		} else {
			fprintf(stdout,"   m                    :   Mute toggle\n");
		}
		fprintf(stdout,"   p                    :   Power menu\n");
		fprintf(stdout,"   q                    :   Quit\n");
		fprintf(stdout,"   Q                    :   Reboot\n");
		fprintf(stdout,"   r                    :   Instant Replay\n");
		fprintf(stdout,"   s                    :   Settings menu\n");
		fprintf(stdout,"   y                    :   Mute for 5 seconds and send Select\n");
		fprintf(stdout,"   z/x/c/v              :   Auto-mute (%d/%d/%d/%d seconds)\n",60,30,10,5);
		fprintf(stdout,"   Z/X/C/V              :   Reduce or cancel auto-mute (%d/%d/%d/%d seconds)\n",60,30,10,5);
		break;
}
fflush(stdout);
return 0;
}

static void printprompt_boolentry(struct userstate *userstate, int which) {
if (which<0) {
	(ignore)fputs("Value (y/n): ",stdout);
	(ignore)fflush(stdout);
} else if (!which) {
	if (userstate->menus.boolentry.value) (ignore)fputs("True",stdout);
	else (ignore)fputs("False",stdout);
	(ignore)fputc('\n',stdout);
} else {
	(ignore)fputc('\n',stdout);
}
}

static void printprompt_uintentry(struct userstate *userstate, int which) {
if (which<0) {
	if (userstate->menus.uintentry.value) {
		(ignore)fprintf(stdout,"Value: %u",userstate->menus.uintentry.value); // this is odd, unused
	} else {
		(ignore)fputs("Value: ",stdout);
	}
	(ignore)fflush(stdout);
} else if (!which) {
	if (userstate->menus.uintentry.value) {
		char buff20[20];
		snprintf(buff20,20,"%u",userstate->menus.uintentry.value);
		(ignore)fprintf(stdout,"\rValue: %s \rValue: %s",buff20,buff20);
	} else {
		(ignore)fputs("\rValue:  \rValue: ",stdout);
	}
	(ignore)fflush(stdout);
} else {
	(ignore)fputc('\n',stdout);
}
}

int handlekey_menus(int *isquit_out, int *nextkey_out, struct userstate *userstate, struct discover *d, int ch) {
int nextkey=0;
int isquit=0,isquit2=0;
int ign;
char *keypress=NULL;
char **keypresses=NULL;
char buff8[8];

switch (userstate->menus.type) {
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
				(void)voidinit_uintentry(userstate,SET_AUTOMUTE_FIELDINDEX_MENU,0);
				(void)printprompt_uintentry(userstate,-1);
			break;
			case 8: case 127:
				userstate->automute.stop=userstate->automute.start-1;
				break;
			default:
				userstate->menus.type=MAIN_TYPE_MENU;
				userstate->automute.isactive=0;
				break;
		}
		break;
#endif
	case BOOLENTRY_MENUTYPE:
		{
			int value=0;
			switch (ch) {
				case 'y': case 'Y': case '1': case 't': case 'T':
					value=1;
				// no break
				case 'n': case 'N': case '0': case 'f': case 'F':
					userstate->menus.boolentry.value=value;
					userstate->menus.boolentry.isedited=1;
					(void)printprompt_boolentry(userstate,0);
					if (userstate->menus.boolentry.isedited) {
						switch (userstate->menus.boolentry.fieldindex) {
							case QUITPOWER_FIELDINDEX_MENU: userstate->options.isquitonpoweroff=userstate->menus.boolentry.value; break;
						}
					}
					userstate->menus.type=SETTINGS_TYPE_MENU;
					break;
				default:
					(void)printprompt_boolentry(userstate,1);
					userstate->menus.type=SETTINGS_TYPE_MENU;
					break;
			}
		}
		break;
	case UINTENTRY_TYPE_MENU:
		switch (ch) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				userstate->menus.uintentry.value=userstate->menus.uintentry.value*10+ch-48;
				userstate->menus.uintentry.isedited=1;
				(void)printprompt_uintentry(userstate,0);
				break;
			case 8: case 127:
				userstate->menus.uintentry.value/=10;
				(void)printprompt_uintentry(userstate,0);
				break;
			case '\n': case '\r':
				if (userstate->menus.uintentry.isedited) {
					switch (userstate->menus.uintentry.fieldindex) {
#if 0
						case DEF_AUTOMUTE_FIELDINDEX_MENU: userstate->def_automute=userstate->menus.uintentry.value; break;
#endif
#if 0
						case SET_AUTOMUTE_FIELDINDEX_MENU: userstate->automute.stop=time(NULL)-userstate->automute.muteseconds+userstate->menus.uintentry.value; break;
#endif
						case FULLVOLUME_FIELDINDEX_MENU:
							userstate->volume.full=userstate->menus.uintentry.value;
							userstate->volume.target=userstate->volume.full;
							userstate->volume.current=userstate->volume.full;
							break;
						case UP_VOLUMEDELAY_FIELDINDEX_MENU:
							userstate->delays.up_volumestep=1000*userstate->menus.uintentry.value;
							break;
						case DOWN_VOLUMEDELAY_FIELDINDEX_MENU:
							userstate->delays.down_volumestep=1000*userstate->menus.uintentry.value;
							break;
					}
				}
				// no break
			default:
				(void)printprompt_uintentry(userstate,1);
				switch (userstate->menus.uintentry.fieldindex) {
#if 0
					case SET_AUTOMUTE_FIELDINDEX_MENU:
						userstate->menus.type=AUTOMUTE_MENUTYPE;
						userstate->automute.isinentry=0;
						break;
#endif
					default:
						userstate->menus.type=SETTINGS_TYPE_MENU;
						break;
				}
				break;
		}
		break;
	case SETTINGS_TYPE_MENU:
		userstate->menus.type=MAIN_TYPE_MENU;
		switch (ch) {
			case 'v': case 'V':
				fprintf(stdout,"Enter the normal tv volume, for --nomute simulation:\n");
				(void)voidinit_uintentry(userstate,FULLVOLUME_FIELDINDEX_MENU,0);
				(void)printprompt_uintentry(userstate,-1);
				break;
			case 'd': case 'D':
				fprintf(stdout,"Enter a millisecond delay for volume decreases:\n");
				(void)voidinit_uintentry(userstate,DOWN_VOLUMEDELAY_FIELDINDEX_MENU,0);
				(void)printprompt_uintentry(userstate,-1);
				break;
			case 'u': case 'U':
				fprintf(stdout,"Enter a millisecond delay for volume increases:\n");
				(void)voidinit_uintentry(userstate,UP_VOLUMEDELAY_FIELDINDEX_MENU,0);
				(void)printprompt_uintentry(userstate,-1);
				break;
			case 'p': case 'P':
				fprintf(stdout,"Quit program after sending PowerOff:\n");
				(void)voidinit_boolentry(userstate,QUITPOWER_FIELDINDEX_MENU);
				(void)printprompt_boolentry(userstate,-1);
				break;
#if 0
			case 't': case 'T':
				fprintf(stdout,"Tie volume changes to --nomute mute simulation steps:\n");
				(void)voidinit_boolentry(userstate,TIEMUTESTEP_FIELDINDEX_MENU);
				(void)printprompt_boolentry(userstate,-1);
				break;
#endif
#if 0
			case 'z': case 'Z':
				fprintf(stdout,"Enter the number of default seconds for automute:\n");
				(void)voidinit_uintentry(userstate,DEF_AUTOMUTE_FIELDINDEX_MENU,0);
				(void)printprompt_uintentry(userstate,-1);
				break;
#endif
		}
		break;
	case REBOOT_TYPE_MENU:
		userstate->menus.type=MAIN_TYPE_MENU;
		if (ch=='y') {
			keypresses=reboot_global;
		}
		break;
	case POWER_TYPE_MENU:
		userstate->menus.type=MAIN_TYPE_MENU;
		if (ch=='F') {
			keypress="PowerOff";
			if (userstate->options.isquitonpoweroff) isquit2=1;
		} else if (ch=='T') {
			keypress="Power";
		}
		break;
	case KEYBOARD_TYPE_MENU:
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
				userstate->menus.type=MAIN_TYPE_MENU;
				nextkey=ch;
				break;
			case '\t':
			case 27:
				userstate->menus.type=MAIN_TYPE_MENU;
				break;
			case '\n': case '\r': keypress="Enter"; break;
			case 8: case 127: keypress="Backspace"; break;
			default:
				if (userstate->terminal.isverbose) {
					(void)reset_printstate(userstate);
					fprintf(stderr,"Got unknown ch: %d\r\n",ch);
				}
				break;
		}
		break;
	case MAIN_TYPE_MENU:
		switch (ch) {
			case 3: case 4: isquit=1; break;
			case 'a': case 'A': userstate->menus.type=KEYBOARD_TYPE_MENU; break;
			case ',': case '<': case '[': case '{': keypress="Rev"; break;
			case '.': case '>': case ']': case '}': keypress="Fwd"; break;
			case '/': case '?': keypress="Search"; break;
			case '0': keypress="InputTuner"; break;
			case '1': keypress="InputHDMI1"; break;
			case '2': keypress="InputHDMI2"; break;
			case '3': keypress="InputHDMI3"; break;
			case '4': keypress="InputHDMI4"; break;
			case '5': keypress="InputAV1"; break;
			case 8: case 127: keypress="Back"; break;
			case 'd': case 'D': if (start_discover(d)) GOTOERROR; break;
			case 'f': case 'F': keypress="FindRemote"; break;
			case '-': case '_':
				(void)volumedown_action(userstate);
				break;
			case '+': case '=':
				(void)volumeup_action(userstate);
				break;
			case 'm': if (sendmute_action(&ign,userstate,d,0)) GOTOERROR; break;
			case 'M': if (sendmute_action(&ign,userstate,d,1)) GOTOERROR; break;
//			case 'p': case 'P': keypress="PowerOff"; break;
			case 'p': userstate->menus.type=POWER_TYPE_MENU; break;
			case 'i': case 'I': keypress="Info"; break;
			case 'q': isquit=1; break;
			case 'Q': userstate->menus.type=REBOOT_TYPE_MENU; break;
			case 'r': case 'R': keypress="InstantReplay"; break;
			case 's': case 'S': userstate->menus.type=SETTINGS_TYPE_MENU; break;
			case 'z': case 'x': case 'c': case 'v': case 'y':
				{
					int step=0;
					char *finalkeypress=NULL;
					switch (ch) {
						case 'z': step=60; break; case 'x': step=30; break;
						case 'c': step=10; break; case 'v': step=5; break;
						case 'y': step=5; finalkeypress="Select"; break;
					}
					if (changemute_action(userstate,d,step,finalkeypress)) GOTOERROR;
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
				userstate->menus.type=AUTOMUTE_MENUTYPE;
				(ignore)print_menus(userstate);
				if (!userstate->automute.isactive) {
					int isnotsent;
					if (ch=='Z') (void)reset_automute(&userstate->automute,2*userstate->def_automute);
					else (void)reset_automute(&userstate->automute,userstate->def_automute);
					if (sendmute_action(&isnotsent,userstate,d,0)) GOTOERROR;
					if (isnotsent) {
						userstate->menus.type=MAIN_TYPE_MENU;
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
				if (userstate->terminal.isverbose) {
					(void)reset_printstate(userstate);
					fprintf(stderr,"Got unknown ch: %d\r\n",ch);
				}
				break;
		}
		break;
}

if (keypress) {
	int issent;
	if (sendkeypress_action(&issent,userstate,d,keypress)) {
		fprintf(stderr,"Error sending http request\n");
	} else {
		if (issent && isquit2) isquit=1;
	}
}
if (keypresses) {
	if (sendkeypresses_action(userstate,d,keypresses)) {
		fprintf(stderr,"Error sending http request\n");
	}
}

*isquit_out=isquit;
*nextkey_out=nextkey;
return 0;
error:
	return -1;
}

int handlebutton_menus(struct userstate *userstate, struct discover *discover, int code, struct joystick *js) {
return userstate->menus.handlebutton(userstate,discover,code,js);
}
static int snes_handlebutton(struct userstate *userstate, struct discover *discover, int code, struct joystick *js) {
char *keypress=NULL;
char **keypresses=NULL;
int mutestep=0;

switch (code) {
	case LEFT_DPAD_JOYSTICK:
		if (userstate->menus.jsmode==NAV_JSMODE_MENUS) {
			if (js->bitmask&BIT_SELECT_JOYSTICK) { // hold down select press LEFT to get 40 lefts
				keypresses=left40_global;
			} else {
				keypress="Left";
			}
		} else if (userstate->menus.jsmode==VIEW_JSMODE_MENUS)  {
			if (js->bitmask&BIT_SELECT_JOYSTICK) {
				keypress="Rev";
			} else {
				mutestep=30;
			}
		}
		break;
	case UP_DPAD_JOYSTICK:
		if (userstate->menus.jsmode==NAV_JSMODE_MENUS) keypress="Up";
		else if (userstate->menus.jsmode==VIEW_JSMODE_MENUS) mutestep=10;
		break;
	case RIGHT_DPAD_JOYSTICK:
		if (userstate->menus.jsmode==NAV_JSMODE_MENUS) {
			if (js->bitmask&BIT_SELECT_JOYSTICK) { // hold down select and press RIGHT to get 40 rights
				keypresses=right40_global; 
			} else {
				keypress="Right";
			}
		} else if (userstate->menus.jsmode==VIEW_JSMODE_MENUS)  {
			if (js->bitmask&BIT_SELECT_JOYSTICK) {
				keypress="Fwd";
			} else {
				mutestep=60;
			}
		}
		break;
	case DOWN_DPAD_JOYSTICK:
		if (userstate->menus.jsmode==NAV_JSMODE_MENUS) keypress="Down";
		else if (userstate->menus.jsmode==VIEW_JSMODE_MENUS) mutestep=5;
		break;
	case LEFT_SHOULDER_JOYSTICK:
		if (userstate->menus.jsmode==NAV_JSMODE_MENUS) keypress="Back";
		else if (userstate->menus.jsmode==VIEW_JSMODE_MENUS) keypress="InstantReplay";
		break;
	case RIGHT_SHOULDER_JOYSTICK:
		keypress="Select";
		break;
	case A_JOYSTICK:
		if (js->bitmask&BIT_SELECT_JOYSTICK) {
			keypress="VolumeUp";
		} else {
			(void)volumeup_action(userstate);
		}
		break;
	case B_JOYSTICK:
		if (js->bitmask&BIT_SELECT_JOYSTICK) {
			keypress="VolumeDown";
		} else {
			(void)volumedown_action(userstate);
		}
		break;
	case X_JOYSTICK:
		if (userstate->menus.jsmode==NAV_JSMODE_MENUS) {
			if (js->inarow==4) keypresses=reboot2_global;
			else keypress="Home";
		} else {
			keypress="Play";
		}
		break;
	case Y_JOYSTICK:
		keypress="Info";
		break;
	case START_JOYSTICK:
		if ((js->bitmask&(BIT_START_JOYSTICK|BIT_SELECT_JOYSTICK))==(BIT_START_JOYSTICK|BIT_SELECT_JOYSTICK)) keypress="Power";
		else userstate->menus.jsmode=VIEW_JSMODE_MENUS;
		break;
	case SELECT_JOYSTICK:
		userstate->menus.jsmode=NAV_JSMODE_MENUS;
		if ((js->bitmask&(BIT_START_JOYSTICK|BIT_SELECT_JOYSTICK))==(BIT_START_JOYSTICK|BIT_SELECT_JOYSTICK)) keypress="Power";
		break;
}

if (mutestep) {
	if (changemute_action(userstate,discover,mutestep,NULL)) GOTOERROR;
}

if (keypress) {
	int issent;
	if (sendkeypress_action(&issent,userstate,discover,keypress)) {
		fprintf(stderr,"Error sending http request\n");
	}
}
if (keypresses) {
	if (sendkeypresses_action(userstate,discover,keypresses)) {
		fprintf(stderr,"Error sending http request\n");
	}
}
return 0;
error:
	return -1;
}
static int full_handlebutton(struct userstate *userstate, struct discover *discover, int code, struct joystick *js) {
char *keypress=NULL;
char **keypresses=NULL;
int mutestep=0;
char *finalkeypress=NULL;

switch (code) {
	case nLEFT_DPAD_JOYSTICK:
		if (js->bitmask&BIT_SELECT_JOYSTICK) { // hold down select press LEFT to get 40 lefts
			keypresses=left40_global;
		} else {
			keypress="Left";
		}
		break;
	case vLEFT_DPAD_JOYSTICK: mutestep=30; break;
	case nUP_DPAD_JOYSTICK: keypress="Up"; break;
	case vUP_DPAD_JOYSTICK: mutestep=10; break;
	case nRIGHT_DPAD_JOYSTICK:
		if (js->bitmask&BIT_SELECT_JOYSTICK) { // hold down select and press RIGHT to get 40 rights
			keypresses=right40_global; 
		} else {
			keypress="Right";
		}
		break;
	case vRIGHT_DPAD_JOYSTICK: mutestep=60; break;
	case nDOWN_DPAD_JOYSTICK: keypress="Down"; break;
	case vDOWN_DPAD_JOYSTICK: mutestep=5; break;
	case nLEFT_SHOULDER_JOYSTICK: keypress="Info"; break;
	case vLEFT_SHOULDER_JOYSTICK:
		if (js->bitmask&BIT_SELECT_JOYSTICK) keypress="Rev";
		else keypress="InstantReplay";
		break;
	case nRIGHT_SHOULDER_JOYSTICK: keypress="Back"; break;
	case vRIGHT_SHOULDER_JOYSTICK:
		if (js->bitmask&BIT_SELECT_JOYSTICK) keypress="Fwd";
		else keypress="Select";
		break;
	case nA_JOYSTICK: case vA_JOYSTICK:
		if (js->bitmask&BIT_SELECT_JOYSTICK) {
			keypress="VolumeUp";
		} else {
			(void)volumeup_action(userstate);
		}
		break;
	case nB_JOYSTICK: case vB_JOYSTICK:
		if (js->bitmask&BIT_SELECT_JOYSTICK) {
			keypress="VolumeDown";
		} else {
			(void)volumedown_action(userstate);
		}
		break;
	case nX_JOYSTICK: case vX_JOYSTICK: keypress="Play"; break;
	case nY_JOYSTICK: case vY_JOYSTICK:
		mutestep=5;
		finalkeypress="Select";
		break;
	case xSTART_JOYSTICK:
		break;
	case xSELECT_JOYSTICK:
		break;
	case xHOME_JOYSTICK:
		if (js->bitmask&BIT_SELECT_JOYSTICK) {
			keypress="Power";
			js->inarow=0;
		} else if (js->inarow==5) {
			keypresses=reboot2_global;
			js->inarow=0;
		} else if (js->inarow>=2) keypress="Home";
		break;
}

if (mutestep) {
	if (changemute_action(userstate,discover,mutestep,finalkeypress)) GOTOERROR;
}

if (keypress) {
	int issent;
	if (sendkeypress_action(&issent,userstate,discover,keypress)) {
		fprintf(stderr,"Error sending http request\n");
	}
}
if (keypresses) {
	if (sendkeypresses_action(userstate,discover,keypresses)) {
		fprintf(stderr,"Error sending http request\n");
	}
}
return 0;
error:
	return -1;
}

int repeat_handlebutton_menus(struct userstate *userstate, struct discover *discover, int code) {
return userstate->menus.repeat_handlebutton(userstate,discover,code);
}

static int snes_repeat_handlebutton(struct userstate *userstate, struct discover *discover, int code) {
char *keypress=NULL;
if (userstate->menus.jsmode!=NAV_JSMODE_MENUS) return 0;
switch (code) {
	case LEFT_DPAD_JOYSTICK: keypress="Left"; break;
	case UP_DPAD_JOYSTICK: keypress="Up"; break;
	case RIGHT_DPAD_JOYSTICK: keypress="Right"; break;
	case DOWN_DPAD_JOYSTICK: keypress="Down"; break;
}
if (keypress) {
	int issent;
	if (sendkeypress_action(&issent,userstate,discover,keypress)) {
		fprintf(stderr,"Error sending http request\n");
	}
}
return 0;
}
static int full_repeat_handlebutton(struct userstate *userstate, struct discover *discover, int code) {
char *keypress=NULL;
switch (code) {
	case nLEFT_DPAD_JOYSTICK: keypress="Left"; break;
	case nUP_DPAD_JOYSTICK: keypress="Up"; break;
	case nRIGHT_DPAD_JOYSTICK: keypress="Right"; break;
	case nDOWN_DPAD_JOYSTICK: keypress="Down"; break;
}
if (keypress) {
	int issent;
	if (sendkeypress_action(&issent,userstate,discover,keypress)) {
		fprintf(stderr,"Error sending http request\n");
	}
}
return 0;
}

void voidinit_buttons_menus(int *isfound_out, struct userstate *userstate) {
int isfound=1;
switch (userstate->joystick.devicetype) {
	case FULL_TYPE_DEVICE_JOYSTICK:
		userstate->menus.handlebutton=full_handlebutton;
		userstate->menus.repeat_handlebutton=full_repeat_handlebutton;
		userstate->menus.jsmode=NONE_JSMODE_MENUS;
		break;
	case SNES_TYPE_DEVICE_JOYSTICK:
		userstate->menus.handlebutton=snes_handlebutton;
		userstate->menus.repeat_handlebutton=snes_repeat_handlebutton;
		userstate->menus.jsmode=NAV_JSMODE_MENUS;
		break;
	default:
		isfound=0;
}
if (isfound_out) *isfound_out=isfound;
}
