
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0500
//#endif

#include <Windows.h>
#include <WinCon.h>
#include <stdio.h>
#include <Shlwapi.h>
//#include <conio.h>
#include <vector>
#include "..\common\common.hpp"
extern "C" {
#include "..\common\ConEmuCheck.h"
}
#include "ConEmuD.h"

WARNING("����������� ����� ������� ������� SetForegroundWindow �� GUI ����, ���� � ������ �������");
WARNING("����������� �������� ��� � ��� ������������� ��������");

#ifdef _DEBUG
//  �����������������, ����� ����� ����� ������� �������� (conemuc.exe) �������� MessageBox, ����� ����������� ����������
//	#define SHOW_STARTED_MSGBOX
#endif

WARNING("!!!! ���� ����� ��� ��������� ������� ���������� ������� ���");
// � ��������� ��� � RefreshThread. ���� �� �� 0 - � ������ ������ (100��?)
// �� ������������� ���������� ������� � �������� ��� � 0.

#ifdef _DEBUG
#define MCHKHEAP { int MDEBUG_CHK=_CrtCheckMemory(); _ASSERTE(MDEBUG_CHK); }
#else
#define MCHKHEAP
#endif

#ifndef _DEBUG
#define FORCE_REDRAW_FIX
#else
#undef FORCE_REDRAW_FIX
#endif


WARNING("�������� ���-�� ����� ����������� ������������� ������ ����������� �������, � �� ������ �� �������");

WARNING("����� ������ ����� ������������ �������� ������� GUI ���� (���� ��� ����). ���� ����� ���� ������� �� far, � CMD.exe");

WARNING("���� GUI ����, ��� �� ���������� �� �������� - �������� ���������� ���� � �������� ���������� ����� ������");

WARNING("� ��������� ������� �� ����������� �� EVENT_CONSOLE_UPDATE_SIMPLE �� EVENT_CONSOLE_UPDATE_REGION");
// ������. ��������� cmd.exe. �������� �����-�� ����� � ��������� ������ � �������� 'Esc'
// ��� Esc ������� ������� ������ �� ���������, � ����� � ������� ���������!




/*  Global  */
DWORD   gnSelfPID = 0;
HANDLE  ghConIn = NULL, ghConOut = NULL;
HWND    ghConWnd = NULL;
HANDLE  ghExitEvent = NULL;
BOOL    gbAlwaysConfirmExit = FALSE;
BOOL    gbAttachMode = FALSE;
DWORD   gdwMainThreadId = 0;
//int       gnBufferHeight = 0;
wchar_t* gpszRunCmd = NULL;
HANDLE  ghCtrlCEvent = NULL, ghCtrlBreakEvent = NULL;

enum tag_RunMode {
    RM_UNDEFINED = 0,
    RM_SERVER,
    RM_COMSPEC
} gnRunMode = RM_UNDEFINED;

struct tag_Srv {
	DWORD dwProcessGroup;
	//
    HANDLE hServerThread;   DWORD dwServerThreadId;
    HANDLE hRefreshThread;  DWORD dwRefreshThread;
    HANDLE hWinEventThread; DWORD dwWinEventThread;
    HANDLE hInputThread;    DWORD dwInputThreadId;
    //
    CRITICAL_SECTION csProc;
    CRITICAL_SECTION csConBuf;
	// ������ ��������� ��� �����, ����� ����������, ����� ������� ��� �� �����.
	// ��������, ��������� FAR, �� �������� Update, FAR �����������...
	std::vector<DWORD> nProcesses;
    //
    wchar_t szPipename[MAX_PATH], szInputname[MAX_PATH], szGuiPipeName[MAX_PATH];
    //
    HANDLE hConEmuGuiAttached;
    HWINEVENTHOOK hWinHook;
    BOOL bContentsChanged; // ������ ������ ���������� ������ ���� ������
    wchar_t* psChars;
    WORD* pnAttrs;
		DWORD nBufCharCount;
	WORD* ptrLineCmp;
		DWORD nLineCmpSize;
    DWORD dwSelRc; CONSOLE_SELECTION_INFO sel; // GetConsoleSelectionInfo
    DWORD dwCiRc; CONSOLE_CURSOR_INFO ci; // GetConsoleCursorInfo
    DWORD dwConsoleCP, dwConsoleOutputCP, dwConsoleMode;
    DWORD dwSbiRc; CONSOLE_SCREEN_BUFFER_INFO sbi; // MyGetConsoleScreenBufferInfo
	//USHORT nUsedHeight; // ������, ������������ � GUI - ������ ���� ���������� gcrBufferSize.Y
	SHORT nTopVisibleLine; // ��������� � GUI ����� ���� �������������. ���� -1 - ��� ����������, ���������� ������� ��������
    DWORD nMainTimerElapse;
    BOOL  bConsoleActive;
    HANDLE hRefreshEvent; // ServerMode, ���������� �������, � ���� ���� ��������� - �������� � GUI
    HANDLE hChangingSize; // FALSE �� ����� ����� ������� �������
    BOOL  bNeedFullReload, bForceFullReload;
    DWORD nLastUpdateTick;
} srv = {0};

struct tag_Cmd {
	DWORD dwProcessGroup;
	DWORD dwFarPID;
	CONSOLE_SCREEN_BUFFER_INFO sbi;
} cmd = {0};

COORD gcrBufferSize = {80,25};
BOOL  gbParmBufferSize = FALSE;
SHORT gnBufferHeight = 0;

HANDLE ghLogSize = NULL;
wchar_t* wpszLogSizeFile = NULL;


BOOL gbInRecreateRoot = FALSE;

//#define CES_NTVDM 0x10 -- common.hpp
DWORD dwActiveFlags = 0;


BOOL WINAPI DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}



int ComspecInit()
{
    TODO("���������� ��� ������������� ��������, � ���� ��� FAR - ��������� ��� (��� ����������� � ����� �������)");
	TODO("������ �������� �� GUI, ���� ��� ����, ����� - �� ���������");
	TODO("GUI ����� ��������������� ������ � ������ ������ ���������");

	int nNewBufferHeight = 0;
	COORD crNewSize = cmd.sbi.dwSize;
	SMALL_RECT rNewWindow = cmd.sbi.srWindow;

	CESERVER_REQ *pIn = NULL, *pOut = NULL;
	int nSize = sizeof(CESERVER_REQ)-sizeof(pIn->Data)+3*sizeof(DWORD);
	pIn = (CESERVER_REQ*)calloc(nSize,1);
	if (pIn) {
		pIn->nCmd = CECMD_CMDSTARTSTOP;
		pIn->nSize = nSize;
		pIn->nVersion = CESERVER_REQ_VER;
		((DWORD*)(pIn->Data))[0] = TRUE;
		((DWORD*)(pIn->Data))[1] = (DWORD)ghConWnd;
		((DWORD*)(pIn->Data))[2] = gnSelfPID;

		pOut = ExecuteGuiCmd(ghConWnd, pIn);
		if (pOut) {
			nNewBufferHeight = ((DWORD*)(pOut->Data))[0];
			crNewSize.X = (SHORT)((DWORD*)(pOut->Data))[1];
			crNewSize.Y = (SHORT)((DWORD*)(pOut->Data))[2];
			if (rNewWindow.Right >= crNewSize.X) // ������ ��� �������� �� ���� ������ ���������
				rNewWindow.Right = crNewSize.X-1;
			free(pOut);

			gnBufferHeight = nNewBufferHeight;
		}
	}

	if (MyGetConsoleScreenBufferInfo(ghConOut, &cmd.sbi)) {
		if (gnBufferHeight > nNewBufferHeight)
			nNewBufferHeight = gnBufferHeight;
		//SMALL_RECT rc = {0}; 
		//COORD crNew = {cmd.sbi.dwSize.X,cmd.sbi.dwSize.Y};
		SetConsoleSize(nNewBufferHeight, crNewSize, rNewWindow, "ComspecInit");
	}
    return 0;
}

void ComspecDone(int aiRc)
{
	WARNING("������� � GUI CONEMUCMDSTOPPED");

    TODO("��������� ������ ����� ���� (���� �������� - FAR) ��� ������� ��������. ������ ������ ������� � ��������� ���������� ������� � ������ ����� ������� ���������� � ConEmuC!");
    
    // ������� ������ ������ (������ � ������)
	if (cmd.sbi.dwSize.X && cmd.sbi.dwSize.Y) {
		SMALL_RECT rc = {0};
		SetConsoleSize(0, cmd.sbi.dwSize, rc, "ComspecDone");
	}

	CESERVER_REQ *pIn = NULL, *pOut = NULL;
	int nSize = sizeof(CESERVER_REQ)-sizeof(pIn->Data)+3*sizeof(DWORD);
	pIn = (CESERVER_REQ*)calloc(nSize,1);
	if (pIn) {
		pIn->nCmd = CECMD_CMDSTARTSTOP;
		pIn->nSize = nSize;
		pIn->nVersion = CESERVER_REQ_VER;
		((DWORD*)(pIn->Data))[0] = TRUE;
		((DWORD*)(pIn->Data))[1] = (DWORD)ghConWnd;
		((DWORD*)(pIn->Data))[2] = gnSelfPID;

		pOut = ExecuteGuiCmd(ghConWnd, pIn);
		if (pOut) {
			free(pOut);
		}
	}

	SafeCloseHandle(ghCtrlCEvent);
	SafeCloseHandle(ghCtrlBreakEvent);
}

WARNING("�������� LogInput(INPUT_RECORD* pRec) �� ��� ����� ������� 'ConEmuC-input-%i.log'");
void CreateLogSizeFile()
{
    if (ghLogSize) return; // ���
    
    DWORD dwErr = 0;
    wchar_t szFile[MAX_PATH+64], *pszDot;
    if (!GetModuleFileName(NULL, szFile, MAX_PATH)) {
        dwErr = GetLastError();
        wprintf(L"GetModuleFileName failed! ErrCode=0x%08X\n", dwErr);
        return; // �� �������
    }
    if ((pszDot = wcsrchr(szFile, L'.')) == NULL) {
        wprintf(L"wcsrchr failed!\n%s\n", szFile);
        return; // ������
    }
    wsprintfW(pszDot, L"-size-%i.log", gnSelfPID);
    
    ghLogSize = CreateFileW ( szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (ghLogSize == INVALID_HANDLE_VALUE) {
        ghLogSize = NULL;
        dwErr = GetLastError();
        wprintf(L"CreateFile failed! ErrCode=0x%08X\n%s\n", dwErr, szFile);
        return;
    }
    
    wpszLogSizeFile = _wcsdup(szFile);
    // OK, ��� �������
    LPCSTR pszCmdLine = GetCommandLineA();
    if (pszCmdLine) {
        WriteFile(ghLogSize, pszCmdLine, strlen(pszCmdLine), &dwErr, 0);
        WriteFile(ghLogSize, "\r\n", 2, &dwErr, 0);
    }
    LogSize(NULL, "Startup");
}

void LogSize(COORD* pcrSize, LPCSTR pszLabel)
{
    if (!ghLogSize) return;
    
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
	// � �������� ��� �������� �������� ��������
    GetConsoleScreenBufferInfo(ghConOut ? ghConOut : GetStdHandle(STD_OUTPUT_HANDLE), &lsbi);
    
    char szInfo[192] = {0};
    LPCSTR pszThread = "<unknown thread>";
    
    DWORD dwId = GetCurrentThreadId();
    if (dwId == gdwMainThreadId)
            pszThread = "MainThread";
            else
    if (dwId == srv.dwServerThreadId)
            pszThread = "ServerThread";
            else
    if (dwId == srv.dwRefreshThread)
            pszThread = "RefreshThread";
            else
    if (dwId == srv.dwWinEventThread)
            pszThread = "WinEventThread";
            else
    if (dwId == srv.dwInputThreadId)
            pszThread = "InputThread";
            
    /*HDESK hDesk = GetThreadDesktop ( GetCurrentThreadId() );
    HDESK hInp = OpenInputDesktop ( 0, FALSE, GENERIC_READ );*/
    
            
    SYSTEMTIME st; GetLocalTime(&st);
    if (pcrSize) {
        sprintf(szInfo, "%i:%02i:%02i CurSize={%ix%i} ChangeTo={%ix%i} %s %s\r\n",
            st.wHour, st.wMinute, st.wSecond,
            lsbi.dwSize.X, lsbi.dwSize.Y, pcrSize->X, pcrSize->Y, pszThread, (pszLabel ? pszLabel : ""));
    } else {
        sprintf(szInfo, "%i:%02i:%02i CurSize={%ix%i} %s %s\r\n",
            st.wHour, st.wMinute, st.wSecond,
            lsbi.dwSize.X, lsbi.dwSize.Y, pszThread, (pszLabel ? pszLabel : ""));
    }
    
    //if (hInp) CloseDesktop ( hInp );
    
    DWORD dwLen = 0;
    WriteFile(ghLogSize, szInfo, strlen(szInfo), &dwLen, 0);
    FlushFileBuffers(ghLogSize);
}

// ������� ����������� ������� � ����
int ServerInit()
{
    int iRc = 0;
    DWORD dwErr = 0;
    HANDLE hWait[2] = {NULL,NULL};
	wchar_t szComSpec[MAX_PATH+1], szSelf[MAX_PATH+1];

	TODO("����� ���������, ����� ComSpecC ��� ����?");
	if (GetEnvironmentVariable(L"ComSpec", szComSpec, MAX_PATH)) {
		wchar_t* pszSlash = wcsrchr(szComSpec, L'\\');
		if (pszSlash) {
			if (_wcsnicmp(pszSlash, L"\\conemuc.", 9)) {
				// ���� ��� �� �� - ��������� � ComSpecC
				SetEnvironmentVariable(L"ComSpecC", szComSpec);
			}
		}
	}
	if (GetModuleFileName(NULL, szSelf, MAX_PATH)) {
		SetEnvironmentVariable(L"ComSpec", szSelf);
	}

    srv.bContentsChanged = TRUE;
    srv.nMainTimerElapse = 10;
    srv.bConsoleActive = TRUE; TODO("������������ ���������� ������� Activate/Deactivate");
    srv.bNeedFullReload = TRUE; srv.bForceFullReload = FALSE;
	srv.nTopVisibleLine = -1; // ���������� ��������� �� ��������

    
    InitializeCriticalSection(&srv.csConBuf);
    InitializeCriticalSection(&srv.csProc);

    // �������� ���������� ��� ����������, ����� �� ������� ���������
    wsprintfW(srv.szPipename, CEGUIATTACHED, (DWORD)ghConWnd);
    srv.hConEmuGuiAttached = CreateEvent(NULL, TRUE, FALSE, srv.szPipename);
    _ASSERTE(srv.hConEmuGuiAttached!=NULL);
    if (srv.hConEmuGuiAttached) ResetEvent(srv.hConEmuGuiAttached);
    
    // ������������� ���� ������
    wsprintfW(srv.szPipename, CESERVERPIPENAME, L".", gnSelfPID);
    wsprintfW(srv.szInputname, CESERVERINPUTNAME, L".", gnSelfPID);

    // ������ ������ � Lucida. ����������� ��� ���������� ������.
    if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.before");
    SetConsoleFontSizeTo(ghConWnd, 4, 6);
    if (ghLogSize) LogSize(NULL, ":SetConsoleFontSizeTo.after");
    if (gbParmBufferSize && gcrBufferSize.X && gcrBufferSize.Y) {
        SMALL_RECT rc = {0};
        SetConsoleSize(gnBufferHeight, gcrBufferSize, rc, ":ServerInit.SetFromArg"); // ����� ����������? ���� ����� ��� �������
    }

    if (IsIconic(ghConWnd)) { // ������ ����� ����������!
        WINDOWPLACEMENT wplCon = {sizeof(wplCon)};
        GetWindowPlacement(ghConWnd, &wplCon);
        wplCon.showCmd = SW_RESTORE;
        SetWindowPlacement(ghConWnd, &wplCon);
    }


    // ����� �������� ������� ��������� �������
    ReloadConsoleInfo();

    //
    srv.hRefreshEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (!srv.hRefreshEvent) {
        dwErr = GetLastError();
        wprintf(L"CreateEvent(hRefreshEvent) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_REFRESHEVENT; goto wrap;
    }

    srv.hChangingSize = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (!srv.hChangingSize) {
        dwErr = GetLastError();
        wprintf(L"CreateEvent(hChangingSize) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_REFRESHEVENT; goto wrap;
    }
    SetEvent(srv.hChangingSize);

    
    // ��������� ���� ���������� �� ��������
    srv.hRefreshThread = CreateThread( 
        NULL,              // no security attribute 
        0,                 // default stack size 
        RefreshThread,     // thread proc
        NULL,              // thread parameter 
        0,                 // not suspended 
        &srv.dwRefreshThread); // returns thread ID 

    if (srv.hRefreshThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(RefreshThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CREATEREFRESHTHREAD; goto wrap;
    }
    
    
    // The client thread that calls SetWinEventHook must have a message loop in order to receive events.");
    hWait[0] = CreateEvent(NULL,FALSE,FALSE,NULL);
    _ASSERTE(hWait[0]!=NULL);
    srv.hWinEventThread = CreateThread( 
        NULL,              // no security attribute 
        0,                 // default stack size 
        WinEventThread,      // thread proc
        hWait[0],              // thread parameter 
        0,                 // not suspended 
        &srv.dwWinEventThread);      // returns thread ID 
    if (srv.hWinEventThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(WinEventThread) failed, ErrCode=0x%08X\n", dwErr); 
        SafeCloseHandle(hWait[0]);
        hWait[0]=NULL; hWait[1]=NULL;
        iRc = CERR_WINEVENTTHREAD; goto wrap;
    }
    hWait[1] = srv.hWinEventThread;
    dwErr = WaitForMultipleObjects(2, hWait, FALSE, 10000);
    SafeCloseHandle(hWait[0]);
    hWait[0]=NULL; hWait[1]=NULL;
    if (!srv.hWinHook) {
        _ASSERT(dwErr == WAIT_TIMEOUT);
        if (dwErr == WAIT_TIMEOUT) { // �� ���� ����� ���� �� ������
            #pragma warning( push )
            #pragma warning( disable : 6258 )
            TerminateThread(srv.hWinEventThread,100);
            #pragma warning( pop )
            SafeCloseHandle(srv.hWinEventThread);
        }
        // ������ �� ����� ��� ��������, ���� ��� ���������, ������� ����������
        SafeCloseHandle(srv.hWinEventThread);
        iRc = CERR_WINHOOKNOTCREATED; goto wrap;
    }

    // ��������� ���� ��������� ������  
    srv.hServerThread = CreateThread( 
        NULL,              // no security attribute 
        0,                 // default stack size 
        ServerThread,      // thread proc
        NULL,              // thread parameter 
        0,                 // not suspended 
        &srv.dwServerThreadId);      // returns thread ID 

    if (srv.hServerThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(ServerThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CREATESERVERTHREAD; goto wrap;
    }

    // ��������� ���� ��������� ������� (����������, ����, � ��.)
    srv.hInputThread = CreateThread( 
        NULL,              // no security attribute 
        0,                 // default stack size 
        InputThread,      // thread proc
        NULL,              // thread parameter 
        0,                 // not suspended 
        &srv.dwInputThreadId);      // returns thread ID 

    if (srv.hInputThread == NULL) 
    {
        dwErr = GetLastError();
        wprintf(L"CreateThread(InputThread) failed, ErrCode=0x%08X\n", dwErr); 
        iRc = CERR_CREATEINPUTTHREAD; goto wrap;
    }

    if (gbAttachMode) {
        HWND hGui = NULL, hDcWnd = NULL;
        UINT nMsg = RegisterWindowMessage(CONEMUMSG_ATTACH);
        DWORD dwStart = GetTickCount(), dwDelta = 0, dwCur = 0;
        // ���� � ������� ���� �� ��������� (GUI ��� ��� �� �����������) ������� ���
        while (!hDcWnd && dwDelta <= 5000) {
            while ((hGui = FindWindowEx(NULL, hGui, VirtualConsoleClassMain, NULL)) != NULL) {
                hDcWnd = (HWND)SendMessage(hGui, nMsg, (WPARAM)ghConWnd, (LPARAM)gnSelfPID);
                if (hDcWnd != NULL) {
                    break;
                }
            }
            if (hDcWnd) break;

            Sleep(500);
            dwCur = GetTickCount(); dwDelta = dwCur - dwStart;
        }
        if (!hDcWnd) {
            wprintf(L"Available ConEmu GUI window not found!\n");
            iRc = CERR_ATTACHFAILED; goto wrap;
        }
    }

wrap:
    return iRc;
}

// ��������� ��� ���� � ������� �����������
void ServerDone(int aiRc)
{
    // ��������� ����������� � �������
    if (srv.dwWinEventThread && srv.hWinEventThread) {
        PostThreadMessage(srv.dwWinEventThread, WM_QUIT, 0, 0);
        // �������� ��������, ���� ���� ���� ����������
        if (WaitForSingleObject(srv.hWinEventThread, 500) != WAIT_OBJECT_0) {
            #pragma warning( push )
            #pragma warning( disable : 6258 )
            TerminateThread ( srv.hWinEventThread, 100 ); // ��� ��������� �� �����...
            #pragma warning( pop )
        }
        SafeCloseHandle(srv.hWinEventThread);
    }
    if (srv.hInputThread) {
        #pragma warning( push )
        #pragma warning( disable : 6258 )
        TerminateThread ( srv.hInputThread, 100 ); TODO("������� ���������� ����������");
        #pragma warning( pop )
        SafeCloseHandle(srv.hInputThread);
    }

    if (srv.hServerThread) {
        #pragma warning( push )
        #pragma warning( disable : 6258 )
        TerminateThread ( srv.hServerThread, 100 ); TODO("������� ���������� ����������");
        #pragma warning( pop )
        SafeCloseHandle(srv.hServerThread);
    }
    if (srv.hRefreshThread) {
        if (WaitForSingleObject(srv.hRefreshThread, 100)!=WAIT_OBJECT_0) {
            _ASSERT(FALSE);
            #pragma warning( push )
            #pragma warning( disable : 6258 )
            TerminateThread(srv.hRefreshThread, 100);
            #pragma warning( pop )
        }
        SafeCloseHandle(srv.hRefreshThread);
    }
    
    if (srv.hRefreshEvent) {
        SafeCloseHandle(srv.hRefreshEvent);
    }
    if (srv.hChangingSize) {
        SafeCloseHandle(srv.hChangingSize);
    }
    if (srv.hWinHook) {
        UnhookWinEvent(srv.hWinHook); srv.hWinHook = NULL;
    }
    
    if (srv.psChars) { free(srv.psChars); srv.psChars = NULL; }
    if (srv.pnAttrs) { free(srv.pnAttrs); srv.pnAttrs = NULL; }
	if (srv.ptrLineCmp) { free(srv.ptrLineCmp); srv.ptrLineCmp = NULL; }
    DeleteCriticalSection(&srv.csConBuf);
    DeleteCriticalSection(&srv.csProc);
}



DWORD WINAPI ServerThread(LPVOID lpvParam) 
{ 
   BOOL fConnected = FALSE;
   DWORD dwInstanceThreadId = 0, dwErr = 0;
   HANDLE hPipe = NULL, hInstanceThread = NULL;
   
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and the loop is repeated. 
 
   for (;;) 
   { 
      hPipe = CreateNamedPipe( 
          srv.szPipename,               // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
          PIPE_TYPE_MESSAGE |       // message type pipe 
          PIPE_READMODE_MESSAGE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          PIPEBUFSIZE,              // output buffer size 
          PIPEBUFSIZE,              // input buffer size 
          0,                        // client time-out 
          NULL);                    // default security attribute 

      _ASSERTE(hPipe != INVALID_HANDLE_VALUE);
      
      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          dwErr = GetLastError();
          wprintf(L"CreatePipe failed, ErrCode=0x%08X\n", dwErr); 
          Sleep(50);
          //return 99;
          continue;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      if (fConnected) 
      { 
      // Create a thread for this client. 
         hInstanceThread = CreateThread( 
            NULL,              // no security attribute 
            0,                 // default stack size 
            InstanceThread,    // thread proc
            (LPVOID) hPipe,    // thread parameter 
            0,                 // not suspended 
            &dwInstanceThreadId);      // returns thread ID 

         if (hInstanceThread == NULL) 
         {
            dwErr = GetLastError();
            wprintf(L"CreateThread(Instance) failed, ErrCode=0x%08X\n", dwErr);
            Sleep(50);
            //return 0;
            continue;
         }
         else {
             SafeCloseHandle(hInstanceThread); 
         }
       } 
      else {
        // The client could not connect, so close the pipe. 
         SafeCloseHandle(hPipe); 
      }
   } 
   return 1; 
} 

DWORD WINAPI InputThread(LPVOID lpvParam) 
{ 
   BOOL fConnected, fSuccess; 
   //DWORD srv.dwServerThreadId;
   HANDLE hPipe = NULL; 
   DWORD dwErr = 0;
   
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and the loop is repeated. 
 
   for (;;) 
   { 
      hPipe = CreateNamedPipe( 
          srv.szInputname,              // pipe name 
          PIPE_ACCESS_INBOUND,      // goes from client to server only
          PIPE_TYPE_MESSAGE |       // message type pipe 
          PIPE_READMODE_MESSAGE |   // message-read mode 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          sizeof(INPUT_RECORD),     // output buffer size 
          sizeof(INPUT_RECORD),     // input buffer size 
          0,                        // client time-out
          NULL);                    // default security attribute 

      _ASSERTE(hPipe != INVALID_HANDLE_VALUE);
      
      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          dwErr = GetLastError();
          wprintf(L"CreatePipe failed, ErrCode=0x%08X\n", dwErr);
          Sleep(50);
          //return 99;
          continue;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      if (fConnected) 
      { 
          //TODO:
          DWORD cbBytesRead, cbWritten;
          INPUT_RECORD iRec; memset(&iRec,0,sizeof(iRec));
          while ((fSuccess = ReadFile( 
             hPipe,        // handle to pipe 
             &iRec,        // buffer to receive data 
             sizeof(iRec), // size of buffer 
             &cbBytesRead, // number of bytes read 
             NULL)) != FALSE)        // not overlapped I/O 
          {
              // ������������� ����������� ���������� ����
              if (iRec.EventType == 0xFFFF) {
                  SafeCloseHandle(hPipe);
                  break;
              }
              if (iRec.EventType) {
	              // ��������� ENABLE_PROCESSED_INPUT � GetConsoleMode
	              #define ALL_MODIFIERS (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)
	              #define CTRL_MODIFIERS (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)
				//if (iRec.EventType == KEY_EVENT && iRec.Event.KeyEvent.wVirtualKeyCode == 'C' &&
				//  (iRec.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED)
				// )
				//{
				//  /*DWORD dwMode = 0;
				//  HANDLE hConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
				//	  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				//  GetConsoleMode(hConIn, &dwMode);
				//  SetConsoleMode(hConIn, dwMode&~ENABLE_PROCESSED_INPUT);
				//  CloseHandle(hConIn);*/
				//
				//  PostMessage(ghConWnd, WM_KEYDOWN, 17,0x001D0001);
				//  PostMessage(ghConWnd, WM_KEYDOWN, 67,0x002E0001);
				//  PostMessage(ghConWnd, WM_KEYUP, 67,0xC02E0001);
				//  PostMessage(ghConWnd, WM_KEYUP, 17,0xC01D0001);
				//  iRec.EventType = 0;
				//}
	              if (iRec.EventType == KEY_EVENT && iRec.Event.KeyEvent.bKeyDown &&
					  (iRec.Event.KeyEvent.wVirtualKeyCode == 'C' || iRec.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
					 )
				  {
						BOOL lbRc = FALSE;
						DWORD dwEvent = (iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? CTRL_C_EVENT : CTRL_BREAK_EVENT;
					  //&& (srv.dwConsoleMode & ENABLE_PROCESSED_INPUT)

						//The SetConsoleMode function can disable the ENABLE_PROCESSED_INPUT mode for a console's input buffer, 
						//so CTRL+C is reported as keyboard input rather than as a signal. 
						// CTRL+BREAK is always treated as a signal
						if (
							(iRec.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS) &&
							((iRec.Event.KeyEvent.dwControlKeyState & ALL_MODIFIERS) 
							== (iRec.Event.KeyEvent.dwControlKeyState & CTRL_MODIFIERS))
							)
						{
							//iRec.EventType = 0;

						#ifdef xxxxxx // ��� ��������, �� � "ping" �������� ������ CTRL_BREAK_EVENT
							BOOL lbRc = FALSE;
							DWORD dwEvent =
							(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ?
								CTRL_C_EVENT : CTRL_BREAK_EVENT;
							lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);
						#endif

						#ifdef xxxxx // �� ��������
							BOOL lbRc = FALSE;
							//SendMessage(ghConWnd, WM_KEYDOWN, VK_CANCEL, 0x01460001);
							lbRc = GenerateConsoleCtrlEvent(
							(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? 0 : 1,
							0);
						#endif


						#ifdef xxxxx // ��� ���������� ������� � ConEmuC, ������� �������� ��� ComSpec
						DWORD *pdwPID = NULL;
						int nCount = GetProcessCount(&pdwPID);
						wchar_t szEvtName[128];
						for (int i=nCount-1; pdwPID && i>=0; i--) {
							wsprintf(szEvtName, (iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? CESIGNAL_C : CESIGNAL_BREAK, pdwPID[i]);
							HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, szEvtName);
							if (hEvent) {
								lbRc = TRUE; PulseEvent(hEvent); CloseHandle(hEvent); break;
							}
						}

						if (!lbRc)
						#endif

						// ����� ��������, ������� �� ��������� ������� � ������ CREATE_NEW_PROCESS_GROUP
						// ����� � ��������������� ������� (WinXP SP3) ������ �����, � ��� ���������
						// �� Ctrl-Break, �� ������� ���������� Ctrl-C
						lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);

						/*BOOL lbRc = FALSE;
						DWORD dwEvent =
						(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ?
						CTRL_C_EVENT : CTRL_BREAK_EVENT;
						lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);*/

						/*for (i = nCount-1; !lbRc && i>=0; i--) {
						if (pdwPID[i] == 0 || pdwPID[i] == gnSelfPID) continue;
						lbRc = GenerateConsoleCtrlEvent(
						(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? 0 : 1,
						pdwPID[i]);
						if (!lbRc) dwErr = GetLastError();
						}
						if (!lbRc)
						lbRc = GenerateConsoleCtrlEvent(
						(iRec.Event.KeyEvent.wVirtualKeyCode == 'C') ? 0 : 1,
						srv.dwProcessGroup);
						if (!lbRc)
						lbRc = GenerateConsoleCtrlEvent(dwEvent, 0);*/
						//iRec.EventType = 0;
						/*PostMessage(ghConWnd, WM_KEYDOWN, VK_CONTROL, 0);
						PostMessage(ghConWnd, WM_KEYDOWN, iRec.Event.KeyEvent.wVirtualKeyCode, 0);
						PostMessage(ghConWnd, WM_KEYUP, iRec.Event.KeyEvent.wVirtualKeyCode, 0);
						PostMessage(ghConWnd, WM_KEYUP, VK_CONTROL, 0);*/
					  }
		          }
              
		          if (iRec.EventType) {
	                  fSuccess = WriteConsoleInput(ghConIn, &iRec, 1, &cbWritten);
	                  _ASSERTE(fSuccess && cbWritten==1);
	              }
              }
              // next
              memset(&iRec,0,sizeof(iRec));
          }
      } 
      else 
        // The client could not connect, so close the pipe. 
         SafeCloseHandle(hPipe);
   } 
   return 1; 
} 
 
DWORD WINAPI InstanceThread(LPVOID lpvParam) 
{ 
    CESERVER_REQ in={0}, *pIn=NULL, *pOut=NULL;
    DWORD cbBytesRead, cbWritten, dwErr = 0;
    BOOL fSuccess; 
    HANDLE hPipe; 

    // The thread's parameter is a handle to a pipe instance. 
    hPipe = (HANDLE) lpvParam; 
 
    // Read client requests from the pipe. 
    memset(&in, 0, sizeof(in));
    fSuccess = ReadFile(
        hPipe,        // handle to pipe 
        &in,          // buffer to receive data 
        sizeof(in),   // size of buffer 
        &cbBytesRead, // number of bytes read 
        NULL);        // not overlapped I/O 

    if ((!fSuccess && ((dwErr = GetLastError()) != ERROR_MORE_DATA)) ||
            cbBytesRead < 12 || in.nSize < 12)
    {
        goto wrap;
    }

    if (in.nSize > cbBytesRead)
    {
        DWORD cbNextRead = 0;
        pIn = (CESERVER_REQ*)calloc(in.nSize, 1);
        if (!pIn)
            goto wrap;
        *pIn = in;
        fSuccess = ReadFile(
            hPipe,        // handle to pipe 
            ((LPBYTE)pIn)+cbBytesRead,  // buffer to receive data 
            in.nSize - cbBytesRead,   // size of buffer 
            &cbNextRead, // number of bytes read 
            NULL);        // not overlapped I/O 
        if (fSuccess)
            cbBytesRead += cbNextRead;
    }

    if (!GetAnswerToRequest(pIn ? *pIn : in, &pOut) || pOut==NULL) {
        goto wrap;
    }

    // Write the reply to the pipe. 
    fSuccess = WriteFile( 
        hPipe,        // handle to pipe 
        pOut,         // buffer to write from 
        pOut->nSize,  // number of bytes to write 
        &cbWritten,   // number of bytes written 
        NULL);        // not overlapped I/O 

    // ���������� ������
    free(pOut);

    //if (!fSuccess || pOut->nSize != cbWritten) break; 

// Flush the pipe to allow the client to read the pipe's contents 
// before disconnecting. Then disconnect the pipe, and close the 
// handle to this pipe instance. 

    FlushFileBuffers(hPipe); 
    DisconnectNamedPipe(hPipe);
wrap:
    SafeCloseHandle(hPipe); 

    return 1;
}

// �� ��������� ������ �������� �� �������� �������� - �� ����������� �� ����������
// ����������� �� �������... ��������, ��� ���������� �� ������ ����� ��������
// � ��� ��������� �����.
DWORD ReadConsoleData(CESERVER_CHAR** pCheck /*= NULL*/, BOOL* pbDataChanged /*= NULL*/)
{
    WARNING("���� ������� prcDataChanged(!=0 && !=1) - ��������� �� ������� ������ ���, � ������ ���������");
    WARNING("��� ����������� ������� - ����� �������, ��� ����� � ������ ���� ��������� � ������ �������");
    WARNING("� ������ ���� � ���� ������ ��� �� ������� - ������ �������� � ������ �������� �����");
    WARNING("���� ������� prcChanged - � ���� ��������� ������� �������� �������������, ��� ����������� ��������� � GUI");
    BOOL lbRc = TRUE, lbChanged = FALSE;
    DWORD cbDataSize = 0; // Size in bytes of ONE buffer
    srv.bContentsChanged = FALSE;
    EnterCriticalSection(&srv.csConBuf);
    //RECT rcReadRect = {0};

    USHORT TextWidth=0, TextHeight=0;
    DWORD TextLen=0;
    COORD coord;
    
    //TODO: � ����� �� srWindow ������ ����� ��������?
    TextWidth = srv.sbi.dwSize.X; //max(srv.sbi.dwSize.X, (srv.sbi.srWindow.Right - srv.sbi.srWindow.Left + 1));
	// ��� ������ ���� ���������� � CorrectVisibleRect
	//if (gnBufferHeight == 0) {
	//	// ������ ��� ��� �� ������ ������������ �� ��������� ������ BufferHeight
	//	if (srv.sbi.dwMaximumWindowSize.Y < srv.sbi.dwSize.Y)
	//		gnBufferHeight = srv.sbi.dwSize.Y; // ��� ���������� �������� �����
	//}
	if (gnBufferHeight == 0) {
		TextHeight = srv.sbi.dwSize.Y; //srv.sbi.srWindow.Bottom - srv.sbi.srWindow.Top + 1;
	} else {
		//��� ������ BufferHeight ����� ������� �� �������!
		TextHeight = gcrBufferSize.Y;
	}
    TextLen = TextWidth * TextHeight;
    if (TextLen > srv.nBufCharCount) {
        lbChanged = TRUE;
        free(srv.psChars);
        srv.psChars = (wchar_t*)calloc(TextLen*2,sizeof(wchar_t));
        _ASSERTE(srv.psChars!=NULL);
        free(srv.pnAttrs);
        srv.pnAttrs = (WORD*)calloc(TextLen*2,sizeof(WORD));
        _ASSERTE(srv.pnAttrs!=NULL);
        if (srv.psChars && srv.pnAttrs) {
            srv.nBufCharCount = TextLen;
        } else {
            srv.nBufCharCount = 0; // ������ ��������� ������
            lbRc = FALSE;
        }
    }
	if (TextWidth > srv.nLineCmpSize) {
		free(srv.ptrLineCmp);
		srv.ptrLineCmp = (WORD*)calloc(TextWidth*2,sizeof(WORD));
		_ASSERTE(srv.ptrLineCmp!=NULL);
		if (srv.ptrLineCmp) {
			srv.nLineCmpSize = TextWidth*2;
		} else {
			srv.nLineCmpSize = 0;
		}
	}
    //TODO: ����� ���-���� �� {0,srv.sbi.srWindow.Top} �������� �����?
    //coord.X = srv.sbi.srWindow.Left;
	//coord.Y = srv.sbi.srWindow.Top;
	coord.X = 0;
	coord.Y = srv.sbi.srWindow.Top;

    //TODO: ������������ ���������� ������ �� srv.bContentsChanged
    // ����� ������� - �������� srv.bContentsChanged � FALSE
    // ����� ������� ����������, 
    // ��� ������������� - ������� ����� CriticalSection
    
    if (srv.psChars && srv.pnAttrs) {
        //dwAllSize += TextWidth*TextHeight*4;
        
        // Get attributes (first) and text (second)
        // [Roman Kuzmin]
        // In FAR Manager source code this is mentioned as "fucked method". Yes, it is.
        // Functions ReadConsoleOutput* fail if requested data size exceeds their buffer;
        // MSDN says 64K is max but it does not say how much actually we can request now.
        // Experiments show that this limit is floating and it can be much less than 64K.
        // The solution below is not optimal when a user sets small font and large window,
        // but it is safe and practically optimal, because most of users set larger fonts
        // for large window and ReadConsoleOutput works OK. More optimal solution for all
        // cases is not that difficult to develop but it will be increased complexity and
        // overhead often for nothing, not sure that we really should use it.
        
        DWORD nbActuallyRead;
        if (!ReadConsoleOutputCharacter(ghConOut, srv.psChars, TextLen, coord, &nbActuallyRead) || !ReadConsoleOutputAttribute(ghConOut, srv.pnAttrs, TextLen, coord, &nbActuallyRead))
        {
            WORD* ConAttrNow = srv.pnAttrs; wchar_t* ConCharNow = srv.psChars;
            for(int y = 0; y < (int)TextHeight; y++, coord.Y++)
            {
                ReadConsoleOutputCharacter(ghConOut, ConCharNow, TextWidth, coord, &nbActuallyRead); ConCharNow += TextWidth;
                ReadConsoleOutputAttribute(ghConOut, ConAttrNow, TextWidth, coord, &nbActuallyRead); ConAttrNow += TextWidth;
            }
        }

        cbDataSize = TextLen * 2; // Size in bytes of ONE buffer

        if (!lbChanged) {
			// ��� ����� ������ srv.nBufCharCount, � �� TextLen, ����� �� ������������ ��� ������ ���
            if (memcmp(srv.psChars, srv.psChars+srv.nBufCharCount, TextLen*sizeof(wchar_t)))
                lbChanged = TRUE;
            else if (memcmp(srv.pnAttrs, srv.pnAttrs+srv.nBufCharCount, TextLen*sizeof(WORD)))
                lbChanged = TRUE;
        }

    } else {
        lbRc = FALSE;
    }

    if (pbDataChanged)
        *pbDataChanged = lbChanged;

    LeaveCriticalSection(&srv.csConBuf);
    return cbDataSize;
}

BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out)
{
    BOOL lbRc = FALSE;
    int nContentsChanged = FALSE;

    switch (in.nCmd) {
        case CECMD_GETSHORTINFO:
        case CECMD_GETFULLINFO:
        {
            if (srv.szGuiPipeName[0] == 0) { // ��������� ���� � CVirtualConsole ��� ������ ���� �������
                wsprintf(srv.szGuiPipeName, CEGUIPIPENAME, L".", (DWORD)ghConWnd); // ��� gnSelfPID
            }
            nContentsChanged = (in.nCmd == CECMD_GETFULLINFO) ? 1 : 0;

            _ASSERT(ghConOut && ghConOut!=INVALID_HANDLE_VALUE);
            if (ghConOut==NULL || ghConOut==INVALID_HANDLE_VALUE)
                return FALSE;

            *out = CreateConsoleInfo(NULL, nContentsChanged);

            lbRc = TRUE;
        } break;
        case CECMD_SETSIZE:
        {
            int nOutSize = sizeof(CESERVER_REQ) + sizeof(CONSOLE_SCREEN_BUFFER_INFO) - 1;
            *out = (CESERVER_REQ*)calloc(nOutSize,1);
            if (*out == NULL) return FALSE;
            (*out)->nCmd = 0;
            (*out)->nSize = nOutSize;
            (*out)->nVersion = CESERVER_REQ_VER;
            if (in.nSize >= (12 + sizeof(USHORT)+sizeof(COORD)+sizeof(SHORT)+sizeof(SMALL_RECT))) {
                USHORT nBufferHeight = 0;
                COORD  crNewSize = {0,0};
                SMALL_RECT rNewRect = {0};
				SHORT  nNewTopVisible = -1;
                memmove(&nBufferHeight, in.Data, sizeof(USHORT));
                memmove(&crNewSize, in.Data+sizeof(USHORT), sizeof(COORD));
				memmove(&nNewTopVisible, in.Data+sizeof(USHORT)+sizeof(COORD), sizeof(SHORT));
                memmove(&rNewRect, in.Data+sizeof(USHORT)+sizeof(COORD)+sizeof(SHORT), sizeof(SMALL_RECT));

                (*out)->nCmd = CECMD_SETSIZE;

				srv.nTopVisibleLine = nNewTopVisible;
                SetConsoleSize(nBufferHeight, crNewSize, rNewRect, ":CECMD_SETSIZE");
            }
            PCONSOLE_SCREEN_BUFFER_INFO psc = (PCONSOLE_SCREEN_BUFFER_INFO)(*out)->Data;

            MyGetConsoleScreenBufferInfo(ghConOut, psc);

            lbRc = TRUE;
        } break;
        
        case CECMD_RECREATE:
        {
	        lbRc = FALSE; // ������� ���������� �� ���������
			// ���-�� ��� �� ����� ������...
#ifdef xxxxxx
			EnterCriticalSection(&srv.csProc);
	        int i, nCount = (srv.nProcesses.size()+5); // + ��������� ������
	        _ASSERTE(nCount>0);
			if (nCount <= 0) {
				LeaveCriticalSection(&srv.csProc);
				break;
			}
	        
	        DWORD *pdwPID = (DWORD*)calloc(nCount, sizeof(DWORD));
	        _ASSERTE(pdwPID!=NULL);
	        
	        // ������� ������� ����� ������, � �� ��� ������� ������� WinEvent ����� ��������� �� vector
	        std::vector<DWORD>::iterator iter = srv.nProcesses.begin();
	        i = 0;
	        while (iter != srv.nProcesses.end() && i < nCount) {
		        DWORD dwPID = *iter;
		        if (dwPID && dwPID != gnSelfPID) {
			        pdwPID[i] = dwPID;
			    }
			    iter ++;
	        }
			LeaveCriticalSection(&srv.csProc); // ����� �� ��� ������ �� ������
	        
			gbInRecreateRoot = TRUE;

	        // ������ ����� ������
	        BOOL fSuccess = TRUE;
	        DWORD dwErr = 0;
	        for (i=nCount-1; i>=0; i--) {
		        if (pdwPID[i] == 0) continue;
		        HANDLE hProcess = OpenProcess ( PROCESS_TERMINATE, FALSE, pdwPID[i] );
		        if (hProcess == NULL) {
			        dwErr = GetLastError(); fSuccess = FALSE; break;
		        }
		        fSuccess = TerminateProcess(hProcess, 100);
		        dwErr = GetLastError();
		        CloseHandle(hProcess);
		        if (!fSuccess) break;
	        }
			free(pdwPID); pdwPID = NULL; // ������ �� ���������

	        if (fSuccess) {
		        // Clear console!
			    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
			    HANDLE hCon = ghConOut ? ghConOut : GetStdHandle(STD_OUTPUT_HANDLE);
			    if (MyGetConsoleScreenBufferInfo(hCon, &lsbi)) {
				    DWORD dwWritten = 0; COORD cr = {0,0};
				    FillConsoleOutputCharacter(hCon, L' ', lsbi.dwSize.X*lsbi.dwSize.Y, cr, &dwWritten);
				    FillConsoleOutputAttribute(hCon, 7, lsbi.dwSize.X*lsbi.dwSize.Y, cr, &dwWritten);
		        }
		        // Create process!
			    PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
			    STARTUPINFOW si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
			    wprintf(L"\r\nRestarting root process: %s\r\n", gpszRunCmd);
			    fSuccess = CreateProcessW(NULL, gpszRunCmd, NULL,NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
			    dwErr = GetLastError();
			    if (!fSuccess)
			    {
			        wprintf (L"Can't create process, ErrCode=0x%08X!\n", dwErr);
					gbAlwaysConfirmExit = TRUE; // ����� ������� ����� �� �����������, � ���� ����� ������
					SetEvent(ghExitEvent); // ����������...
			    }
	        }
#endif
        } break;
    }
    
	if (gbInRecreateRoot) gbInRecreateRoot = FALSE;
    return lbRc;
}


DWORD WINAPI WinEventThread(LPVOID lpvParam)
{
    DWORD dwErr = 0;
    HANDLE hStartedEvent = (HANDLE)lpvParam;
    
    
    // "�����" ��� ���������� �������
    srv.hWinHook = SetWinEventHook(EVENT_CONSOLE_CARET,EVENT_CONSOLE_END_APPLICATION,
        NULL, (WINEVENTPROC)WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT /*| WINEVENT_SKIPOWNPROCESS ?*/);
    dwErr = GetLastError();
    if (!srv.hWinHook) {
        dwErr = GetLastError();
        wprintf(L"SetWinEventHook failed, ErrCode=0x%08X\n", dwErr); 
        SetEvent(hStartedEvent);
        return 100;
    }
    SetEvent(hStartedEvent); hStartedEvent = NULL; // ����� �� ����� �� ���������

    MSG lpMsg;
    while (GetMessage(&lpMsg, NULL, 0, 0))
    {
        //if (lpMsg.message == WM_QUIT) { // GetMessage ���������� FALSE ��� ��������� ����� ���������
        //  lbQuit = TRUE; break;
        //}
        TranslateMessage(&lpMsg);
        DispatchMessage(&lpMsg);
    }
    
    // ������� ���
    if (srv.hWinHook) {
        UnhookWinEvent(srv.hWinHook); srv.hWinHook = NULL;
    }
    
    return 0;
}

DWORD WINAPI RefreshThread(LPVOID lpvParam)
{
    DWORD /*dwErr = 0,*/ nWait = 0;
    HANDLE hEvents[2] = {ghExitEvent, srv.hRefreshEvent};
    CONSOLE_CURSOR_INFO lci = {0}; // GetConsoleCursorInfo
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // MyGetConsoleScreenBufferInfo
    BOOL lbQuit = FALSE;

    while (!lbQuit)
    {
        nWait = WAIT_TIMEOUT;

        // ��������� ��������
        TODO("����� ����� ��������� ����� ��������, ���� ������� ���������");
        nWait = WaitForMultipleObjects ( 2, hEvents, FALSE, /*srv.bConsoleActive ? srv.nMainTimerElapse :*/ 10 );
        if (nWait == WAIT_OBJECT_0)
            break; // ����������� ���������� ����

        if (nWait == WAIT_TIMEOUT) {
            // � ���������, ������������� ��������� ������� �� �������� (���� ������� �� � ������)
            if (MyGetConsoleScreenBufferInfo(ghConOut, &lsbi)) {
                if (memcmp(&srv.sbi.dwCursorPosition, &lsbi.dwCursorPosition, sizeof(lsbi.dwCursorPosition))) {
                    nWait = (WAIT_OBJECT_0+1);
                }
            }
            if ((nWait == WAIT_TIMEOUT) && GetConsoleCursorInfo(ghConOut, &lci)) {
                if (memcmp(&srv.ci, &lci, sizeof(srv.ci))) {
                    nWait = (WAIT_OBJECT_0+1);
                }
            }
            

			#ifdef FORCE_REDRAW_FIX
            DWORD dwCurTick = GetTickCount();
            DWORD dwDelta = dwCurTick - srv.nLastUpdateTick;
            if (dwDelta > 1000) {
				TODO("�� �����-�� �������, ������, � GUI �������� �� ��� ��������� �������... ��������� ���?");
				srv.bNeedFullReload = TRUE;
				srv.bForceFullReload = TRUE;
	            srv.nLastUpdateTick = GetTickCount();
	            nWait = (WAIT_OBJECT_0+1);
            }
			#endif
        }

        if (nWait == (WAIT_OBJECT_0+1)) {
            // ����������, ���� �� ��������� � �������
            ReloadFullConsoleInfo();
        }
    }
    
    return 0;
}

//Minimum supported client Windows 2000 Professional 
//Minimum supported server Windows 2000 Server 
void WINAPI WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hwnd != ghConWnd) {
        _ASSERTE(hwnd); // �� ����, ��� ������ ���� ����� ����������� ����, ��������
        return;
    }

    //BOOL bNeedConAttrBuf = FALSE;
    CESERVER_CHAR ch = {{0,0}};
    #ifdef _DEBUG
    WCHAR szDbg[128];
    #endif

    switch(event)
    {
    case EVENT_CONSOLE_START_APPLICATION:
        //A new console process has started. 
        //The idObject parameter contains the process identifier of the newly created process. 
        //If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.

        #ifdef _DEBUG
        wsprintfW(szDbg, L"EVENT_CONSOLE_START_APPLICATION(PID=%i%s)\n", idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
        OutputDebugString(szDbg);
        #endif

        if (((DWORD)idObject) != gnSelfPID) {
            EnterCriticalSection(&srv.csProc);
            srv.nProcesses.push_back(idObject);
            LeaveCriticalSection(&srv.csProc);

            if (idChild == CONSOLE_APPLICATION_16BIT) {
                //DWORD ntvdmPID = idObject;
                dwActiveFlags |= CES_NTVDM;
                SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
            }
            //
            //HANDLE hIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
            //                  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            //if (hIn != INVALID_HANDLE_VALUE) {
            //  HANDLE hOld = ghConIn;
            //  ghConIn = hIn;
            //  SafeCloseHandle(hOld);
            //}
        }
        return; // ���������� ������ �� ���������

    case EVENT_CONSOLE_END_APPLICATION:
        //A console process has exited. 
        //The idObject parameter contains the process identifier of the terminated process.

        #ifdef _DEBUG
        wsprintfW(szDbg, L"EVENT_CONSOLE_END_APPLICATION(PID=%i%s)\n", idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
        OutputDebugString(szDbg);
        #endif

        if (((DWORD)idObject) != gnSelfPID) {
            std::vector<DWORD>::iterator iter;
            EnterCriticalSection(&srv.csProc);
            for (iter=srv.nProcesses.begin(); iter!=srv.nProcesses.end(); iter++) {
                if (((DWORD)idObject) == *iter) {
                    srv.nProcesses.erase(iter);
                    if (idChild == CONSOLE_APPLICATION_16BIT) {
                        //DWORD ntvdmPID = idObject;
                        dwActiveFlags &= ~CES_NTVDM;
                        //TODO: �������� ����� ������� ������� NTVDM?
                        SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
                    }
                    // ��������� � ������� �� ��������?
                    if (srv.nProcesses.size() == 0 && !gbInRecreateRoot) {
                        LeaveCriticalSection(&srv.csProc);
                        SetEvent(ghExitEvent);
                        return;
                    }
                    break;
                }
            }
            LeaveCriticalSection(&srv.csProc);
            //
            //HANDLE hIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
            //                  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            //if (hIn != INVALID_HANDLE_VALUE) {
            //  HANDLE hOld = ghConIn;
            //  ghConIn = hIn;
            //  SafeCloseHandle(hOld);
            //}
        }
        return; // ���������� ������ �� ���������

    case EVENT_CONSOLE_UPDATE_REGION: // 0x4002 
        {
        //More than one character has changed.
        //The idObject parameter is a COORD structure that specifies the start of the changed region. 
        //The idChild parameter is a COORD structure that specifies the end of the changed region.
            #ifdef _DEBUG
            COORD crStart, crEnd; memmove(&crStart, &idObject, sizeof(idObject)); memmove(&crEnd, &idChild, sizeof(idChild));
            wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_REGION({%i, %i} - {%i, %i})\n", crStart.X,crStart.Y, crEnd.X,crEnd.Y);
            OutputDebugString(szDbg);
            #endif
            //bNeedConAttrBuf = TRUE;
            //// ���������� ������, ��������� �������, � ��.
            //ReloadConsoleInfo();
            srv.bNeedFullReload = TRUE;
            SetEvent(srv.hRefreshEvent);
        }
        return; // ���������� �� ������� � ����

    case EVENT_CONSOLE_UPDATE_SCROLL: //0x4004
        {
        //The console has scrolled.
        //The idObject parameter is the horizontal distance the console has scrolled. 
        //The idChild parameter is the vertical distance the console has scrolled.
            #ifdef _DEBUG
            wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SCROLL(X=%i, Y=%i)\n", idObject, idChild);
            OutputDebugString(szDbg);
            #endif
            //bNeedConAttrBuf = TRUE;
            //// ���������� ������, ��������� �������, � ��.
            //if (!ReloadConsoleInfo())
            //  return;
            srv.bNeedFullReload = TRUE;
            SetEvent(srv.hRefreshEvent);
        }
        return; // ���������� �� ������� � ����

    case EVENT_CONSOLE_UPDATE_SIMPLE: //0x4003
        {
        //A single character has changed.
        //The idObject parameter is a COORD structure that specifies the character that has changed.
        //Warning! � ������� ��  ���������� ��� ������!
        //The idChild parameter specifies the character in the low word and the character attributes in the high word.
            memmove(&ch.crStart, &idObject, sizeof(idObject));
            ch.crEnd = ch.crStart;
            ch.data[0] = (WCHAR)LOWORD(idChild); ch.data[1] = HIWORD(idChild);
            #ifdef _DEBUG
            wsprintfW(szDbg, L"EVENT_CONSOLE_UPDATE_SIMPLE({%i, %i} '%c'(\\x%04X) A=%i)\n", ch.crStart.X,ch.crStart.Y, ch.data[0], ch.data[0], ch.data[1]);
            OutputDebugString(szDbg);
            #endif
            // ���������� ������, ��������� �������, � ��.
			BOOL lbLayoutChanged = ReloadConsoleInfo(TRUE);

			SHORT nYCorrected = CorrectTopVisible(ch.crStart.Y);

			// ���� ������ �� ������ � ������������ � GUI ����� - ������ ������
			if (nYCorrected < 0 || nYCorrected >= min(gcrBufferSize.Y,srv.sbi.dwSize.Y))
				return;

			int nIdx = ch.crStart.X + nYCorrected * srv.sbi.dwSize.X;
			_ASSERTE(nIdx>=0 && (DWORD)nIdx<srv.nBufCharCount);

            if (!lbLayoutChanged) { // ���� Layout �� ���������
                // ���� Reload==FALSE, � ch �� ������ ����������� �������/�������� � srv.psChars/srv.pnAttrs - ����� �����");
                if (srv.psChars[nIdx] == (wchar_t)ch.data[0] && srv.pnAttrs[nIdx] == ch.data[1])
                    return; // ������ �� ��������
            }

            // ���������
			srv.psChars[nIdx] = (wchar_t)ch.data[0];
			// ����� �� ������������ ������ ��� ����� ������ - �� ������ ������������ == srv.nBufCharCount
			srv.psChars[nIdx+srv.nBufCharCount] = (wchar_t)ch.data[0];
			srv.pnAttrs[nIdx] = (WORD)ch.data[1];
			srv.pnAttrs[nIdx+srv.nBufCharCount] = (WORD)ch.data[1];

            // � ���������
            ReloadFullConsoleInfo(&ch);
        } return;

    case EVENT_CONSOLE_CARET: //0x4001
        {
        //Warning! WinXPSP3. ��� ������� �������� ������ ���� ������� � ������. 
        //         � � ConEmu ��� ������� �� � ������, ��� ��� ������ �� �����������.
        //The console caret has moved.
        //The idObject parameter is one or more of the following values:
        //      CONSOLE_CARET_SELECTION or CONSOLE_CARET_VISIBLE.
        //The idChild parameter is a COORD structure that specifies the cursor's current position.
            #ifdef _DEBUG
            COORD crWhere; memmove(&crWhere, &idChild, sizeof(idChild));
            wsprintfW(szDbg, L"EVENT_CONSOLE_CARET({%i, %i} Sel=%c, Vis=%c\n", crWhere.X,crWhere.Y, 
                ((idObject & CONSOLE_CARET_SELECTION)==CONSOLE_CARET_SELECTION) ? L'Y' : L'N',
                ((idObject & CONSOLE_CARET_VISIBLE)==CONSOLE_CARET_VISIBLE) ? L'Y' : L'N');
            OutputDebugString(szDbg);
            #endif
            SetEvent(srv.hRefreshEvent);
            // ���������� ������, ��������� �������, � ��.
            //if (ReloadConsoleInfo())
            //  return;
        } return;

    case EVENT_CONSOLE_LAYOUT: //0x4005
        {
        //The console layout has changed.
            #ifdef _DEBUG
            OutputDebugString(L"EVENT_CONSOLE_LAYOUT\n");
            #endif
            //bNeedConAttrBuf = TRUE;
            TODO("� ����� ������ ��� ������� ����������?");
            //// ���������� ������, ��������� �������, � ��.
            //if (!ReloadConsoleInfo())
            //  return;
            srv.bNeedFullReload = TRUE;
            SetEvent(srv.hRefreshEvent);
        }
        return; // ���������� �� ������� � ����
    }


    //// ���������� ����� ����� ������ ���� CVirtualConsole ��� �������� ��������� ����
    //if (srv.szGuiPipeName[0] == 0)
    //  return;

    //CESERVER_REQ* pOut = 
    //  CreateConsoleInfo (
    //      (event == EVENT_CONSOLE_UPDATE_SIMPLE) ? &ch : NULL,
    //      FALSE/*bNeedConAttrBuf*/
    //  );
    //_ASSERTE(pOut!=NULL);
    //if (!pOut)
    //  return;

    ////Warning! WinXPSP3. EVENT_CONSOLE_CARET �������� ������ ���� ������� � ������. 
    ////         � � ConEmu ��� ������� �� � ������, ��� ��� ������ �� �����������.
    //// �.�. ���������� ���������� ������� - ��������� GUI �����
    ////if (event != EVENT_CONSOLE_CARET) { // ���� ���������� ������ ��������� ������� - ������������ ���������� �� �����
    ////    srv.bContentsChanged = TRUE;
    ////}
    ////SetEvent(hGlblUpdateEvt);

    ////WARNING("��� ��������� ���������� � GUI ����� CEGUIPIPENAME");
    ////WARNING("���� ���� �������������� � ������ CVirtualConsole �� �����������");

    //SendConsoleChanges ( pOut );

    //free ( pOut );
}

// ��������������� ������ nY ���, ����� �� ������ �������������� ������, ������������� � GUI
// ������������ ����� �������� ������� ����� ������� (��� ������� � GUI, ���� ������������� '�������������')
SHORT CorrectTopVisible(int nY)
{
	int nYCorrected = nY;
	if (srv.nTopVisibleLine != -1) {
		// ������� ������ � GUI �������������
		nYCorrected = nY - srv.nTopVisibleLine;
	} else if (srv.sbi.dwSize.Y <= gcrBufferSize.Y) {
		// ���� ������� ������ ������ �� ������ ������������ � GUI - ������ �� ��������
		nYCorrected = nY;
	} else {
		// ����� �� Y ������� (0-based) ������ ������� ������� ������
		nYCorrected = nY - srv.sbi.srWindow.Top;
	}
	return nYCorrected;
}

void SendConsoleChanges(CESERVER_REQ* pOut)
{
    //srv.szGuiPipeName ���������������� ������ ����� ����, ��� GUI ����� ����� ����������� ����
    if (srv.szGuiPipeName[0] == 0)
        return;

    HANDLE hPipe = NULL;
    DWORD dwErr = 0, dwMode = 0;
    BOOL fSuccess = FALSE;

    // Try to open a named pipe; wait for it, if necessary. 
    while (1) 
    { 
        hPipe = CreateFile( 
            srv.szGuiPipeName,  // pipe name 
            GENERIC_WRITE, 
            0,              // no sharing 
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe 
            0,              // default attributes 
            NULL);          // no template file 

        // Break if the pipe handle is valid. 
        if (hPipe != INVALID_HANDLE_VALUE) 
            break; // OK, �������

        // Exit if an error other than ERROR_PIPE_BUSY occurs. 
        dwErr = GetLastError();
        if (dwErr != ERROR_PIPE_BUSY) 
            return;

        // All pipe instances are busy, so wait for 100 ms.
        if (!WaitNamedPipe(srv.szGuiPipeName, 100) ) 
            return;
    }

    // The pipe connected; change to message-read mode. 
    dwMode = PIPE_READMODE_MESSAGE; 
    fSuccess = SetNamedPipeHandleState( 
        hPipe,    // pipe handle 
        &dwMode,  // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL);    // don't set maximum time 
    _ASSERT(fSuccess);
	if (!fSuccess) {
		CloseHandle(hPipe);
        return;
	}


    // ���������� ������ � ����
    DWORD dwWritten = 0;
    fSuccess = WriteFile ( hPipe, pOut, pOut->nSize, &dwWritten, NULL);
    _ASSERTE(fSuccess && dwWritten == pOut->nSize);
}

CESERVER_REQ* CreateConsoleInfo(CESERVER_CHAR* pCharOnly, int bCharAttrBuff)
{
    CESERVER_REQ* pOut = NULL;
    DWORD dwAllSize = sizeof(CESERVER_REQ), nSize;

    // 1
    HWND hWnd = NULL;
    dwAllSize += (nSize=sizeof(hWnd)); _ASSERTE(nSize==4);
    // 2
    // ��� ����� �������� GetTickCount(), ����� GUI �������� �� '�������' ����� ������� �������
    dwAllSize += (nSize=sizeof(DWORD));
    // 3
    // �� ����� ������ ����������� ������� ����� ����������� ���������� ���������
    // ������� �������� ���������� - �������� ����� ������ � CriticalSection(srv.csProc);
    //EnterCriticalSection(&srv.csProc);
    //DWORD dwProcCount = srv.nProcesses.size()+20;
    //LeaveCriticalSection(&srv.csProc);
    //dwAllSize += sizeof(DWORD)*(dwProcCount+1); // PID ��������� + �� ����������
    dwAllSize += (nSize=sizeof(DWORD)); // ������ ��������� ����������� � GUI, ��� ��� ���� - ������ 0 (Reserved)
    // 4
    //DWORD srv.dwSelRc = 0; CONSOLE_SELECTION_INFO srv.sel = {0}; // GetConsoleSelectionInfo
    dwAllSize += sizeof(srv.dwSelRc)+((srv.dwSelRc==0) ? (nSize=sizeof(srv.sel)) : 0);
    // 5
    //DWORD srv.dwCiRc = 0; CONSOLE_CURSOR_INFO srv.ci = {0}; // GetConsoleCursorInfo
    dwAllSize += sizeof(srv.dwCiRc)+((srv.dwCiRc==0) ? (nSize=sizeof(srv.ci)) : 0);
    // 6, 7, 8
    //DWORD srv.dwConsoleCP=0, srv.dwConsoleOutputCP=0, srv.dwConsoleMode=0;
    dwAllSize += 3*sizeof(DWORD);
    // 9
    //DWORD srv.dwSbiRc = 0; CONSOLE_SCREEN_BUFFER_INFO srv.sbi = {{0,0}}; // MyGetConsoleScreenBufferInfo
    //if (!MyGetConsoleScreenBufferInfo(ghConOut, &srv.sbi)) { srv.dwSbiRc = GetLastError(); if (!srv.dwSbiRc) srv.dwSbiRc = -1; }
    dwAllSize += sizeof(srv.dwSbiRc)+((srv.dwSbiRc==0) ? (nSize=sizeof(srv.sbi)) : 0);
    // 10
    dwAllSize += sizeof(DWORD) + (pCharOnly ? (nSize=sizeof(CESERVER_CHAR)) : 0);
    // 11
    DWORD OneBufferSize = 0;
    dwAllSize += sizeof(DWORD);
    if (bCharAttrBuff) {
        TODO("���������� ��� �������� ������ ������������� �������������� � BufferHeight");
        //if (bCharAttrBuff == 2 && OneBufferSize == (srv.nBufCharCount*2)) {
        //    _ASSERTE(srv.nBufCharCount>0);
        //    OneBufferSize = srv.nBufCharCount*2;
        //} else {
        OneBufferSize = ReadConsoleData(); // returns size in bytes of ONE buffer
		//}
		if (OneBufferSize > (200*100*2)) {
			_ASSERTE(OneBufferSize && OneBufferSize<=(200*100*2));
		}
        #ifdef _DEBUG
        if (gnBufferHeight == 0) {
            _ASSERTE(OneBufferSize == (srv.sbi.dwSize.X*srv.sbi.dwSize.Y*2));
        }
        #endif
        if (OneBufferSize) {
            dwAllSize += OneBufferSize * 2;
        }
    }

    // ��������� ������
    pOut = (CESERVER_REQ*)calloc(dwAllSize+sizeof(CESERVER_REQ), 1); // ������ ������� � ������
    _ASSERT(pOut!=NULL);
    if (pOut == NULL) {
        return FALSE;
    }

    // �������������
    pOut->nSize = dwAllSize;
    pOut->nCmd = bCharAttrBuff ? CECMD_GETFULLINFO : CECMD_GETSHORTINFO;
    pOut->nVersion = CESERVER_REQ_VER;

    // �������
    LPBYTE lpCur = (LPBYTE)(pOut->Data);

    // 1
    hWnd = GetConsoleWindow();
    _ASSERTE(hWnd == ghConWnd);
    *((DWORD*)lpCur) = (DWORD)hWnd;
    lpCur += sizeof(hWnd);

    // 2
    *((DWORD*)lpCur) = GetTickCount();
    lpCur += sizeof(DWORD);

    // 3
    // �� ����� ������ ����������� ������� ����� ����������� ���������� ���������
    // ������� �������� ���������� - �������� ����� ������ � CriticalSection(srv.csProc);
    *((DWORD*)lpCur) = 0; lpCur += sizeof(DWORD);
    //EnterCriticalSection(&srv.csProc);
    //DWORD dwTestCount = srv.nProcesses.size();
    //_ASSERTE(dwTestCount<=dwProcCount);
    //if (dwTestCount < dwProcCount) dwProcCount = dwTestCount;
    //*((DWORD*)lpCur) = dwProcCount; lpCur += sizeof(DWORD);
    //for (DWORD n=0; n<dwProcCount; n++) {
    //  *((DWORD*)lpCur) = srv.nProcesses[n];
    //  lpCur += sizeof(DWORD);
    //}
    //LeaveCriticalSection(&srv.csProc);

    // 4
    nSize=sizeof(srv.sel); *((DWORD*)lpCur) = (srv.dwSelRc == 0) ? nSize : 0; lpCur += sizeof(DWORD);
    if (srv.dwSelRc == 0) {
        memmove(lpCur, &srv.sel, nSize); lpCur += nSize;
    }

    // 5
    nSize=sizeof(srv.ci); *((DWORD*)lpCur) = (srv.dwCiRc == 0) ? nSize : 0; lpCur += sizeof(DWORD);
    if (srv.dwCiRc == 0) {
        memmove(lpCur, &srv.ci, nSize); lpCur += nSize;
    }

    // 6
    *((DWORD*)lpCur) = srv.dwConsoleCP; lpCur += sizeof(DWORD);
    // 7
    *((DWORD*)lpCur) = srv.dwConsoleOutputCP; lpCur += sizeof(DWORD);
    // 8
    *((DWORD*)lpCur) = srv.dwConsoleMode; lpCur += sizeof(DWORD);

    // 9
    //if (!MyGetConsoleScreenBufferInfo(ghConOut, &srv.sbi)) { srv.dwSbiRc = GetLastError(); if (!srv.dwSbiRc) srv.dwSbiRc = -1; }
    nSize=sizeof(srv.sbi); *((DWORD*)lpCur) = (srv.dwSbiRc == 0) ? nSize : 0; lpCur += sizeof(DWORD);
    if (srv.dwSbiRc == 0) {
        memmove(lpCur, &srv.sbi, nSize); lpCur += nSize;
    }

    // 10
    *((DWORD*)lpCur) = pCharOnly ? sizeof(CESERVER_CHAR) : 0; lpCur += sizeof(DWORD);
    if (pCharOnly) {
        memmove(lpCur, pCharOnly, sizeof(CESERVER_CHAR)); lpCur += (nSize=sizeof(CESERVER_CHAR));
    }

    // 11 - ����� ����� 0, ���� ����� � ������� �� �������
    *((DWORD*)lpCur) = OneBufferSize; lpCur += sizeof(DWORD);
    if (OneBufferSize && OneBufferSize!=(DWORD)-1) {
        memmove(lpCur, srv.psChars, OneBufferSize); lpCur += OneBufferSize;
        memmove(lpCur, srv.pnAttrs, OneBufferSize); lpCur += OneBufferSize;
    }
    
    if (ghLogSize) {
        static COORD scr_Last = {-1,-1};
        if (scr_Last.X != srv.sbi.dwSize.X || scr_Last.Y != srv.sbi.dwSize.Y) {
            LogSize(&srv.sbi.dwSize, ":CreateConsoleInfo(query only)");
            scr_Last = srv.sbi.dwSize;
        }
    }

    return pOut;
}

BOOL ReloadConsoleInfo(BOOL abSkipCursorCharCheck/*=FALSE*/)
{
    BOOL lbChanged = FALSE;
    //CONSOLE_SELECTION_INFO lsel = {0}; // GetConsoleSelectionInfo
    CONSOLE_CURSOR_INFO lci = {0}; // GetConsoleCursorInfo
    DWORD ldwConsoleCP=0, ldwConsoleOutputCP=0, ldwConsoleMode=0;
    CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}}; // MyGetConsoleScreenBufferInfo

	TODO("������-�� Selection ����� ������������ ���� � GUI, ��� ��� ��� ������ �������� �����, ��� ��������� ��������");
	srv.dwSelRc = 0; memset(&srv.sel, 0, sizeof(srv.sel));
    //if (!GetConsoleSelectionInfo(&lsel)) { srv.dwSelRc = GetLastError(); if (!srv.dwSelRc) srv.dwSelRc = -1; } else {
    //    srv.dwSelRc = 0;
    //    if (memcmp(&srv.sel, &lsel, sizeof(srv.sel))) {
    //        srv.sel = lsel;
    //        lbChanged = TRUE;
    //    }
    //}

    if (!GetConsoleCursorInfo(ghConOut, &lci)) { srv.dwCiRc = GetLastError(); if (!srv.dwCiRc) srv.dwCiRc = -1; } else {
        srv.dwCiRc = 0;
        if (memcmp(&srv.ci, &lci, sizeof(srv.ci))) {
            srv.ci = lci;
            lbChanged = TRUE;
        }
    }

    ldwConsoleCP = GetConsoleCP(); if (srv.dwConsoleCP!=ldwConsoleCP) { srv.dwConsoleCP = ldwConsoleCP; lbChanged = TRUE; }
    ldwConsoleOutputCP = GetConsoleOutputCP(); if (srv.dwConsoleOutputCP!=ldwConsoleOutputCP) { srv.dwConsoleOutputCP = ldwConsoleOutputCP; lbChanged = TRUE; }
    ldwConsoleMode=0; GetConsoleMode(ghConIn, &ldwConsoleMode); if (srv.dwConsoleMode!=ldwConsoleMode) { srv.dwConsoleMode = ldwConsoleMode; lbChanged = TRUE; }

    if (!MyGetConsoleScreenBufferInfo(ghConOut, &lsbi)) { srv.dwSbiRc = GetLastError(); if (!srv.dwSbiRc) srv.dwSbiRc = -1; } else {
        srv.dwSbiRc = 0;
        if (memcmp(&srv.sbi, &lsbi, sizeof(srv.sbi))) {
			if (srv.psChars && srv.pnAttrs && !abSkipCursorCharCheck
				&& !(srv.bForceFullReload || srv.bNeedFullReload)
				&& memcmp(&srv.sbi.dwCursorPosition, &lsbi.dwCursorPosition, sizeof(lsbi.dwCursorPosition))
				)
			{
				// � ��������� ������� �� ����������� �� EVENT_CONSOLE_UPDATE_SIMPLE �� EVENT_CONSOLE_UPDATE_REGION
				// ������. ��������� cmd.exe. �������� �����-�� ����� � ��������� ������ � �������� 'Esc'
				// ��� Esc ������� ������� ������ �� ���������, � ����� � ������� ���������!

				// ������, ���� ���� ��������� � ��������/��������� ������ �� ������� ��� ������,
				// ��� �� ������� ������ ������� - ���������� ������� ��������� - bForceFullReload=TRUE
				int nCount = min(lsbi.dwSize.X, (int)srv.nLineCmpSize);
				if (nCount && srv.ptrLineCmp) {
					DWORD nbActuallyRead = 0;
					COORD coord = {0,0};
					DWORD nBufferShift = 0;
					for (int i=0; i<=1; i++) {
						if (i==0) {
							// ������ �� ������� ��� ������
							coord.Y = srv.sbi.dwCursorPosition.Y;
							// � ������ ������ ������� ������...
							nBufferShift = coord.Y - srv.sbi.srWindow.Top;
						} else {
							// ������ �� ������� ����� ������
							if (coord.Y == lsbi.dwCursorPosition.Y) break; // ������ �� ��������, ������ �������
							coord.Y = lsbi.dwCursorPosition.Y;
							// � ������ ������ ������� ������...
							nBufferShift = coord.Y - srv.sbi.srWindow.Top;
						}
						// �����������
						if (nBufferShift < 0 || nBufferShift >= (USHORT)gcrBufferSize.Y)
							continue; // ����� �� ������� � GUI �������
						nBufferShift = nBufferShift*gcrBufferSize.X;

						if (ReadConsoleOutputCharacter(ghConOut, (wchar_t*)srv.ptrLineCmp, nCount, coord, &nbActuallyRead)) {
							if (memcmp(srv.ptrLineCmp, srv.psChars+nBufferShift, nbActuallyRead*2)) {
								srv.bForceFullReload = TRUE; break;
							}
						}
						if (ReadConsoleOutputAttribute(ghConOut, srv.ptrLineCmp, nCount, coord, &nbActuallyRead)) {
							if (memcmp(srv.ptrLineCmp, srv.pnAttrs+nBufferShift, nbActuallyRead*2)) {
								srv.bForceFullReload = TRUE; break;
							}
						}
					}
				}
			}
			if ((lsbi.srWindow.Bottom - lsbi.srWindow.Top)>lsbi.dwMaximumWindowSize.Y) {
				_ASSERTE((lsbi.srWindow.Bottom - lsbi.srWindow.Top)<lsbi.dwMaximumWindowSize.Y);
			}
            srv.sbi = lsbi;
            lbChanged = TRUE;
        }
    }

    return lbChanged;
}

BOOL MyGetConsoleScreenBufferInfo(HANDLE ahConOut, PCONSOLE_SCREEN_BUFFER_INFO apsc)
{
	BOOL lbRc = FALSE;

	lbRc = GetConsoleScreenBufferInfo(ahConOut, apsc);
	if (lbRc) {
		if (gnBufferHeight) {
			if (gnBufferHeight <= (apsc->dwMaximumWindowSize.Y * 1.2))
				gnBufferHeight = max(300, (SHORT)(apsc->dwMaximumWindowSize.Y * 1.2));
		}
		// ���� ��������� ���� �� ������ - �� ����������� ������ ��, ����� ��� ������� FAR
		// ���������� ������ � ������� �������
		if (((apsc->srWindow.Right+1) < apsc->dwSize.X)
			|| ((gnBufferHeight == 0) && ((apsc->srWindow.Bottom+1) < apsc->dwSize.Y))
			)
		{
			RECT rcConPos; GetWindowRect(ghConWnd, &rcConPos);
			int nNewWidth = GetSystemMetrics(SM_CXSCREEN);
			int nNewHeight = (gnBufferHeight != 0) ? (rcConPos.bottom - rcConPos.top) : GetSystemMetrics(SM_CYSCREEN);

			MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, nNewWidth, nNewHeight, TRUE);
			lbRc = GetConsoleScreenBufferInfo(ahConOut, apsc);
		}
		CorrectVisibleRect(apsc);
	}

	return lbRc;
}

void CorrectVisibleRect(CONSOLE_SCREEN_BUFFER_INFO* pSbi)
{
	// ���������� �������������� ���������
	pSbi->srWindow.Left = 0; pSbi->srWindow.Right = pSbi->dwSize.X - 1;
	if (gnBufferHeight == 0) {
		// ������ ��� ��� �� ������ ������������ �� ��������� ������ BufferHeight
		if (pSbi->dwMaximumWindowSize.Y < pSbi->dwSize.Y)
			gnBufferHeight = pSbi->dwSize.Y; // ��� ���������� �������� �����
	}
	// ���������� ������������ ��������� ��� �������� ������
	if (gnBufferHeight == 0) {
		pSbi->srWindow.Top = 0; pSbi->srWindow.Bottom = pSbi->dwSize.Y - 1;
	} else if (srv.nTopVisibleLine!=-1) {
		// � ��� '���������' ������ ������� ����� ���� �������������
		pSbi->srWindow.Top = srv.nTopVisibleLine;
		pSbi->srWindow.Bottom = min( (pSbi->dwSize.Y-1), (srv.nTopVisibleLine+gcrBufferSize.Y-1) );
	} else {
		// ������ ������������ ������ ������ �� ������������� � GUI �������
		TODO("������-�� ������ �� ��� ��������� ������� ���, ����� ������ ��� �����");
		pSbi->srWindow.Bottom = min( (pSbi->dwSize.Y-1), (pSbi->srWindow.Top+gcrBufferSize.Y-1) );
	}
}

void ReloadFullConsoleInfo(CESERVER_CHAR* pCharOnly/*=NULL*/)
{
    CESERVER_CHAR* pCheck = NULL;
    BOOL lbInfChanged = FALSE, lbDataChanged = FALSE;
    DWORD dwBufSize = 0;

    lbInfChanged = ReloadConsoleInfo(pCharOnly!=NULL);

    if (srv.bNeedFullReload || srv.bForceFullReload) {
		BOOL bForce = srv.bForceFullReload;
        srv.bNeedFullReload = FALSE;
		srv.bForceFullReload = FALSE;
        dwBufSize = ReadConsoleData(&pCheck, &lbDataChanged);
        if (lbDataChanged && pCheck != NULL) {
            pCharOnly = pCheck;
            lbDataChanged = FALSE; // ��������� ����������� ����� pCharOnly
		} else if (!lbDataChanged && bForce) {
			// ��� ���� ����� ����������, �� ������������� ���������� lbDataChanged
			// ��� ������, ��� �� �����-�� ������� ������ � ����� �������� ������������ ������... ���-�� ���, ���� �� �����
			lbDataChanged = TRUE;
			OutputDebugString(L"!!! Forced full console data send\n");
		}
    }

    if (lbInfChanged || lbDataChanged || pCharOnly) {
        TODO("����� ����������� ������� ������������ ������������� � ���������� ������ ���");
        CESERVER_REQ* pOut = CreateConsoleInfo(lbDataChanged ? NULL : pCharOnly, lbDataChanged ? 2 : 0);
        _ASSERTE(pOut!=NULL); if (!pOut) return;
        SendConsoleChanges(pOut);
        free(pOut);
        // ��������� ��������� ������������
        if (lbDataChanged && srv.nBufCharCount) {
            MCHKHEAP
            memmove(srv.psChars+srv.nBufCharCount, srv.psChars, srv.nBufCharCount*sizeof(wchar_t));
            memmove(srv.pnAttrs+srv.nBufCharCount, srv.pnAttrs, srv.nBufCharCount*sizeof(WORD));
            MCHKHEAP
        }
    }
    
    //
    srv.nLastUpdateTick = GetTickCount();

    if (pCheck) free(pCheck);
}

// BufferHeight  - ������ ������ (0 - ��� ���������)
// crNewSize     - ������ ���� (������ ���� == ������ ������)
// rNewRect      - ��� (BufferHeight!=0) ���������� new upper-left and lower-right corners of the window
BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel)
{
    _ASSERTE(ghConWnd);
    if (!ghConWnd) return FALSE;
    
    if (ghLogSize) LogSize(&crNewSize, asLabel);

    _ASSERTE(crNewSize.X>=MIN_CON_WIDTH && crNewSize.Y>=MIN_CON_HEIGHT);

    // �������� ������������ �������
    if (crNewSize.X</*4*/MIN_CON_WIDTH)
        crNewSize.X = /*4*/MIN_CON_WIDTH;
    if (crNewSize.Y</*3*/MIN_CON_HEIGHT)
        crNewSize.Y = /*3*/MIN_CON_HEIGHT;

    gnBufferHeight = BufferHeight;
    gcrBufferSize = crNewSize;

    RECT rcConPos = {0};
    GetWindowRect(ghConWnd, &rcConPos);

    BOOL lbRc = TRUE;
    DWORD nWait = 0;
    BOOL lbNeedChange = TRUE;
    CONSOLE_SCREEN_BUFFER_INFO csbi = {{0,0}};
    if (MyGetConsoleScreenBufferInfo(ghConOut, &csbi)) {
        lbNeedChange = (csbi.dwSize.X != crNewSize.X) || (csbi.dwSize.Y != crNewSize.Y);
    }

    if (srv.hChangingSize) { // �� ����� ������� ConEmuC
        nWait = WaitForSingleObject(srv.hChangingSize, 300);
        _ASSERTE(nWait == WAIT_OBJECT_0);
        ResetEvent(srv.hChangingSize);
    }

    // case: simple mode
    if (BufferHeight == 0)
    {
        if (lbNeedChange) {
            DWORD dwErr = 0;
            // ���� ����� �� ������� - ������ ������� ������ ���������
            MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
            //specified width and height cannot be less than the width and height of the console screen buffer's window
            lbRc = SetConsoleScreenBufferSize(ghConOut, crNewSize);
                if (!lbRc) dwErr = GetLastError();
            //TODO: � ���� ������ ������ ���� ������� �� ������� ������?
                //WARNING("�������� ��� �����");
            MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
        }

    } else {
        // ������� ������ ��� BufferHeight
		COORD crHeight = {crNewSize.X, BufferHeight};

		GetWindowRect(ghConWnd, &rcConPos);
        MoveWindow(ghConWnd, rcConPos.left, rcConPos.top, 1, 1, 1);
        lbRc = SetConsoleScreenBufferSize(ghConOut, crHeight); // � �� crNewSize - ��� "�������" �������
        //������ ���������� ������ �� ������!
		RECT rcCurConPos = {0};
		GetWindowRect(ghConWnd, &rcCurConPos); //X-Y �����, �� ������ - ������
        MoveWindow(ghConWnd, rcCurConPos.left, rcCurConPos.top, GetSystemMetrics(SM_CXSCREEN), rcConPos.bottom-rcConPos.top, 1);

		rNewRect.Left = 0;
		if (!rNewRect.Right || !rNewRect.Bottom) {
			rNewRect.Right = crHeight.X-1;
			rNewRect.Bottom = min( (crHeight.Y-1), (rNewRect.Top+gcrBufferSize.Y-1) );
		}
        SetConsoleWindowInfo(ghConOut, TRUE, &rNewRect);
    }

    if (srv.hChangingSize) { // �� ����� ������� ConEmuC
        SetEvent(srv.hChangingSize);
    }

    return lbRc;
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    /*SafeCloseHandle(ghLogSize);
    if (wpszLogSizeFile) {
        DeleteFile(wpszLogSizeFile);
        free(wpszLogSizeFile); wpszLogSizeFile = NULL;
    }*/

    return TRUE;
}

int GetProcessCount(DWORD **rpdwPID)
{
	//DWORD dwErr = 0; BOOL lbRc = FALSE;
	DWORD *pdwPID = NULL; int nCount = 0, i;
	EnterCriticalSection(&srv.csProc);
	nCount = srv.nProcesses.size();
	if (nCount > 0 && rpdwPID) {
		pdwPID = (DWORD*)calloc(nCount, sizeof(DWORD));
		_ASSERTE(pdwPID!=NULL);
		if (pdwPID) {
			std::vector<DWORD>::iterator iter = srv.nProcesses.begin();
			i = 0;
			while (iter != srv.nProcesses.end()) {
				pdwPID[i++] = *iter;
				iter ++;
			}
		}
	}
	LeaveCriticalSection(&srv.csProc);
	if (rpdwPID)
		*rpdwPID = pdwPID;
	return nCount;
}