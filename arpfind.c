#include "arpfind.h"

int arpGet(struct arpmac *srcmac,char *ifname, char *ipStr)
{
    //调用arpGet获取下一跳的mac地址
    struct arpreq nextarp;
    struct sockaddr_in *sin;
    int socketfd = 0;
    memset(&nextarp, 0, sizeof(struct arpreq));
    sin = (struct sockaddr_in*)&nextarp.arp_pa;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr(ipStr);
    strncpy(nextarp.arp_dev, ifname, 15);
    int i = 0;
    printf("ip addr is :");
    printf("%X\n", sin->sin_addr.s_addr);
    printf("ifname is :");
    for(i = 0; i < 6; i++) {
        printf("%c", ifname[i]);
    }
    printf("\n");
    printf("dst ip is %X\n", sin->sin_addr.s_addr);
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    int ret;
    if((ret = ioctl(socketfd, SIOCDARP, &nextarp)) == 0) {
        printf("get dst mac!\n");
        int k;
        for(k = 0; k < 6; k++) {
            srcmac->mac[k] = nextarp.arp_ha.sa_data[k];
        }
    }
    printf("arpget return value is %d\n", ret);
    return 0;
}
