#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "utils.h"
#include <map>


#define WM_SEND_DATA (WM_USER + 2)  //单纯发送数据
#define WM_SHOW_STATUS (WM_USER + 3) //展示状态
#define WM_SHOW_WATCH (WM_USER + 4) //远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000) //自定义消息处理

//业务逻辑和流程是随时可能发生改变的

class CClientController
{ //希望在整个生命周期中有个线程，或者让主线程阻塞
public:
	static CClientController* getInstance(); //获取全局唯一对象

	int Invoke(CWnd*& pMainWnd); //启动
	int InitController();
	//发送消息
	//LRESULT SendMessage(MSG msg);
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
	// 返回true 成功， false 失败
	bool SendCommandPacket(
		HWND hWnd, //数据包收到后，需要应答的窗口
		int nCmd, bool bAutoClose = true,
		BYTE* pData = NULL, size_t nLength = 0,
		WPARAM wParam = 0);

	int GetImage(CImage& image) { //mem to image的功能，可以封装走 -> 多线程时代弃用

		CClientSocket* pClient = CClientSocket::getInstance(); //不应该从client中拿
		return Cutils::Bytes2Image(image, pClient->GetPack().strData); 
		//经过调试，这里拿到pack是空！因为响应的数据放在lstPack中传给了watchDlg的m_image缓冲区中，没有保存在网络层中
	}

	int DownFile(CString strPath);
	void DownloadEnd();

	void StartWatchScreen();

protected:
	void threadWatchScreen();
	static void threadWatchScreenEntry(void *arg);

	//static unsigned __stdcall threadDownloadEntry(void* arg) {}


	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg) { //设置了一些对象的父对象
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
			TRACE("controller has released!\r\n");
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
			//CClientController::getInstance(); //使用全局函数的时候要加上域
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

