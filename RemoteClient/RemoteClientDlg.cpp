
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#include "ClientSocket.h"
#include <atlstr.h> // 包含 CString 相关的头文件
#include <stdlib.h> // 包含 atoi 函数的头文件

#include "ClientController.h"
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
private:
	int SendCommandPacket(int nCmd, BYTE* pData = NULL, size_t nLength = 0);

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
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESSCTRL, m_server_address);
	DDX_Text(pDX, IDC_PORTCTRL, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

////V层引入了M层的Socket和C层的controller的头文件
//int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose,BYTE* pData, size_t nLength)
//{
//	//里面直接调用socket对象的动作都变成调用controller中的函数
//	return CClientController::getInstance()->SendCommandPacket(nCmd, bAutoClose, pData, nLength);
//
//}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CRemoteClientDlg::OnBnClickedButton1)
	//ON_BN_CLICKED(IDC_BUTTON2, &CRemoteClientDlg::OnBnClickedButton2)

	//ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CRemoteClientDlg::OnLvnItemchangedList1)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD, &CRemoteClientDlg::OnDownload)
	ON_COMMAND(ID_DELETEFILE, &CRemoteClientDlg::OnDeletefile)
	ON_COMMAND(ID_RUNFILE, &CRemoteClientDlg::OnRunfile)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_DIR, &CRemoteClientDlg::OnTvnSelchangedTreeDir)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESSCTRL, &CRemoteClientDlg::OnIpnFieldchangedIpaddressctrl)
	ON_EN_CHANGE(IDC_PORTCTRL, &CRemoteClientDlg::OnEnChangePortctrl)
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
	m_server_address = 0x7f000001; //本地回环
	m_nPort = _T("9527");

	//更新连接数据
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(m_server_address, _tstoi(m_nPort));

	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this); //初始化下载框
	m_dlgStatus.ShowWindow(SW_HIDE);
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



void CRemoteClientDlg::OnBnClickedButton1()
{
	int ret = CClientController::getInstance()->SendCommandPacket(1981);
	TRACE("Get revice %d\r\n", ret);
}


void CRemoteClientDlg::OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	std::list<CPacket> lstPackets;
	int ret = CClientController::getInstance()->SendCommandPacket(1, true, NULL, 0, &lstPackets);
	if (ret == -1 || (lstPackets.size() <= 0)) {
		AfxMessageBox(_T("命令处理失败"));
		return;
	}
	CPacket& head = lstPackets.front();

	std::string drivers = head.strData;
	CString dr;

	// 删除树控件中的所有项
	m_Tree.DeleteAllItems();

	// 循环遍历字符串
	for (size_t i = 0; i < drivers.size(); ++i) {
		// 如果遇到逗号，将当前子字符串插入树控件并清空临时字符串
		if (drivers[i] == ',') {
			dr += ":"; //windows磁盘的标号
			//dr += CString(drivers.substr(drivers.size() - dr.GetLength()).c_str());
			//m_Tree.InsertItem(dr, TVI_ROOT, TVI_LAST);
			HTREEITEM hTmp = m_Tree.InsertItem(CString(dr), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTmp, TVI_LAST);
			//m_Tree.InsertItem(dr, TVI_ROOT, TVI_LAST);
			dr.Empty();
			continue;
		}
		// 将字符添加到临时字符串中
		dr += drivers[i];
	}

	 //插入最后一个子字符串
	if (!dr.IsEmpty()) {
		dr += ":"; //windows磁盘的标号
		HTREEITEM hTmp = m_Tree.InsertItem(CString(dr), TVI_ROOT, TVI_LAST);
		m_Tree.InsertItem(NULL, hTmp, TVI_LAST);
	}
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {
	//指定一个节点，展开所有文件信息
	
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	TRACE(strRet);
	return strRet;
}
void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse); //得到鼠标的坐标，是一个全局的
	m_Tree.ScreenToClient(&ptMouse); //坐标转换

	//去判断一下点击是否是点中了某个节点
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse);
	if (hTreeSelected == NULL) {
		return;
	}
	//看看选中的东西有没有子节点
	if (m_Tree.GetChildItem(hTreeSelected) == NULL) return; //因为每个目录都加入了空的子节点，所以没有的话就是文件
	DeleteTreeChildrenItem(hTreeSelected); //防止无限增长
	m_List.DeleteAllItems();
	//准备获取这个节点的信息
	CString strPath = GetPath(hTreeSelected); //拿到这个节点的路径，准备传给server端

	//没有调成多字节字符集之前类型转换就没有用
	BYTE* pData = (BYTE*)(LPCTSTR)strPath;
	//传给服务器：得到文件信息
	std::list<CPacket> lstPackets;
	int nCmd = CClientController::getInstance()->SendCommandPacket(2, false, 
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength(), &lstPackets); //如果不是多字符集:这里只传进去了D而不是"D:\\"
	if (lstPackets.size() > 0) {
		TRACE("lstPacket.size = %d\r\n", lstPackets.size());
		//往后处理
		std::list<CPacket>::iterator it = lstPackets.begin();
		for (; it != lstPackets.end(); it++) {
			PFILEINFO pInfo = (PFILEINFO)(*it).strData.c_str();
			if (pInfo->HasNext == FALSE) continue;
			if (pInfo->IsDirectory) {
				if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..") {
					continue;
				}
				HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
				m_Tree.InsertItem("", hTemp, TVI_LAST); //插入内容，parent，查到上一个的后面
			}
			else {
				//是文件的情况
				/*m_List.InsertItem(0, pInfo->szFileName);*/
				m_List.InsertItem(m_List.GetItemCount(), pInfo->szFileName);
				m_List.UpdateWindow();
				TRACE("文件名：%s", pInfo->szFileName);
				//2个原因导致问题：服务端传送的太快；客户端的m_List的属性只有是small icon才能正常显示所有文件
			}
		}
	}

	//注意：有一个点上不断双击时，需要保证不累积

}
void CRemoteClientDlg::LoadFileCurrent()
{
	//此时的HTREE和list已经是选中状态
	HTREEITEM hTree = m_Tree.GetSelectedItem();
		//准备获取这个节点的信息
	CString strPath = GetPath(hTree); //拿到这个节点的路径，准备传给server端

	//清空列表
	m_List.DeleteAllItems();

	//没有调成多字节字符集之前类型转换就没有用
	BYTE* pData = (BYTE*)(LPCTSTR)strPath;
	//传给服务器：得到文件信息
	int nCmd = CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, 
		strPath.GetLength()); //如果不是多字符集:这里只传进去了D而不是"D:\\"
	//CClientController::getInstance()->
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPack().strData.c_str();
	
	while (pInfo->HasNext) {
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		//从do-while改为while: 因为server的发送逻辑：如果是空路径或者没有权限就会直接发回一个空，没有内容，所以需要先判断HasNext
		if (!pInfo->IsDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ask:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPack().strData.c_str();
	}  //当hasnext为空，说明没有

	//注意：有一个点上不断双击时，需要保证不累积
	CClientController::getInstance()->CloseSocket();
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL)  m_Tree.DeleteItem(hSub); //防止重复获取
	} while (hSub != NULL);
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// 处理双击：文件和目录分别有逻辑， 还有.和..的处理
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList); //转换成client的坐标
	int ListSelected = m_List.HitTest(ptList); //返回一个序号，表明哪个列表被选中了
	if (ListSelected < 0) return;
	//要弹出一个菜单
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK); //加载菜单
	CMenu* pPopup = menu.GetSubMenu(0); //选子菜单的第一个
	if (pPopup != NULL) {
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}


void CRemoteClientDlg::OnDownload()
{
	//菜单函数的按键是无参数的
	int nListSelected = m_List.GetSelectionMark(); //取到鼠标点到的标签
	CString strFile = m_List.GetItemText(nListSelected, 0); //拿到文件名
	HTREEITEM hSelected = m_Tree.GetSelectedItem(); //获取被选择的树
	strFile = GetPath(hSelected) + strFile;//完整路径
	int ret = CClientController::getInstance()->DownFile(strFile); //调用c层的成员
	
	if (ret != 0) {
		MessageBox(_T("下载失败！"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
	BeginWaitCursor(); //光标设置为沙漏
	
}


void CRemoteClientDlg::OnDeletefile()
{
	// 删除文件
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件命令执行失败！！！");
	}
	//刷新文件列表
	LoadFileCurrent(); //TODO：文件夹中文件显示有缺漏
}


void CRemoteClientDlg::OnRunfile()
{
	// 打开文件
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败！！！");
	}

}


void CRemoteClientDlg::OnTvnSelchangedTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd) {
	case 4:
		{
			CString strFile = (LPCSTR)lParam;
			int ret = CClientController::getInstance()->SendCommandPacket(wParam >> 1, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());

		}
		break;
	case 5: //鼠标操作 
		{
		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
		}
		break;
	case 6:
	case 7:
	case 8:
		{ //6,7,8都是只发送一个命令没有data
			ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, NULL, 0);
		}
		break;
	default:
		return -1;
	}
	return ret;
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();

}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}

//输入框变化的时候更新数据
void CRemoteClientDlg::OnIpnFieldchangedIpaddressctrl(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(m_server_address, _tstoi(m_nPort));
}


void CRemoteClientDlg::OnEnChangePortctrl()
{
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(m_server_address, _tstoi(m_nPort));
}
