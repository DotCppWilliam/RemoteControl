#include "pch.h"
#include "ClientSocket.h"
#include <afxwin.h>

CClientSocket* CClientSocket::m_instance = nullptr;
CClientSocket::CHelper CClientSocket::m_helper;

#pragma warning(disable:4996)  // ��fopen������

CPacket::CPacket(const BYTE* pdata, size_t& size, CPacket& packet)
	: head(0), length(0), cmd(0), sum(0)
{
	size_t i = 0;
	// �ҵ���ͷλ��
	for (; i < size; i++)
	{
		if (*(WORD*)(pdata + i) == PACK_HEAD)
		{
			// 1. ������ͷ
			packet.head = *(WORD*)(pdata + i);
			i += 2;	// ������ͷ,ָ����ĳ���λ��
			break;
		}
	}

	// ��ǰ�Ƿ���һ�������İ�,�������ȡ������У��
	// ���û����Щ������,���ܽ���һ�������İ�
	if (i + 4 + 2 + 2 > size)
	{
		size = 0;
		return;
	}

	// 2. �������ĳ���
	packet.length = *(DWORD*)(pdata + i);
	i += 4;	// �������ĳ���,ָ���������ĵ�ַ
	if (length + i > size)	// ��δ��ȫ�յ�. �򷵻�,����ʧ��
	{
		size = 0;
		return;
	}


	// 3. ������������
	packet.cmd = *(WORD*)(pdata + i);
	i += 2;	// ָ��������ݵĵ�ַ

	// 4. ��������
	if (packet.length > 4)	// ����4��ʾ���˿��������У���,����������Ҫ����
	{
		packet.data.resize(packet.length - 2 - 2);	// ����洢���ݵĴ�С
		// memcpy������
		memcpy((void*)packet.data.c_str(), pdata + i, packet.length - 4);
		i += packet.length - 4;	// ָ���У��ĵ�ַ
	}

	// 5. ����У���
	if (i > size)	// ����ʧ��,��û�н������������ݰ�
	{
		size = 0;
		return;
	}
	packet.sum = *(WORD*)(pdata + i);
	i += 2; // ָ��У��͵�λ��

	// ����У���,Ȼ��Ͱ���У���ƥ��3
	WORD data_sum = 0;
	size_t dsize = packet.data.size();
	for (int j = 0; j < dsize; j++)
		data_sum += BYTE(packet.data[j]) & 0xFF;

	if (data_sum == packet.sum)	// У���ƥ��ɹ�,���͹�����������
	{
		size = i;	// (cmd + data + sum) + head + length
		return;
	}

	// У���ƥ��ʧ��,����ʧ��
	size = 0;
}
//#pragma pack(pop)








std::string GetErrInfo(int errNo)
{
	std::string ret;
	LPVOID msgBuf = nullptr;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		nullptr,
		errNo,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&msgBuf, 0, nullptr
	);
	ret = (char*)msgBuf;
	LocalFree(msgBuf);
	return ret;
}

/* ��ʼ��socket */
bool CClientSocket::InitSocket(DWORD addr, USHORT port)
{
	if (m_socket != INVALID_SOCKET)
		CloseSocket();

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET)
	{
		TRACE("�ͻ��˴����׽���ʧ��!!\r\n");
		return false;
	}
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;	// IPV_4
	serv_adr.sin_addr.s_addr = htonl(addr);
	serv_adr.sin_port = htons(port);

	if (serv_adr.sin_addr.s_addr == INADDR_NONE)
	{
		AfxMessageBox(_T("ָ����IP��ַ������"));
		return false;
	}

	int ret = connect(m_socket, (sockaddr*)&serv_adr, sizeof(serv_adr));
	if (ret == -1)
	{
		AfxMessageBox(_T("����ʧ��"));
		TRACE("����ʧ��: %d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()));
		return false;
	}

	return true;
}


/*  */
int CClientSocket::DealCommand()
{
	if (m_socket == INVALID_SOCKET) return -1;

	char* buffer = m_buffer.data();
	memset(buffer, 0, BUFFER_SIZE);
	size_t index = 0;
	while (true)
	{
		size_t len = recv(m_socket, buffer + index, sizeof(buffer), 0);
		if (len <= 0)
			return -1;

		index += len;
		len = index;
		CPacket packet((BYTE*)buffer, len, m_packet);
		if (len > 0)
		{
			// ���Ż�,�������
			memmove(buffer, buffer + len, BUFFER_SIZE - len);
			index -= len;
			return 0;
		}
	}
	return -1;
}