#ifndef __MYRIP_H
#define __MYRIP_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#define RIP_VERSION    	2
#define RIP_REQUEST    	1
#define RIP_RESPONSE   	2
#define RIP_INFINITY  	16
#define RIP_PORT		520
#define RIP_PACKET_HEAD	4
#define RIP_MAX_PACKET  504
#define RIP_MAX_ENTRY   25
#define ROUTE_MAX_ENTRY 256
#define RIP_GROUP		"224.0.0.9"

#define RIP_CHECK_OK    1
#define RIP_CHECK_FAIL  0
#define AddRoute        24
#define DelRoute        25

typedef struct RipEntry
{
	unsigned short usFamily;
	unsigned short usTag;
	struct in_addr stAddr;
	struct in_addr stPrefixLen;
	struct in_addr stNexthop;
	unsigned int uiMetric;
}TRipEntry;

typedef struct  RipPacket
{
	unsigned char ucCommand;
	unsigned char ucVersion;
	unsigned short usZero;
	TRipEntry RipEntries[RIP_MAX_ENTRY];
}TRipPkt;


typedef struct RouteEntry
{
	struct RouteEntry *pstNext;
	struct in_addr stIpPrefix;
	struct in_addr  uiPrefixLen;
	struct in_addr stNexthop;
	unsigned int   uiMetric;
	char  *pcIfname;
}TRtEntry;

typedef struct SockRoute
{
	unsigned int uiPrefixLen;
	struct in_addr stIpPrefix;
	unsigned int uiIfindex;
	struct in_addr stNexthop;
	unsigned int uiCmd;
}TSockRoute;

void route_SendForward(unsigned int uiCmd,TRtEntry *pstRtEntry, unsigned if_index);
void requestpkt_Encapsulate();
void rippacket_Receive();
void rippacket_Send(struct in_addr stSourceIp, struct in_addr local_addr);
void rippacket_Multicast(TRipPkt* packet, struct in_addr local_addr, int len);
void request_Handle(struct in_addr stSourceIp);
void response_Handle(struct in_addr stSourceIp, int rip_amount);
void rippacket_Update();
void routentry_Insert();
void localinterf_GetInfo();
void ripdaemon_Start();

#endif
