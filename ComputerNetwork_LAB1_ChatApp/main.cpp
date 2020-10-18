#include<iostream>
#include<stdio.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#define SPORT 8086
#define SERVERIP "192.168.0.102"
int main()	//Client��
{
	//��ʹ��ǰ��ʼ����Э��ʹ�ð汾
	WORD wVersionRequest = MAKEWORD(2, 2);
	WSADATA wsadata;
	WSAStartup(wVersionRequest, &wsadata);

	//�ͻ��˳�ʼ��socket
	SOCKET socketClient = socket(AF_INET, SOCK_STREAM, 0);

	//��������ַ��Ϣ
	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;					//IPv4
	addrSrv.sin_port = htons(SPORT);				//�������ļ����˿�
	//addrSrv.sin_addr.s_addr = inet_addr(SERVERIP);	//inet_addr()���ַ�����ʽ��ip��ַת��Ϊ�����ֽ���
	//inet_pton(AF_INET, SERVERIP, &addrSrv.sin_addr);	//���ı��������°汾����inet_pton()
	InetPton(AF_INET, SERVERIP, &addrSrv.sin_addr);

	char IPv4[16] = { 0 };
	//connect�������������������
	if (connect(socketClient, (SOCKADDR*)&addrSrv, sizeof(addrSrv)) != 0) //ע��connect��3������namelen��ָ����ڶ���ָ����ָsockaddr_in��С
	{
		inet_ntop(AF_INET, &addrSrv.sin_addr.s_addr, IPv4, SPORT);
		printf("Failed to connect to %s:%d\n", IPv4, SPORT);
		printf("Error code %d\n", WSAGetLastError());
		system("pause");
		return 0;
	}
	inet_ntop(AF_INET, &addrSrv.sin_addr.s_addr, IPv4, SPORT);
	printf("Successfully connect to %s:%d\n", IPv4, SPORT);

	int sendlen = 100, recvlen = 100;
	char sendBuf[1024] = { 0 };
	char recvBuf[1024] = { 0 };
	int iResult = 0;
	while (true)
	{
		printf("Sending message to server: ");
		scanf_s("%s", &sendBuf,1024);
		//strcpy_s(sendBuf, 1024, "Testing connection, send from client to server.");

		send(socketClient, sendBuf, sendlen, 0);
		printf("Message sent.\n");

		printf("Receiving...\n");
		iResult = recv(socketClient, recvBuf, recvlen, 0);
		
		if (iResult == 0)
		{
			printf("Connection closed\n");
			break;
		}
		else if (iResult > 0)
			printf("Receiving %d bytes from server.\n", iResult);
		else
		{
			printf("A failure for recv() in receiving message from server.\n");
			printf("Error code %d\n", WSAGetLastError());
			break;
		}
		printf("Receiving message from server: ");
		printf("%s\n", recvBuf);
		strcpy_s(recvBuf, 1024, "");
	}

	closesocket(socketClient);	//���ӽ����ر�socket
	WSACleanup();				//�ͷ�Socket dll��Դ
	system("pause");
	return 0;
}