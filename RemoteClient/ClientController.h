#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include <map>

#define WM_SEND_PACK (WM_USER + 1)  //���Ͱ�����
#define WM_SEND_DATA (WM_USER + 2)  //������������
#define WM_SHOW_STATUS (WM_USER + 3) //չʾ״̬
#define WM_SHOW_WATCH (WM_USER + 4) //Զ�̼��
#define WM_SEND_MESSAGE (WM_USER+0x1000) //�Զ�����Ϣ����

class CClientController
{ //ϣ�������������������и��̣߳����������߳�����
public:
	static CClientController* getInstance(); //��ȡȫ��Ψһ����

	int Invoke(CWnd*& pMainWnd); //����
	int InitController();
	//������Ϣ
	LRESULT SendMessage(MSG msg);

protected:

	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg) { //������һЩ����ĸ�����
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
	//�Ի������
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	unsigned m_nThreadID;
	static CClientController* m_instance;
	class CHelper {
		//helper����socket�ĳ�Ա��helper���ͷŵ�ʱ�������delete��������
	public:
		CHelper() {
			CClientController::getInstance(); //ʹ��ȫ�ֺ�����ʱ��Ҫ������
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

