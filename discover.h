/*
 * discover.h
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
struct discover {
	time_t expires;
	struct {
		uint32_t ipv4;
		unsigned short port;
	} found;
	struct {
		int fd;
	} udp;
	struct persist_discover {
		struct {
			uint32_t ipv4;
		} interface;
		struct {
			int isfilter;
			struct id_roku id;
		} filter;
		int timeout;
	} persist;
};
H_CLEARFUNC(discover);

struct reply_discover {
	uint32_t ipv4;
	unsigned short port;
	struct id_roku id_roku;
};
H_CLEARFUNC(reply_discover);

void voidinit_discover(struct discover *d, int timeout, uint32_t multicastip);
void deinit_discover(struct discover *d);
void setstatic_discover(struct discover *d, uint32_t ipv4);
int start_discover(struct discover *d);
int check_discover(int *isalt_out, struct reply_discover *reply_inout, struct discover *d, int altfd);
void setfilter_discover(struct discover *d, char *sn);
