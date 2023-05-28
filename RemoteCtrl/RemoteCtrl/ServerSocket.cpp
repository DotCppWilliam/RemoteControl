#include "pch.h"
#include "ServerSocket.h"

CServerSocket* CServerSocket::m_instance = nullptr;
CServerSocket::CHelper CServerSocket::m_helper;

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
	if (length > 4)	// ����4��ʾ���˿��������У���,����������Ҫ����
	{
		packet.data.resize(packet.length - 2 - 2);	// ����洢���ݵĴ�С
		memcpy((void*)packet.data.c_str(), pdata + i, packet.length - 4);
		i += packet.length - 4;	// ָ���У��ĵ�ַ
	}

	// 5. ����У���
	packet.sum = *(WORD*)(pdata + i);
	i += 2; // ָ��У��͵�λ��

	// ����У���,Ȼ��Ͱ���У���ƥ��
	WORD data_sum = 0;
	size_t dsize = data.size();
	for (int j = 0; j < dsize; j++)
		data_sum += BYTE(data[j]) & 0xFF;

	if (data_sum == sum)	// У���ƥ��ɹ�,���͹�����������
	{
		size = i;	// (cmd + data + sum) + head + length
		return;
	}

	// У���ƥ��ʧ��,����ʧ��
	size = 0;
}
//#pragma pack(pop)


/* ��ʼ��socket */
bool CServerSocket::InitSocket()
{
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;	// IPV_4
	serv_adr.sin_addr.s_addr = INADDR_ANY;
	serv_adr.sin_port = htons(SERV_PORT);

	// �󶨶˿ںź͵�ַ
	if (bind(m_socket, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		return false;
	if (listen(m_socket, 1) == -1)
		return false;
	return true;
}

/* ���տͻ������� */
bool CServerSocket::AcceptClient()
{
	sockaddr_in client_adr;
	memset(&client_adr, 0, sizeof(client_adr));
	int cli_sz = sizeof(client_adr);
	m_client = accept(m_socket, (sockaddr*)&client_adr, &cli_sz);
	if (m_client == -1)
		return false;
	return true;
}

/* ����ͻ��˷��͵����� */
int CServerSocket::DealCommand()
{
	if (m_client == INVALID_SOCKET) return -1;

	TRACE("�����: �ͻ��˵��׽�����: %d\r\n", m_client);

	char* buffer = new char[BUFFER_SIZE];
	if (buffer == nullptr)
		return -2;
	memset(buffer, 0, BUFFER_SIZE);
	size_t index = 0;
	while (true)
	{
		size_t len = recv(m_client, buffer + index, sizeof(buffer), 0);
		if (len <= 0)
			break;

		index += len;
		len = index;
		CPacket packet((BYTE*)buffer, len, m_packet);
		if (len > 0)
		{
			// ���Ż�,�������
			memmove(buffer, buffer + len, BUFFER_SIZE - len);
			index -= len;
			delete [] buffer;
			return 0;
		}
	}

	delete [] buffer;
	return -1;
}
