/*
 * someutils.c - subset of utils.c
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "conventions.h"

unsigned int slowtou(char *str) {
unsigned int ret=0;
switch (*str) {
	case '1': ret=1; break;
	case '2': ret=2; break;
	case '3': ret=3; break;
	case '4': ret=4; break;
	case '5': ret=5; break;
	case '6': ret=6; break;
	case '7': ret=7; break;
	case '8': ret=8; break;
	case '9': ret=9; break;
	case '+':
	case '0': break;
	default: return 0; break;
}
while (1) {
	str++;
	switch (*str) {
		case '9': ret=ret*10+9; break;
		case '8': ret=ret*10+8; break;
		case '7': ret=ret*10+7; break;
		case '6': ret=ret*10+6; break;
		case '5': ret=ret*10+5; break;
		case '4': ret=ret*10+4; break;
		case '3': ret=ret*10+3; break;
		case '2': ret=ret*10+2; break;
		case '1': ret=ret*10+1; break;
		case '0': ret=ret*10; break;
		default: return ret; break;
	}
}
return ret;
}

unsigned int getuint32(unsigned char *buff) {
return buff[0]+(buff[1]*256)+(buff[2]*65536)+ (buff[3]*16777216);	
}

int readn(int fd, unsigned char *msg, unsigned int len) {
while (len) {
	ssize_t k;
	k=read(fd,(char *)msg,len);
	if (k<1) {
		if ((k<0) && (errno==EINTR)) continue;
		return -1;
	}
	len-=k;
	msg+=k;
}
return 0;
}

unsigned int hextou(char *str) {
unsigned int ret=0;
switch (*str) {
	case '1': ret=1; break;
	case '2': ret=2; break;
	case '3': ret=3; break;
	case '4': ret=4; break;
	case '5': ret=5; break;
	case '6': ret=6; break;
	case '7': ret=7; break;
	case '8': ret=8; break;
	case '9': ret=9; break;
	case 'a': case 'A': ret=10; break;
	case 'b': case 'B': ret=11; break;
	case 'c': case 'C': ret=12; break;
	case 'd': case 'D': ret=13; break;
	case 'e': case 'E': ret=14; break;
	case 'f': case 'F': ret=15; break;
	case '0': break;
	default: return 0; break;
}
while (1) {
	str++;
	switch (*str) {
		case '0': ret=(ret<<4); break;
		case '1': ret=(ret<<4)|1; break;
		case '2': ret=(ret<<4)|2; break;
		case '3': ret=(ret<<4)|3; break;
		case '4': ret=(ret<<4)|4; break;
		case '5': ret=(ret<<4)|5; break;
		case '6': ret=(ret<<4)|6; break;
		case '7': ret=(ret<<4)|7; break;
		case '8': ret=(ret<<4)|8; break;
		case '9': ret=(ret<<4)|9; break;
		case 'a': case 'A': ret=(ret<<4)|10; break;
		case 'b': case 'B': ret=(ret<<4)|11; break;
		case 'c': case 'C': ret=(ret<<4)|12; break;
		case 'd': case 'D': ret=(ret<<4)|13; break;
		case 'e': case 'E': ret=(ret<<4)|14; break;
		case 'f': case 'F': ret=(ret<<4)|15; break;
		default: return ret; break;
	}
}
return ret;
}
