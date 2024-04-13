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

int main() {
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr) {
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else {
            CCommand cmd; 
            CServerSocket* pserver = CServerSocket::getInstance();

            int ret = pserver->Run(&CCommand::RunCommand, &cmd); 
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
