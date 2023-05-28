
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#include <atlconv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_serv_adr(0)
	, m_port(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, ID_IPADDR_SERV, m_serv_adr);
	DDX_Text(pDX, ID_PORT, m_port);
	DDX_Control(pDX, IDC_TREE1, m_dirTree);
}



int CRemoteClientDlg::SendCmdPacket(command cmd, BYTE* data, size_t len)
{
	UpdateData();
	bool ret;
	CClientSocket* pClient = CClientSocket::getInstance();
	ret = pClient->InitSocket(m_serv_adr, atoi(CW2A(m_port.GetBuffer())));
	if (!ret)
	{
		AfxMessageBox(_T("控制端: 网络初始化失败"));
		return -1;
	}

	CPacket pack(cmd, data, len);
	ret = pClient->SendData(pack);
	if (ret)
		TRACE("客户端发送数据成功\r\n");

	if (pClient->DealCommand() != 0)
	{
		pClient->CloseSocket();
		return -1;
	}
	TRACE("客户端收到被控端发送过来的命令: %d\r\n", pClient->GetPacket().cmd);

	pClient->CloseSocket();

	return 0;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_FILEINFO, &CRemoteClientDlg::OnBnClickedFileinfo)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_serv_adr = 0x7F000001;
	m_port = _T("9527");
	UpdateData(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#include <string>

void CRemoteClientDlg::OnBnClickedBtnTest()
{
	SendCmdPacket(command(1024));
}

// 查看对方文件信息
void CRemoteClientDlg::OnBnClickedFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = SendCmdPacket(CMD_DRIVER);
	if (ret != 0)
	{
		AfxMessageBox(_T("控制端: 命令处理失败!!!\r\n"));
		return;
	}

	CClientSocket* pclient = CClientSocket::getInstance();
	std::string drivers = pclient->GetPacket().data;
	std::string dr;


	m_dirTree.DeleteAllItems();


	// 插入中文字符会乱码
	size_t size = drivers.size();
	for (size_t i = 0; i < size; i++)
	{
		if (drivers[i] == ',')
		{
			dr += ":";
			CString str(dr.c_str());
			TRACE("控制端: %s\r\n", str);
			m_dirTree.InsertItem((LPCTSTR)str, TVI_ROOT, TVI_LAST);
			
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}

}
