/*
 * blockspool.c - spool data over struct blockmem
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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define DEBUG
#include "conventions.h"
#include "blockmem.h"

#include "blockspool.h"

CLEARFUNC(blockspool);

int init_blockspool(struct blockspool *bs) {
if (sizeof(int)!=4) GOTOERROR;
if (init_blockmem(&bs->blockmem,0)) GOTOERROR;
return 0;
error:
	return -1;
}

void deinit_blockspool(struct blockspool *bs) {
deinit_blockmem(&bs->blockmem);
}

void reset_blockspool(struct blockspool *bs) {
reset_blockmem(&bs->blockmem);
}

void uchar_blockspool(struct blockspool *bs, unsigned char uc) {
struct node_blockmem *node;
node=bs->blockmem.current;
{
	if (node->max==node->num) {
		if (addnode_blockmem(node,0)) GOTOERROR;
		node=node->next;
		bs->blockmem.current=node;
	}
	node->data[node->num]=uc;
	node->num+=1;
}
return;
error:
	bs->error=1;	
}
void mem_blockspool(struct blockspool *bs, unsigned char *ustr, unsigned int len) { // dupe of append_ w/o return
struct node_blockmem *node;
node=bs->blockmem.current;
while (len) {
	unsigned int room;
	unsigned int k;
	room=node->max-node->num;
	if (!room) {
		if (addnode_blockmem(node,0)) GOTOERROR;
		node=node->next;
		bs->blockmem.current=node;
		room=node->max;
	}
	k=room;
	if (len<k) k=len;
	memcpy(node->data+node->num,ustr,k);
	node->num+=k;
	ustr+=k;
	len-=k;
}
return;
error:
	bs->error=1;	
}
int append_blockspool(struct blockspool *bs, unsigned char *ustr, unsigned int len) { // dupe of mem_ but with return
struct node_blockmem *node;
node=bs->blockmem.current;
while (len) {
	unsigned int room;
	unsigned int k;
	room=node->max-node->num;
	if (!room) {
		if (addnode_blockmem(node,0)) GOTOERROR;
		node=node->next;
		bs->blockmem.current=node;
		room=node->max;
	}
	k=room;
	if (len<k) k=len;
	memcpy(node->data+node->num,ustr,k);
	node->num+=k;
	ustr+=k;
	len-=k;
}
return 0;
error:
	bs->error=1;	
	return -1;
}

unsigned int sizeof_blockspool(struct blockspool *bs) {
struct node_blockmem *node;
unsigned int count=0;

node=&bs->blockmem.node;
while (1) {
	count+=node->num;
	node=node->next;
	if (!node) break;
}
return count;
}
