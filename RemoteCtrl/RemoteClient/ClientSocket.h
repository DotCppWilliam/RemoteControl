#pragma once

#include "Mouse.h"
#include <string>
#include <vector>

#define BUFFER_SIZE	4096	// ��������С
#define PACK_HEAD	0xFEFF	// ��ͷ���ֽڵ�����


enum command {
	CMD_ERR = -1,
	CMD_DRIVER = 1,		// CMD_DRIVER: ��ȡ���еĴ��̷�
	CMD_DIR,			// CMD_DIR: ��ȡָ��Ŀ¼����Ϣ
	CMD_RUN,			// CMD_RUN: ����ĳ���ļ�
	CMD_DLFILE,			// CMD_DLFILE: ����ĳ���ļ�
	CMD_SCREEN,			// ��Ļ
	CMD_MOUSE,			// ������
	CMD_LOCK_MACHINE,	// ����
	CMD_UNLOCK_MACHINE	// ����
};


// �洢�ļ���Ϣ
struct SFileInfo
{
	SFileInfo() :
		isDir(false),
		isValid(true),
		hasNext(true)
	{
		memset(filename, 0, sizeof(filename));
	}

	bool isDir;     // �Ƿ���Ŀ¼, 0: �� 1: ��
	bool isValid;   // �Ƿ���Ч 
	bool hasNext;   // �Ƿ�����Ŀ¼ 
	char filename[256]; // �洢�ļ���
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
		TRACE("client packet: head=%d, length=%d, cmd=%d, sum=%d\r\n", head, length, cmd, sum);
	}
	~CPacket() {}

	int size() { return length + 6; }	// �����������Ĵ�С

	const char* GetPData()
	{
		return data.c_str();
	}

	const char* packData()	// ����һ������������������,û���������õĶ���
	{
		pack_data.resize(size());
		BYTE* ptr = (BYTE*)pack_data.c_str();
		*(WORD*)ptr = head; ptr += 2;
		*(DWORD*)ptr = length; ptr += 4;
		*(WORD*)ptr = cmd; ptr += 2;
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




class CClientSocket
{
public:
	static CClientSocket* getInstance()
	{
		if (m_instance == nullptr)
			m_instance = new CClientSocket;
		return m_instance;
	}


	/* ��ʼ��socket */
	bool InitSocket(DWORD addr, USHORT port);

	/* �ر��׽��� */
	void CloseSocket()
	{
		if (m_socket != INVALID_SOCKET)
		{
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			m_buffer.clear();
			index = 0;
		}
	}

	/* ����ͻ��˷��͵����� */
	int DealCommand();

	/* �������� */
	bool SendData(const char* data, size_t size)
	{
		if (m_socket == INVALID_SOCKET) return false;
		return send(m_socket, data, (int)size, 0) > 0;
	}

	bool SendData(CPacket& packet)
	{
		if (m_socket == INVALID_SOCKET) return false;
		return send(m_socket, packet.packData(), packet.size(), 0) > 0;
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

	CPacket& GetPacket()
	{
		return m_packet;
	}

private:
	// ���졢����
	CClientSocket()
	{
		if (InitSockEnv() == FALSE)
		{
			MessageBox(nullptr, _T("�޷���ʼ���׽��ֻ���"),
				_T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(EXIT_FAILURE);
		}
		m_buffer.resize(BUFFER_SIZE);
		
		m_socket = INVALID_SOCKET;	// ��ʼ��
	}
	~CClientSocket()
	{
		closesocket(m_socket);
		WSACleanup();
	}

	// �������졢������ֵ�����
	CClientSocket(const CClientSocket&) {}
	CClientSocket& operator=(const CClientSocket&) {}

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
			CClientSocket* tmp = m_instance;
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
			CClientSocket::getInstance();
		}
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};
private:
	// ��Ա����
	SOCKET m_socket = INVALID_SOCKET;	// �����׽���
	CPacket m_packet;
	std::vector<char> m_buffer;
	size_t index = 0;

	static CClientSocket* m_instance;
	static CHelper m_helper;
};
