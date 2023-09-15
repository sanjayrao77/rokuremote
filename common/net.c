/*
 * net.c - simple socket routines
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
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "conventions.h"
#include "someutils.h"
#include "net.h"

int nodelay_net(int fd) {
int yesint=1;
return setsockopt(fd,IPPROTO_TCP,TCP_NODELAY, (char*)&yesint,sizeof(int));
}

/* see also timeout_writen */
int writemsg_net(int fd, unsigned char *message, unsigned int chs, time_t expire) {
struct timeval tv;
int k;
fd_set rset;
FD_ZERO(&rset);
FD_SET(fd,&rset);
while (chs) {
	if (time(NULL)>expire) return -1;
	tv.tv_sec=5;
	tv.tv_usec=0;
	switch (select(fd+1,NULL,&rset,NULL,&tv)) {
		case -1: return -1;
		case 0: 
			 FD_SET(fd,&rset);
			 continue;
	}
	k=write(fd,message,chs);
	if (k<1) return -1;
	message+=k;
	chs-=k;
}
return 0;
}

int readmsg_net(int fd, unsigned char *message, int chs, time_t expire) {
int k;
struct timeval tv;
fd_set rset;

FD_ZERO(&rset);
FD_SET(fd,&rset);
while (chs) {
	if (time(NULL)>expire) return -1;
	tv.tv_sec=5;
	tv.tv_usec=0;
	switch (select(fd+1,&rset,NULL,NULL,&tv)) {
		case -1:
			if (errno==EINTR) continue;
			return -1;
		case 0:
			 FD_SET(fd,&rset);
			 continue;
	}
	k=read(fd,message,chs);
	if (k<1) return -1;
	message+=k;
	chs-=k;
}
return 0;
}

int timeout_makeconnectsocket_net(int *istimeout_out, int *fd_out, unsigned char *ip4, unsigned short port, time_t expires) {
struct sockaddr_in sa;
int fd=-1;

memset(&sa,0,sizeof(struct sockaddr_in));
sa.sin_family=AF_INET;
sa.sin_addr.s_addr=getuint32(ip4);
sa.sin_port=htons(port);
#if 0
		fprintf(stderr,"%s:%d starting timeout_makeconnectsocket, %d seconds to go\n",__FILE__,__LINE__,
			(int)(expires-time(NULL)));
#endif
while (1) {
#ifdef SOCK_NONBLOCK
	fd=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0);
#else
	fd=socket(AF_INET,SOCK_STREAM,0);
#endif
	if (fd>=0) break;
	fprintf(stderr,"%s:%d error making socket: %s\n",__FILE__,__LINE__,strerror(errno));
	if (expires < time(NULL)) goto timeout;
	sleep(1);
}
#ifndef SOCK_NONBLOCK
{
	if (0>fcntl(fd,F_SETFL,O_NONBLOCK)) fprintf(stderr,"%s:%d error setting O_NONBLOCK: %s\n",__FILE__,__LINE__,strerror(errno));
}
#endif
#if 0
		fprintf(stderr,"%s:%d made socket, %d seconds to go\n",__FILE__,__LINE__,
			(int)(expires-time(NULL)));
#endif
while (1) {
#if 0
		fprintf(stderr,"%s:%d calling connect, %d seconds to go\n",__FILE__,__LINE__,
			(int)(expires-time(NULL)));
#endif
	if (!connect(fd,(struct sockaddr*)&sa,sizeof(struct sockaddr_in))) 
		break;
	if (errno==EISCONN) break; // for macos
	if (errno==EINPROGRESS) {
		struct timeval tv;
		fd_set wset;
		if (expires < time(NULL)) goto timeout;
#if 0
		fprintf(stderr,"%s:%d waiting for connect, %d seconds to go\n",__FILE__,__LINE__,
			(int)(expires-time(NULL)));
#endif
		FD_ZERO(&wset);
		FD_SET(fd,&wset);
		tv.tv_sec=2;
		tv.tv_usec=0;
		if (0>select(fd+1,NULL,&wset,NULL,&tv)) {
			if (errno!=EAGAIN) GOTOERROR;
		}
		continue;
	}
	if (errno==EAGAIN) continue;
	fprintf(stderr,"%s:%d error in connect: %s\r\n",__FILE__,__LINE__,strerror(errno));
	GOTOERROR;
}
*istimeout_out=0;
*fd_out=fd;
return 0;
timeout:
	ifclose(fd);
	*istimeout_out=1;
	*fd_out=-1;
	return 0;
error:
	ifclose(fd);
	return -1;
}

int getlineheader_net(int fd, char **msg_out, unsigned char **overread_out,
		unsigned int *overreadlen_out, unsigned int maxmsglen, time_t expire) {
/* don't free overread as it shares a pointer with msg */
char *msg=NULL;
unsigned int msglen=0;
int i,j,r;
unsigned char *overread=NULL;
unsigned int overreadlen=0;

if (!maxmsglen) maxmsglen=10*1024;

msg=malloc(maxmsglen+1);
if (!msg) GOTOERROR;
while (1) {
	if (msglen==maxmsglen) {
		char *temp;
		int m=maxmsglen*2;
		if (m>1024*1024) GOTOERROR; /* sanity */
		temp=realloc(msg,m+1);
		if (!temp) GOTOERROR;
		msg=temp;
		maxmsglen=m;
	}
	r=readpacket_net(fd,(unsigned char *)msg+msglen,maxmsglen-msglen,expire);
	if (r<1) {
#ifdef DEBUG
#if 0
		fprintf(stderr,"Error reading header: (%u) \"",msglen);
		fwrite(msg,msglen,1,stderr);
		fprintf(stderr,"\"\r\n");
#endif
#endif
		GOTOERROR;
	} else {
#ifdef DEBUG
#if 0
		fprintf(stderr,"Reading header: (%u) \"",r);
		fwrite(msg+msglen,r,1,stderr);
		fprintf(stderr,"\"\r\n");
#endif
#endif
	}

	i=msglen-4;
	if (i<0) i=0;

	msglen+=r;

	j=msglen-4;

/* 042313 -- speedup bug, used to have "for (i=0;..." */
	for (;i<=j;i++) {
		if (!memcmp(msg+i,"\r\n\r\n",4)) {
			overread=(unsigned char *)msg+i+4;
			overreadlen=msglen-i-4;
			msglen=i+2;
			goto doublebreak;
		}
		if (!memcmp(msg+i,"\n\n",2)) {
			overread=(unsigned char *)msg+i+2;
			overreadlen=msglen-i-2;
			msglen=i+1;
			goto doublebreak;
		}
	}
	if (j>=0) {
		if (!memcmp(msg+i+1,"\n\n",2)) {
			overread=(unsigned char *)msg+i+3;
			overreadlen=msglen-i-3;
			msglen=i+2;
			break;
		}
		if (!memcmp(msg+i+2,"\n\n",2)) {
			overread=(unsigned char *)msg+i+4;
			overreadlen=msglen-i-4;
			msglen=i+3;
			break;
		}
	}
}
doublebreak:
msg[msglen]='\0';

#ifdef PRINTREQUEST
fprintf(stderr,"%s\n",msg);
#endif

*msg_out=msg;
*overread_out=overread;
*overreadlen_out=overreadlen;
return 0;
error:
	return -1;
}

int readpacket_net(int fd, unsigned char *dest, int max, time_t expire) {
struct timeval tv;
fd_set rset;
int k;

while (1) {
	FD_ZERO(&rset);
	FD_SET(fd,&rset);
	tv.tv_sec=5;
	tv.tv_usec=0;
	switch (select(fd+1,&rset,NULL,NULL,&tv)) {
		case -1:
			if (errno==EINTR) continue;
			return -1;
		case 0:
			if (time(NULL)>expire) return -1;
			continue;
	}
	k=read(fd,dest,max);
	if (k<0) return -1;
	return k;
}
/* unreachable */
return -1;
}

