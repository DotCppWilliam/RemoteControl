#pragma once

#include "pch.h"
#include "framework.h"

#define SERV_PORT	9527	// �˿ں�
#define BUFFER_SIZE	4096	// ��������С

// �����������ݰ�
class CPacket
{
public:
	CPacket() : head(0), length(0), cmd(0), sum(0) {}
	CPacket(const BYTE* pdata, size_t& size, CPacket& packet);
	~CPacket() {}
public:
	WORD head;			// ��ͷ �̶�Ϊ: 0xFE FF		2�ֽ�
	DWORD length;		// ���ĳ���,�ӿ������ʼ�� ��У�����	4�ֽ�
	WORD cmd;			// ��������	2�ֽ�
	std::string data;	// ��������	
	WORD sum;			// ��У��	2�ֽ�
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

	/* ��ʼ��socket */
	bool InitSocket();

	/* ���տͻ������� */
	bool AcceptClient();

	/* ����ͻ��˷��͵����� */
	int DealCommand();

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
	CPacket m_packet;

	static CServerSocket* m_instance;
	static CHelper m_helper;
};



//extern CServerSocket server;
