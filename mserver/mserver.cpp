// OneServerMain.cpp  
#include <Winsock2.h>    
#include <windows.h>
#include <mstcpip.h>
#include <map>
#pragma comment(lib,"WS2_32")
#pragma warning(disable:4996)
#pragma warning (disable : 4005)
using namespace std;
map<SOCKET, FILE*> socks;
int main()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)return -1;
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(4444);

	if (SOCKET_ERROR == bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)))return -1;

	listen(sockSrv, 1024);
	printf("服务器就绪\n");

	void acceptsock(LPVOID sockSrv);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)acceptsock, (LPVOID)sockSrv, 0, NULL);
	SOCKET sockcurrent = NULL;
	while (true)
	{
		char buf[1024] = { 0 };
		gets_s(buf);
		if (!strcmp(buf, "select"))
		{
			printf("请输入选择连接编号\n");
			scanf("%u", &sockcurrent);
			getchar();
		}
		else if (!strcmp(buf, "show"))
		{
			printf("当前连接数 %d\n", socks.size());
			for (map<SOCKET, FILE*>::iterator it = socks.begin(); it != socks.end(); ++it)
			{
				printf("sokcet值 %u\n", it->first);
			}
			printf("当前socket %u\n", sockcurrent);
		}
		else if (!strcmp(buf, "openlocal"))
		{
			if (socks.find(sockcurrent) == socks.end())
			{
				printf("当前选择SOCKET不存在\n");
			}
			else if (socks[sockcurrent] != NULL)
			{
				printf("请先关闭文件描述符\n");
			}
			else
			{
				char localpath[1024];
				printf("请输入重定向文件地址\n");
				gets_s(localpath);
				if ((socks[sockcurrent] = fopen(localpath, "wb")) != NULL)
				{
					printf("文件打开成功\n");
				}
				else
				{
					printf("文件打开失败\n");
				}
			}
		}
		else if (!strcmp(buf, "closelocal"))
		{
			if (socks.find(sockcurrent) == socks.end())
			{
				printf("当前选择SOCKET不存在\n");
			}
			else if (socks[sockcurrent] != NULL)
			{
				fclose(socks[sockcurrent]);
				socks[sockcurrent] = NULL;
				printf("文件关闭成功\n");
			}
			else
			{
				printf("文件已关闭\n");
			}
		}
		else if (!strcmp(buf, "uplocal"))
		{
			if (socks.find(sockcurrent) == socks.end())
			{
				printf("当前选择SOCKET不存在\n");
			}
			else
			{
				char localpath[1024];
				printf("请输入传输文件地址\n");
				gets_s(localpath);
				FILE* fdw;
				if ((fdw = fopen(localpath, "rb")) != NULL)
				{
					printf("文件打开成功\n");
					printf("开始传输\n");
					while (feof(fdw) == 0)
					{
						int len = fread(buf, 1, 1024, fdw);
						int i = 0;
						while (!send(sockcurrent, buf, len, 0) && i < 16)i++;
					}
					printf("传输结束\n");
					fclose(fdw);
				}
				else
				{
					printf("文件打开失败\n");
				}
			}
		}
		else
		{
			if (socks.find(sockcurrent) == socks.end())
			{
				printf("当前选择SOCKET不存在\n");
			}
			else
			{
				strcat(buf, "\n");
				send(sockcurrent, buf, strlen(buf), 0);
			}
		}

	}
	return 0;
}
void acceptsock(LPVOID sockSrv)
{
	while (true) {
		SOCKET sockconn = accept((SOCKET)sockSrv, NULL, NULL);
		send(sockconn, "ifconfig\n", 9, 0);
		void receivemessage(LPVOID IpParameter);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receivemessage, (LPVOID)sockconn, 0, NULL);
	}
	WSACleanup();
}

void receivemessage(LPVOID IpParameter)
{
	SOCKET sockconn = (SOCKET)IpParameter;
	int keepAlive = 1; // 开启keepalive属性
	setsockopt(sockconn, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));
	struct tcp_keepalive in_keep_alive = { 0 };
	unsigned long ul_in_len = sizeof(struct tcp_keepalive);
	struct tcp_keepalive out_keep_alive = { 0 };
	unsigned long ul_out_len = sizeof(struct tcp_keepalive);
	unsigned long ul_bytes_return = 0;
	in_keep_alive.onoff = 1;                    /*打开keepalive*/
	in_keep_alive.keepaliveinterval = 5000; /*发送keepalive心跳时间间隔-单位为毫秒*/
	in_keep_alive.keepalivetime = 1000;         /*多长时间没有报文开始发送keepalive心跳包-单位为毫秒*/
	WSAIoctl(sockconn, SIO_KEEPALIVE_VALS, (LPVOID)&in_keep_alive, ul_in_len,
		(LPVOID)&out_keep_alive, ul_out_len, &ul_bytes_return, NULL, NULL);
	socks[sockconn] = NULL;
	char recvBuf[1025];
	int len;
	long long filesize = 0;
	long long filecount = 0;
	while ((len = recv(sockconn, recvBuf, 1024, 0)) > 0) {
		recvBuf[len] = 0;
		if (socks[sockconn] != NULL)
		{
			filesize = filesize + len;
			filecount++;
			fwrite(recvBuf, 1, len, socks[sockconn]);
			if (filecount % 100 == 0)
			{
				printf("now recv file %lld kb\n", filesize / 1000);
			}
		}
		else
		{
			printf("%s", recvBuf);
		}
	}
	closesocket(sockconn);
	socks.erase(sockconn);
}