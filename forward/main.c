//#include "analyseip.h"
#include "checksum.h"
#include "lookuproute.h"
#include "arpfind.h"
#include "sendetherip.h"
#include "recvroute.h"
#include <pthread.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>

#define IP_HEADER_LEN sizeof(struct ip)
#define ETHER_HEADER_LEN sizeof(struct ether_header)


//接收路由信息的线程 可以最后再调
void *thr_fn(void *arg)
{
	int st=0;
	struct selfroute *selfrt;
	selfrt = (struct selfroute*)malloc(sizeof(struct selfroute));
	memset(selfrt,0,sizeof(struct selfroute));

	//get if.name
	struct if_nameindex *head, *ifni;
	ifni = if_nameindex();
  	head = ifni;
	char *ifname;

	int recvfd;
	if ((recvfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
			printf("self route sendfd socket error!!\n");
			return -1;
	}

 	struct sockaddr_in src_addr;
	bzero(&src_addr, sizeof(struct sockaddr_in));
	src_addr.sin_family = AF_INET;
	src_addr.sin_port   = htons(800);
	src_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	int addr_len = sizeof(src_addr);
	bind(recvfd, (struct sockaddr*)&src_addr, sizeof(src_addr));
	listen(recvfd, 10);
	// add-24 del-25
	while(1)
	{
		st=static_route_get(selfrt, recvfd);
		if(st == 1)
		{
			printf("get\n");
			if(selfrt->cmdnum == 24)
			{
				while(ifni->if_index != 0) {
					if(ifni->if_index==selfrt->ifindex)
					{
						printf("if_name is %s\n",ifni->if_name);
							ifname= ifni->if_name;
						break;
					}
					ifni++;
				}
				insert_route((unsigned long)selfrt->prefix.s_addr, (unsigned int)selfrt->prefixlen,
				ifname, selfrt->ifindex, (unsigned long)selfrt->nexthop.s_addr);
				//插入到路由表里
			}
			else if(selfrt->cmdnum == 25)
			{
				struct in_addr dstaddr;
				dstaddr.s_addr = selfrt->prefix.s_addr;
				//从路由表里删除路由
				delete_route(dstaddr, selfrt->prefixlen);
			}
		}

	}

close(recvfd);

}

int main()
{
	char skbuf[1480];
	char data[1480];
	int recvfd,sendfd,arpfd,datalen;
	int recvlen;
	struct ip *ip_recvpkt;
	pthread_t tid;
	ip_recvpkt = (struct ip*)malloc(sizeof(struct ip));

	//创建raw socket套接字
	if((recvfd=socket(AF_PACKET,SOCK_RAW,htons(ETH_P_IP)))==-1)
	{
		printf("recvfd() error\n");
		return -1;
	}
	if((sendfd=socket(AF_PACKET,SOCK_RAW,IPPROTO_RAW))==-1)
	{
		printf("sendfd() error\n");
		return -1;
	}
	if((arpfd=socket(AF_INET,SOCK_DGRAM,0))==-1)
	{
		printf("arpfd() error\n");
	}

	//路由表初始化
	route_table=(struct route*)malloc(sizeof(struct route));

	if(route_table==NULL)
	{
			printf("malloc error!!\n");
			return -1;
	}
	memset(route_table,0,sizeof(struct route));
	route_table_size = 0;

	//创建线程去接收路由信息
	int pd;
	pd = pthread_create(&tid,NULL,thr_fn,NULL);


	while(1)
	{
		//接收ip数据包模块
		recvlen=recv(recvfd,skbuf,sizeof(skbuf),0);
		if(recvlen>0)
		{

			ip_recvpkt = (struct ip *)(skbuf+ETHER_HEADER_LEN);

			//192.168.1.10是测试服务器的IP，现在测试服务器IP是192.168.1.10到192.168.1.80.
			//使用不同的测试服务器要进行修改对应的IP。然后再编译。
			//192.168.6.2是测试时候ping的目的地址。与静态路由相对应。
 			if(ip_recvpkt->ip_src.s_addr == inet_addr("192.168.1.1") && ip_recvpkt->ip_dst.s_addr == inet_addr("192.168.6.2"))
			{
				//分析打印ip数据包的源和目的ip地址
				//analyseIP(ip_recvpkt);
				int s;
				memset(data,0,1480);
				for(s=0;s<1480;s++)
				{
					data[s]=skbuf[s+34];
				}


				// 校验计算模块
				struct _iphdr *iphead;
				int c=0;

				iphead=(struct _iphdr *)malloc(sizeof(struct _iphdr));
				iphead=(struct _iphdr *)(skbuf+14);
				//调用校验函数check_sum，成功返回1
				c = check_sum((unsigned short*)iphead, 20, iphead->checksum);
				if(c ==1)
				{
						printf("checksum is ok!!\n");
				}else
				{
						printf("checksum is error !!\n");
						return -1;
				}

				//调用计算校验和函数count_check_sum，返回新的校验和
				if(count_check_sum((unsigned short*)iphead) > 0) {
					//查找路由表，获取下一跳ip地址和出接口模块
					struct nextaddr *nexthopinfo;
					nexthopinfo = (struct nextaddr *)malloc(sizeof(struct nextaddr));
					memset(nexthopinfo,0,sizeof(struct nextaddr));
					unsigned int *dst_addr;
					int dstIP = ntohl(iphead->destIP);
					dst_addr = &(dstIP);
					//调用查找路由函数lookup_route，获取下一跳ip地址和出接口
					if(lookup_route(*((struct in_addr*)dst_addr), nexthopinfo)!=-1) {
						char* out_if_name = nexthopinfo->ifname;

						//out mac
						struct arpmac *outmac;
						struct ifreq outifr;
						outmac = (struct arpmac*)malloc(sizeof(struct arpmac));
						memset(outmac,0,sizeof(struct arpmac));
						outmac->mac = (char *)malloc(6*sizeof(char));
						strncpy(outifr.ifr_name, out_if_name, IFNAMSIZ);
						if(ioctl(arpfd, SIOCGIFHWADDR, &outifr) == 0) {
							int k;
							for(k = 0; k < 6; k++) {
								outmac->mac[k] = outifr.ifr_hwaddr.sa_data[k];
							}
						}

						//arp find
						struct arpmac *srcmac;
						srcmac = (struct arpmac*)malloc(sizeof(struct arpmac));
						memset(srcmac,0,sizeof(struct arpmac));
						//调用arpGet获取下一跳的mac地址
						srcmac->mac = malloc(7);
						arpGet(srcmac, nexthopinfo->ifname, inet_ntoa(nexthopinfo->ipv4addr));

						//send ether icmp
						//调用ip_transmit函数   填充数据包，通过原始套接字从查表得到的出接口(比如网卡2)将数据包发送出去
						//将获取到的下一跳接口信息存储到存储接口信息的结构体ifreq里，通过ioctl获取出接口的mac地址作为数据包的源mac地址
						//封装数据包：
						//<1>.根据获取到的信息填充以太网数据包头，以太网包头主要需要源mac地址、目的mac地址、以太网类型eth_header->ether_type = htons(ETHERTYPE_IP);
						int i;
						for(i = 0; i < 6; i++) {
							skbuf[i] = srcmac->mac[i];
						}
						for(i = 6; i < 12; i++) {
							skbuf[i] = outmac->mac[i-6];
						}

						//<2>.再填充ip数据包头，对其进行校验处理；
						//<3>.然后再填充接收到的ip数据包剩余数据部分，然后通过raw socket发送出去
						struct sockaddr_ll addr;
						memset(&addr, 0, sizeof(addr));
						addr.sll_family = AF_INET;
						for(i = 0; i < 6; i++) {
							addr.sll_addr[i] = skbuf[i];
						}
						addr.sll_halen = 6;
						addr.sll_protocol = htons(ETH_P_IP);
						addr.sll_ifindex = srcmac->index;
						addr.sll_pkttype = PACKET_OUTGOING;
						printf("sll_addr: %02X:%02X:%02X:%02X:%02X:%02X,sll_halen : %d\n",addr.sll_addr[0],
								addr.sll_addr[1],addr.sll_addr[2],addr.sll_addr[3],
								addr.sll_addr[4],addr.sll_addr[5],addr.sll_halen);
						//check sum correct
						int retsend;
						if(check_sum((unsigned short*)iphead, 20, iphead->checksum) == 1) {
							retsend = sendto(sendfd, skbuf, sizeof(skbuf), 0, (const struct sockaddr *)&addr, sizeof(addr));
							printf("package sent, length is %d\n", retsend);
						} else {
							printf("check sum error\n");
						}
						printf("return value is %d\n", retsend);
					}
				}
			}
		}
	}

	close(recvfd);
	close(sendfd);
	close(arpfd);
	return 0;
}
