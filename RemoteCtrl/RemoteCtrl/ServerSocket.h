#pragma once

#include "pch.h"
#include "framework.h"
#include "Mouse.h"

#define SERV_PORT	9527	// 端口号
#define BUFFER_SIZE	4096	// 缓冲区大小
#define PACK_HEAD	0xFEFF	// 包头两字节的内容






enum command { 
	CMD_DRIVER = 1,		// CMD_DRIVER: 获取所有的磁盘符
	CMD_DIR,			// CMD_DIR: 获取指定目录的信息
	CMD_RUN,			// CMD_RUN: 运行某个文件
	CMD_DLFILE,			// CMD_DLFILE: 下载某个文件
	CMD_SCREEN,			// 屏幕
	CMD_MOUSE,			// 鼠标操作
	CMD_LOCK_MACHINE,	// 锁机
	CMD_UNLOCK_MACHINE	// 解锁
};

#pragma pack(push)	// 保存对齐的长度到栈中
#pragma pack(1)		// 对齐长度为1
// 解析网络数据包
class CPacket
{
public:
	CPacket() : head(0), length(0), cmd(0), sum(0) {}
	CPacket(const BYTE* pdata, size_t& size, CPacket& packet);
	CPacket(WORD _cmd, const BYTE* pdata, size_t size)
		: head(0), length(0), cmd(0), sum(0)
	{
		head = PACK_HEAD;
		length = (DWORD)(size + 4);
		cmd = _cmd;
		if (size > 0)
		{
			data.resize(size);
			memcpy((BYTE*)data.c_str(), pdata, size);
		}
		else
			data.clear();

		for (int i = 0; i < size; i++)
			sum += (BYTE)pdata[i] & 0xFF;
	}
	~CPacket() {}

	int size() { return length + 6; }	// 返回整个包的大小
	const char* packData()	// 返回一个包含整个包的数据,没有其他无用的东西
	{
		pack_data.resize(size());
		BYTE* ptr = (BYTE*)pack_data.c_str();
		*(WORD*)ptr = head; ptr += 2;
		*(DWORD*)ptr = length; ptr += 4;
		memcpy(ptr, data.c_str(), data.size()); ptr += data.size();
		*(WORD*)ptr = sum;

		return pack_data.c_str();
	}

public:
	WORD head;			// 包头 固定为: 0xFE FF		2字节
	DWORD length;		// 包的长度,从控制命令开始到 和校验结束	4字节
	WORD cmd;			// 控制命令	2字节
	std::string data;	// 包的数据	
	WORD sum;			// 和校验	2字节
	std::string pack_data;	// 存储整个包的所有数据
};
#pragma pack(pop)	// 恢复为原来的对齐长度

class CServerSocket
{
public:
	static CServerSocket* getInstance()
	{
		if (m_instance == nullptr)
			m_instance = new CServerSocket;
		return m_instance;
	}


	/* 初始化socket */
	bool InitSocket();

	/* 接收客户端连接 */
	bool AcceptClient();

	/* 处理客户端发送的命令 */
	int DealCommand();

	/* 发送数据 */
	bool SendData(const char* data, size_t size)
	{
		if (m_client == INVALID_SOCKET) return false;
		return send(m_client, data, (int)size, 0) > 0;
	}

	bool SendData(CPacket& packet)
	{
		if (m_client == INVALID_SOCKET) return false;
		return send(m_client, packet.packData(), packet.size(), 0) > 0;
	}

	/* 获取文件信息 */
	bool GetFilePath(std::string& path)
	{
		if (m_packet.cmd >= CMD_DIR && m_packet.cmd <= CMD_DLFILE)
		{
			path = m_packet.data;
			return true;
		}
		return false;
	}

	/* 获取鼠标信息 */
	bool GetMouseEvent(Mouse& mouse)
	{
		if (m_packet.cmd == CMD_MOUSE)
		{
			memcpy(&mouse, m_packet.data.c_str(), sizeof(mouse));
			return true;
		}
		return false;
	}

private:
// 构造、析构
	CServerSocket() 
	{
		if (InitSockEnv() == FALSE)
		{
			MessageBox(nullptr, _T("无法初始化套接字环境"),
				_T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(EXIT_FAILURE);
		}
		m_socket = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() 
	{
		closesocket(m_socket);
		WSACleanup();
	}

// 拷贝构造、拷贝赋值运算符
	CServerSocket(const CServerSocket&) {}
	CServerSocket& operator=(const CServerSocket&) {}

// 成员函数
	/* 初始化Winsock库 */
	BOOL InitSockEnv()
	{
		// TODO: 在此处为应用程序的行为编写代码。
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
			return FALSE;
		return TRUE;
	}

	/* 清除Winsock库 */
	static void releaseInstance()
	{
		if (m_instance != nullptr)
		{
			CServerSocket* tmp = m_instance;
			m_instance = nullptr;
			delete tmp;
		}
	}
// 私有类
	class CHelper
	{
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};
// 成员变量
	SOCKET m_socket = INVALID_SOCKET;	// 监听套接字
	SOCKET m_client = INVALID_SOCKET;	// 客户端套接字
	CPacket m_packet;

	static CServerSocket* m_instance;
	static CHelper m_helper;
};



//extern CServerSocket server;
