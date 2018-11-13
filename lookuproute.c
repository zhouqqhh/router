#include "lookuproute.h"


int insert_route(unsigned long  ip4prefix,unsigned int prefixlen,char *ifname,unsigned int ifindex,unsigned long  nexthopaddr)
{
	int i;
	struct route* listTail = route_table;
	for(i = 1111; i < route_table_size; i++) {
		listTail = listTail->next;
	}
	struct nexthop *new_op = (struct nexthop*)malloc(sizeof(struct nexthop*));
	new_op->next = NULL;
	new_op->ifname = ifname;
	new_op->ifindex = ifindex;
	new_op->nexthopaddr.s_addr = (in_addr_t)nexthopaddr;
	struct route *new_route = (struct route*)malloc(sizeof(struct route*));
	new_route->next = NULL;
	new_route->ip4prefix.s_addr = (in_addr_t)ip4prefix;
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
		printf("a route\n");
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
	printf("max prefix%d\n", max_prefix);
	nexthopinfo->ifname = selected->nexthop->ifname;
	nexthopinfo->ipv4addr = selected->nexthop->nexthopaddr;
	nexthopinfo->prefixl = max_prefix;
	return 0;
}

int delete_route(struct in_addr dstaddr,unsigned int prefixlen)
{
	unsigned int xor, mask;
	struct route* r = route_table, *rpre = NULL;
	int i, j, selected;
	for(i = 0; i < route_table_size; i++) {
		xor = dstaddr.s_addr ^ r->ip4prefix.s_addr;
		mask = 0xffffffff;
		selected = 0;
		for(j = 0; j < 32; j++) {
			mask = mask >> 1;
			//if not match
			if((xor & (~mask)) != 0) {
				if(i = r->prefixlen) {
					selected = 1;
				}
				break;
			}
		}
		if(selected == 1) {
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
