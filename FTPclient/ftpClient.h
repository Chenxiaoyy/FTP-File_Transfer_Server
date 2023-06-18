#pragma once
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <stdbool.h>
#define SPORT 8888
//������
enum MSGTAG
{
	MSG_FILENAME = 1,   //�ļ���
	MSG_FILESIZE = 2,   //�ļ���С
	MSG_RAEDY_READ = 3, // ׼������
	MSG_SENDFILE = 4,       //����
	MSG_SUCCESSED = 5,   //�������

	MSG_OPENFILE_FAILD = 6  //���߿ͻ����ļ��Ҳ���
};

#pragma pack(1)//���ýṹ��1�ֽڶ���
#define PACKET_SIZE (1024 - sizeof(int) * 3)
struct MsgHeader // ��װ��Ϣͷ
{
	enum MSGTAG msgID; //��ǰ��Ϣ���
	union MyUnion {
		struct {
			int fileSize;   //�ļ���С
			char fileName[256]; //�ļ���
		}fileInfo;

		struct
		{
			int nStart;   //���ı��
			int nsize;  //�ð������ݴ�С
			char buf[PACKET_SIZE];
		}packet;
	};
};
#pragma pack() // 
// ��ʼ��socket��
bool initSocket();
// �ر�socket��
bool closeSocket();
// �����ͻ�������
void connectToHost();
// ������Ϣ
bool processMsg(SOCKET);

void downloadFileName(SOCKET serfd);
void readyread(SOCKET, struct MsgHeader*);
//д�ļ�
bool writeFile(SOCKET, struct MsgHeader*);

