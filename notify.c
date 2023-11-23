/*
 * notify.c - watch /dev/input for bluetooth gamepads
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
#include <limits.h>
#ifdef __linux__
#include <sys/inotify.h>
#endif
#define DEBUG
#include "common/conventions.h"
#include "common/someutils.h"

#include "notify.h"

#ifndef	_SYS_INOTIFY_H
#warning inotify is disabled
#endif

void clear_notify(struct notify *n) {
n->fd=-1;
}

void deinit_notify(struct notify *n) {
ifclose(n->fd);
}

#ifdef	_SYS_INOTIFY_H
int init_notify(struct notify *n, char *path) {
int fd;
if (0>(fd=inotify_init())) GOTOERROR;
if (0>inotify_add_watch(fd,path,IN_CREATE|IN_ATTRIB)) GOTOERROR;
n->fd=fd;
return 0;
error:
	return -1;
}

int readrecord_notify(struct notify *notify) {
unsigned char buffer[sizeof(struct inotify_event)+NAME_MAX+1];
if (0>read(notify->fd,buffer,sizeof(buffer))) GOTOERROR;
return 0;
error:
	return -1;
}
#endif

#ifndef	_SYS_INOTIFY_H
int init_notify(struct notify *n, char *path) {
return 0;
}
int readrecord_notify(struct notify *notify) {
return 0;
}
#endif
