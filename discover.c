/*
 * discover.c - use ssdp to find roku devices
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
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#define DEBUG
#include "common/conventions.h"
#include "common/someutils.h"
#include "roku.h"

#include "discover.h"

CLEARFUNC(reply_discover);

void clear_discover(struct discover *d) {
static struct discover blank={.udp.fd=-1};
*d=blank;
}

void voidinit_discover(struct discover *d, int timeout, uint32_t multicastip) {
d->persist.timeout=timeout;
d->persist.interface.ipv4=multicastip;
}

void deinit_discover(struct discover *d) {
ifclose(d->udp.fd);
}

void setstatic_discover(struct discover *d, uint32_t ipv4) {
d->found.ipv4=ipv4;
d->found.port=8060;
}

static int getsocket(struct discover *d) {
int fd=-1;
if (0>(fd=socket(AF_INET,SOCK_DGRAM,0))) GOTOERROR;

{
	struct sockaddr_in sa;
	memset(&sa,0,sizeof(sa));
	sa.sin_family=AF_INET;
	if (0>bind(fd,(struct sockaddr*)&sa,sizeof(sa))) GOTOERROR;
#if 0
	socklen_t ssa;
	ssa=sizeof(sa);
	if (getsockname(fd,(struct sockaddr*)&sa,&ssa)) GOTOERROR;
	if (ssa==sizeof(sa)) {
		fprintf(stderr,"%s:%d bound on port %u\n",__FILE__,__LINE__,ntohs(sa.sin_port));
	}
#endif
}

struct in_addr iface;
iface.s_addr=d->persist.interface.ipv4;
if (0>setsockopt(fd,IPPROTO_IP,IP_MULTICAST_IF,(char *)&iface,sizeof(iface))) GOTOERROR;

struct ip_mreq ipm;
#define IPV4(a,b,c,d) ((a)|((b)<<8)|((c)<<16)|((d)<<24))
ipm.imr_multiaddr.s_addr=IPV4(239,255,255,250);
ipm.imr_interface.s_addr=d->persist.interface.ipv4;
if (0>setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&ipm,sizeof(ipm))) GOTOERROR;

d->udp.fd=fd;
return 0;
error:
	ifclose(fd);
	return -1;
}

static int sendpacket(struct discover *d) {
struct sockaddr_in sa;
char msg[]="M-SEARCH * HTTP/1.1\r\nHost: 239.255.255.250:1900\r\nMan: \"ssdp:discover\"\r\nST: roku:ecp\r\n\r\n";
int msglen;
int n;

memset(&sa,0,sizeof(sa));
sa.sin_addr.s_addr=inet_addr("239.255.255.250");
sa.sin_port=htons(1900);

msglen=sizeof(msg)-1;

n=sendto(d->udp.fd,msg,msglen,0,(struct sockaddr*)&sa,sizeof(sa));
if (0>n) GOTOERROR;
if (n!=msglen) GOTOERROR;

return 0;
error:
	return -1;
}

static void reset_discover(struct discover *d) {
struct persist_discover persist;

persist=d->persist;
deinit_discover(d);
clear_discover(d);
d->persist=persist;
}

int start_discover(struct discover *d) {

reset_discover(d);

if (getsocket(d)) GOTOERROR;
if (sendpacket(d)) GOTOERROR;

return 0;
error:
	return -1;
}

static int readreply(struct reply_discover *reply, struct discover *d) {
unsigned char buff[512];
uint32_t ipv4=0;
unsigned short port=0;
int k;

reply->id_roku.usn[0]=0;

#if 0
struct sockaddr_in sa;
socklen_t ssa;
ssa=sizeof(sa);
k=recvfrom(d->udp.fd,buff,511,0,(struct sockaddr*)&sa,&ssa);
if (k>0) {
	uint32_t u;
	u=sa.sin_addr.s_addr;
	fprintf(stderr,"%s:%d got reply from %u.%u.%u.%u\n",__FILE__,__LINE__,
		(u>>0)&0xff,
		(u>>8)&0xff,
		(u>>16)&0xff,
		(u>>24)&0xff);
}
#else
k=recv(d->udp.fd,buff,511,0);
#endif
if (k<0) GOTOERROR;
if (k>511) GOTOERROR;
buff[k]=0;

#if 0
fputs("packet start\r\n",stderr);
fputs((char *)buff,stderr);
fputs("packet end\r\n",stderr);
#endif

if (k<100) return 0;
if (memcmp(buff,"HTTP/1.1 200 OK",15)) return 0;
char *cursor;
cursor=(char *)buff;
while (1) {
	char *temp;
	if (!*cursor) break;
	if (cursor[0]=='\r') break;
	if (cursor[0]=='\n') break;
	temp=strchr(cursor,'\r');
	if (temp) {
		*temp=0;
		temp++;
		if (*temp!='\n') return 0;
	} else {
		temp=strchr(cursor,'\n');
		if (!temp) return 0;
		*temp=0;
	}
	if (!strncmp(cursor,"USN: uuid:roku:ecp:",19)) {
#if 0
		fprintf(stderr,"usn: %s\r\n",cursor+19);
#endif
		strncpy(reply->id_roku.usn,cursor+19,MAX_USN_ROKU);
		reply->id_roku.usn[MAX_USN_ROKU]=0;
	} else if (!strncmp(cursor,"LOCATION: http://",17)) {
		char *x;
#if 0
		fprintf(stderr,"location: %s\r\n",cursor+17);
#endif
		unsigned int a,b,c,d;
		x=cursor+17;
		a=slowtou(x);
		x=strchr(x,'.');
		if (!x) return 0;
		x++;
		b=slowtou(x);
		x=strchr(x,'.');
		if (!x) return 0;
		x++;
		c=slowtou(x);
		x=strchr(x,'.');
		if (!x) return 0;
		x++;
		d=slowtou(x);
		x=strchr(x,':');
		if (!x) return 0;
		x++;

		ipv4=(a)|(b<<8)|(c<<16)|(d<<24);
		port=slowtou(x);
#if 0
		fprintf(stderr,"ipv4: %u port: %u\r\n",ipv4,port);
#endif
	}

	cursor=temp+1;
}

#if 0
fprintf(stderr,"%s:%d got reply\r\n",__FILE__,__LINE__);
#endif

reply->ipv4=ipv4;
reply->port=port;
return 0;
error:
	return -1;
}

static int ismatch_filter(struct discover *d, struct reply_discover *reply) {
if (!d->persist.filter.isfilter) return 1;
if (!strcmp(d->persist.filter.id.usn,reply->id_roku.usn)) return 1;
return 0;
}

static inline void checkreply(struct discover *d, struct reply_discover *reply) {
if (!d->found.ipv4) {
	if (ismatch_filter(d,reply)) {
		if (!d->persist.filter.isfilter) {
			d->persist.filter.isfilter=1;
			d->persist.filter.id=reply->id_roku;
		}
		d->found.ipv4=reply->ipv4;
		d->found.port=reply->port;
		d->expires=time(NULL)+d->persist.timeout;
	}
}

}

int readreply_discover(struct reply_discover *reply, struct discover *d) {
clear_reply_discover(reply);
if (readreply(reply,d)) GOTOERROR;
(void)checkreply(d,reply);
return 0;
error:
	return -1;
}

int check_discover(int *isalt_out, struct reply_discover *reply_inout, struct discover *d, int altfd) {
// reply should be cleared before calling this
struct pollfd pollfds[2];
int isalt=0;
int isbreak=0;

pollfds[0].fd=d->udp.fd;
pollfds[0].events=POLLIN;
pollfds[1].fd=altfd;
pollfds[1].events=POLLIN;

while (1) {
	time_t now;
	int r;
	now=time(NULL);
	if (d->expires && (d->expires < now)) {
		close(d->udp.fd);
		d->udp.fd=-1;
		break;
	}
	r=poll(pollfds,2,10);
	if (!r) continue;
	if (r<0) GOTOERROR;
	if (pollfds[0].revents&POLLIN) {
		if (readreply(reply_inout,d)) GOTOERROR;
		(void)checkreply(d,reply_inout);
		isbreak=1;
	}
	if (pollfds[1].revents&POLLIN) {
		isalt=1;
		isbreak=1;
	}
	if (isbreak) break;
}

*isalt_out=isalt;
return 0;
error:
	return -1;
}

void setfilter_discover(struct discover *d, char *sn) {
d->persist.filter.isfilter=1;
strncpy(d->persist.filter.id.usn,sn,MAX_USN_ROKU);
}
