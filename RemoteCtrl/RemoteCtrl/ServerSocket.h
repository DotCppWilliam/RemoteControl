#pragma once

#include "pch.h"
#include "framework.h"

#define SERV_PORT	9527	// 端口号
#define BUFFER_SIZE	4096	// 缓冲区大小

// 解析网络数据包
class CPacket
{
public:
	CPacket() : head(0), length(0), cmd(0), sum(0) {}
	CPacket(const BYTE* pdata, size_t& size, CPacket& packet);
	~CPacket() {}
public:
	WORD head;			// 包头 固定为: 0xFE FF		2字节
	DWORD length;		// 包的长度,从控制命令开始到 和校验结束	4字节
	WORD cmd;			// 控制命令	2字节
	std::string data;	// 包的数据	
	WORD sum;			// 和校验	2字节
};



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

	/*  */
	bool SendData(const char* data, size_t size)
	{
		if (m_client == INVALID_SOCKET) return false;
		return send(m_client, data, size, 0) > 0;
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
