#include "checksum.h"

int check_sum(unsigned short *iphd,int len,unsigned short checksum)
{
    unsigned int sum = 0;
    while(len > 1) {
        sum += *iphd++;
        len -= 2;
    }
    if (len) {
        sum += *(unsigned char*)iphd;
    }
    while(sum >> 16) {
        sum = (sum >> 16) + (sum & 0xffff);
    }
    if((unsigned short)~sum == 0) {
        return 1;
    }
    return 0;
}
unsigned short count_check_sum(unsigned short *iphd)
{
    struct _iphdr* iphder = (struct _iphdr*)iphd;
    iphder->checksum += 1;
    iphder->ttl -= 1;
    return iphder->ttl;
}
