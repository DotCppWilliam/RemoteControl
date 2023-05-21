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

	/* ��ʼ��socket */
	bool InitSocket()
	{
		if (m_socket == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;	// IPV_4
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(SERV_PORT);

		// �󶨶˿ںź͵�ַ
		if (bind(m_socket, (sockaddr*)&serv_adr, sizeof(serv_adr) == -1))
			return false;
		if (listen(m_socket, 1) == -1)
			return false;
		return true;
	}

	/* ���տͻ������� */
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
			// TODO: ��������
		}
	}

	/*  */
	bool SendData(const char* data, size_t size)
	{
		if (m_client == INVALID_SOCKET) return false;
		return send(m_client, data, size, 0) > 0;
	}

private:
// ���졢����
	CServerSocket() 
	{
		if (InitSockEnv() == FALSE)
		{
			MessageBox(nullptr, _T("�޷���ʼ���׽��ֻ���"),
				_T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(EXIT_FAILURE);
		}
		m_socket = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() 
	{
		closesocket(m_socket);
		WSACleanup();
	}

// �������졢������ֵ�����
	CServerSocket(const CServerSocket&) {}
	CServerSocket& operator=(const CServerSocket&) {}

// ��Ա����
	/* ��ʼ��Winsock�� */
	BOOL InitSockEnv()
	{
		// TODO: �ڴ˴�ΪӦ�ó������Ϊ��д���롣
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
			return FALSE;
		return TRUE;
	}

	/* ���Winsock�� */
	static void releaseInstance()
	{
		if (m_instance != nullptr)
		{
			CServerSocket* tmp = m_instance;
			m_instance = nullptr;
			delete tmp;
		}
	}
// ˽����
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
// ��Ա����
	SOCKET m_socket = INVALID_SOCKET;	// �����׽���
	SOCKET m_client = INVALID_SOCKET;	// �ͻ����׽���

	static CServerSocket* m_instance;
	static CHelper m_helper;
};



//extern CServerSocket server;
