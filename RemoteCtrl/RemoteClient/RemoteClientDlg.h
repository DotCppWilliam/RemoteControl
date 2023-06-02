
// RemoteClientDlg.h: 头文件
//

#pragma once

#include "ClientSocket.h"

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	
private:
	int SendCmdPacket(command cmd, bool autoClosed = true, BYTE* data = nullptr, size_t len = 0);
	CString GetPath(HTREEITEM hTree);
	void DelTreeChildItem(HTREEITEM hTree);
	void LoadFileInfo();

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_serv_adr;
	CString m_port;
	afx_msg void OnBnClickedFileinfo();
private:
	CTreeCtrl m_dirTree;
public:
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	CListCtrl m_list;	
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
};
