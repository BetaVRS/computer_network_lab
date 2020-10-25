#include<iostream>
#include<fstream>
#include<stdio.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#include"ClientStruct.h"
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#define SPORT 8086
#define SERVERIP "127.0.0.1"
#define BUFFSIZE 4096
#define NAMELEN 64

HANDLE mutex = CreateMutex(NULL, FALSE, NULL);	//利用互斥量实现线程的同步
HANDLE mutexFileIO = CreateMutex(NULL, FALSE, NULL);
HANDLE mutexSocket = CreateMutex(NULL, FALSE, NULL);
//以下为线程需要同步使用的数据
u_short uid = 0;		//用户uid
char userName[NAMELEN] = { 0 };	//用户名
SOCKET socketClient = NULL;
u_short *onlineUser = new u_short[0];
u_short userNum = 0;
bool userChecked = true;
bool inChat = false;

bool login(SOCKET sock);	//连接到服务器，初始化UID和user name
int sendMessage();
int getMessage(char* buffer);
int checkOnlineList();
int getOnlineList(char* buffer);
int logout();
int getinput(const char *buffer);	//获取键盘输入，方法待定

void show_menu();

DWORD WINAPI threadClient(LPVOID lpPrama);

int main()	//Client端
{
	//库使用前初始化，协议使用版本
	WORD wVersionRequest = MAKEWORD(2, 2);
	WSADATA wsadata;
	WSAStartup(wVersionRequest, &wsadata);

	//客户端初始化socket
	socketClient = socket(AF_INET, SOCK_STREAM, 0);
	//设置socket为非阻塞模式
	u_long is_non_block = 1;
	int ret = ioctlsocket(socketClient, FIONBIO, &is_non_block);
	if (ret != 0)
	{
		printf("Failed to set socket to non block mode.\n");
		closesocket(socketClient);
		WSACleanup();
		return 0;
	}

	//服务器地址信息
	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;					//IPv4
	addrSrv.sin_port = htons(SPORT);				//服务器的监听端口
	//addrSrv.sin_addr.s_addr = inet_addr(SERVERIP);	//inet_addr()将字符串形式的ip地址转换为网络字节序
	//inet_pton(AF_INET, SERVERIP, &addrSrv.sin_addr);	//上文报错，改用新版本函数inet_pton()
	InetPton(AF_INET, SERVERIP, &addrSrv.sin_addr);

	char IPv4[16] = { 0 };
	int retv = -1;
	int wsaErro = 0;
	//connect用于与服务器建立连接
	for (int i = 5; i > 0 && wsaErro != WSAEISCONN; i--)	//尝试5次连接,每次间隔500ms
	{
		retv = connect(socketClient, (SOCKADDR*)&addrSrv, sizeof(addrSrv));
		wsaErro = WSAGetLastError();
		if (wsaErro != WSAEISCONN)
		{
			inet_ntop(AF_INET, &addrSrv.sin_addr.s_addr, IPv4, SPORT);
			printf("Failed to connect to %s:%d\n", IPv4, SPORT);
			printf("Error code %d\n", WSAGetLastError());
			Sleep(500);
		}
	}
	if (wsaErro != WSAEISCONN)
	{
		printf("Server no response! Closing client.\n");
		closesocket(socketClient);
		WSACleanup();
		system("pause");
		return 0;
	}
	inet_ntop(AF_INET, &addrSrv.sin_addr.s_addr, IPv4, SPORT);
	printf("Successfully connect to %s:%d\n", IPv4, SPORT);

	//unsigned short uid;
	//char userName[NAMELEN];
	bool loged= login(socketClient);
	if (!loged)
	{
		printf("Failed to login to server, Closing client.\n");
		closesocket(socketClient);
		WSACleanup();
		system("pause");
		return 0;
	}
	printf("Logged in as %u:%s\n", uid, userName);
	printf("Press enter to continue: ");
	get_cmd();

	//创建线程来处理socket通信，程序逻辑在主线程处理
	HANDLE hThread = CreateThread(NULL, 0, threadClient, (LPVOID)(&loged), 0, NULL);
	CloseHandle(hThread);
	char cmd;
	while (loged)
	{
		show_menu();
		printf("Command: ");
		cmd = get_cmd();
		switch (cmd)
		{
		case '0': checkOnlineList();break;
		case '1': sendMessage();break;
		case '2': break;
		case 'q': logout(); loged = false; break;
		default:  break;
		}
	}
	printf("Closing client...\n");
	closesocket(socketClient);	//链接结束关闭socket
	WSACleanup();				//释放Socket dll资源
	system("pause");
	return 0;
}

bool login(SOCKET sock)
{
	char buffer[128] = { 0 };
	char sendBuf[3] = { 0 };
	char result[16] = { 0 };
	printf("Enter the username: ");
	scanf_s("%s", buffer, NAMELEN);
	printf("Are you sure to use name: \"%s\"? (Y/N)\n", buffer);
	if (!get_YN())
		return false;
	strcpy_s(userName, NAMELEN, buffer);

	BYTE func = FUNCODE::Login;	//用户登陆func码为0
	uid = 0;		//初始用户名为0，等待Server分配
	//指针类型强制转换，将数据写入字符串中发送
	sendBuf[2] = func;
	//strncat_s(buffer, 1024, (char*)(&uid), sizeof(unsigned short));
	//strncat_s(buffer, 1024, (char*)(&func), sizeof(BYTE));
	//strcpy_s(buffer+3, 125, userName);
	send(sock, sendBuf, 3, 0);
	int ret = 0;
	int wsaErro = WSAEWOULDBLOCK;
	for (int i = 5; wsaErro==WSAEWOULDBLOCK && i > 0; i--)	//等待登陆响应500ms
	{
		printf("Waiting for login accept..\n");
		ret = recv(sock, result, 16, 0);
		wsaErro = WSAGetLastError();
		printf("Error code %d\nrecv() return value %d\n", wsaErro, ret);
		Sleep(500);
	}
	if (wsaErro != 0 || ret == 0)
		return false;
	func = result[2];
	if (func != FUNCODE::LoginSuccess)	//潜在的bug，如果登陆的同时有client发送消息
		return false;
	char temp[2] = { result[0],result[1] };
	uid = *((unsigned short*)temp);
	return true;
}

int sendMessage()
{	
	system("cls");
	printf("Entering chat room.\n");
	char file[20] = { 0 };
	char head[20] = { 0 };
	char msg[BUFFSIZE] = { 0 };
	sprintf_s(file, 20, "chat%u.log", uid);	

	WaitForSingleObject(mutexFileIO, INFINITE);
	ifstream fin(file);
	if(fin.is_open())
		while (!fin.eof())
		{
			fin.getline(head, BUFFSIZE);
			fin.getline(msg, BUFFSIZE);
			printf("%s\n%s\n\n", head, msg);
		}
	ReleaseMutex(mutexFileIO);

	u_short tid;
	char cid[6];
	printf("Press esc to quit.\n");
	printf("To whom: ");
	if (!get_line(cid, 6))
		return 0;
	tid = atoi(cid);
	if (tid == 0)
	{
		printf("Invalid userID!\n");
		return 0;
	}
	printf("Sending message to %u:\n", tid);
	WaitForSingleObject(mutex, INFINITE);
	inChat = true;
	ReleaseMutex(mutex);
	BYTE func = FUNCODE::Message;
	char sendBuf[BUFFSIZE] = { 0 };
	char buffer[BUFFSIZE] = { 0 };
	while (get_line(buffer, BUFFSIZE))
	{
		sendBuf[0] = ((char*)&uid)[0];
		sendBuf[1] = ((char*)&uid)[1];
		sendBuf[2] = func;
		sendBuf[3] = ((char*)&tid)[0];
		sendBuf[4] = ((char*)&tid)[1];
		//strncat_s(sendBuf, BUFFSIZE, (char*)(&uid), sizeof(u_short));
		//strncat_s(sendBuf, BUFFSIZE, (char*)(&func), sizeof(BYTE));
		//strncat_s(sendBuf, BUFFSIZE, (char*)(&tid), sizeof(u_short));
		strcpy_s(sendBuf + 5, BUFFSIZE - 5, buffer);
		WaitForSingleObject(mutex,INFINITE);
		send(socketClient, sendBuf, strlen(sendBuf + 5) + 6, 0);
		ReleaseMutex(mutex);
	}
	WaitForSingleObject(mutex, INFINITE);
	inChat = false;
	ReleaseMutex(mutex);
	//printf("sendBuf: %s\n", sendBuf);	
	return 0;
}

int getMessage(char* buffer)
{
	char file[20] = { 0 };
	sprintf_s(file, 20, "chat%u.log", uid);

	char temp[2] = { buffer[0],buffer[1] };
	u_short fid = *((u_short*)temp);
	char head[20] , msg[BUFFSIZE];
	strcpy_s(msg, BUFFSIZE, buffer + 5);
	sprintf_s(head, 20, "From %u:", fid);

	WaitForSingleObject(mutexFileIO, INFINITE);
	ofstream fout(file, ios_base::app);
	fout << head << endl << msg << endl;
	ReleaseMutex(mutexFileIO);
	WaitForSingleObject(mutex, INFINITE);
	if (inChat)
	{
		printf("+------------------------\n");
		printf("|%s\n|%s\n", head, msg);
		printf("+------------------------\n");
	}
	ReleaseMutex(mutex);
	return 0;
}

int checkOnlineList()
{
	char sendBuf[3] = { 0 };
	sendBuf[2] = FUNCODE::OnlineList;
	printf("Sending request to server...\n");
	WaitForSingleObject(mutex, INFINITE);
	send(socketClient, sendBuf, 3, 0);
	ReleaseMutex(mutex);
	printf("Waiting for thread to get online users' info...\n");
	for (int i = 5; i > 0; i--)
	{
		WaitForSingleObject(mutex, INFINITE);
		if (userChecked)
		{
			printf("Data not received!\n");
			ReleaseMutex(mutex);
		}
		else
			break;
		printf("Sleep 500.\n");
		Sleep(500);
	}
	if (userChecked)
	{
		printf("No user data received from server!\n");
		return 1;
	}
	printf("There are currently %u users online. They are:\n", userNum);
	for (int i = 0; i < userNum; i++)
		printf("%u\n", onlineUser[i]);
	userChecked = true;
	ReleaseMutex(mutex);
	char buffer[10];
	printf("Press esc to return: ");
	chk_exit(buffer, 10);
	return 0;
}

int getOnlineList(char * buffer)
{
	WaitForSingleObject(mutex, INFINITE);
	printf("Getting online list...\n");
	userChecked = false;
	char temp[2] = { buffer[3],buffer[4] };
	userNum = *((u_short*)temp);
	delete onlineUser;
	onlineUser = new u_short[userNum];
	for (int i = 0; i < userNum; i++)
	{
		temp[0] = buffer[i * 2 + 5];
		temp[1] = buffer[i * 2 + 6];
		onlineUser[i] = *((u_short*)temp);
	}
	printf("Online list updated.\n");
	ReleaseMutex(mutex);
	return 0;
}

int logout()
{
	printf("Logging out!\n");
	char sendBuf[3] = { 0 };
	sendBuf[2] = FUNCODE::Logout;
	WaitForSingleObject(mutex,INFINITE);
	send(socketClient, sendBuf, 3, 0);
	ReleaseMutex(mutex);
	printf("Logout info sent.\n");
	return 0;
}

int getinput(const char * buffer)
{
	scanf_s("%s", buffer, BUFFSIZE);
	return 0;
}

void show_menu()
{
	system("cls");
	printf("Welcom back! %s. Your UID is %u\n", userName, uid);
	printf("Type a single character to excute command or esc to quit:\n");
	printf("0: Check online users.\n");
	printf("1: Enter chat room\n");
	//printf("2: \n");
	printf("q: Logout\n");
}

DWORD __stdcall threadClient(LPVOID lpPrama)
{
	bool *logged = (bool*)lpPrama;
	int ret;
	BYTE func;
	char recvBuf[BUFFSIZE] = { 0 };

	while (*logged)
	{

		WaitForSingleObject(mutex, INFINITE);
		//printf("Thread checking data from recv()...\n");
		ret = recv(socketClient, recvBuf, BUFFSIZE, 0);
		ReleaseMutex(mutex);
		if (ret > 0)
		{
			func = recvBuf[2];
			switch (func)
			{
			case FUNCODE::Message:	  getMessage(recvBuf);		break;
			case FUNCODE::OnlineList: getOnlineList(recvBuf);	break;
			case FUNCODE::Logout: break;
			default: break;
			}
			continue;
		}
		//printf("No more data! sleep for 500.\n");
		Sleep(500);
	}
	return 0;
}
