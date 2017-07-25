/*
 * Copyright (c) 2001-2017 Mellanox Technologies, Ltd. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <clocale>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/socket.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <linux/filter.h>
#include <sys/mman.h>
#include <poll.h>
#include <linux/net_tstamp.h>
#include <dlfcn.h>

#include "vma/util/vtypes.h"
#include "vma_extra.h"

#ifdef HAVE_MP_RQ
#define HAVE_MP_RQ_NETMAP
#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>
#endif

using namespace std;

#ifdef HAVE_MP_RQ_NETMAP

#define	MAX_RINGS	1000
#define	PRINT_PERIOD_SEC	5
#define	PRINT_PERIOD		1000000 * PRINT_PERIOD_SEC
#define	MAX_SOCKETS_PER_RING	4096
#define	STRIDE_SIZE	2048

class RXSock;
class CommonCyclicRing;

typedef void (*validatePackets)(uint8_t*, size_t, CommonCyclicRing*);
typedef void (*validatePacket)(uint8_t*, RXSock*);
typedef void (*printInfo)(RXSock*);
typedef void* (*Process_func)(void*);

class RXSock {
public:
	uint64_t	rxCount;
	int		rxDrop;
	uint64_t	statTime;
	int		lastBlockId;
	int		LastSequenceNumber;
	int		lastPacketType;
	int		index;
	int		fd;
	int		ring_fd;
	uint16_t	rtpPayloadType;
	uint16_t	sin_port;
	struct ip_mreqn	mc;
	char		ipAddress[INET_ADDRSTRLEN];
	int bad_packets;
	validatePacket	fvalidatePacket;
	printInfo	fprintinfo;
};

class CommonCyclicRing {
	public:
	unsigned long printCount;
	int numOfSockets;
	int ring_id;
	int ring_fd;
	RXSock* hashedSock[MAX_SOCKETS_PER_RING];
	std::ofstream * pOutputfile;
	std::vector<sockaddr_in*> addr_vect;
	std::vector<RXSock*> sock_vect;
	CommonCyclicRing():printCount(0),numOfSockets(0),ring_fd(0){
		for (int i=0; i < MAX_SOCKETS_PER_RING; i++ ) {
			hashedSock[i] = 0;
		}
	}
	struct vma_api_t *vma_api;
	size_t	min_s;
	size_t	max_s;
	int 	flags;
	void PrintInfo();
	void printTime();
};

struct flow_param {
	int ring_id;
	unsigned short hash;
	sockaddr_in addr;
};

static unsigned short hashIpPort2(sockaddr_in addr )
{
	int hash = ((size_t)(addr.sin_addr.s_addr) * 59) ^ ((size_t)(addr.sin_port) << 16);
	unsigned char smallHash = (unsigned char)(((unsigned char) ((hash*19) >> 24 ) )  ^ ((unsigned char) ((hash*17) >> 16 )) ^ ((unsigned char) ((hash*5) >> 8) ) ^ ((unsigned char) hash));
	unsigned short mhash = ((((addr.sin_addr.s_addr >>24) & 0x7) << 8) | smallHash ) ;
//	printf("0x%x\n",addr.sin_addr.s_addr);
	return mhash;
}

static inline unsigned long long int time_get_usec()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (((unsigned long long int) tv.tv_sec * 1000000LL)
			+ (unsigned long long int) tv.tv_usec);
}

static int CreateRingProfile(bool CommonFdPerRing, int RingProfile, int user_id, int RxSocket )
{
	vma_ring_alloc_logic_attr profile;
	profile.engress = 0;
	profile.ingress = 1;
	profile.ring_profile_key = RingProfile;
	if (CommonFdPerRing ) {
		profile.user_id =user_id;
		profile.comp_mask = VMA_RING_ALLOC_MASK_RING_PROFILE_KEY|
							VMA_RING_ALLOC_MASK_RING_USER_ID	|
							VMA_RING_ALLOC_MASK_RING_INGRESS;

		// if we want several Fd's per ring, we need to assign RING_LOGIC_PER_THREAD / RING_LOGIC_PER_CORE
		profile.ring_alloc_logic = RING_LOGIC_PER_USER_ID;
	} else {
		profile.comp_mask = VMA_RING_ALLOC_MASK_RING_PROFILE_KEY|
							VMA_RING_ALLOC_MASK_RING_INGRESS;
		// if we want several Fd's per ring, we need to assign RING_LOGIC_PER_THREAD / RING_LOGIC_PER_CORE
		profile.ring_alloc_logic = RING_LOGIC_PER_SOCKET;
	}
	return setsockopt(RxSocket, SOL_SOCKET, SO_VMA_RING_ALLOC_LOGIC,&profile, sizeof(profile));
}

static int OpenRxSocket(int ring_id, sockaddr_in* addr, uint32_t ssm, const char *device,
						struct ip_mreqn *mc, int RingProfile, bool CommonFdPerRing)
{
	int i_ret;
	struct timeval timeout = { 0, 1 };
	int i_opt = 1;
	struct ifreq ifr;
	struct sockaddr_in *p_addr;

	// Create the socket
	int RxSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (RxSocket < 0) {
		printf("%s: Failed to create socket (%s)\n",
		__func__,std::strerror(errno));
		return 0;
	}

	// Enable socket reuse ( for multi channels bind to a single socket )
	i_ret = setsockopt(RxSocket, SOL_SOCKET, SO_REUSEADDR,
			(void *) &i_opt, sizeof(i_opt));
	if (i_ret < 0) {
		close(RxSocket);
		RxSocket = 0;
		printf("%s: Failed to set SO_REUSEADDR (%s)\n",
		__func__, strerror(errno));
		return 0;
	}
	fcntl(RxSocket, F_SETFL, O_NONBLOCK);
	CreateRingProfile(CommonFdPerRing, RingProfile, ring_id, RxSocket );
	// bind to specific device
	struct ifreq interface;
	strncpy(interface.ifr_ifrn.ifrn_name, device, IFNAMSIZ);
	//printf("%s SO_BINDTODEVICE %s\n",__func__,interface.ifr_ifrn.ifrn_name);
	if (setsockopt(RxSocket, SOL_SOCKET, SO_BINDTODEVICE,
			(char *) &interface, sizeof(interface)) < 0) {
		printf("%s: Failed to bind to device (%s)\n",
		__func__, strerror(errno));
		close(RxSocket);
		RxSocket = 0;
		return 0;
	}

	// bind to socket
	i_ret = bind(RxSocket, (struct sockaddr *)addr, sizeof(struct sockaddr));
	if (i_ret < 0) {
		printf("%s: Failed to bind to socket (%s)\n",__func__,strerror(errno));
		close(RxSocket);
		RxSocket = 0;
		return 0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, device, IFNAMSIZ);
	// Get device IP
	i_ret = ioctl(RxSocket, SIOCGIFADDR, &ifr);
	if (i_ret < 0) {
		printf("%s: Failed to obtain interface IP (%s)\n",__func__,	strerror(errno));
		close(RxSocket);
		RxSocket = 0;
		return 0;
	}

	if (((addr->sin_addr.s_addr & 0xFF) >= 224) && ((addr->sin_addr.s_addr & 0xFF) <= 239)) {
		p_addr = (struct sockaddr_in *) &(ifr.ifr_addr);
		if (ssm == 0) {
			struct ip_mreqn mreq;
			// join the multicast group on specific device
			memset(&mreq, 0, sizeof(struct ip_mreqn));

			mreq.imr_multiaddr.s_addr = addr->sin_addr.s_addr;
			mreq.imr_address.s_addr = p_addr->sin_addr.s_addr;
			*mc = mreq;
			// RAFI MP_RING is created
			i_ret = setsockopt(RxSocket, IPPROTO_IP,
					IP_ADD_MEMBERSHIP, &mreq,
					sizeof(struct ip_mreqn));

			if (i_ret < 0) {
				printf("%s: add membership to (0X%08X) on (0X%08X) failed. (%s)\n",__func__,mreq.imr_multiaddr.s_addr,
					mreq.imr_address.s_addr,
					strerror(errno));
				close(RxSocket);
				RxSocket = 0;
				return 0;
			}
		} else {
			struct ip_mreq_source mreqs;
			// join the multicast group on specific device
			memset(&mreqs, 0, sizeof(struct ip_mreq_source));

			mreqs.imr_multiaddr.s_addr = addr->sin_addr.s_addr;
			mreqs.imr_interface.s_addr = p_addr->sin_addr.s_addr;
			mreqs.imr_sourceaddr.s_addr = ssm;

			i_ret = setsockopt(RxSocket, IPPROTO_IP,
					IP_ADD_SOURCE_MEMBERSHIP, &mreqs,
					sizeof(struct ip_mreq_source));

			if (i_ret < 0) {
				printf("%s: add membership to (0X%08X), ssm (0X%08X) failed. (%s)\n",__func__,
					mreqs.imr_multiaddr.s_addr,
					mreqs.imr_sourceaddr.s_addr,
					strerror(errno));
				close(RxSocket);
				RxSocket = 0;
				return 0;
			}
		}
	}

	// Set max receive timeout
	i_ret = setsockopt(RxSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
			sizeof(struct timeval));
	if (i_ret < 0) {
		printf("%s: Failed to set SO_RCVTIMEO (%s)\n",__func__,
				strerror(errno));
		close(RxSocket);
		RxSocket = 0;
		return 0;
	}
	return RxSocket;
}

#define	IP_HEADER_OFFSET	14
#define	IP_HEADER_SIZE	20
#define	IP_DEST_OFFSET	(IP_HEADER_OFFSET+ 16)
#define	UDP_HEADER_OFFSET	(IP_HEADER_SIZE + IP_HEADER_OFFSET )
#define	PORT_DEST_OFFSET	(UDP_HEADER_OFFSET + 2)

static void AddFlow(flow_param flow,CommonCyclicRing* rings[], int &uniqueRings)
{
	int ring_id = flow.ring_id;
	if ( rings[ring_id] == NULL ) {
		rings[ring_id] = new CommonCyclicRing;
		rings[ring_id]->ring_id =ring_id;
		uniqueRings++;
	}
	rings[ring_id]->numOfSockets++;
	sockaddr_in* pAddr = new sockaddr_in;
	*pAddr = flow.addr;
	rings[ring_id]->addr_vect.push_back(pAddr);
}

static void destroyFlows(CommonCyclicRing* rings[])
{
	for ( int i=0; i < MAX_RINGS; i++ ) {
		if (rings[i] != NULL ) {
			for (std::vector<sockaddr_in*>::iterator it = rings[i]->addr_vect.begin(); it!=rings[i]->addr_vect.end(); ++it) {
				delete *it;
			}
			for (std::vector<RXSock*>::iterator it = rings[i]->sock_vect.begin(); it!=rings[i]->sock_vect.end(); ++it) {
				delete *it;
			}
			delete rings[i];
		}
	}
}

static CommonCyclicRing* pRings[MAX_RINGS];
static void init_ring_helper(CommonCyclicRing* pRing);

extern "C"
struct nm_desc *nm_open_vma(const char *nm_ifname, const struct nmreq *req, uint64_t flags, const struct nm_desc *arg)
{
	NOT_IN_USE(req);
	NOT_IN_USE(flags);
	NOT_IN_USE(arg);

	struct nm_desc *d = NULL;
	d = (struct nm_desc *)calloc(1, sizeof(*d));
	if (d == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	char *ifname = (char *)nm_ifname;
//	char *token = std::strtok((char*)nm_ifname, ":");
//	while (token != NULL) {
//		ifname = token;
//		token = std::strtok(NULL, ":");
//	}
//	printf("===>ifname=%s\n", ifname);

	bool ringPerFd = false;
	for (int j = 0; j < MAX_RINGS; j++) {
		pRings[j] = NULL;
	}

	std::string ip;
	std::string line;
	int port;
	int ring_id;
	int lineNum = 0;
	int sock_num = 0, socketRead = 0;
	char HashColision[MAX_RINGS][MAX_SOCKETS_PER_RING] = {0};
	int uniqueRings;
	int hash_colision_cnt = 0;
	char *cnfif_file = new char[IFNAMSIZ*2];

	strcpy(cnfif_file,ifname);
	strcat(cnfif_file,".txt");

	std::ifstream infile(cnfif_file);
	if (!infile) {
		printf("configuration file %s not found\n", cnfif_file);
		return NULL;
	}
	while (std::getline(infile, line)) {
		if ((line[0] == '#') || ((line[0] == '/') && (line[1] == '/'))) {
			continue;
		}
		std::istringstream iss(line);
		struct flow_param flow;
		if (iss >> ip >> port) {
			socketRead++;
			flow.addr.sin_family = AF_INET;
			flow.addr.sin_port = htons(port);
			flow.addr.sin_addr.s_addr = inet_addr(ip.c_str());
			flow.addr.sin_addr.s_addr = ntohl(ntohl(flow.addr.sin_addr.s_addr));
			printf("adding ip %s port %d,\n", ip.c_str(), port);
			flow.hash = hashIpPort2(flow.addr);
			printf("adding %s:%d hash val %d\n",ip.c_str(),port, flow.hash);
			if (flow.addr.sin_addr.s_addr < 0x01000001) {
				printf("Error - illegal IP %x\n", flow.addr.sin_addr.s_addr);
				return NULL;
			}
		} else {
			continue;
		}
		if (iss >> ring_id) {
		} else {
			printf("no common rings\n");
			ring_id = lineNum;
			lineNum++;
			ringPerFd = true;
		}
		printf("ring_id = %d\n",ring_id);
		flow.ring_id = ring_id;
		if (HashColision[ring_id][flow.hash] == 0) {
			HashColision[ring_id][flow.hash] = 1;
			// add the fd to the ring, if needed create a ring, update num of rings, and num of flows within this ring.
			AddFlow(flow, pRings, uniqueRings);
		} else {
			hash_colision_cnt++;
			printf("Hash socket colision found , socket%s:%d - dropped, total %d\n",ip.c_str(),port,hash_colision_cnt);
		}
		if (socketRead == sock_num) {
			printf("read %d sockets from the file\n", socketRead);
			break;
		}
	}
	d->req.nr_rx_rings = ring_id;
	d->req.nr_ringid = ring_id;

	int prof = 0;
	struct vma_api_t *vma_api = vma_get_api();
	vma_ring_type_attr ring;
	ring.ring_type = VMA_RING_CYCLIC_BUFFER;
	// buffer size is 2^17 = 128MB
	ring.ring_cyclicb.num = (1<<17);
	// user packet size ( not including the un-scattered data
	ring.ring_cyclicb.stride_bytes = 1400;
	//ring.ring_cyclicb.comp_mask = VMA_RING_TYPE_MASK;
	int res = vma_api->vma_add_ring_profile(&ring, &prof);
	if (res) {
		printf("failed adding ring profile");
		return NULL;
	}
	// for every ring, open sockets
	for (int i=0; i< MAX_RINGS; i++) {
		if (pRings[i] == NULL ) {
			continue;
		}
		std::vector<sockaddr_in*>::iterator it;
		for (it = pRings[i]->addr_vect.begin();
				it!=pRings[i]->addr_vect.end(); ++it) {
			struct ip_mreqn mc;
			printf("Adding socket to ring %d\n",i);
			RXSock* pSock = new RXSock;
			pSock->fd = OpenRxSocket(pRings[i]->ring_id, *it, 0, ifname, &mc, prof, !ringPerFd);
			if (pSock->fd <= 0) {
				printf("Error OpenRxSocket failed. %d\n", i);
				return NULL;
			}
			memcpy(&pSock->mc, &mc, sizeof(mc));
			pSock->LastSequenceNumber = -1;
			pSock->lastBlockId = -1;
			pSock->rxCount = 0;
			pSock->rxDrop = 0;
			pSock->statTime = time_get_usec() + 1000*i;
			pSock->index = i;
			pSock->bad_packets = 0;
			pSock->sin_port = ntohs((*it)->sin_port);
			unsigned short hash = hashIpPort2(**it);
			//printf("hash value is %d\n",hash);
			if ( NULL != pRings[i]->hashedSock[hash] ) {
				printf ("Collision, reshuffle your ip addresses \n");
				return NULL;
			}
			pRings[i]->hashedSock[hash] = pSock;
			inet_ntop(AF_INET, &((*it)->sin_addr), pSock->ipAddress, INET_ADDRSTRLEN);
			pRings[i]->sock_vect.push_back(pSock);
			pRings[i]->min_s = 1000;
			pRings[i]->max_s = 5000;
			pRings[i]->flags = MSG_DONTWAIT;
			pRings[i]->vma_api = vma_get_api();
			init_ring_helper(pRings[i]);
		}
	}
	return d;
}

void init_ring_helper(CommonCyclicRing* pRing)
{
	int sock_len = pRing->numOfSockets;
	for (int i = 0; i < sock_len; i++) {
		int ring_fd_num = pRing->vma_api->get_socket_rings_num(pRing->sock_vect[i]->fd);
		int* ring_fds = new int[ring_fd_num];
		pRing->vma_api->get_socket_rings_fds(pRing->sock_vect[i]->fd, ring_fds, ring_fd_num);
		pRing->sock_vect[i]->ring_fd = *ring_fds;
		pRing->ring_fd = *ring_fds;
		delete[] ring_fds;
	}
}

static inline int cb_buffer_read(int ring, vma_completion_cb_t *completion)
{
	completion->packets = 0;
	return pRings[ring]->vma_api->vma_cyclic_buffer_read(pRings[ring]->ring_fd, completion, pRings[ring]->min_s, pRings[ring]->max_s, pRings[ring]->flags);
}

extern "C"
u_char *nm_nextpkt_vma(struct nm_desc *d, struct nm_pkthdr *hdr)
{
	int ring = d->req.nr_ringid;
	uint8_t *data = NULL;
	struct vma_completion_cb_t completion;

	for (int j = 0; j < 10; j++) {
		int res = cb_buffer_read(ring, &completion);
		if (res == -1) {
			printf("vma_cyclic_buffer_read returned -1");
			return NULL;
		}
		if (completion.packets == 0) {
			continue;
		}
		hdr->len = hdr->caplen = completion.packets * STRIDE_SIZE;
		hdr->buf = data = ((uint8_t *)completion.payload_ptr);
		break;
	}

	return (u_char *)data;
}

extern "C"
int nm_dispatch_vma(struct nm_desc *d, int cnt, nm_cb_t cb, u_char *arg)
{
	NOT_IN_USE(arg);
	struct vma_completion_cb_t completion;
	int ring = d->req.nr_ringid;
	//int n = d->last_rx_ring - d->first_rx_ring + 1;
	//int ri = d->cur_rx_ring;
	int c = 0, got = 0;
	d->hdr.buf = NULL;
	d->hdr.flags = NM_MORE_PKTS;
	d->hdr.d = d;

	if (cnt == 0)
		cnt = -1;
	/* cnt == -1 means infinite, but rings have a finite amount
	 * of buffers and the int is large enough that we never wrap,
	 * so we can omit checking for -1
	 */
	// <tbd>
	for (c = 0; cnt != got; c++) {
		//
		int res = cb_buffer_read(ring, &completion);
		if (res == -1) {
			printf("vma_cyclic_buffer_read returned -1");
			return 0;
		}
		if (completion.packets == 0) {
			continue;
		}
		// <tbd>
		got++;
		d->hdr.len = d->hdr.caplen = completion.packets * STRIDE_SIZE;
		d->hdr.buf = ((uint8_t *)completion.payload_ptr);
	}
	if (d->hdr.buf) {
		cb(arg, &d->hdr, d->hdr.buf);
	}

	return got;
}

extern "C"
int nm_close_vma(struct nm_desc *d)
{
	destroyFlows(pRings);
	if (d == NULL) {
		return EINVAL;
	}

	free(d);
	return 0;
}
#endif
