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

protected:

	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg) { //设置了一些对象的父对象
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;

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
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	unsigned m_nThreadID;
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

