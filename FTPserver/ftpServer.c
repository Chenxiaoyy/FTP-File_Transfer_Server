#include <stdio.h>

#include "ftpServer.h"

char g_recvBuf[1024]; // 用来接受客户端的信息
char *g_fileBuf;      // 存储文件内容
int g_fileSize; //文件大小

int main() {

	initSocket();

	listToClient();

	closeSocket();
	return 0;
}

// 初始化socket库
bool initSocket() {
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		printf("WSAStartup faild:%d\n", WSAGetLastError());
		return false;
	}
	return true;
}
// 关闭socket库
bool closeSocket() {
	if (0 != WSACleanup()) {
		printf("WSACleanup faild:%d\n", WSAGetLastError());
		return false;
	}
	return true;
}

// 监听客户端连接
void listToClient() {
	//创建socket套接字 地址，端口号
	// INVALID_SOCKET 0      SOCKET_ERROR -1
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == serfd) {
		printf("socket faild:%d\n", WSAGetLastError());
		return ;
	}
	//给socket绑定IP地址和端口号
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;  //必须和创建socket指定的一样
	serAddr.sin_port = htons(SPORT); //htons把本地字节序转为网络字节序
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY; // 监听本机所有网卡
	int bind_flag = bind(serfd,(struct sockaddr*)&serAddr, sizeof(serAddr));
	if (0 != bind_flag) {
		printf("bind faild:%d\n", WSAGetLastError());
		return ;
	}
	// 监听客户端链接
	if (0 != listen(serfd, 10)) {
		printf("listen faild:%d\n", WSAGetLastError());
		return;
	}
	//有客户端链接，那么就要接收链接了
	struct sockaddr_in cliAddr;
	int len = sizeof(cliAddr);
	SOCKET clifd = accept(serfd, (struct sockaddr*) &cliAddr, &len);
	if (INVALID_SOCKET == clifd) {
		printf("accept faild:%d\n", WSAGetLastError());
		return;
	}
	printf("new client connect success...\n");
	//开始处理消息
	while (processMsg(clifd)) {
		Sleep(100);
	}

}

// 处理消息
bool processMsg(SOCKET clifd) {
	// 成功接受消息，返回接受到的字节数
	int nRes = recv(clifd, g_recvBuf, 1024, 0);
	if (nRes <= 0) {
		printf("客户端下线...%d\n", WSAGetLastError());
		return;
	}
	//获取接收到的消息
	struct MsgHeader *msg = ( struct MsgHeader*)g_recvBuf;
	struct MsgHeader exitmsg;
	switch (msg->msgID)
	{
		case MSG_FILENAME:
			printf("%s\n", msg->fileInfo.fileName);
			readFile(clifd, msg);
			break;
		case MSG_SENDFILE:
			sendFile(clifd, msg);
			break;
		case MSG_SUCCESSED:
			exitmsg.msgID = MSG_SUCCESSED;
			if (SOCKET_ERROR == send(clifd, (char *)&exitmsg, sizeof(struct MsgHeader), 0))
			{
				printf("readFile send faild:%d", WSAGetLastError());
			}
			printf("再见\n");
			closesocket(clifd);
			return false;
			break;
		default:
			break;
	}
	return true;
}
/*
1，客户端请求下载文件 -》 文件名发送给服务器
2，服务器接受客户端发送的文件名 -》 根据文件名，找到文件；把文件大小发送给客户端
3，客户端接收到文件大小-》准备开始接收，开辟内存     准备告诉服务器，可以开始发送
4，服务器接受到开始发送的指令-》开始发送数据
5，开始接受数据，存起来 -》 接受完成，告诉服务器接收完成
6，关闭连接。。。

*/

bool readFile(SOCKET clifd, struct MsgHeader* pMsg)
{
	FILE *pread = fopen(pMsg->fileInfo.fileName, "rb");
	if (pread == NULL) {
		printf("找不到[%s]文件...\n", pMsg->fileInfo.fileName); 
		struct MsgHeader msg;
		msg.msgID = MSG_OPENFILE_FAILD;
		if (SOCKET_ERROR == send(clifd, (char *)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("readFile send faild:%d", WSAGetLastError());
		}
		return false;
	}
	//获取文件大小
	fseek(pread, 0, SEEK_END);
	g_fileSize = ftell(pread);
	fseek(pread, 0, SEEK_SET);
	//把文件大小发送给客户端  
	struct MsgHeader msg;
	msg.msgID = MSG_FILESIZE;
	msg.fileInfo.fileSize = g_fileSize;

    //E:\\考研\\pdf文件\\专业课\\程序是怎样跑起来的.pdf   
    //E:\考研\pdf文件\专业课\程序是怎样跑起来的.pdf

	//E:\考研\pdf文件\专业课\1.txt
	char tfname[200] = { 0 }, text[100];
	_splitpath(pMsg->fileInfo.fileName,NULL,NULL,tfname,text);

	strcat(tfname, text);
	strcpy(msg.fileInfo.fileName, tfname);
	send(clifd, (char *)&msg, sizeof(struct MsgHeader), 0);

	//读取文件内容
	g_fileBuf = calloc(g_fileSize + 1, sizeof(char));
	if (g_fileBuf == NULL) {
		printf("内存不足，请重试\n");
		return false;
	} 
	fread(g_fileBuf, sizeof(char), g_fileSize, pread);
	g_fileBuf[g_fileSize] = '\0';
	fclose(pread);
	return true;
};

bool sendFile(SOCKET clifd, struct MsgHeader* pmsg) {
	struct MsgHeader msg;          //告诉客户端准备发送文件
	msg.msgID = MSG_RAEDY_READ;
	//文件长度大于每个数据包的大小
	for (size_t i = 0; i < g_fileSize; i += PACKET_SIZE) 
	{
		msg.packet.nStart = i;
		//包的大小大于文件总数据的大小
		if (i + PACKET_SIZE + 1 > g_fileSize) {
			msg.packet.nsize = g_fileSize - i;
		}
		else {
			msg.packet.nsize = PACKET_SIZE;
		}

		memcpy(msg.packet.buf, g_fileBuf + msg.packet.nStart, msg.packet.nsize);
		if (SOCKET_ERROR == send(clifd, (char*) &msg, sizeof(struct MsgHeader), 0)) {
			printf("发送失败:%d\n", WSAGetLastError());
		}
	}
	return true;
}
