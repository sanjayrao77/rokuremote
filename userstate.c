/*
 * userstate.c - manage current state and user options
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
#define DEBUG
#include "common/conventions.h"
#include "roku.h"
#include "discover.h"
#include "automute.h"
#include "joystick.h"

#include "userstate.h"

void clear_mainoptions(struct mainoptions *o) {
static struct mainoptions blank;
*o=blank;

o->stdinfd=STDIN_FILENO;
o->discovertimeout=30;
o->isinteractive=1;
}

void clear_userstate(struct userstate *g) {
static struct userstate blank={.menus.type=MAIN_TYPE_MENU,.volume.full=15,.volume.current=15,.volume.target=15};
// .def_automute=30
*g=blank;
}

void set_printstate(struct userstate *uo, int newstate) {
if (uo->terminal.printstate) {
	if (uo->terminal.printstate!=newstate) fputc('\n',stdout);
}
uo->terminal.printstate=newstate;
}
void reset_printstate(struct userstate *uo) {
if (uo->terminal.printstate==NONE_PRINTSTATE_TERMINAL) return;
fputc('\n',stdout);
uo->terminal.printstate=NONE_PRINTSTATE_TERMINAL;
}

void setdirty_volume(struct userstate *u) {
if (u->volume._isdirty) return;
u->volume._isdirty=1;
u->volume.errorfuse=5;
}

