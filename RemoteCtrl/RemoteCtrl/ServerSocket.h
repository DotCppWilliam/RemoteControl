#pragma once

#include "pch.h"
#include "framework.h"
#include "Mouse.h"

#define SERV_PORT	9527	// �˿ں�
#define BUFFER_SIZE	4096	// ��������С
#define PACK_HEAD	0xFEFF	// ��ͷ���ֽڵ�����






enum command { 
	CMD_DRIVER = 1,		// CMD_DRIVER: ��ȡ���еĴ��̷�
	CMD_DIR,			// CMD_DIR: ��ȡָ��Ŀ¼����Ϣ
	CMD_RUN,			// CMD_RUN: ����ĳ���ļ�
	CMD_DLFILE,			// CMD_DLFILE: ����ĳ���ļ�
	CMD_SCREEN,			// ��Ļ
	CMD_MOUSE,			// ������
	CMD_LOCK_MACHINE,	// ����
	CMD_UNLOCK_MACHINE	// ����
};

#pragma pack(push)	// �������ĳ��ȵ�ջ��
#pragma pack(1)		// ���볤��Ϊ1
// �����������ݰ�
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

	int size() { return length + 6; }	// �����������Ĵ�С
	const char* packData()	// ����һ������������������,û���������õĶ���
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
	WORD head;			// ��ͷ �̶�Ϊ: 0xFE FF		2�ֽ�
	DWORD length;		// ���ĳ���,�ӿ������ʼ�� ��У�����	4�ֽ�
	WORD cmd;			// ��������	2�ֽ�
	std::string data;	// ��������	
	WORD sum;			// ��У��	2�ֽ�
	std::string pack_data;	// �洢����������������
};
#pragma pack(pop)	// �ָ�Ϊԭ���Ķ��볤��

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

	/* �������� */
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

	/* ��ȡ�ļ���Ϣ */
	bool GetFilePath(std::string& path)
	{
		if (m_packet.cmd >= CMD_DIR && m_packet.cmd <= CMD_DLFILE)
		{
			path = m_packet.data;
			return true;
		}
		return false;
	}

	/* ��ȡ�����Ϣ */
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
