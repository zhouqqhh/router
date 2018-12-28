#include "rip.h"

TRtEntry *g_pstRouteEntry = NULL;
TRipPkt *request_send_packet = NULL;
TRipPkt *response_send_packet = NULL;
TRipPkt *response_recv_packet = NULL;
TRipPkt *request_recv_packet = NULL;
TRipPkt *update_send_packet = NULL;
int interface_amount;
struct in_addr pcLocalAddr[10];//存储本地接口ip地址
char *pcLocalName[10]={};//存储本地接口的接口名
struct in_addr pcLocalMask[10];
int pcLocalIndex[10];

void requestpkt_Encapsulate()
{
	//封装请求包  command =1,version =2,family =0,metric =16
	printf("Encapsulating\n");
	request_send_packet->ucCommand = RIP_REQUEST;
	request_send_packet->ucVersion = RIP_VERSION;
	request_send_packet->usZero = 0;
	request_send_packet->RipEntries[0].usFamily = 0;
	request_send_packet->RipEntries[0].uiMetric = htonl(RIP_INFINITY);
}


/*****************************************************
*Func Name:    rippacket_Receive
*Description:  接收rip报文
*Input:
*
*Output:
*
*Ret  ：
*
*******************************************************/
void rippacket_Receive()
{
	printf("");
	//接收ip设置
	struct sockaddr_in local_sockaddr;

	local_sockaddr.sin_family = AF_INET;
	local_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_sockaddr.sin_port = htons(RIP_PORT);
	//创建并绑定socket
	int iRecvfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (iRecvfd < 0) {
		perror("Failed to create receive socket!\n");
		exit(-1);
	}
	if (bind(iRecvfd, (struct sockaddr*)&local_sockaddr, sizeof(struct sockaddr))<0)
	{
		perror("setsockopt\n");
		return ;
	}
	//防止绑定地址冲突，仅供参考
	//设置地址重用
	int  iReUseddr = 1;
	if (setsockopt(iRecvfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&iReUseddr,sizeof(iReUseddr))<0)
	{
		perror("setsockopt\n");
		return ;
	}
	//设置端口重用
	int  iReUsePort = 1;
	if (setsockopt(iRecvfd,SOL_SOCKET ,SO_REUSEPORT,(const char*)&iReUsePort,sizeof(iReUsePort))<0)
	{
		perror("setsockopt\n");
		return ;
	}

	//把本地地址加入到组播中
	int i;
	for (i = 0; i < interface_amount; i++) {
		struct ip_mreq req;
		req.imr_interface = pcLocalAddr[i];
		printf("add to multicast %s\n", inet_ntoa(pcLocalAddr[i]));
		req.imr_multiaddr.s_addr = inet_addr(RIP_GROUP);
		if (setsockopt(iRecvfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(struct ip_mreq)) < 0) {
			perror("Failed to set sockopt1\n");
			exit(-1);
		}
	}


	//防止组播回环的参考代码
	int loop = 0;
	int err = setsockopt(iRecvfd,IPPROTO_IP, IP_MULTICAST_LOOP,&loop, sizeof(loop));
	if(err < 0)
	{
		perror("setsockopt():IP_MULTICAST_LOOP");
	}

	char buf[RIP_MAX_PACKET];
	while(1)
	{
		memset(buf, 0, sizeof(buf));
		struct sockaddr_in recv_addr;
		memset(&recv_addr, 0, sizeof(struct sockaddr_in));

		// 接收rip报文   存储接收源ip地址
		int packetLen, sockaddr_len = sizeof(struct sockaddr_in);
		if ((packetLen = recvfrom(iRecvfd, buf, sizeof(buf), 0, (struct sockaddr*)&recv_addr, &sockaddr_len)) < 0) {
			perror("Failed to receive\n");
		} else  {
			printf("Receive success\n");
		}
		TRipPkt* tmp = (TRipPkt *)buf;
		// 判断command类型，request 或 response
		// 接收到的信息存储到全局变量里，方便request_Handle和response_Handle处理
		if (tmp->ucCommand == RIP_RESPONSE) {
			response_recv_packet = tmp;
			response_Handle(recv_addr.sin_addr, (packetLen - 4) / 20);
		} else if (tmp->ucCommand == RIP_REQUEST) {
			request_recv_packet = tmp;
			request_Handle(recv_addr.sin_addr);
		} else {
			printf("Not a response or request\n");
		}
	}
}


/*****************************************************
*Func Name:    rippacket_Send
*Description:  向接收源发送响应报文
*Input:
*	  1.stSourceIp    ：接收源的ip地址，用于发送目的ip设置
*Output:
*
*Ret  ：
*
*******************************************************/
void rippacket_Send(struct in_addr stSourceIp, struct in_addr local_addr)
{
	printf("send rip packet to %s\n",  inet_ntoa(stSourceIp));
	//本地ip设置
	struct sockaddr_in local_sockaddr;
	local_sockaddr.sin_family = AF_INET;
	local_sockaddr.sin_addr = local_addr;
	local_sockaddr.sin_port = htons(RIP_PORT);

	//目的ip设置
	struct sockaddr_in dst_sockaddr;
	dst_sockaddr.sin_family = AF_INET;
	dst_sockaddr.sin_addr = stSourceIp;
	dst_sockaddr.sin_port = htons(RIP_PORT);

	int iSendfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (iSendfd < 0) {
		perror("Failed to create iSendfd socket!\n");
		exit(-1);
	}

	//防止绑定地址冲突，仅供参考
	//设置地址重用
	int  iReUseddr = 1;
	if (setsockopt(iSendfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&iReUseddr,sizeof(iReUseddr))<0)
	{
		perror("setsockopt\n");
		return ;
	}
	//设置端口重用
	int  iReUsePort = 1;
	if (setsockopt(iSendfd,SOL_SOCKET ,SO_REUSEPORT,(const char*)&iReUsePort,sizeof(iReUsePort))<0)
	{
		perror("setsockopt\n");
		return ;
	}
	//把本地地址加入到组播中
	struct ip_mreq req;
	req.imr_interface = local_addr;
	req.imr_multiaddr.s_addr = inet_addr(RIP_GROUP);
	if (setsockopt(iSendfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(struct ip_mreq)) < 0) {
		perror("Failed to set sockopt2\n");
		exit(-1);
	}

	//创建并绑定socket
	if (bind(iSendfd, (struct sockaddr*)&local_sockaddr, sizeof(local_sockaddr)) < 0) {
		perror("Failed to bind isock\n");
		exit(-1);
	}

	//发送
	if (sendto(iSendfd, response_send_packet, RIP_MAX_PACKET, 0, (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
		perror("Failed to send response!\n");
		exit(-1);
	}

	//close
	close(iSendfd);
	return;
}

/*****************************************************
*Func Name:    rippacket_Multicast
*Description:  组播请求报文
*Input:
*	  1.pcLocalAddr   ：本地ip地址
*Output:
*
*Ret  ：
*
*******************************************************/
void rippacket_Multicast(TRipPkt* packet, struct in_addr local_addr, int len)
{
	printf("    Multicasting from %s\n", inet_ntoa(local_addr));
	//本地ip设置
	struct sockaddr_in local_sockaddr;
	local_sockaddr.sin_family = AF_INET;
	local_sockaddr.sin_addr = local_addr;
	local_sockaddr.sin_port = htons(RIP_PORT);

	//目的ip设置
	struct sockaddr_in dst_sockaddr;
	dst_sockaddr.sin_family = AF_INET;
	dst_sockaddr.sin_addr.s_addr = inet_addr(RIP_GROUP);
	dst_sockaddr.sin_port = htons(RIP_PORT);

	//创建并绑定socket
	int iSendfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (iSendfd < 0) {
		perror("Failed to create isock\n");
		exit(-1);
	}

	//防止绑定地址冲突，仅供参考
	//设置地址重用
	int  iReUseddr = 1;
	if (setsockopt(iSendfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&iReUseddr,sizeof(iReUseddr))<0)
	{
		perror("setsockopt\n");
		return ;
	}
	//设置端口重用
	int  iReUsePort = 1;
	if (setsockopt(iSendfd,SOL_SOCKET ,SO_REUSEPORT,(const char*)&iReUsePort,sizeof(iReUsePort))<0)
	{
		perror("setsockopt\n");
		return ;
	}

	if (bind(iSendfd, (struct sockaddr*)&local_sockaddr, sizeof(local_sockaddr)) < 0) {
		perror("Failed to bind isock\n");
		exit(-1);
	}

	//把本地地址加入到组播中
	struct ip_mreq req;
	req.imr_interface = local_addr;
	req.imr_multiaddr.s_addr = inet_addr(RIP_GROUP);
	if (setsockopt(iSendfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(struct ip_mreq)) < 0) {
		perror("Failed to set sockopt3\n");
		exit(-1);
	}

	//防止组播回环的参考代码

	int loop = 0;
	int err = setsockopt(iSendfd,IPPROTO_IP, IP_MULTICAST_LOOP,&loop, sizeof(loop));
	if(err < 0)
	{
		perror("setsockopt():IP_MULTICAST_LOOP");
	}

	//发送
	if (sendto(iSendfd, packet, len, 0, (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
		perror("Failed to multicast\n");
		exit(-1);
	}

	//close
	close(iSendfd);
	return;
}

/*****************************************************
*Func Name:    request_Handle
*Description:  响应request报文
*Input:
*	  1.stSourceIp   ：接收源的ip地址
*Output:
*
*Ret  ：
*
*******************************************************/
void request_Handle(struct in_addr stSourceIp)
{
	int entry, i;
	printf("Handling request from %s\n", inet_ntoa(stSourceIp));
	//处理request报文
	TRtEntry *tmp = g_pstRouteEntry;
	while (tmp != NULL) {
		response_send_packet->ucCommand = RIP_RESPONSE;
		response_send_packet->ucVersion = RIP_VERSION;
		response_send_packet->usZero = 0;
		for (entry = 0; entry < RIP_MAX_ENTRY; entry++) {
			printf("entry %d\n", entry);
			response_send_packet->RipEntries[entry].stAddr = tmp->stIpPrefix;
			response_send_packet->RipEntries[entry].stNexthop = tmp->stNexthop;
			//遵循水平分裂算法
			if (tmp->stNexthop.s_addr == stSourceIp.s_addr) {
				response_send_packet->RipEntries[entry].uiMetric = RIP_INFINITY;
			} else {
				response_send_packet->RipEntries[entry].uiMetric = tmp->uiMetric;
			}
			response_send_packet->RipEntries[entry].stPrefixLen = tmp->uiPrefixLen;
			response_send_packet->RipEntries[entry].usFamily = htons(2);
			response_send_packet->RipEntries[entry].usTag = 0;
			tmp = tmp->pstNext;
			if (tmp == NULL) {
				break;
			}
		}
		if (entry > 0) {
			for (i = 0; i < interface_amount; i++) {
				//send only if the interface directly connect source ip
				printf("Handling request from %s\n", inet_ntoa(stSourceIp));
				if ((pcLocalAddr[i].s_addr & pcLocalMask[i].s_addr) ==
					(stSourceIp.s_addr & pcLocalMask[i].s_addr)) {
					printf("Handling request from %s\n", inet_ntoa(stSourceIp));
						rippacket_Send(stSourceIp, pcLocalAddr[i]);
					}
			}
		}
	}
	//回送response报文，command置为RIP_RESPONSE
	return;
}

/*****************************************************
*Func Name:    response_Handle
*Description:  响应response报文
*Input:
*	  1.stSourceIp   ：接收源的ip地址
*Output:
*
*Ret  ：
*
*******************************************************/
void response_Handle(struct in_addr stSourceIp, int rip_amount)
{
	printf("Handling response from %s\n", inet_ntoa(stSourceIp));
	//处理response报文
	int i, ifindex;
	int find = 0, update = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//figure out interface if_index
		printf("stSourceIp %s\n", inet_ntoa(stSourceIp));
	for (i = 0; i < interface_amount; i++) {
		printf("pc local addr %s\n", inet_ntoa(pcLocalAddr[i]));
			printf("pc local mask %s\n", inet_ntoa(pcLocalMask[i]));
				printf("pc local index %d\n", pcLocalIndex[i]);
				printf("%X\n", pcLocalAddr[i].s_addr & pcLocalMask[i].s_addr);
				printf("%X\n",stSourceIp.s_addr & pcLocalMask[i].s_addr);

		if ((pcLocalAddr[i].s_addr & pcLocalMask[i].s_addr) == (stSourceIp.s_addr & pcLocalMask[i].s_addr)) {
			ifindex = pcLocalIndex[i];
			break;
		}
	}
	if (i >= interface_amount) {
		ifindex = -1;
	}
	printf("ifindex is %d\n", ifindex);

	printf("receive rip amount %d\n", rip_amount);
	TRtEntry *tmp;
	for (i = 0; i < rip_amount; i++) {
		tmp = g_pstRouteEntry;
		find = 0;
		printf("This is a recv route:\n");
		printf("stIpPrefix %s\n", inet_ntoa((response_recv_packet->RipEntries[i]).stAddr));
		printf("prefix lenght is %s\n", inet_ntoa(response_recv_packet->RipEntries[i].stPrefixLen));
		int tmpMeric = ntohl(response_recv_packet->RipEntries[i].uiMetric);
		while (tmp != NULL) {
			printf("looking for rip route table...\n");
			if (tmp->stIpPrefix.s_addr == response_recv_packet->RipEntries[i].stAddr.s_addr) {
				//tmp->pcIfname =
				find = 1;
				unsigned int pre_step = htonl(tmp->uiMetric);
				unsigned int recv_step = tmpMeric;
				if (recv_step >= RIP_INFINITY) {
					//delete and send to engine
					tmp->uiMetric = RIP_INFINITY;
					update = 1;
					route_SendForward(DelRoute, tmp, ifindex);
				} else if (tmp->stNexthop.s_addr == response_recv_packet->RipEntries[i].stNexthop.s_addr) {
					//update table and send to engine
					route_SendForward(DelRoute, tmp, ifindex);
					tmp->uiMetric = htonl(recv_step + 1);
					if (recv_step + 1 != pre_step) {
						update = 1;
					}
					route_SendForward(AddRoute, tmp, ifindex);
				} else if (recv_step + 1 <= pre_step) {
					route_SendForward(DelRoute, tmp, ifindex);
					tmp->uiMetric = htonl(recv_step + 1);
					tmp->stNexthop = stSourceIp;
					if (recv_step + 1 < pre_step || tmp->stNexthop.s_addr != stSourceIp.s_addr) {
						update = 1;
					}
					if (tmp->uiMetric > RIP_INFINITY) {
						update = 1;
						tmp->uiMetric = RIP_INFINITY;
					}
					route_SendForward(AddRoute, tmp, ifindex);
				}
			}
			tmp = tmp->pstNext;
		}
		if (find == 1) {
			printf("already have, just update\n");
		} else {
			printf("haven't found, insert it\n");
			printf("metric is %d\n", tmpMeric);
		}
		if (!find && tmpMeric < RIP_INFINITY - 1) {
			//insert and send to engine
			tmp = (TRtEntry *)malloc(sizeof(TRtEntry));
			tmp->stIpPrefix = response_recv_packet->RipEntries[i].stAddr;
			tmp->stNexthop = stSourceIp;
			tmp->uiPrefixLen = response_recv_packet->RipEntries[i].stPrefixLen;
			tmp->uiMetric = htonl(ntohl(response_recv_packet->RipEntries[i].uiMetric) + 1);
			//tmp->pcIfname =
			printf("inserting a route\n");
			printf("IP prefix is %s\n", inet_ntoa(tmp->stIpPrefix));
			printf("Prefix length is %s\n", inet_ntoa(tmp->uiPrefixLen));
			printf("Next hop is %s\n", inet_ntoa(tmp->stNexthop));
			printf("send if index %d\n", ifindex);
			route_SendForward(AddRoute, tmp, ifindex);

			tmp->pstNext = g_pstRouteEntry;
			g_pstRouteEntry = tmp;
			update = 1;
		}
	}

	if (update) {
		rippacket_Update();
	}
	return;
}

/*****************************************************
*Func Name:    route_SendForward
*Description:  响应response报文
*Input:
*	  1.uiCmd        ：插入命令
*	  2.pstRtEntry   ：路由信息
*Output:
*
*Ret  ：
*
*******************************************************/
void route_SendForward(unsigned int uiCmd,TRtEntry *pstRtEntry, unsigned if_index)
{
	//建立tcp短连接，发送插入、删除路由表项信息到转发引擎
	int sendfd, i;
	int sendfdlen = 0, prefixlen = 0;
	int tcp_count = 0;
	struct sockaddr_in dst_addr;
	char buf[sizeof(struct SockRoute)];
	unsigned unsigned_prefix = ntohl(pstRtEntry->uiPrefixLen.s_addr);

	for (i = 0; i < 32; i++) {
		if ((unsigned_prefix & 0x01) == 1) {
			prefixlen = 32 - i;
			break;
		}
		unsigned_prefix >>= 1;
	}

	memset(buf, 0, sizeof(buf));
	struct SockRoute *sockrt;
	sockrt = &buf;
	sockrt->uiPrefixLen = prefixlen;
	sockrt->stIpPrefix = pstRtEntry->stIpPrefix;
	if (if_index == -1) {
		sockrt->uiIfindex = if_nametoindex(pstRtEntry->pcIfname);
	} else {
		sockrt->uiIfindex = if_index;
	}
	sockrt->stNexthop = pstRtEntry->stNexthop;
	sockrt->uiCmd = uiCmd;

	bzero(&dst_addr, sizeof(struct sockaddr_in));
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(800);
	dst_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if((sendfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("sockrt socket sending error!\n");
		exit(-1);
	}

	while (tcp_count < 6) {
		if (connect(sendfd, (struct sockaddr*)&dst_addr, sizeof(dst_addr)) < 0) {
			tcp_count++;
		} else {
			break;
		}
		usleep(10000);
	}

	if(tcp_count < 6) {
		sendfdlen = send(sendfd, buf, sizeof(buf), 0);
		if (sendfdlen <= 0) {
			perror("socketrt send error!\n");
			exit(-1);
		} else {
			perror("socketrt send okk!");
		}
		usleep(50000);
		close(sendfd);
	}
	return;
}

void rippacket_Update()
{
	printf("    Updating rip packet\n");
	//遍历rip路由表，封装更新报文
	struct timeval tv;
	gettimeofday(&tv, NULL);
	TRtEntry *tmp;
	int i, ifindex;

	for (ifindex = 0; ifindex < interface_amount; ifindex++) {
		tmp = g_pstRouteEntry;
		while (tmp != NULL) {
			update_send_packet->ucCommand = RIP_RESPONSE;
			update_send_packet->ucVersion = RIP_VERSION;
			update_send_packet->usZero = 0;

			for (i = 0; i < RIP_MAX_ENTRY; i++) {
				if (tmp == NULL) {
					break;
				}
				//注意水平分裂算法
				if (tmp->uiMetric < RIP_INFINITY && ((pcLocalAddr[ifindex].s_addr & pcLocalMask[ifindex].s_addr) != tmp->stNexthop.s_addr)) {
					update_send_packet->RipEntries[i].stAddr = tmp->stIpPrefix;
					update_send_packet->RipEntries[i].stNexthop = tmp->stNexthop;
					update_send_packet->RipEntries[i].uiMetric = htonl(tmp->uiMetric);
					update_send_packet->RipEntries[i].stPrefixLen = tmp->uiPrefixLen;
					update_send_packet->RipEntries[i].usFamily = htons(2);
					update_send_packet->RipEntries[i].usTag = 0;
				} else {
					i--;
				}
				tmp = tmp->pstNext;
			}
			if (i > 0) {
				rippacket_Multicast(update_send_packet, pcLocalAddr[ifindex], i * 20 + 4);
			}
		}
	}
}

void checkTableValid() {
	//remove items that is not valid
}

void* update_thread_func(void* arg) {
	while(1) {
		printf("Updating!");
		rippacket_Update();
		checkTableValid();
		sleep(30);
	}
}

void ripdaemon_Start()
{
	int ret;
	//创建更新线程，30s更新一次,向组播地址更新Update包
	pthread_t update_pthread;
	ret = pthread_create(&update_pthread, NULL, update_thread_func, NULL);
	if (ret != 0) {
		perror("update thread create error!\n");
		exit(-1);
	}

	//封装请求报文，并组播
	requestpkt_Encapsulate();
	for (int i = 0; i < interface_amount; i++) {
		rippacket_Multicast(request_send_packet, pcLocalAddr[i], 24);
	}

	//接收rip报文
	rippacket_Receive();
	return;
}

void routentry_Insert()
{
 //将本地接口表添加到rip路由表里
 TRtEntry *p = g_pstRouteEntry;
 int i = 0;
 while(i < 10 && pcLocalAddr[i].s_addr != 0) {

  TRtEntry *entry = (TRtEntry*)malloc(sizeof(TRtEntry));
  entry->stIpPrefix.s_addr = pcLocalAddr[i].s_addr & pcLocalMask[i].s_addr;
  entry->uiPrefixLen = pcLocalMask[i];
  entry->stNexthop.s_addr = inet_addr("0.0.0.0");
  entry->uiMetric = 1;
  entry->pcIfname = pcLocalName[i];
  entry->pstNext = NULL;
  route_SendForward(AddRoute, entry, -1);
  p->pstNext = entry;
  p = p->pstNext;
  i += 1;
 }
 printf("finish insert straight route!\n");
 g_pstRouteEntry = g_pstRouteEntry->pstNext;
 p = g_pstRouteEntry;
 while(p != NULL) {
	 printf("ip prefix %s\n", inet_ntoa(p->stIpPrefix));
	 p = p->pstNext;
 }
 return;
}

void localinterf_GetInfo()
{
 struct ifaddrs *pstIpAddrStruct = NULL;
 struct ifaddrs *pstIpAddrStCur  = NULL;
 void *pAddrPtr=NULL;
 const char *pcLo = "127.0.0.1";

 getifaddrs(&pstIpAddrStruct); //linux系统函数
 pstIpAddrStCur = pstIpAddrStruct;

 int i = 0;
 while(pstIpAddrStruct != NULL)
 {
  if(pstIpAddrStruct->ifa_addr->sa_family==AF_INET)
  {
			pAddrPtr = &((struct sockaddr_in *)pstIpAddrStruct->ifa_addr)->sin_addr;
			char cAddrBuf[INET_ADDRSTRLEN];
			memset(&cAddrBuf,0,sizeof(INET_ADDRSTRLEN));
			inet_ntop(AF_INET, pAddrPtr, cAddrBuf, INET_ADDRSTRLEN);
			if(strcmp((const char*)&cAddrBuf,pcLo) != 0)
			{
				pcLocalAddr[i] = ((struct sockaddr_in *)(pstIpAddrStruct->ifa_addr))->sin_addr;
				pcLocalName[i] = (char *)malloc(sizeof(IF_NAMESIZE));
				pcLocalMask[i] = ((struct sockaddr_in *)(pstIpAddrStruct->ifa_netmask))->sin_addr;
				strcpy(pcLocalName[i],(const char*)pstIpAddrStruct->ifa_name);
				pcLocalIndex[i] = if_nametoindex(pcLocalName[i]);
				i++;
			}
		}
  pstIpAddrStruct = pstIpAddrStruct->ifa_next;
 }
 interface_amount = i;
 printf("interface amount is %d\n", i);
 freeifaddrs(pstIpAddrStCur);//linux系统函数
 return ;}

int main(int argc,char* argv[])
{
	g_pstRouteEntry = (TRtEntry *)malloc(sizeof(TRtEntry));
	request_send_packet = (TRipPkt *)malloc(sizeof(TRipPkt));
	response_send_packet = (TRipPkt *)malloc(sizeof(TRipPkt));
	update_send_packet = (TRipPkt *)malloc(sizeof(TRipPkt));
	response_recv_packet = (TRipPkt *)malloc(sizeof(TRipPkt));
	request_recv_packet = (TRipPkt *)malloc(sizeof(TRipPkt));
	if(g_pstRouteEntry == NULL)
	{
		perror("g_pstRouteEntry malloc error !\n");
		return -1;
	}
	localinterf_GetInfo();
	routentry_Insert();
	ripdaemon_Start();
	return 0;
}
