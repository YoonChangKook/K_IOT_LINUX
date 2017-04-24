#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <gigaiotplatform.h>

#define THIS_IP				"127.0.0.1"
#define THIS_PORT			5001		// from outside
#define OUT_PORT			5002		// to main process

// 테스트 스트림
#define TagID_CNVY_LIGHT0	"ca"
#define TagID_COLEC_LIGHT0	"va"
#define TagID_CNVY_LIGHT1	"cb"
#define TagID_COLEC_LIGHT1	"vb"

// 값
#define BUFFER_SIZE			128
#define LIGHT_ON_VALUE		1
#define LIGHT_OFF_VALUE		0
#define ERROR_VALUE			0.1

// UDP
struct sockaddr_in	send_addr;
SOCKET				sockfd_send;
struct sockaddr_in	recv_addr;
SOCKET				sockfd_recv;
pthread_t			udp_thread;
// TCP
struct sockaddr_in	server_addr, client_addr;
SOCKET				server_fd, client_fd;
char				tcp_buf[10];
pthread_t			tcp_thread;
#ifndef MSG_WAITALL
#define MSG_WAITALL			0x8
#endif

#pragma pack(push, 1)
typedef struct IOT_PACKET{
	unsigned char device_id;
	unsigned char control;
}IOT_PACKET, *LPIOT_PACKET;
#pragma pack(pop)

void* tcp_receiver(void* arg);
void* udp_receiver(void* arg);
void InitReceive(int port);
void InitSend(int port);
void gigaiotplatform_eventcb(GIGAIOTPLATFORM_EVENT events);
void gigaiotplatform_recvcb_DevCommChAthnRespVO(const DevCommChAthnRespVO* devCommChAthnRespVO);
static void ControlDevice(const ItgCnvyDataVO* itgCnvyDataVO);
void gigaiotplatform_recvcb_ItgCnvyDataVO(const ItgCnvyDataVO * itgCnvyDataVO);

void* tcp_receiver(void* arg)
{
	LPIOT_PACKET lp_packet;
	int result;
	int len;

	InitReceive(THIS_PORT);

	printf("Start Receive\n");
	while (1)
	{
		if ((result = recv(client_fd, tcp_buf, sizeof(IOT_PACKET), MSG_WAITALL)) == 0)
		{
			printf("Close Socket\n");
			close(client_fd);

			printf("Wait client...\n");
			len = sizeof(client_addr);
			client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
			if (client_fd < 0)
			{
				printf("TCP accept failed\n");
				return;
			}
			printf("Client Accepted\n");
		}
		else if (result == SOCKET_ERROR)
		{
			printf("Receive Error\n");
			close(client_fd);

			printf("Wait client...\n");
			len = sizeof(client_addr);
			client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
			if (client_fd < 0)
			{
				printf("TCP accept failed\n");
				return;
			}
			printf("Client Accepted\n");
		}

		lp_packet = (LPIOT_PACKET)tcp_buf;

		printf("Received { %d, %d }\n", lp_packet->device_id, lp_packet->control);

		if (lp_packet->device_id == 0)
		{
			GIGAIOTPLATFORM_RsltCd rsltCd = GIGAIOTPLATFORM_RsltCd_SUCCESS;
			rsltCd = gigaiotplatform_send_msg_ItgColecDataVO_Num(TagID_COLEC_LIGHT0, (double)lp_packet->control);
			if (rsltCd != GIGAIOTPLATFORM_RsltCd_SUCCESS) {
				printf("Itg ERROR!\n");
			}
		}
		else if (lp_packet->device_id == 1)
		{
			GIGAIOTPLATFORM_RsltCd rsltCd = GIGAIOTPLATFORM_RsltCd_SUCCESS;
			rsltCd = gigaiotplatform_send_msg_ItgColecDataVO_Num(TagID_COLEC_LIGHT1, (double)lp_packet->control);
			if (rsltCd != GIGAIOTPLATFORM_RsltCd_SUCCESS) {
				printf("Itg ERROR!\n");
			}
		}
	}
}

void* udp_receiver(void* arg)
{
	struct sockaddr_in pcaddr;
	int addrlen = sizeof(pcaddr);
	IOT_PACKET packet;
	int recvlen;

	while (1)
	{
		if (sockfd_recv == INVALID_SOCKET)
		{
			printf("Receive UDP socket error\n");
			return;
		}

		memset(&packet, 0, sizeof(packet));
		if ((recvlen = recvfrom(sockfd_recv, (char*)&packet, sizeof(packet), 0, (struct sockaddr_in*)&pcaddr, &addrlen)) == SOCKET_ERROR)
		{
			printf("Receive Error\n");
			continue;
		}

		printf("Received { %d, %d }\n", packet.device_id, packet.control);

		if (packet.device_id == 0)
		{
			GIGAIOTPLATFORM_RsltCd rsltCd = GIGAIOTPLATFORM_RsltCd_SUCCESS;
			rsltCd = gigaiotplatform_send_msg_ItgColecDataVO_Num(TagID_COLEC_LIGHT0, (double)packet.control);
			if (rsltCd != GIGAIOTPLATFORM_RsltCd_SUCCESS) {
				printf("Itg ERROR!\n");
			}
		}
		else if (packet.device_id == 1)
		{
			GIGAIOTPLATFORM_RsltCd rsltCd = GIGAIOTPLATFORM_RsltCd_SUCCESS;
			rsltCd = gigaiotplatform_send_msg_ItgColecDataVO_Num(TagID_COLEC_LIGHT1, (double)packet.control);
			if (rsltCd != GIGAIOTPLATFORM_RsltCd_SUCCESS) {
				printf("Itg ERROR!\n");
			}
		}
	}
}

void InitReceive(int port)
{
	// Make UDP Socket
	if ((sockfd_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		printf("socket error\n");
		return;
	}

	// Set addr
	recv_addr.sin_family = AF_INET;
	recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	recv_addr.sin_port = htons(port);

	// No need to bind when sending
	if (bind(sockfd_recv, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) == SOCKET_ERROR)
	{
		printf("bind error\n");
		return;
	}
}

void InitSend(int port)
{
	// Make UDP Socket
	if ((sockfd_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		printf("socket error\n");
		return;
	}

	// Set addr
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = inet_addr(THIS_IP);
	send_addr.sin_port = htons(port);
}

void gigaiotplatform_eventcb(GIGAIOTPLATFORM_EVENT events) {

	GIGAIOTPLATFORM_RsltCd rsltCd = GIGAIOTPLATFORM_RsltCd_SUCCESS;

	switch (events) {

	case GIGAIOTPLATFORM_EVENT_CONNECTED:
		// [TXTR-IF-224] 장비TCP채널인증요청
		// Connection이 성공적으로 연결되었을 시, 장비TCP채널인증요청(DevCommChAthnRqtVO) 전송
		rsltCd = gigaiotplatform_send_msg_DevCommChAthnRqtVO();
		if (rsltCd != GIGAIOTPLATFORM_RsltCd_SUCCESS) {
			printf("Dev ERROR!\n");
			return;
		}

		printf("gigaiotplatform_eventcb() BEV_EVENT_CONNECTED = %d \n", rsltCd);		
		break;

	case GIGAIOTPLATFORM_EVENT_TIMEOUT:
		puts("gigaiotplatform_eventcb() BEV_EVENT_TIMEOUT");

		break;

	case GIGAIOTPLATFORM_EVENT_ERROR:
		puts("gigaiotplatform_eventcb() GIGAIOTPLATFORM_EVENT_ERROR");

		break;

	default:
		printf("gigaiotplatform_eventcb() UNKNWON(%d)\n", events);

		break;
	}
	return;

}

static char *pDeviceId=NULL;

void gigaiotplatform_recvcb_DevCommChAthnRespVO(const DevCommChAthnRespVO* devCommChAthnRespVO) 
{
	printf("==> athnRqtNo = %s \n", devCommChAthnRespVO->athnRqtNo); //외부시스템아이디
	printf("==> athnNo = %s \n", devCommChAthnRespVO->athnNo); //인증번호
	printf("==> respMsg = %s \n", devCommChAthnRespVO->respMsg); //응답메시지

	pDeviceId = gigaiotplatform_getDevId();
	
	printf("[TEST] pDeviceId: %s\n", pDeviceId);
	
	if (strcmp(devCommChAthnRespVO->respCd, "100") == 0) {
		gigaiotplatform_start_timer(1);

		GIGAIOTPLATFORM_RsltCd rsltCd = GIGAIOTPLATFORM_RsltCd_SUCCESS;
		rsltCd = gigaiotplatform_send_msg_ItgColecDataVO_Num(TagID_COLEC_LIGHT0, (double)0);
		if (rsltCd != GIGAIOTPLATFORM_RsltCd_SUCCESS) {
			printf("Itg ERROR!\n");
			return;
		}

		rsltCd = GIGAIOTPLATFORM_RsltCd_SUCCESS;
		rsltCd = gigaiotplatform_send_msg_ItgColecDataVO_Num(TagID_COLEC_LIGHT1, (double)1);
		if (rsltCd != GIGAIOTPLATFORM_RsltCd_SUCCESS) {
			printf("Itg ERROR!\n");
			return;
		}

		// start thread
		//pthread_create(&tcp_thread, NULL, &tcp_receiver, (void *)NULL);
		pthread_create(&udp_thread, NULL, &udp_receiver, (void *)NULL);
	}

	return;
}

static void ControlDevice(const ItgCnvyDataVO* itgCnvyDataVO) {
	DevCnvyDataVO* devCnvyDataVO;
	IOT_PACKET packet;
	int devIdx = 0;

	for (devIdx = 0; devIdx < itgCnvyDataVO->devCnvyDataVOsCnt; devIdx++) {
		devCnvyDataVO = itgCnvyDataVO->devCnvyDataVOs[devIdx];

		// Check Device
		if (strcmp(devCnvyDataVO->devId, pDeviceId) != 0)
		{
			continue;
		}

		CnvyRowVO* cnvyRowVO;
		int cnvyIdx = 0;
		for (cnvyIdx = 0; cnvyIdx < devCnvyDataVO->cnvyRowVOsCnt; cnvyIdx++) {
			cnvyRowVO = devCnvyDataVO->cnvyRowVOs[cnvyIdx];
			int snsnIdx = 0;
			for (snsnIdx = 0; snsnIdx < cnvyRowVO->snsnDataInfoVOsCnt; snsnIdx++) {
				SnsnDataInfoVO* snsnData = cnvyRowVO->snsnDataInfoVOs[snsnIdx];

				// Light Control
				if (strcmp(snsnData->dataTypeCd, TagID_CNVY_LIGHT0) == 0)
				{
					packet.device_id = 0;
					if (LIGHT_ON_VALUE - ERROR_VALUE < snsnData->snsnVal &&
						snsnData->snsnVal < LIGHT_ON_VALUE + ERROR_VALUE)
					{
						packet.control = 1;
						sendto(sockfd_send, (const char *)&packet, sizeof(packet), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
						printf("LIGHT_ON\n");
					}
					else if (LIGHT_OFF_VALUE - ERROR_VALUE < snsnData->snsnVal &&
						snsnData->snsnVal < LIGHT_OFF_VALUE + ERROR_VALUE)
					{
						packet.control = 0;
						sendto(sockfd_send, (const char *)&packet, sizeof(packet), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
						printf("LIGHT_OFF\n");
					}
				}
				else if (strcmp(snsnData->dataTypeCd, TagID_CNVY_LIGHT1) == 0)
				{
					packet.device_id = 1;
					if (LIGHT_ON_VALUE - ERROR_VALUE < snsnData->snsnVal &&
						snsnData->snsnVal < LIGHT_ON_VALUE + ERROR_VALUE)
					{
						packet.control = 1;
						sendto(sockfd_send, (const char *)&packet, sizeof(packet), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
						printf("LIGHT_ON\n");
					}
					else if (LIGHT_OFF_VALUE - ERROR_VALUE < snsnData->snsnVal &&
						snsnData->snsnVal < LIGHT_OFF_VALUE + ERROR_VALUE)
					{
						packet.control = 0;
						sendto(sockfd_send, (const char *)&packet, sizeof(packet), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
						printf("LIGHT_OFF\n");
					}
				}
			}
		}
	}

	return;
}


void gigaiotplatform_recvcb_ItgCnvyDataVO(const ItgCnvyDataVO * itgCnvyDataVO)
{
	unsigned char encodeBuf[100];

	ControlDevice(itgCnvyDataVO);
}

int main() {
	// 1. gigaiotplatform library 초기화
	gigaiotplatform_init();

	InitSend(OUT_PORT);
	InitReceive(THIS_PORT);

	// 2. callback function 등록
	gigaiotplatform_setcb_event(gigaiotplatform_eventcb);	// connection event

	gigaiotplatform_setcb_DevCommChAthnRespVO(gigaiotplatform_recvcb_DevCommChAthnRespVO);
	gigaiotplatform_setcb_ItgCnvyDataVO(gigaiotplatform_recvcb_ItgCnvyDataVO);

	// 3. 3MP서버와 connection
	GIGAIOTPLATFORM_RsltCd rsltCd = gigaiotplatform_connect();
	if (rsltCd != GIGAIOTPLATFORM_RsltCd_SUCCESS) {
		puts("gigaiotplatform_connect() fail");
	}

	// 4. gigaiotplatform library 해제
	gigaiotplatform_free();

	return 0;
}



