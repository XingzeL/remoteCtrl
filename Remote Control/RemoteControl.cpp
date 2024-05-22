#include "pch.h"
#include "framework.h"
#include "Remote Control.h"
#include "ServerSocket.h"

#include <chrono>
#include "utils.h"
#include "Command.h"
#include <conio.h>
#include "CSafeQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define INVOKE_PATH _T("C:\\Users\\lixingze\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteControl.exe")

// 唯一的应用程序对象

CWinApp theApp;
//server; ServerSocket.h中声明的外部变量server在main外直接使用会报错
using namespace std;

//业务和通用
bool ChooseAutoInvoke(const CString& strPath) {
    TCHAR wcsSystem[MAX_PATH] = _T("");
    GetSystemDirectory(wcsSystem, MAX_PATH);
    //CString strPath = (_T("C:\\Windows\\SysWOW64\\RemoteControl.exe")); //注意是sys32还是64
    if (PathFileExists(strPath)) {
        return true; //如果已经提醒一次且建立了软链接，就不执行此函数
    }

    CString strInfo = _T("该程序只允许用于合法用途！\n");
    strInfo += _T("继续运行该程序将是的这台机器处于被监控状态!\n");
    strInfo += _T("如果不希望这样，请按取消。\n");
    strInfo += _T("按下'是'将使程序被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下'否'此程序将只会运行一次，不会留下任何东西！\n");

    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath);
        if (!Cutils::WriteStartupDir(strPath)) //或者加上返回值，第一个方法失败了适用第二个方法
        {
            if (ret == FALSE) {
                MessageBox(NULL, _T("设置自动开机启动失败!\r\n程序启动失败"),
                    _T("错误"), MB_ICONERROR | MB_TOPMOST);
                return false;
            }
        }
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}

#define IOCP_LIST_PUSH 1
#define IOCP_LIST_POP 2
#define IOCP_LIST_EMPTY 0

enum {
    IocpListEmpty,
    IocpListPush,
    IocpListPop
};

typedef struct IocpParam {
    int nOperator; //操作码
    std::string strData; //数据
    _beginthread_proc_type cbFunc; //从beginthread参数里拿的类型
    IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
        nOperator = op;
        strData = sData;
        cbFunc = cb;
    }
    IocpParam() {
        nOperator = -1;
    }
}IOCP_PARAM;

//epoll是同步
//iocp 的accept会给内核投递一个请求，来连接的时候，在get那里会得到响应
//rtsp
void threadmain(HANDLE hIOCP) {
    std::list<std::string> lstString; //字符串的list用来写入和提取
    DWORD dwTransferred = 0; //拿到了多少字节
    ULONG_PTR CompletionKey = 0; //拿到的数据
    OVERLAPPED* pOverlapped = NULL;
    int count = 0, count0 = 0;
    while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
    {
        if ((dwTransferred == 0) || (CompletionKey == NULL)) {
            printf("thread is prepare to exit!\r\n");
            break;
        }
        IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
        if (pParam->nOperator == IocpListPush) {
            lstString.push_back(pParam->strData);
            count++;
        }
        else if (pParam->nOperator == IocpListPop) {
            printf("%p  %d\r\n", pParam->cbFunc, lstString.size());
            std::string* pstr = NULL;
            if (lstString.size() > 0) {
                pstr = new std::string(lstString.front());
                lstString.pop_front();
            }
            if (pParam->cbFunc) {
                pParam->cbFunc(pstr);

            }
            count0++;
        }
        else if (pParam->nOperator == IocpListEmpty) {
            lstString.clear();
        }
        delete pParam;
    }
    printf("count %d count0 %d\r\n", count, count0);
}

void threadQueueEntry(HANDLE hIOCP) //线程函数
{
    threadmain(hIOCP);
    _endthread(); //endthread()在线程业务的作用域之外，如果endthread和业务放在一起，
    //因为线程的结束会直接终止上下文，会让局部对象来不及调用析构函数，造成内存泄漏。这种泄漏不是new造成的
    //开线程函数的习惯：需要一个entry和一个thread的功能函数，在功能函数外调用endthread，这样保证内部的对象生命周期都结束
    //总结：endthread()不要和局部对象处于同一个作用域
}

void func(void* arg) {
    //回调：打印以下传来的消息
    std::string* pstr = (std::string*)arg;
    if (pstr != NULL) {
        printf("pop from list:%s\r\n", pstr->c_str());
        delete pstr;
    }
    else {
        printf("list is empty, no data!\r\n");
    }
}

void test()
{
    printf("press any key  to exit ...  \r\n");
    SafeQueue<std::string> lstStrings;
    ULONGLONG tick = GetTickCount64();
    ULONGLONG tick0 = GetTickCount64(), total = GetTickCount64();
    int count = 0, count0 = 0;
    while (GetTickCount64() - total <= 1000) //设计理念：请求和实现分离了
    {
        if (GetTickCount64() - tick0 > 13) {
            //激活IOCP并且传给他一个消息,给队列添加一个元素
            lstStrings.PushBack("hello world");

            tick0 = GetTickCount64();
            count0++;
        }

        if (GetTickCount64() - tick > 20) {
            //激活IOCP并且传给他一个消息
            std::string str;
            lstStrings.PopFront(str);
            tick = GetTickCount64();
            printf("pop from queue:%s\r\n", str.c_str());
        }
        Sleep(1); //防止CPU卡死
    }


    printf("exit done! size %d\r\n", lstStrings.Size());
    lstStrings.Clear();
    printf("exit done! size %d\r\n", lstStrings.Size());
}

int main() {
    if (!Cutils::Init()) return 1;
    
    for (int i = 0; i < 100; ++i) {
        test();
    }
    

    ::exit(0);
    
    //一个粗糙但是结构清晰的IOCP
    
    /*
    *     if (Cutils::IsAdmin()) {
        if (!Cutils::Init()) return 1;
        OutputDebugString(L"current is run as administrator!\r\n");



        if (ChooseAutoInvoke(INVOKE_PATH)) {
            CCommand cmd;
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
            //在外面调用command类的静态函数,传入的参数是&cmd，是一个Command对象指针在里面作为thiz调用成员函数
            switch (ret) {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                break;
            case -2:
                MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), \
                    MB_OK | MB_ICONERROR);
                break;
            }
        }

    } //是否提权
    else {
        OutputDebugString(L"current is run as normal user!\r\n");
        //获取管理员权限，使用该权限创建进程
        if (Cutils::RunAsAdmin() == false)
        {
            Cutils::ShowError();
        }
        return 0;
    }
    */


    return 0;
}
