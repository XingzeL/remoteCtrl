
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"
#include "WatchDialog.h"
#ifndef WM_SEND_PACK_ACK
#define WM_SEND_PACK_ACK (WM_USER+2)
#endif
// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数
	void LoadFileInfo(); 
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	//int SendCommandPacket(int nCmd, bool bAutoClose = true,BYTE* pData = NULL, size_t nLength = 0);

	void DeleteTreeChildrenItem(HTREEITEM hTree);
	CString GetPath(HTREEITEM hTree);

	void LoadFileCurrent();
	//static void threadEntryForDownLoadFile(void* arg);
	//static void threadEntryForWatchData(void* arg); //静态函数作为框架，写线程相关的启动，结束等但是不能访问this
	//void threadDownFile();
	//void threadWatchData(); //成员函数可以访问this指针
public:



private:
	//CImage m_image; //图像缓存
	bool m_isFull; //图像缓存是否有数据
	bool m_isClosed; //监视是否关闭

// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	//afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedConnect();
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownload();
	afx_msg void OnDeletefile();
	afx_msg void OnRunfile();
	afx_msg void OnTvnSelchangedTreeDir(NMHDR* pNMHDR, LRESULT* pResult);

	//自定义消息
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressctrl(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangePortctrl();

	afx_msg LRESULT OnSendPakcetAck(WPARAM wParam, LPARAM lParam);
};
