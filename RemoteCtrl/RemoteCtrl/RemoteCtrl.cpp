// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <io.h>
#include <list>
#include <atlimage.h>
#include "LockDialog.h"

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

CLockDialog lockDlg;    // 锁住机器的屏幕对象

unsigned threadId = 0;  // 线程Id


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

    intptr_t hfind = 0;
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


/* 获取鼠标 */
int MouseEvent()
{
    Mouse mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse))
    {
        DWORD flags = 0; 
        // 处理是什么按键
        switch (mouse.button)
        {
        case LEFT_BUTTON:   flags = 1;  break;  // 左键
        case RIGHT_BUTTON:  flags = 2;  break;  // 右键
        case MIDDLE_BUTTON: flags = 4;  break;  // 中键
        case MOUSE:         flags = 8;  break;  // 没有按键,鼠标移动
        }

        if (flags != 8)
            SetCursorPos(mouse.point.x, mouse.point.y); // 设置鼠标的位置

        // 处理动作
        switch (mouse.action)
        {
        case 0: flags |= 0x10; break;   // 单击
        case 1: flags |= 0x20; break;   // 双击
        case 2: flags |= 0x40; break;   // 按下
        case 3: flags |= 0x80; break;   // 释放
        }

        // 处理不同的组合情况
        switch (flags)
        {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        /* 处理左键 */
        case 0x11: // 左键单击
        {
			// GetMessageExtraInfo: 获取鼠标的附加信息
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        }
        case 0x21: // 左键双击
        {
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        }
        case 0x41: // 左键按下
        {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        }
        case 0x81: // 左键释放
		{
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		}
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        /* 处理右键 */
		case 0x12: // 右键单击
		{
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		}
		case 0x22: // 右键双击
		{
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		}

		case 0x42: // 右键按下
		{
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		}
		case 0x82: // 右键释放
		{
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		}

        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        /* 处理中键 */
		case 0x14: // 中键单击
		{
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		}
		case 0x24: // 中键双击
		{
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		}
		case 0x44: // 中键按下
		{
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		}
		case 0x84: // 中键释放
		{
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		}
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        case 0x8:  // 鼠标移动
		{
            mouse_event(MOUSEEVENTF_MOVE, mouse.point.x, mouse.point.y, 0, GetMessageExtraInfo());
			break;
		}

        default: break;
        }

        CPacket pack(CMD_MOUSE, nullptr, 0);
        CServerSocket::getInstance()->SendData(pack);
    }
    else
    {
        OutputDebugString(_T("获取鼠标参数失败!!!"));
        return -1;
    }

    return 0;
}


/* 发送屏幕(也就是屏幕的截图) */
int SendScreen()
{
    CImage screen;
    HDC hscreen = ::GetDC(nullptr);
    // 获取屏幕的颜色位深度,也就是每个像素所占用的位数
    int bitPerPixel = GetDeviceCaps(hscreen, BITSPIXEL);  
    int width = GetDeviceCaps(hscreen, HORZRES);    // 屏幕分辨率宽
    int height = GetDeviceCaps(hscreen, VERTRES);   // 屏幕分辨率高
    screen.Create(width, height, bitPerPixel);  // 创建一个屏幕缓冲区对象

    // 获取屏幕的图像拷贝到屏幕缓冲区中
    BitBlt(screen.GetDC(), 0, 0, width, height, hscreen, 0, 0, SRCCOPY);
    ReleaseDC(nullptr, hscreen);    // 释放屏幕设备上下文句柄以便其他程序使用

    // 测试执行的效率
    //ULONGLONG tick = GetTickCount64();
    //screen.Save(_T("test.png"), Gdiplus::ImageFormatPNG);// 保存到文件中
    //TRACE("png %d\n", GetTickCount64() - tick);

    // 分配一个可移动的内存块,返回一个句柄
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (mem == nullptr) return -1;

    // 将mem转换为IStream接口对象,即内存块与IStream关联,这样就可以通过流操作内存
    LPSTREAM pstream;
    HRESULT ret = CreateStreamOnHGlobal(mem, TRUE, &pstream);
    if (ret == S_OK)
    {
        screen.Save(pstream, Gdiplus::ImageFormatJPEG);  // 将图像数据写入到流中
        LARGE_INTEGER bg = { 0 };
        pstream->Seek(bg, STREAM_SEEK_SET, nullptr);    // 定位到流的头部
        PBYTE data = (PBYTE)GlobalLock(mem);    // 将全局内存块锁定,并返回内存块首地址
        SIZE_T size = GlobalSize(mem);  // 获得这个内存块的大小

        // 发送给控制端
        CPacket pack(CMD_SCREEN, data, size);
        CServerSocket::getInstance()->SendData(pack);

        GlobalUnlock(mem);  // 解锁
    }

    pstream->Release();
    GlobalFree(mem);
    screen.ReleaseDC();

    return 0;
}


/* 子线程, 执行锁机操作 */
unsigned __stdcall threadLockDlg(void* arg)
{
    TRACE("子线程: %d\r\n", GetCurrentThreadId());
	lockDlg.Create(ID_DIALOG_INFO, nullptr);
	lockDlg.ShowWindow(SW_SHOW);        // 非模态
	lockDlg.SetWindowPos(&lockDlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	HDC hscreen = ::GetDC(nullptr);
	// 设置为全屏
	CRect rect(0, 0, GetDeviceCaps(hscreen, HORZRES), GetDeviceCaps(hscreen, VERTRES));
	lockDlg.MoveWindow(rect);
	// 限制鼠标功能
	ShowCursor(false);

	rect.right = 1;
	rect.bottom = 1;
	ClipCursor(rect);   // 限制鼠标活动范围
	// 隐藏任务栏
	ShowWindow(FindWindow(_T("Shell_TrayWnd"), nullptr), SW_HIDE);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))     // 从消息队列获取消息
	{
		TranslateMessage(&msg); // 将键盘消息转换为字符消息.
		DispatchMessage(&msg);  // 将收到的消息分派给窗口过程进行处理
		if (msg.message == WM_KEYDOWN)  // 以下处理键盘按下的消息
		{
			TRACE("msg: %08X wparm: %08x lparm: %08x\r\n", msg.message, msg.wParam, msg.lParam);
			if (msg.wParam == 0x1b) // 按esc键退出
				break;
		}
	}

    ShowWindow(FindWindow(_T("Shell_TrayWnd"), nullptr), SW_SHOW);
	lockDlg.DestroyWindow();
	ShowCursor(true);
    _endthreadex(0);

    return 0;
}

/* 锁机 */
int LockMachine()
{
    if (lockDlg.m_hWnd == nullptr || lockDlg.m_hWnd == INVALID_HANDLE_VALUE)
    {
        // 创建一个线程
        // 1. 线程安全属性 2. 堆栈大小 3. 线程起点函数 4. 参数 5. 标志 6. 返回线程id
        _beginthreadex(nullptr, 0, threadLockDlg, nullptr, 0, &threadId);
        TRACE("threadid: %d\r\n", threadId);
    }
    
    CPacket pack(CMD_LOCK_MACHINE, nullptr, 0);
    CServerSocket::getInstance()->SendData(pack);
    
    return 0;
} 


/* 解锁 */
int UnLockMachine()
{
    PostThreadMessage(threadId, WM_KEYDOWN, 0x1b, 0);
    CPacket pack(CMD_UNLOCK_MACHINE, nullptr, 0);
    CServerSocket::getInstance()->SendData(pack);

    return 0;
}

// 根据发送过来的命令做相应的操作
int ExecuteCmd(int cmd)
{
    int ret = 0;
	switch (cmd)
	{
	case CMD_DRIVER:    // 获取所有的磁盘符
        ret = MakeDriverInfo();
		break;
	case CMD_DIR:       // 获取指定目录的文件信息
        ret = MakeDirInfo();
		break;
	case CMD_RUN:       // 运行某个文件
        ret = RunFile();
		break;
	case CMD_DLFILE:    // 下载文件
        ret = DownloadFile();
		break;
	case CMD_MOUSE:     // 鼠标
        ret = MouseEvent();
		break;
	case CMD_SCREEN:    // 发送屏幕内容-> 发送屏幕的截图
        ret = SendScreen();
		break;
	case CMD_LOCK_MACHINE:
        ret = LockMachine();  // 锁机
		break;
	case CMD_UNLOCK_MACHINE:
        ret = UnLockMachine();// 解锁
		break;
    case 1024:
        TRACE("被控端收到客户端发来的命令: %d\r\n", cmd);

        CPacket pac(2000, nullptr, 0);
        CServerSocket::getInstance()->SendData(pac);

        break;
	}
    return ret;
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
            // 业务逻辑:
            CServerSocket* pserv = CServerSocket::getInstance();
            int count = 0;
            if (pserv->InitSocket() == false)
            {
                MessageBox(nullptr, _T("网络初始化异常,请检查网络状态"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(EXIT_FAILURE);
            }
            while (CServerSocket::getInstance() != nullptr)
            {
                if (pserv->AcceptClient() == false)
                {
                    if (count >= 3)
                    {
                        MessageBox(nullptr, _T("多次无法正常连接用户,结束进程"), _T("连接用户失败"), MB_OK | MB_ICONERROR);
                        exit(EXIT_FAILURE);
                    }
					MessageBox(nullptr, _T("无法连接用户,正在重试"), _T("连接用户失败"), MB_OK | MB_ICONERROR);
                    count++;
                }
                int ret = pserv->DealCommand();
                if (ret == 0)
                {
                    ret = ExecuteCmd(pserv->GetPacket().cmd);
                    if (ret != 0)
                        TRACE("执行命令失败: %d ret=%d\r\n", pserv->GetPacket().cmd, ret);
                    pserv->CloseSocket();   // 短连接
                }
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
