/*
 * net.h
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
int nodelay_net(int fd);
int writemsg_net(int fd, unsigned char *message, unsigned int chs, time_t expire);
int readmsg_net(int fd, unsigned char *message, int chs, time_t expire);
int timeout_makeconnectsocket_net(int *istimeout_out, int *fd_out, unsigned char *ip4, unsigned short port, time_t expires);
int getlineheader_net(int fd, char **msg_out, unsigned char **overread_out,
		unsigned int *overreadlen_out, unsigned int maxmsglen, time_t expire);
int readpacket_net(int fd, unsigned char *dest, int max, time_t expire);
