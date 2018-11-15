#include "lookuproute.h"


int insert_route(unsigned long  ip4prefix,unsigned int prefixlen,char *ifname,unsigned int ifindex,unsigned long  nexthopaddr)
{
	int i;
	ip4prefix = htonl(ip4prefix);
	nexthopaddr = htonl(nexthopaddr);
	struct route* listTail = route_table;
	for(i = 1; i < route_table_size; i++) {
		listTail = listTail->next;
	}
	struct nexthop *new_op = (struct nexthop*)malloc(sizeof(struct nexthop*));
	new_op->next = NULL;
	new_op->ifname = malloc(15);
	if_indextoname(ifindex, new_op->ifname);
	new_op->ifindex = ifindex;
	nexthopaddr = htonl(nexthopaddr);
	new_op->nexthopaddr.s_addr = (in_addr_t)nexthopaddr;
	struct route *new_route = (struct route*)malloc(sizeof(struct route*));
	new_route->next = NULL;
	new_route->ip4prefix.s_addr = (in_addr_t)ip4prefix;
	printf("insert prefix %x\n", new_route->ip4prefix.s_addr);
	new_route->prefixlen = prefixlen;
	new_route->nexthop = new_op;
	if(route_table_size == 0) {
		route_table = new_route;
	} else {
		listTail->next = new_route;
	}
	route_table_size++;
}

int lookup_route(struct in_addr dstaddr,struct nextaddr *nexthopinfo)
{
	struct route* r = route_table;
	struct route* selected = NULL;
	unsigned int max_prefix = 0, mask, xor, i, j;
	for(j = 0; j < route_table_size; j++) {
		xor = dstaddr.s_addr ^ r->ip4prefix.s_addr;
		mask = 0xffffffff;
		for(i = 0; i < 32; i++) {
			mask = mask >> 1;
			//if not match
			if((xor & (~mask)) != 0) {
				if(i >= r->prefixlen && i > max_prefix) {
					max_prefix = i;
					selected = r;
				}
				break;
			}
		}
		r = r->next;
	}

	if(selected == NULL) {
		return -1;
	}
	nexthopinfo->ifname = selected->nexthop->ifname;
	nexthopinfo->ipv4addr = selected->nexthop->nexthopaddr;
	nexthopinfo->prefixl = max_prefix;
	return 0;
}

int delete_route(struct in_addr dstaddr,unsigned int prefixlen)
{
	unsigned int xor, mask;
	dstaddr.s_addr = htonl(dstaddr.s_addr);
	struct route* r = route_table, *rpre = NULL;
	int i, j, selected;
	for(i = 0; i < route_table_size; i++) {
		if(dstaddr.s_addr == r->ip4prefix.s_addr && prefixlen == r->prefixlen) {
			if(rpre != NULL) {
				rpre->next = r->next;
			}
			i--;
			route_table_size--;
			selected = 0;
		}
		rpre = r;
		r = r->next;
	}
}
