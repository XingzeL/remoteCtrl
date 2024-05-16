#include "pch.h"
#include "framework.h"
#include "Remote Control.h"
#include "ServerSocket.h"

#include <chrono>
#include "utils.h"
#include "Command.h"

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


int main() {
    if (Cutils::IsAdmin()) {
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

    return 0;
}
