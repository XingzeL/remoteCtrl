#pragma once

class Cutils
{
public:
    static void Dump(BYTE* pData, size_t nSize) {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++) {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0)) strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }

    static bool IsAdmin() {
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

    static bool RunAsAdmin() {
        //Win专业版：本地策略组：开启Administrator账号，禁止空密码只能登录本控制台
        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath,
            CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi); //登录的方式启动进程

        if (!ret) {
            ShowError();
            MessageBox(NULL, _T("创建进程失败"), _T("程序错误"), 0);
            ::exit(0);
        }
        WaitForSingleObject(pi.hProcess, INFINITE); //等待handle
        CloseHandle(pi.hProcess); //进程运行完成后关闭
        CloseHandle(pi.hThread);
        return true;
    }

    static void ShowError()
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
        MessageBox(NULL, lpMessageBuf, _T("发生错误"), 0);
        LocalFree(lpMessageBuf); //释放系统分配的内存
    }

    static BOOL WriteStartupDir(const CString& strPath) {

        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CopyFile(sPath, strPath, FALSE); //FALSE：文件存在就直接覆盖
        return ret;
    }

    static bool WriteRegisterTable(const CString& strPath) {
        /*
            通过修改注册表来实现开机启动
        */
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
            return false;
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
            return false;
        }
        RegCloseKey(hKey);
        return true;
    }

    static bool Init() //用于带MFC命令行项目的初始化(通用)
    {
        HMODULE hModule = ::GetModuleHandle(nullptr);
        if (hModule == nullptr) {
            wprintf(L"错误: GetModuleHandle 失败\n");
            return false;
        }
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
            wprintf(L"错误: MFC 初始化失败\n");
            return false;
        }
        return true;
    }
};

