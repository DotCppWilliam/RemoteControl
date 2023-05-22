// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <io.h>
#include <list>
#include <fstream>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma warning(disable:4996)  // 对fopen不报错

// 设置入口点为 mainCRTStartup, 子系统设置为 windows.则没有窗口,在后台运行

//#pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup") // 控制台
//#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")
//#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

// 存储文件信息
struct SFileInfo
{
    SFileInfo() :
        isDir(false), 
        isValid(true),
        hasNext(true)
    {
        memset(filename, 0, sizeof(filename));
    }

    bool isDir;     // 是否是目录, 0: 否 1: 是
    bool isValid;   // 是否有效 
    bool hasNext;   // 是否还有子目录 
    char filename[256]; // 存储文件名
};


/* 获取所有的磁盘符 */
int MakeDriverInfo()
{
    std::string result;
    for (int i = 1; i <= 26; i++)   // 1==> A, 2==>B, ..., 26==Z
    {
        if (_chdrive(i) == 0)
        {
			if (result.size() > 0)
				result += ",";
            result += 'a' + i - 1;
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());
  
    //CServerSocket::getInstance()->SendData(CPacket(1, (BYTE*)&result, result.size()));
	return 0;
}


/* 获取指定目录的所有文件信息 */
int MakeDirInfo()
{
    std::string path;
    if (CServerSocket::getInstance()->GetFilePath(path) == false)   // 获取包数据中的目录
    {
        OutputDebugString(_T("当前命令不是获取文件列表,解析命令失败!!!"));
        return -1;
    }

    if (_chdir(path.c_str()) != 0)  // 切换到指定目录
    {
        SFileInfo finfo;
        finfo.isValid = false;  // 无效的
        finfo.isDir = true;     // 是个目录
        finfo.hasNext = false;  // 没有子目录或文件
        memcpy(finfo.filename, path.c_str(), path.size());
        // 给控制端发送回应,无法切换到指定目录
        CPacket pack(CMD_DIR, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->SendData(pack);

        OutputDebugString(_T("没有权限访问目录!!!"));
        return -2;
    }

    int hfind = 0;
    _finddata_t fdata;
    if ((hfind = _findfirst("*", &fdata)) == -1)  // 查找指定目录的所有文件 通配符 *
    {
        OutputDebugString(_T("没有找到任何文件!!!"));
        return -3;
    }

    do // 获取指定目录的所有文件信息给控制端
    {
        SFileInfo finfo;
        finfo.isDir = (fdata.attrib & _A_SUBDIR) != 0;  // 判断是否是目录
        memcpy(finfo.filename, fdata.name, strlen(fdata.name));
        CPacket pack(CMD_DIR, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->SendData(pack);
    } while (!_findnext(hfind, &fdata));

    // 指定目录遍历完,给控制端发送解析完成的响应
    SFileInfo finfo;
    finfo.hasNext = false;
	CPacket pack(CMD_DIR, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstance()->SendData(pack);

    return 0;
}


/* 运行文件 */
int RunFile()
{
    std::string path;
    CServerSocket::getInstance()->GetFilePath(path);
    ShellExecuteA(nullptr, nullptr, path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

	CPacket pack(CMD_RUN, nullptr, 0);
	CServerSocket::getInstance()->SendData(pack);

    return 0;
}

/* 下载某个文件 */
int DownloadFile()
{
	std::string path;
	CServerSocket::getInstance()->GetFilePath(path);
	ShellExecuteA(nullptr, nullptr, path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    long long dataLen = 0;
    FILE* file = fopen(path.c_str(), "rb");
    if (file == nullptr)
    {
        CPacket pack(CMD_DLFILE, (BYTE*)&dataLen, sizeof(dataLen));
        CServerSocket::getInstance()->SendData(pack);
        return -1;
    }

    // 向控制端先发送一个文件的大小
    fseek(file, 0, SEEK_END);
    dataLen = _ftelli64(file);
    CPacket head(CMD_DLFILE, (BYTE*)&dataLen, sizeof(dataLen));
    fseek(file, 0, SEEK_SET);


    char buffer[1024];
    size_t len = 0;
    do 
    {
        len = fread(buffer, 1, 1024, file);
        CPacket packet(CMD_DLFILE, (BYTE*)buffer, len);
        CServerSocket::getInstance()->SendData(packet);
    } while (len != 0);

    CPacket packet(CMD_DLFILE, nullptr, 0);
    CServerSocket::getInstance()->SendData(packet);
    fclose(file);

    return 0;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            command ret = CMD_DIR;
     //       // 业务逻辑:
     //       CServerSocket* pserv = CServerSocket::getInstance();
     //       int count = 0;
     //       if (pserv->InitSocket() == false)
     //       {
     //           MessageBox(nullptr, _T("网络初始化异常,请检查网络状态"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
     //           exit(EXIT_FAILURE);
     //       }
     //       while (CServerSocket::getInstance() != nullptr)
     //       {
     //           if (pserv->AcceptClient() == false)
     //           {
     //               if (count >= 3)
     //               {
     //                   MessageBox(nullptr, _T("多次无法正常连接用户,结束进程"), _T("连接用户失败"), MB_OK | MB_ICONERROR);
     //                   exit(EXIT_FAILURE);
     //               }
					//MessageBox(nullptr, _T("无法连接用户,正在重试"), _T("连接用户失败"), MB_OK | MB_ICONERROR);
     //               count++;
     //           }
     //           ret = pserv->DealCommand();
     //           // TODO:
     //       }
            
            // 根据发送过来的命令做相应的操作
            switch (ret)
            {
			case CMD_DRIVER:    // 获取所有的磁盘符
				MakeDriverInfo();
				break;
            case CMD_DIR:       // 获取指定目录的文件信息
                MakeDirInfo();
                break;     
            case CMD_RUN:       // 运行某个文件
                RunFile();
                break;
            case CMD_DLFILE:
                DownloadFile();
                break;
            }

        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
