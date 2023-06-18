#pragma once
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <stdbool.h>
#define SPORT 8888

//定义标记
enum MSGTAG
{
	MSG_FILENAME = 1,   //文件名
	MSG_FILESIZE = 2,   //文件大小
	MSG_RAEDY_READ = 3, // 准备发送
	MSG_SENDFILE = 4,       //发送
	MSG_SUCCESSED = 5,   //传输完成

	MSG_OPENFILE_FAILD = 6  //告诉客户端文件找不到
};

#pragma pack(1)//设置结构体1字节对齐
#define PACKET_SIZE (1024 - sizeof(int) * 3)
struct MsgHeader // 封装消息头
{
	enum MSGTAG msgID; //当前消息标记
	union MyUnion {
		struct {
			int fileSize;   //文件大小
			char fileName[256]; //文件名
		}fileInfo;

		struct 
		{
			int nStart;   //包的编号
			int nsize;  //该包的数据大小
			char buf[PACKET_SIZE];
		}packet;
	};
};
#pragma pack() // 

// 初始化socket库
bool initSocket();
// 关闭socket库
bool closeSocket();
// 监听客户端连接
void listToClient();
// 处理消息
bool processMsg(SOCKET);
//读文件
bool readFile(SOCKET, struct MsgHeader*);
//发送文件
bool sendFile(SOCKET, struct MsgHeader*);

