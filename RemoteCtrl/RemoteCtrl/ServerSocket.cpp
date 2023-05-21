#include "pch.h"
#include "ServerSocket.h"

CServerSocket* CServerSocket::m_instance = nullptr;
CServerSocket::CHelper CServerSocket::m_helper;


CPacket::CPacket(const BYTE* pdata, size_t& size, CPacket& packet)
{
	size_t i = 0;
	// 找到包头位置
	for (; i < size; i++)
	{
		if (*(WORD*)(pdata + i) == 0xFEFF)
		{
			// 1. 解析包头
			packet.head = *(WORD*)(pdata + i);
			i += 2;	// 跳过包头,指向包的长度位置
			break;
		}
	}

	// 当前是否是一个完整的包,包含长度、命令、和校验
	// 如果没有这些则跳过,不能解析一个完整的包
	if (i + 4 + 2 + 2 > size)
	{
		size = 0;
		return;
	}

	// 2. 解析包的长度
	packet.length = *(DWORD*)(pdata + i);
	i += 4;	// 跳过包的长度,指向控制命令的地址
	if (length + i > size)	// 包未完全收到. 则返回,解析失败
	{
		size = 0;
		return;
	}


	// 3. 解析控制命令
	packet.cmd = *(DWORD*)(pdata + i);
	i += 2;	// 指向包的数据的地址

	// 4. 解析数据
	if (length > 4)	// 大于4表示除了控制命令和校验和,还有数据需要解析
	{
		packet.data.resize(length - 2 - 2);	// 分配存储数据的大小
		memcpy((void*)packet.data.c_str(), pdata + i, length - 4);
		i += length - 4;	// 指向和校验的地址
	}

	// 5. 解析校验和
	packet.sum = *(WORD*)(pdata + i);
	i += 2; // 指向校验和的位置

	// 计算校验和,然后和包的校验和匹配
	WORD data_sum = 0;
	int dsize = data.size();
	for (i = 0; i < dsize; i++)
		data_sum += BYTE(data[i]) & 0xFF;

	if (data_sum == sum)	// 校验和匹配成功,发送过程数据无误
	{
		size = i;	// (cmd + data + sum) + head + length
		return;
	}

	// 校验和匹配失败,解析失败
	size = 0;
}

/* 初始化socket */
bool CServerSocket::InitSocket()
{
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;	// IPV_4
	serv_adr.sin_addr.s_addr = INADDR_ANY;
	serv_adr.sin_port = htons(SERV_PORT);

	// 绑定端口号和地址
	if (bind(m_socket, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		return false;
	if (listen(m_socket, 1) == -1)
		return false;
	return true;
}

/* 接收客户端连接 */
bool CServerSocket::AcceptClient()
{
	sockaddr_in client_adr;
	int cli_sz = sizeof(client_adr);
	m_client = accept(m_socket, (sockaddr*)&client_adr, &cli_sz);
	if (m_client == -1)
		return false;
	return true;
}

/* 处理客户端发送的命令 */
int CServerSocket::DealCommand()
{
	if (m_client == INVALID_SOCKET) return -1;

	char* buffer = new char[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	size_t index = 0;
	while (true)
	{
		size_t len = recv(m_client, buffer, sizeof(buffer), 0);
		if (len <= 0)
			return -1;

		index += len;
		len = index;
		CPacket packet((BYTE*)buffer, len, m_packet);
		if (len > 0)
		{
			// 可优化,解决拷贝
			memmove(buffer, buffer + len, BUFFER_SIZE - len);
			index -= len;
			return m_packet.cmd;
		}
	}
	return -1;
}
