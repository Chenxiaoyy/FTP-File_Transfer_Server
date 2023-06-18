#include <stdio.h>

#include "ftpClient.h"

char g_recvBuf[1024];  //接受消息的缓冲区
char *g_fileBuf;      // 存储文件内容
int g_fileSize;       //文件大小
char g_fileName[256];      //保存服务器发过来的文件名

int main() {
	initSocket();
	
	connectToHost();

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
	printf("initSocket...\n");
	return true;
}
// 关闭socket库
bool closeSocket() {
	if (0 != WSACleanup()) {
		printf("WSACleanup faild:%d\n", WSAGetLastError());
		return false;
	}
	printf("closeSocket...\n");
	return true;
}

// 监听客户端连接
void connectToHost() {
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
	serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 服务器IP地址
	//连接到服务器
	if (0 != connect(serfd, (struct sockaddr*)& serAddr, sizeof(serAddr))) {
		printf("connect faild:%d\n", WSAGetLastError());
		return;
	}
	printf("connect...\n");	
	downloadFileName(serfd);
	while (processMsg(serfd)) {
		//Sleep(100);
	}
}

// 处理消息
bool processMsg(SOCKET serfd) {
	recv(serfd, g_recvBuf, 1024, 0);
	struct MsgHeader* msg = (struct MsgHeader*) g_recvBuf;

	switch (msg->msgID)
	{
		case MSG_OPENFILE_FAILD:
			downloadFileName(serfd);
		case MSG_FILESIZE:
			readyread(serfd, msg);
			break;
		case MSG_RAEDY_READ:
			writeFile(serfd, msg);
			break;
		case MSG_SUCCESSED:
			printf("再见\n");
			closesocket(serfd);
			return false;
			break; 
		default:
			break;
	}

	return true;
}

void downloadFileName(SOCKET serfd) {
	printf("请输入文件地址:");
	char fileName[1024] = "11111111111111";
	gets_s(fileName, 1024);  // 获取要下载的文件
	struct MsgHeader file;
	file.msgID = MSG_FILENAME;
	strcpy(file.fileInfo.fileName, fileName);

	send(serfd, (char *) &file, sizeof(struct MsgHeader), 0);
}

void readyread(SOCKET serfd, struct MsgHeader* pmsg)
{
	// 准备内存 pmsg->fileInfo.fileSize
	strcpy(g_fileName, pmsg->fileInfo.fileName);
	g_fileSize = pmsg->fileInfo.fileSize;
	g_fileBuf = calloc(g_fileSize + 1, sizeof(char));  //申请空间
	if (g_fileBuf == NULL) {
		printf("内存不足\n");
	}
	else 
	{
		struct MsgHeader msg;
		msg.msgID = MSG_SENDFILE;
		if (SOCKET_ERROR == send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0)) {
			printf("send error: %d", WSAGetLastError());
			return;
		}
	}
	printf("size:%d    filename:%s \n", pmsg->fileInfo.fileSize, pmsg->fileInfo.fileName);

}

bool writeFile(SOCKET serfd, struct MsgHeader* pmsg) 
{
	if (g_fileBuf == NULL) {
		return false;
	}
	int nStart = pmsg->packet.nStart;
	//判断文件是否发送完整，服务器是否发送完了数据
	int nSize = pmsg->packet.nsize;
	memcpy(g_fileBuf + nStart, pmsg->packet.buf, nSize);
	printf("packet:%d  %d\n", nStart + nSize, g_fileSize);
	if (nStart + nSize >= g_fileSize) {
		FILE *pwrite = fopen(g_fileName,"wb");
		if (pwrite == NULL) {
			printf("write file error..\n");
			return false;
		}
		fwrite(g_fileBuf, sizeof(char), g_fileSize, pwrite);
		fclose(pwrite);
		free(g_fileBuf);
		g_fileBuf = NULL;
		struct MsgHeader msg;
		msg.msgID = MSG_SUCCESSED;
		send(serfd, (char*)&msg, sizeof(struct MsgHeader),0);
		return false;
	}
	return true;
}
