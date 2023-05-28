#include "pch.h"
#include "ClientSocket.h"
#include <afxwin.h>

CClientSocket* CClientSocket::m_instance = nullptr;
CClientSocket::CHelper CClientSocket::m_helper;

#pragma warning(disable:4996)  // 对fopen不报错

CPacket::CPacket(const BYTE* pdata, size_t& size, CPacket& packet)
	: head(0), length(0), cmd(0), sum(0)
{
	size_t i = 0;
	// 找到包头位置
	for (; i < size; i++)
	{
		if (*(WORD*)(pdata + i) == PACK_HEAD)
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
	packet.cmd = *(WORD*)(pdata + i);
	i += 2;	// 指向包的数据的地址

	// 4. 解析数据
	if (packet.length > 4)	// 大于4表示除了控制命令和校验和,还有数据需要解析
	{
		packet.data.resize(packet.length - 2 - 2);	// 分配存储数据的大小
		// memcpy有问题
		memcpy((void*)packet.data.c_str(), pdata + i, packet.length - 4);
		i += packet.length - 4;	// 指向和校验的地址
	}

	// 5. 解析校验和
	if (i > size)	// 解析失败,并没有接收完整的数据包
	{
		size = 0;
		return;
	}
	packet.sum = *(WORD*)(pdata + i);
	i += 2; // 指向校验和的位置

	// 计算校验和,然后和包的校验和匹配3
	WORD data_sum = 0;
	size_t dsize = packet.data.size();
	for (int j = 0; j < dsize; j++)
		data_sum += BYTE(packet.data[j]) & 0xFF;

	if (data_sum == packet.sum)	// 校验和匹配成功,发送过程数据无误
	{
		size = i;	// (cmd + data + sum) + head + length
		return;
	}

	// 校验和匹配失败,解析失败
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

/* 初始化socket */
bool CClientSocket::InitSocket(DWORD addr, USHORT port)
{
	if (m_socket != INVALID_SOCKET)
		CloseSocket();

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET)
	{
		TRACE("客户端创建套接字失败!!\r\n");
		return false;
	}
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;	// IPV_4
	serv_adr.sin_addr.s_addr = htonl(addr);
	serv_adr.sin_port = htons(port);

	if (serv_adr.sin_addr.s_addr == INADDR_NONE)
	{
		AfxMessageBox(_T("指定的IP地址不存在"));
		return false;
	}

	int ret = connect(m_socket, (sockaddr*)&serv_adr, sizeof(serv_adr));
	if (ret == -1)
	{
		AfxMessageBox(_T("连接失败"));
		TRACE("连接失败: %d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()));
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
			// 可优化,解决拷贝
			memmove(buffer, buffer + len, BUFFER_SIZE - len);
			index -= len;
			return 0;
		}
	}
	return -1;
}