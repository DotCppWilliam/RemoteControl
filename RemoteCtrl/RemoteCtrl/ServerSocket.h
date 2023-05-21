#pragma once

#include "pch.h"
#include "framework.h"

#define SERV_PORT	9527

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
	bool InitSocket()
	{
		if (m_socket == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;	// IPV_4
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(SERV_PORT);

		// 绑定端口号和地址
		if (bind(m_socket, (sockaddr*)&serv_adr, sizeof(serv_adr) == -1))
			return false;
		if (listen(m_socket, 1) == -1)
			return false;
		return true;
	}

	/* 接收客户端连接 */
	bool AcceptClient()
	{
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_socket, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1)
			return false;
		return true;
	}

	/*  */
	int DealCommand()
	{
		if (m_client == INVALID_SOCKET) return false;
		char buffer[1024];
		while (true)
		{
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0)
				return -1;
			// TODO: 处理命令
		}
	}

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

	static CServerSocket* m_instance;
	static CHelper m_helper;
};



//extern CServerSocket server;
