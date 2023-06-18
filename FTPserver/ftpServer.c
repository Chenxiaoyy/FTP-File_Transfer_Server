#include <stdio.h>

#include "ftpServer.h"

char g_recvBuf[1024]; // �������ܿͻ��˵���Ϣ
char *g_fileBuf;      // �洢�ļ�����
int g_fileSize; //�ļ���С

int main() {

	initSocket();

	listToClient();

	closeSocket();
	return 0;
}

// ��ʼ��socket��
bool initSocket() {
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		printf("WSAStartup faild:%d\n", WSAGetLastError());
		return false;
	}
	return true;
}
// �ر�socket��
bool closeSocket() {
	if (0 != WSACleanup()) {
		printf("WSACleanup faild:%d\n", WSAGetLastError());
		return false;
	}
	return true;
}

// �����ͻ�������
void listToClient() {
	//����socket�׽��� ��ַ���˿ں�
	// INVALID_SOCKET 0      SOCKET_ERROR -1
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == serfd) {
		printf("socket faild:%d\n", WSAGetLastError());
		return ;
	}
	//��socket��IP��ַ�Ͷ˿ں�
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;  //����ʹ���socketָ����һ��
	serAddr.sin_port = htons(SPORT); //htons�ѱ����ֽ���תΪ�����ֽ���
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY; // ����������������
	int bind_flag = bind(serfd,(struct sockaddr*)&serAddr, sizeof(serAddr));
	if (0 != bind_flag) {
		printf("bind faild:%d\n", WSAGetLastError());
		return ;
	}
	// �����ͻ�������
	if (0 != listen(serfd, 10)) {
		printf("listen faild:%d\n", WSAGetLastError());
		return;
	}
	//�пͻ������ӣ���ô��Ҫ����������
	struct sockaddr_in cliAddr;
	int len = sizeof(cliAddr);
	SOCKET clifd = accept(serfd, (struct sockaddr*) &cliAddr, &len);
	if (INVALID_SOCKET == clifd) {
		printf("accept faild:%d\n", WSAGetLastError());
		return;
	}
	printf("new client connect success...\n");
	//��ʼ������Ϣ
	while (processMsg(clifd)) {
		Sleep(100);
	}

}

// ������Ϣ
bool processMsg(SOCKET clifd) {
	// �ɹ�������Ϣ�����ؽ��ܵ����ֽ���
	int nRes = recv(clifd, g_recvBuf, 1024, 0);
	if (nRes <= 0) {
		printf("�ͻ�������...%d\n", WSAGetLastError());
		return;
	}
	//��ȡ���յ�����Ϣ
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
			printf("�ټ�\n");
			closesocket(clifd);
			return false;
			break;
		default:
			break;
	}
	return true;
}
/*
1���ͻ������������ļ� -�� �ļ������͸�������
2�����������ܿͻ��˷��͵��ļ��� -�� �����ļ������ҵ��ļ������ļ���С���͸��ͻ���
3���ͻ��˽��յ��ļ���С-��׼����ʼ���գ������ڴ�     ׼�����߷����������Կ�ʼ����
4�����������ܵ���ʼ���͵�ָ��-����ʼ��������
5����ʼ�������ݣ������� -�� ������ɣ����߷������������
6���ر����ӡ�����

*/

bool readFile(SOCKET clifd, struct MsgHeader* pMsg)
{
	FILE *pread = fopen(pMsg->fileInfo.fileName, "rb");
	if (pread == NULL) {
		printf("�Ҳ���[%s]�ļ�...\n", pMsg->fileInfo.fileName); 
		struct MsgHeader msg;
		msg.msgID = MSG_OPENFILE_FAILD;
		if (SOCKET_ERROR == send(clifd, (char *)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("readFile send faild:%d", WSAGetLastError());
		}
		return false;
	}
	//��ȡ�ļ���С
	fseek(pread, 0, SEEK_END);
	g_fileSize = ftell(pread);
	fseek(pread, 0, SEEK_SET);
	//���ļ���С���͸��ͻ���  
	struct MsgHeader msg;
	msg.msgID = MSG_FILESIZE;
	msg.fileInfo.fileSize = g_fileSize;

    //E:\\����\\pdf�ļ�\\רҵ��\\������������������.pdf   
    //E:\����\pdf�ļ�\רҵ��\������������������.pdf

	//E:\����\pdf�ļ�\רҵ��\1.txt
	char tfname[200] = { 0 }, text[100];
	_splitpath(pMsg->fileInfo.fileName,NULL,NULL,tfname,text);

	strcat(tfname, text);
	strcpy(msg.fileInfo.fileName, tfname);
	send(clifd, (char *)&msg, sizeof(struct MsgHeader), 0);

	//��ȡ�ļ�����
	g_fileBuf = calloc(g_fileSize + 1, sizeof(char));
	if (g_fileBuf == NULL) {
		printf("�ڴ治�㣬������\n");
		return false;
	} 
	fread(g_fileBuf, sizeof(char), g_fileSize, pread);
	g_fileBuf[g_fileSize] = '\0';
	fclose(pread);
	return true;
};

bool sendFile(SOCKET clifd, struct MsgHeader* pmsg) {
	struct MsgHeader msg;          //���߿ͻ���׼�������ļ�
	msg.msgID = MSG_RAEDY_READ;
	//�ļ����ȴ���ÿ�����ݰ��Ĵ�С
	for (size_t i = 0; i < g_fileSize; i += PACKET_SIZE) 
	{
		msg.packet.nStart = i;
		//���Ĵ�С�����ļ������ݵĴ�С
		if (i + PACKET_SIZE + 1 > g_fileSize) {
			msg.packet.nsize = g_fileSize - i;
		}
		else {
			msg.packet.nsize = PACKET_SIZE;
		}

		memcpy(msg.packet.buf, g_fileBuf + msg.packet.nStart, msg.packet.nsize);
		if (SOCKET_ERROR == send(clifd, (char*) &msg, sizeof(struct MsgHeader), 0)) {
			printf("����ʧ��:%d\n", WSAGetLastError());
		}
	}
	return true;
}
