
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#include <atlconv.h>
#include "locale.h"

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
	DDX_Control(pDX, IDC_TREE_DIR, m_dirTree);
	DDX_Control(pDX, IDC_LIST_FILE, m_list);
}



int CRemoteClientDlg::SendCmdPacket(command cmd, 
	bool autoClosed, 
	BYTE* data, 
	size_t len)
{
	UpdateData();
	bool ret;
	CClientSocket* pClient = CClientSocket::getInstance();
	ret = pClient->InitSocket(m_serv_adr, atoi(m_port.GetBuffer()));
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

	if (autoClosed)
		pClient->CloseSocket();

	return 0;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_FILEINFO, &CRemoteClientDlg::OnBnClickedFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
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



void CRemoteClientDlg::OnBnClickedBtnTest()
{
	SendCmdPacket(command(1024));
}

// 获取对方的所有磁盘符
void CRemoteClientDlg::OnBnClickedFileinfo()
{
	setlocale(LC_CTYPE, "chs");
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
			TRACE("控制端: %s\r\n", dr);
			m_dirTree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}

	if (!dr.empty())
	{
		dr += ":";
		m_dirTree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
	}
}

/* 解析出对方的路径返回 */
CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do
	{
		strTmp = m_dirTree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_dirTree.GetParentItem(hTree);
	} while (hTree != nullptr);
	return strRet;
}

/* 删除当前节点的所有子节点 */
void CRemoteClientDlg::DelTreeChildItem(HTREEITEM hTree)
{
	HTREEITEM hSub = nullptr;
	do
	{
		hSub = m_dirTree.GetChildItem(hTree);
		if (hSub != nullptr) m_dirTree.DeleteItem(hSub);
	} while (hSub != nullptr);
}

/* 获取对方盘符的所有目录或文件信息 */
void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);	// 获取当前鼠标坐标位置
	m_dirTree.ScreenToClient(&ptMouse);	// 将鼠标从屏幕坐标系转换为树形控件的客户区坐标系
	// 判断是否命中了Tree的某个节点,并返回标识符HTREEITEM
	HTREEITEM hTreeSelected = m_dirTree.HitTest(ptMouse, 0);
	if (hTreeSelected == nullptr)
		return;

	// 以下代码用来判断是否点击的是文件，而不是目录或某个盘符
	// 但是有问题,需要重新整理
	/*if (m_dirTree.GetChildItem(hTreeSelected) == NULL)
		return;*/

	m_list.DeleteAllItems();
	DelTreeChildItem(hTreeSelected);

	CString strPath = GetPath(hTreeSelected);
	TRACE("----------> 对方目录: %s\r\n", strPath);
	TRACE("------------------------------------------------> 控制端: 接收文件信息 开始\r\n");
	int ret = SendCmdPacket(CMD_DIR, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	SFileInfo* pInfo = (SFileInfo*)CClientSocket::getInstance()->GetPacket().GetPData();
	TRACE("控制端: [%s] [isDir: %s] [数据包大小: %d] [hasNext: %s]\r\n", pInfo->filename, pInfo->isDir ? "是" : "否", CClientSocket::getInstance()->GetPacket().size(),
		pInfo->hasNext ? "是" : "否");
	CClientSocket* pClient = CClientSocket::getInstance();
	
	while (pInfo->hasNext)
	{
		// 目录显示在左侧目录信息中
		if (pInfo->isDir)	
		{
			HTREEITEM hTmp = m_dirTree.InsertItem(pInfo->filename, hTreeSelected, TVI_LAST);
			m_dirTree.InsertItem("", hTmp, TVI_LAST);

			TRACE("控制端: [%s] [isDir: %s] [数据包大小: %d] [hasNext: %s]\r\n", pInfo->filename, pInfo->isDir ? "是" : "否", CClientSocket::getInstance()->GetPacket().size(),
				pInfo->hasNext ? "是" : "否");
		}
		else
		{
			// 文件信息显示在右侧文件信息中
			m_list.InsertItem(0, pInfo->filename);
			TRACE("控制端: [%s] [isDir: %s] [数据包大小: %d] [hasNext: %s]\r\n", pInfo->filename, pInfo->isDir ? "是" : "否", CClientSocket::getInstance()->GetPacket().size(),
				pInfo->hasNext ? "是" : "否");
		}

		int ret = pClient->DealCommand();
		if (ret != 0)
			break;
		pInfo = (SFileInfo*)CClientSocket::getInstance()->GetPacket().GetPData();
		TRACE("控制端: [%s] [isDir: %s] [数据包大小: %d] [hasNext: %s]\r\n", pInfo->filename, pInfo->isDir ? "是" : "否", CClientSocket::getInstance()->GetPacket().size(),
			pInfo->hasNext ? "是" : "否");
	}
	TRACE("------------------------------------------------> 控制端: 接收文件信息 结束\r\n");
	m_dirTree.Expand(hTreeSelected, TVE_EXPAND);
	pClient->CloseSocket();
}

/* 双击一个节点获取对方这个目录(或盘符)下的所有文件信息 */
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();	// 显示目录和文件信息
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo(); // 显示目录和文件信息
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_list.ScreenToClient(&ptList);
	int listSel = m_list.HitTest(ptList);
	if (listSel < 0) return;

	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != nullptr)
	{
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
			ptMouse.x,
			ptMouse.y,
			this);
	}
}
