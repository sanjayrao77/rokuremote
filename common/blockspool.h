/*
 * blockspool.h
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
struct blockspool {
	struct blockmem blockmem;
	int error;
};
H_CLEARFUNC(blockspool);

int init_blockspool(struct blockspool *bs);
void deinit_blockspool(struct blockspool *bs);
void reset_blockspool(struct blockspool *bs);
int append_blockspool(struct blockspool *bs, unsigned char *ustr, unsigned int len);
void uchar_blockspool(struct blockspool *bs, unsigned char uc);
#define str_blockspool(a,b) mem_blockspool(a,(unsigned char *)(b),strlen(b))
void mem_blockspool(struct blockspool *bs, unsigned char *ustr, unsigned int len);
unsigned int sizeof_blockspool(struct blockspool *bs);
int exportz_blockspool(char **ptr_out, unsigned int *len_out, struct blockspool *bs);
