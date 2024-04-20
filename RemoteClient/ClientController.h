#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "utils.h"
#include <map>

#define WM_SEND_PACK (WM_USER + 1)  //发送包数据
#define WM_SEND_DATA (WM_USER + 2)  //单纯发送数据
#define WM_SHOW_STATUS (WM_USER + 3) //展示状态
#define WM_SHOW_WATCH (WM_USER + 4) //远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000) //自定义消息处理

class CClientController
{ //希望在整个生命周期中有个线程，或者让主线程阻塞
public:
	static CClientController* getInstance(); //获取全局唯一对象

	int Invoke(CWnd*& pMainWnd); //启动
	int InitController();
	//发送消息
	LRESULT SendMessage(MSG msg);
	void UpdataAddress(int nIP, int nPort) {
		CClientSocket* pClient = CClientSocket::getInstance();
		pClient->UpdateAddress(nIP, nPort); //调用M层更新连接数据
	}

	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}

	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}

	int SendPacket(const CPacket& pack) {
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(pack);
	}

	//1 查看磁盘分区
	//2 查看指定目录下的文件
	//3 打开文件
	//4 下载文件
	// 9 删除文件
	// 5 鼠标操作
	// 6 发送屏幕内容
	// 7 锁机
	// 8 解锁
	// 1981 测试连接
	// 返回值是命令号
	int SendCommandPacket(int nCmd, bool bAutoClose = true, 
		BYTE* pData = NULL, size_t nLength = 0) {

		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(CPacket(nCmd, pData, nLength));
		int cmd = DealCommand(); //去接收

		if (bAutoClose) {
			CloseSocket();
		}

		return cmd;
	}

	int GetImage(CImage& image) { //mem to image的功能，可以封装走
		CClientSocket* pClient = CClientSocket::getInstance();
		return Cutils::Bytes2Image(image, pClient->GetPack().strData);
	}

	int DownFile(CString strPath) {
		CFileDialog dlg(FALSE, "*",
			strPath,
			OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, NULL,
			&m_remoteDlg); //有一个默认后缀

		if (dlg.DoModal() == IDOK) {
			m_strRemote = strPath;
			m_strLocal = dlg.GetPathName();
			CString strLocal = dlg.GetPathName(); //MFC的API,得到路径，想要传入线程中
			/*****************添加线程函数****************/
			m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
			//_beginthreadex是想要拿到线程ID的时候使用,里面传入的函数指针需要是unsigned __stdcall

			if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
				//说明线程被创建还不会立刻结束，所以等待超时说明线程被成功创建,否则就是失败
				return -1;
			}
			m_remoteDlg.BeginWaitCursor();
			m_statusDlg.m_info.SetWindowText(_T("命令正在执行中"));
			m_statusDlg.ShowWindow(SW_SHOW);
			m_statusDlg.CenterWindow(&m_remoteDlg);
			m_statusDlg.SetActiveWindow();
			//Sleep(50); //进行一些延时，等待鼠标位置被拿到
		}
		return 0;
	}

	void StartWatchScreen();

protected:
	void threadWatchScreen();
	static void threadWatchScreenEntry(void *arg);
	void threadDownloadFile();
	//static unsigned __stdcall threadDownloadEntry(void* arg) {}
	static void threadDownloadEntry(void* arg);

	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg) { //设置了一些对象的父对象
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
		m_isClosed = true;
	}

	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	static unsigned __stdcall threadEntry(void* arg);
	void threadFunc();

	static void releaseInstance() {
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}

	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	typedef struct MsgInfo{
		MSG msg;
		LRESULT result;

		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));

		}

		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));

		}

		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));

			}
			return *this;
		}
	} MSGINFO, *PMSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;
	//std::map<UUID, PMSGINFO> m_mapMessage;
	//对话框对象
	CWatchDialog m_watchDlg; //远程监视窗口
	CRemoteClientDlg m_remoteDlg; //界面主窗口
	CStatusDlg m_statusDlg;  //状态弹窗
	HANDLE m_hThread;
	HANDLE m_hThreadDownload; //处理意外情况：文件还在下载但是程序结束了，需要一个handle来管理下载线程
	CString m_strRemote; //下载文件的远程路径
	CString m_strLocal; //下载到本地的路径
	unsigned m_nThreadID;
	HANDLE m_hThreadWatch;
	bool m_isClosed; //监视是否关闭

	static CClientController* m_instance;
	class CHelper {
		//helper访问socket的成员，helper被释放的时候会连带delete单例对象
	public:
		CHelper() {
			CClientController::getInstance(); //使用全局函数的时候要加上域
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

