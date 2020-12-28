// ����ARP��ȡIP��MAC.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#pragma pack(1) 
//��̫�����ݱ��ṹ
typedef struct Ethernet_head
{
	u_char DestMAC[6];    //Ŀ��MAC��ַ 6�ֽ�
	u_char SourMAC[6];   //ԴMAC��ַ 6�ֽ�
	u_short EthType;
};

//ARP���ݱ�ͷ�ṹ
typedef struct ARPFrame_t
{
	unsigned short HardwareType; //Ӳ������
	unsigned short ProtocolType; //Э������
	unsigned char HardwareAddLen; //Ӳ����ַ����
	unsigned char ProtocolAddLen; //Э���ַ����
	unsigned short OperationField; //�����ֶ�
	unsigned char SourceMacAdd[6]; //Դmac��ַ
	unsigned long SourceIpAdd; //Դip��ַ
	unsigned char DestMacAdd[6]; //Ŀ��mac��ַ
	unsigned long DestIpAdd; //Ŀ��ip��ַ
};
//arp���ṹ
struct ArpPacket {
	Ethernet_head ed;
	ARPFrame_t ah;
};
//�̲߳���
struct sparam {
	pcap_t *adhandle;
	char *ip;
	unsigned char *mac;
	char *netmask;
};
struct gparam {
	pcap_t *adhandle;
};

struct sparam sp;
struct gparam gp;
char *myBroad;
unsigned char *m_MAC=new unsigned char[6];
char *m_IP;
char *m_mask;
#define IPTOSBUFFERS    12
void ifprint(pcap_if_t *d);
char *iptos(u_long in);
int GetSelfMac(pcap_t *adhandle, const char *ip_addr, unsigned char *ip_mac);
DWORD WINAPI SendArpPacket(LPVOID lpParameter);
DWORD WINAPI GetLivePC(LPVOID lpParameter);
int GetLivePC2(pcap_t *adhandle);
int _tmain(int argc, _TCHAR* argv[])
{
	pcap_if_t *alldevs;
	pcap_if_t *d;
	pcap_addr_t *a;
	int num = 0;
	char errbuf[PCAP_ERRBUF_SIZE];
	struct tm *ltime;
	time_t local_tv_sec;
	char timestr[16];
	struct pcap_pkthdr *header = new pcap_pkthdr;
	const u_char *pkt_data = new u_char;
	int res;

	//��ȡ�����豸�б�
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
	{
		//������
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
	}
	//��ʾ�ӿ��б�
	for (d = alldevs; d != NULL; d = d->next)
	{
		/* �豸��(Name) */
		printf("%s\n", d->name);

		/* �豸����(Description) */
		if (d->description)
			printf("\tDescription: %s\n", d->description);

		printf("\n");
	}
	//ѡ���豸
	d = alldevs;
	cout << "����ѡ����豸�ţ�" << endl;
	cin >> num;
	for (int i = 0; i < num - 1; i++)
		d = d->next;
	ifprint(d);
	//��ָ��������ӿ�
	pcap_t *adhandle;
	if ((adhandle = pcap_open_live(d->name, 65536, PCAP_OPENFLAG_PROMISCUOUS, 1000, errbuf)) == NULL)
	{
		fprintf(stderr, "\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name);
		pcap_freealldevs(alldevs);
		return -1;
	}

	printf("\nlistening on %s...\n", d->description);
	GetSelfMac(adhandle, m_IP, m_MAC);
	printf("MyMAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
		*m_MAC,
		*(m_MAC+1),
		*(m_MAC+2),
		*(m_MAC+3),
		*(m_MAC+4),
		*(m_MAC+5));

	HANDLE sendthread;      //����ARP���߳�
	HANDLE recvthread;       //����ARP���߳�
	sp.adhandle = adhandle;
	sp.ip = m_IP;
	sp.netmask = m_mask;
	sp.mac = m_MAC;
	gp.adhandle = adhandle;
	
		sendthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendArpPacket,
			&sp, 0, NULL);
		/*recvthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GetLivePC, &gp,
			0, NULL);*/
		GetLivePC2(adhandle);
		
	pcap_freealldevs(alldevs);
	CloseHandle(sendthread);
	CloseHandle(recvthread);
	while (1);
	return 0;
}
/* ��ӡ���п�����Ϣ */
void ifprint(pcap_if_t *d)
{
	pcap_addr_t *a;

	/* IP addresses */
	for (a = d->addresses; a; a = a->next) 
	{
		printf("\tAddress Family: #%d\n", a->addr->sa_family);
		switch (a->addr->sa_family)
		{
		case AF_INET:
			printf("\tAddress Family Name: AF_INET\n");
			if (a->addr)
			{
				m_IP = iptos(((struct sockaddr_in *)a->addr)->sin_addr.s_addr);
				printf("\tIP Address: %s\n", m_IP);
			}
			if (a->netmask)
			{
				m_mask = iptos(((struct sockaddr_in *)a->netmask)->sin_addr.s_addr);
				printf("\tNetmask: %s\n", m_mask);
			}
				
			if (a->broadaddr)
			{
				myBroad = iptos(((struct sockaddr_in *)a->broadaddr)->sin_addr.s_addr);
				printf("\tBroadcast Address: %s\n", myBroad);
			}
			if (a->dstaddr)
				printf("\tDestination Address: %s\n", iptos(((struct sockaddr_in *)a->dstaddr)->sin_addr.s_addr));
			break;
		default:
			//printf("\tAddress Family Name: Unknown\n");
			break;
		}
	}
	printf("\n");
}

char *iptos(u_long in)
{
	static char output[IPTOSBUFFERS][3 * 4 + 3 + 1];
	static short which;
	u_char *p;

	p = (u_char *)&in;
	which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
	sprintf(output[which], "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	return output[which];
}
#define ETH_ARP         0x0806  //��̫��֡���ͱ�ʾ�������ݵ����ͣ�����ARP�����Ӧ����˵�����ֶε�ֵΪx0806
#define ARP_HARDWARE    1  //Ӳ�������ֶ�ֵΪ��ʾ��̫����ַ
#define ETH_IP          0x0800  //Э�������ֶα�ʾҪӳ���Э���ַ����ֵΪx0800��ʾIP��ַ
#define ARP_REQUEST     1   //ARP����
#define ARP_REPLY       2      //ARPӦ��
#define HOSTNUM         255   //��������
// ��ȡ�Լ�������MAC��ַ
int GetSelfMac(pcap_t *adhandle, const char *ip_addr, unsigned char *ip_mac) {
	unsigned char sendbuf[42]; //arp���ṹ��С
	int i = -1;
	int res;
	Ethernet_head eh; //��̫��֡ͷ
	ARPFrame_t ah;  //ARP֡ͷ
	struct pcap_pkthdr * pkt_header;
	const u_char * pkt_data=new u_char;
	//���ѿ����ڴ�ռ� eh.dest_mac_add ���� 6���ֽڵ�ֵ��Ϊֵ 0xff��
	memset(eh.DestMAC, 0xff, 6); //Ŀ�ĵ�ַΪȫΪ�㲥��ַ
	memset(eh.SourMAC, 0x0f, 6);
	memset(ah.DestMacAdd, 0x0f, 6);
	memset(ah.SourceMacAdd, 0x00, 6);
	//htons��һ���޷��Ŷ����͵�������ֵת��Ϊ�����ֽ�˳��
	eh.EthType = htons(ETH_ARP);
	ah.HardwareType = htons(ARP_HARDWARE);
	ah.ProtocolType = htons(ETH_IP);
	ah.HardwareAddLen = 6;
	ah.ProtocolAddLen = 4;
	ah.SourceIpAdd = inet_addr("100.100.100.100"); //����������ip
	ah.OperationField = htons(ARP_REQUEST);
	ah.DestIpAdd = inet_addr(ip_addr);
	memset(sendbuf, 0, sizeof(sendbuf));
	memcpy(sendbuf, &eh, sizeof(eh));
	memcpy(sendbuf + sizeof(eh), &ah, sizeof(ah));
	printf("%s", sendbuf);
	if (pcap_sendpacket(adhandle, sendbuf, 42) == 0) {
		printf("\nPacketSend succeed\n");
	}
	else {
		printf("PacketSendPacket in getmine Error: %d\n", GetLastError());
		return 0;
	}
	
	while ((res = pcap_next_ex(adhandle, &pkt_header, &pkt_data)) >= 0) {
		if (*(unsigned short *)(pkt_data + 12) == htons(ETH_ARP)&& *(unsigned short*)(pkt_data + 20) == htons(ARP_REPLY)&& *(unsigned long*)(pkt_data + 38)== inet_addr("100.100.100.100")) {
			for (i = 0; i < 6; i++) {
				ip_mac[i] = *(unsigned char *)(pkt_data + 22 + i);
			}
			printf("��ȡ�Լ�������MAC��ַ�ɹ�!\n");
			break;
		}
	}
	if (i == 6) {
		return 1;
	}
	else {
		return 0;
	}
}
bool flag;
DWORD WINAPI SendArpPacket(LPVOID lpParameter)
{
	sparam *spara = (sparam *)lpParameter;
	pcap_t *adhandle = spara->adhandle;
	char *ip = spara->ip;
	unsigned char *mac = spara->mac;
	char *netmask = spara->netmask;
	printf("ip_mac:%02x-%02x-%02x-%02x-%02x-%02x\n", mac[0], mac[1], mac[2],
		mac[3], mac[4], mac[5]);
	printf("�����IP��ַΪ:%s\n", ip);
	printf("��ַ����NETMASKΪ:%s\n", netmask);
	printf("\n");
	unsigned char sendbuf[42]; //arp���ṹ��С
	Ethernet_head eh;
	ARPFrame_t ah;
	//��ֵMAC��ַ
	memset(eh.DestMAC, 0xff, 6);       //Ŀ�ĵ�ַΪȫΪ�㲥��ַ
	memcpy(eh.SourMAC, mac, 6);
	memcpy(ah.SourceMacAdd, mac, 6);
	memset(ah.DestMacAdd, 0x00, 6);
	eh.EthType = htons(ETH_ARP);//֡����ΪARP3
	ah.HardwareType = htons(ARP_HARDWARE);
	ah.ProtocolType = htons(ETH_IP);
	ah.HardwareAddLen = 6;
	ah.ProtocolAddLen = 4;
	ah.SourceIpAdd = inet_addr(ip); //���󷽵�IP��ַΪ�����IP��ַ
	ah.OperationField = htons(ARP_REQUEST);
	//��������ڹ㲥����arp��
	unsigned long myip = inet_addr(ip);
	unsigned long mynetmask = inet_addr(netmask);
	unsigned long hisip = htonl((myip & mynetmask));
	//��ָ��IP��������
	char desIP[16];
	printf("����Ŀ��IP:");
	scanf("%s", &desIP);
	//char* desIP = "192.168.43.55";
	ah.DestIpAdd = htonl(inet_addr(desIP));
		//����һ��ARP����
		memset(sendbuf, 0, sizeof(sendbuf));
		memcpy(sendbuf, &eh, sizeof(eh));
		memcpy(sendbuf + sizeof(eh), &ah, sizeof(ah));
		//������ͳɹ�
		if (pcap_sendpacket(adhandle, sendbuf, 42) == 0) {
			printf("\nPacketSend succeed\n");
		}
		else {
			printf("PacketSendPacket in getmine Error: %d\n", GetLastError());
		}
	flag = TRUE;
	return 0;
}
DWORD WINAPI GetLivePC(LPVOID lpParameter) //(pcap_t *adhandle)
{
	
	gparam *gpara = (gparam *)lpParameter;
	pcap_t *adhandle = gpara->adhandle;
	int res;
	unsigned char Mac[6];
	struct pcap_pkthdr * pkt_header;
	const u_char * pkt_data;
	while (true) {
		if ((res = pcap_next_ex(adhandle, &pkt_header, &pkt_data)) >= 0) {
			if (*(unsigned short *)(pkt_data + 12) == htons(ETH_ARP)) {
				ArpPacket *recv = (ArpPacket *)pkt_data;
				if (*(unsigned short *)(pkt_data + 20) == htons(ARP_REPLY)) {
					//printf("-------------------------------------------\n");
					printf("IP��ַ:%d.%d.%d.%d   MAC��ַ:",
						recv->ah.SourceIpAdd & 255,
						recv->ah.SourceIpAdd >> 8 & 255,
						recv->ah.SourceIpAdd >> 16 & 255,
						recv->ah.SourceIpAdd >> 24 & 255);
					for (int i = 0; i < 6; i++) {
						Mac[i] = *(unsigned char *)(pkt_data + 22 + i);
						printf("%02x ", Mac[i]);
					}
					printf("\n");
				}
			}
		}
		Sleep(10);
	}
	return 0;
}
int GetLivePC2(pcap_t *adhandle) //(pcap_t *adhandle)
{

	//gparam *gpara = (gparam *)lpParameter;
	//pcap_t *adhandle = gpara->adhandle;
	int res;
	unsigned char Mac[6];
	struct pcap_pkthdr * pkt_header;
	const u_char * pkt_data;
	while (true) {
		if ((res = pcap_next_ex(adhandle, &pkt_header, &pkt_data)) >= 0) {
			if (*(unsigned short *)(pkt_data + 12) == htons(ETH_ARP)) {
				ArpPacket *recv = (ArpPacket *)pkt_data;
				if (*(unsigned short *)(pkt_data + 20) == htons(ARP_REPLY)) {
					printf("IP��ַ:%d.%d.%d.%d   MAC��ַ:",
						recv->ah.SourceIpAdd & 255,
						recv->ah.SourceIpAdd >> 8 & 255,
						recv->ah.SourceIpAdd >> 16 & 255,
						recv->ah.SourceIpAdd >> 24 & 255);
					for (int i = 0; i < 6; i++) {
						Mac[i] = *(unsigned char *)(pkt_data + 22 + i);
						printf("%02x ", Mac[i]);
					}
					printf("\n");
				}
			}
		}
		Sleep(10);
	}
	return 0;
}