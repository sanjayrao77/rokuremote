/*
 * block_hget.c - simple http over struct blockspool
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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#define DEBUG
#include "conventions.h"
#include "blockmem.h"
#include "blockspool.h"
#include "someutils.h"
#include "net.h"

#include "block_hget.h"

struct responseinfo {
	unsigned int httpstatus;
	unsigned int contentlength;
};
SCLEARFUNC(responseinfo);

static int parseheaders(struct responseinfo *ri, char *headers) {
char *temp;
temp=headers;
if (temp) while (1) {
	if (!strncmp(temp,"HTTP/",5)) {
		char *code;
		code=strchr(temp,' ');
		if (!code) GOTOERROR;
		ri->httpstatus=slowtou(code+1);
	} else if (!strncasecmp(temp,"content-length:",15)) {
		char *clen;
		clen=temp+15;
		while (*clen==' ') clen++;
		ri->contentlength=slowtou(clen);
	}

	temp=strchr(temp,'\n');
	if (!temp) break;
	temp++;
}
return 0;
error:
	return -1;
}

int ipv4_hget(struct blockspool *spool, char *host, uint32_t ipv4, unsigned short port, char *uri,
		unsigned char *post, unsigned int postlen, char *extraheaders, time_t expires) {
char *request=NULL,*headers=NULL;
unsigned char *overread=NULL;
struct responseinfo responseinfo;
unsigned int overreadlen;
unsigned int bytesneeded;
int fd=-1;

clear_responseinfo(&responseinfo);
if (!extraheaders) extraheaders="";

if (host) {
	if (!(request=malloc(4096+strlen(uri)+strlen(host)+strlen(extraheaders)))) GOTOERROR;
	sprintf(request,"%s %s HTTP/1.0\r\n"\
	"Host: %s\r\n"\
	"Content-length: %u\r\n"\
	"Connection: close\r\n"\
	"%s"\
	"\r\n",post?"POST":"GET",uri,host,postlen,extraheaders);
} else {
	if (!(request=malloc(4096+strlen(uri)+strlen(extraheaders)))) GOTOERROR;
	sprintf(request,"%s %s HTTP/1.0\r\n"\
	"Content-length: %u\r\n"\
	"Connection: close\r\n"\
	"%s"\
	"\r\n",post?"POST":"GET",uri,postlen,extraheaders);
}

{
	int istimeout;
	if (timeout_makeconnectsocket_net(&istimeout,&fd,(unsigned char *)&ipv4,port,expires)) GOTOERROR;
	if (istimeout) {
		fprintf(stderr,"%s:%d timeout making connection\n",__FILE__,__LINE__);
		GOTOERROR;
	}
}
(ignore)nodelay_net(fd);
if (writemsg_net(fd,(unsigned char *)request,strlen(request),expires)) GOTOERROR;
if (postlen) {
	if (writemsg_net(fd,post,postlen,expires)) GOTOERROR;
}
if (getlineheader_net(fd,&headers,&overread,&overreadlen,0,expires)) GOTOERROR;
if (parseheaders(&responseinfo,headers)) GOTOERROR;
if (responseinfo.httpstatus!=200) GOTOERROR;
bytesneeded=responseinfo.contentlength;
if (bytesneeded<overreadlen) bytesneeded=0;
else bytesneeded-=overreadlen;
(ignore)mem_blockspool(spool,overread,overreadlen);
while (bytesneeded) {
	unsigned int n;
	n=4096;
	if (n>bytesneeded) n=bytesneeded;
	if (readmsg_net(fd,(unsigned char *)request,n,expires)) GOTOERROR;
	(ignore)mem_blockspool(spool,(unsigned char *)request,n);
	bytesneeded-=n;
}

close(fd);
free(request);
iffree(headers);
/* iffree(overread); NO */
return 0;
error:
	ifclose(fd);
	iffree(request);
	iffree(headers);
/* iffree(overread); NO */
	return -1;
}
