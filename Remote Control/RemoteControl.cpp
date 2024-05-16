// Remote Control.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

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


// 唯一的应用程序对象

CWinApp theApp;
//server; ServerSocket.h中声明的外部变量server在main外直接使用会报错
using namespace std;

void WriteRegisterTable(const CString& strPath) {
    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    char sPath[MAX_PATH] = "";
    char sSys[MAX_PATH] = "";
    std::string strExe = "\\RemoteControl.exe "; //注意后面要加空格, 文件名不要有空格

    GetCurrentDirectoryA(MAX_PATH, sPath);
    GetSystemDirectoryA(sSys, sizeof(sSys));
    std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
    int ret = system(strCmd.c_str()); //执行命令，创建一个软链接 1.遇到问题：执行不成功，但返回值是正确的 

    TRACE("ret = %d\r\n", ret);
    //操作注册表
    HKEY hKey = NULL;
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);//KEY_WOW64_64KEY是必要的
    if (ret != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("设置自动开机启动失败!\r\n程序启动失败"),
            _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
    /*
        有可能执行创建到sys32中，但是链接却在sysWOW64中，导致后面出现问题
    */
    //CString strPath = CString(_T("%SystemRoot%\\system32\\Remote Control.exe")); //这里面找不到
    //CString strPath = CString(_T("%SystemRoot%\\SysWOW64\\RemoteControl.exe"));
    RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
    if (ret != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("设置自动开机启动失败!\r\n程序启动失败"),
            _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
    RegCloseKey(hKey);
}

void WriteStartupDir(const CString& strPath) {

    CString strCmd = GetCommandLine(); //获取cmd
    strCmd.Replace(_T("\""), _T("")); //去掉双引号
    BOOL ret = CopyFile(strCmd, strPath, FALSE); //FALSE：文件存在就直接覆盖
    if (ret == FALSE) {
        MessageBox(NULL, _T("设置自动开机启动失败!\r\n程序启动失败"),
            _T("错误"), MB_ICONERROR | MB_TOPMOST);
        ::exit(0);
    }
}

void ChooseAutoInvoke() {
    TCHAR wcsSystem[MAX_PATH] = _T("");
    GetSystemDirectory(wcsSystem, MAX_PATH);
    //CString strPath = (_T("C:\\Windows\\SysWOW64\\RemoteControl.exe")); //注意是sys32还是64
    CString strPath = _T("C:\\Users\\lixingze\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteControl.exe"); //目标文件
    if (PathFileExists(strPath)) {
        return; //如果已经提醒一次且建立了软链接，就不执行此函数
    }

    CString strInfo = _T("该程序只允许用于合法用途！\n");
    strInfo += _T("继续运行该程序将是的这台机器处于被监控状态!\n");
    strInfo += _T("如果不希望这样，请按取消。\n");
    strInfo += _T("按下'是'将使程序被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下'否'此程序将只会运行一次，不会留下任何东西！\n");

    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath);
        WriteStartupDir(strPath); //或者加上返回值，第一个方法失败了适用第二个方法

    }
    else if (ret == IDCANCEL) {
        exit(0);
    }
}

void ShowError()
{
    LPWSTR lpMessageBuf = NULL;
    //strerror(errno); //标准C库的错误
    //应对非标准库的错误：
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&lpMessageBuf, 0, NULL
    );
    OutputDebugString(lpMessageBuf);
    LocalFree(lpMessageBuf); //释放系统分配的内存
}

bool IsAdmin() {
    HANDLE hToken = NULL; 
    //handle是一个void指针，一部分属于用户层，一部分属于os，用户的handle屏蔽掉了数据的结构
    //启发：如果想和第三方合作，使用功能时要用句柄来调用接口，保证自己的技术细节
    //Q:如何实现用handle调用对象
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        //错误及处理
        ShowError();
        return false;
    }
    //拿到了token，进行判断
    TOKEN_ELEVATION eve;
    DWORD len = 0;
    if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
        ShowError();
        return false;
    }
    return eve.TokenIsElevated;
    if (len == sizeof(eve)) {
        //查看是否已经成功提权
        CloseHandle(hToken);

    }
    else {
        //有其他的值，显示以下
        printf("length of tokeninfomation is %d\r\n", len);
        return false;
    }
}

int main() {
    if (IsAdmin()) {
        printf("current is run as administrator!\r\n");
    } //是否提权
    else {
        printf("current is run as normal user!\r\n");
    }
    int nRetCode = 0;
    //ChooseAutoInvoke(); //使用启动项进行的开机启动
    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr) {
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else {
            CCommand cmd; 
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);

            //在外面调用command类的静态函数,传入的参数是&cmd，是一个Command对象指针在里面作为thiz调用成员函数
            switch (ret) {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
            case -2:
                MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), \
                    MB_OK | MB_ICONERROR);
                exit(0);
                break;
            }
                
        }
    }
    else {
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
