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
    int sum = 0, len = 20;
    struct _iphdr* iphead = (struct _iphdr*)iphd;
    iphead->checksum = 0;
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
    return (unsigned short)~sum;
}
