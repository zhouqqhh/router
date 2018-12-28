#include "recvroute.h"

int static_route_get(struct selfroute *selfrt, int recvfd)
{
	struct sockaddr_in src_addr;
	int addr_len = sizeof(src_addr);
	int addr;
	addr = accept(recvfd, (struct sockaddr*)&src_addr, (socklen_t *)&addr_len);
	char a;
	ssize_t ret = read(addr, selfrt, sizeof(struct selfroute));
	// printf("route get return value is %d\n", (int)ret);
	if(ret >= 0) {
		return 1;
	}
	return 0;
}
