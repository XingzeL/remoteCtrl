#include "pch.h"

#include "ClientController.h"

//注意：静态变量成员变量需要在cpp中声明一下，非静态变量就是构造的时候初始化
std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;

CClientController* CClientController::getInstance()
{
    if (m_instance == NULL) {
        m_instance = new CClientController();
        struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = {
            {WM_SEND_PACK, &CClientController::OnSendPack},
            {WM_SEND_DATA, &CClientController::OnSendData},
            {WM_SHOW_STATUS, &CClientController::OnShowStatus},
            {WM_SHOW_WATCH, &CClientController::OnSendWatcher},
            {(UINT) - 1, NULL}
        };
        for (int i = 0; MsgFuncs[i].func != NULL; i++) {
            m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
        }
    }
    return nullptr;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
    pMainWnd = &m_remoteDlg;
    return m_remoteDlg.DoModal();
}

int CClientController::InitController()
{
    //处理一下线程
    m_hThread = (HANDLE)_beginthreadex(NULL, 0,
        &CClientController::threadEntry, this,
        0, &m_nThreadID); //会返回一个不完全的对
    m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg); //创建了状态的窗口
    return 0;
}

LRESULT CClientController::SendMessage(MSG msg)
{
    
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL) return -2;
    MSGINFO info(msg);
    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)&hEvent); //给线程号发送消息，然后想办法拿到返回结果
    //MSGINFO& inf = m_mapMessage.find(uuid)->second;
    WaitForSingleObject(hEvent, -1); //同步：等待事件结束
    return info.result; //拿到消息处理的返回值
}

void CClientController::StartWatchScreen()
{
    m_isClosed = false;
    CWatchDialog dlg(&m_remoteDlg);
    m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry,
        0, this);
    dlg.DoModal();
    m_isClosed = true;
    WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
    Sleep(50);
    while (!m_isClosed) {
        if (m_remoteDlg.isFull() == false) {
            int ret = SendCommandPacket(6);
            if (ret == 6) {
     
                if (GetImage(m_remoteDlg.GetImage()) == 0) {

                    m_remoteDlg.SetImageStatus(true);
                }
                else {
                    TRACE("获取图片失败! ret = %d\r\n", ret);
                }
            }
        }
        Sleep(1);
    }

}

void CClientController::threadWatchScreenEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadWatchScreen();
    _endthread();
}

void CClientController::threadDownloadFile()
{
    FILE* pFile = fopen(m_strLocal, "wb+");
    if (pFile == NULL) {
        AfxMessageBox("本地没有权限保存该文件或者无法创建文件！！");
        m_statusDlg.ShowWindow(SW_HIDE);
        m_remoteDlg.EndWaitCursor(); //结束光标的沙漏
        return;
    }
    CClientSocket* pClient = CClientSocket::getInstance();

    do {

        //int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength()); //1.原先在V层直接调用发送包的函数，出现updata界面冲突

        //int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile); //2.改为在V层发送消息，主线程接收后进行包的发送
        int ret = SendCommandPacket(4, false,
            (BYTE*)(LPCSTR)m_strRemote,
            m_strRemote.GetLength());  //

        if (ret < 0) {
            AfxMessageBox(_T("执行下载命令失败!!!"));
            TRACE("执行下载失败: ret = %d\r\n", ret);
            break;
        }
        //进行接收
        //1.longlong 长度  

        long long nlength = *(long long*)CClientSocket::getInstance()->GetPack().strData.c_str();
        if (nlength == 0) {
            AfxMessageBox("文件长度为零或者无法读取文件！！");
            break; //跳出do的循环，进入外面的关闭连接
        }

        long long nCount = 0; //维护接收的长度信息
        //接收文件：用fopen核fwrite
        //用一个模态的对话框
        while (nCount < nlength) {
            ret = pClient->DealCommand();
            if (ret < 0) {
                AfxMessageBox("传输失败!");
                TRACE("传输失败：ret = %d\r\n", ret);
                break;
            }

            fwrite(pClient->GetPack().strData.c_str(), 1, pClient->GetPack().strData.size(), pFile);
            nCount += pClient->GetPack().strData.size();
        }
    } while (false);
    fclose(pFile);
    pClient->CloseSocket();
    m_statusDlg.ShowWindow(SW_HIDE);
    m_remoteDlg.EndWaitCursor();
    m_remoteDlg.MessageBox(_T("下载完成！"), _T("完成"));
}

void CClientController::threadDownloadEntry(void* arg)
{
    //thiz法：将静态成员函数转换成能访问this的对象成员函数
    CClientController* thiz = (CClientController*)arg;
    thiz->threadDownloadFile();
    _endthread();
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadFunc();
    _endthreadex(0);
    return 0;
}


//#define WM_SEND_PACK (WM_USER + 1)  //发送包数据
//#define WM_SEND_DATA (WM_USER + 2)  //单纯发送数据
//#define WM_SHOW_STATUS (WM_USER + 3) //展示状态
//#define WM_SHOW_WATCH (WM_USER + 4) //远程监控
void CClientController::threadFunc() //控制层的线程，消息处理
{
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0)) { //消息循环，用来接收V层的消息
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_SEND_MESSAGE) { //如果消息是由SendMessages发出的
            MSGINFO* pmsg = (MSGINFO*)msg.wParam;
            HANDLE hEvent = (HANDLE)msg.lParam;
            //拿到了新的message
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end()) {
                //std::map<UUID, PMSGINFO>::iterator it = m_mapMessage.find(*puuid);
                pmsg->result = (this->*it->second)(pmsg->msg.message,
                    pmsg->msg.wParam, pmsg->msg.lParam);
            }
            else {
                pmsg->result = -1;
            }
            SetEvent(hEvent); //event能够标记各自的处理
        }

        else {
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end()) {
                (this->*it->second)(msg.message, msg.wParam, msg.lParam); //拿到一个返回值
                //因为需要调用成员函数，所以前面需要this来调用
            }
        }

    }
}

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    CClientSocket* pClient = CClientSocket::getInstance();
    CPacket* pPacket = (CPacket*)wParam;
    return pClient->Send(*pPacket); //发送包;
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    CClientSocket* pClient = CClientSocket::getInstance();
    char* pBuffer = (char*)wParam;
    return pClient->Send(pBuffer, (int)lParam); //发送包;
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_watchDlg.DoModal();
}
