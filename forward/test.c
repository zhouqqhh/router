int arpGet(char *ifname, char *ipStr)
{
	if(ifname == NULL || ipStr == NULL)
	{
		printf("para is null.\n");
		return -1;
	}

	struct arpreq req;
	struct sockaddr_in *sin;
	int ret = 0;
	int sock_fd = 0;

	memset(&req, 0, sizeof(struct arpreq));

	sin = (struct sockaddr_in *)&req.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(ipStr);

	//arp_dev长度为[16]，注意越界
	strncpy(req.arp_dev, ifname, 15);

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_fd < 0)
	{
		printf("get socket error.\n");
		return -1;
	}
	ret = ioctl(sock_fd, SIOCGARP, &req);
	if(ret < 0)
	{
		printf("test ioctl error.\n");
		close(sock_fd);
		return -1;
	}

	unsigned char *hw = (unsigned char *)req.arp_ha.sa_data;
	printf("%#x-%#x-%#x-%#x-%#x-%#x\n", hw[0], hw[1], hw[2], hw[3], hw[4], hw[5]);
	printf("%#x\n", req.arp_flags);
	close(sock_fd);
	return 0;
}
