
/*
Copyright (c) 2009-2013 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/*

Spacing chars
0020	SPACE
	* sometimes considered a control code
	* other space characters: 2000-200A
	x (no-break space - 00A0)
	x (zero width space - 200B)
	x (word joiner - 2060)
	x (ideographic space - 3000)
	x (zero width no-break space - FEFF)

200C    ZERO WIDTH NON-JOINER
200C    ZERO WIDTH NON-JOINER
200E    LEFT-TO-RIGHT MARK
2060    WORD JOINER
FEFF    ZERO WIDTH NO-BREAK SPACE


*/


/*
200E    LEFT-TO-RIGHT MARK    
    * commonly abbreviated LRM
200F    RIGHT-TO-LEFT MARK    
    * commonly abbreviated RLM
*/

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR
#ifdef _DEBUG
//#define SHOWDEBUGSTEPS
#endif

#include "Header.h"
#include <Tlhelp32.h>
#include "ScreenDump.h"
#include "VirtualConsole.h"
#include "RealConsole.h"
#include "ConEmu.h"
#include "Options.h"
#include "Background.h"
#include "ConEmuPipe.h"
#include "TabID.h"
#include "TabBar.h"
#include "TaskBarGhost.h"
#include "VConGroup.h"


#ifdef _DEBUG
#define DEBUGDRAW_RCONPOS VK_SCROLL // -- ��� ���������� ScrollLock ���������� �������������, ��������������� ��������� ���� RealConsole
#define DEBUGDRAW_DIALOGS VK_CAPITAL // -- ��� ���������� Caps ���������� ��������������, ��������������� ��������� ��������
#define DEBUGDRAW_VERTICALS VK_SCROLL // -- ��� ���������� ScrollLock ���������� ��������������, �������������� ������ �������� (������� ������ ������������� �� �������)
#endif

#define DEBUGSTRDRAW(s) //DEBUGSTR(s)
#define DEBUGSTRCOORD(s) //DEBUGSTR(s)
#define DEBUGSTRFAIL(s) DEBUGSTR(s)

// WARNING("�� ���������� ���� �� ������ �������");
WARNING("�� ���������� ������ ������� ��� �������� ������� �����");
WARNING("��� ������� ������ ������ ������ ����� '���������' �� ����� �� ����, �� ���������� ��������� ������");

WARNING("���� � ��������� ���������� ������ ����� ���������� ��� Alt-F7, Enter. ��������, ���� ����� ���");
// ������ �� �������������� ������ ������ ��������� - ������ ����� ������� ����������� ����������.
// ������ ������ �����������, �� ����� �� ������ "..." �����������

WARNING("� ������ VCon ������� ����� BYTE[256] ��� �������� ������������ ������ (Ctrl,...,Up,PgDn,Add,� ��.");
WARNING("�������������� ����� �������� � ����� {VKEY,wchar_t=0}, � ������� ��������� ��������� wchar_t �� WM_CHAR/WM_SYSCHAR");
WARNING("��� WM_(SYS)CHAR �������� wchar_t � ������, � ������ ��������� VKEY");
WARNING("��� ������������ WM_KEYUP - �����(� ������) wchar_t �� ����� ������, ������ � ������� UP");
TODO("� ������������ - ��������� �������� isKeyDown, � ������� �����");
WARNING("��� ������������ �� ������ ������� (�� �������� � � �������� ������ ������� - ����������� ����� ���� ������� � ����� ���������) ��������� �������� caps, scroll, num");
WARNING("� ����� ���������� �������/������� ��������� ����� �� �� ���������� Ctrl/Shift/Alt");

WARNING("����� ����� ��������������� ���������� ������ ������� ���������� (OK), �� ����������� ���������� ������� �� �������� � GUI - ��� ������� ���������� ������� � ������ ������");


//������, ��� ���������, ������ �������, ���������� �����, � ��...

//#define VCURSORWIDTH  /*2*/ mp_Set->CursorMinSize()
//#define HCURSORHEIGHT /*2*/ mp_Set->CursorMinSize()

//#define Assert(V) if ((V)==FALSE) { wchar_t szAMsg[MAX_PATH*2]; _wsprintf(szAMsg, SKIPLEN(countof(szAMsg)) L"Assertion (%s) at\n%s:%i", _T(#V), _T(__FILE__), __LINE__); Box(szAMsg); }

#ifdef _DEBUG
//#undef HEAPVAL
#define HEAPVAL //HeapValidate(mh_Heap, 0, NULL);
#define CURSOR_ALWAYS_VISIBLE
#else
#define HEAPVAL
#endif

#define ISBGIMGCOLOR(a) (nBgImageColors & (1 << a))

#define CURSOR_PAT_COLOR  0xC0C0C0
#define HILIGHT_PAT_COLOR 0xC0C0C0

#ifndef CONSOLE_SELECTION_NOT_EMPTY
#define CONSOLE_SELECTION_NOT_EMPTY     0x0002   // non-null select rectangle
#endif

#ifdef _DEBUG
#define DUMPDC(f) if (mb_DebugDumpDC) DumpImage((HDC)m_DC, NULL, m_DC.iWidth, m_DC.iHeight, f);
#else
#define DUMPDC(f)
#endif

//#define isCharSpace(c) (c == ucSpace || c == ucNoBreakSpace || c == 0 || (c>=0x2000 && c<=0x200F) || c == 0x2060 || c == 0x3000 || c == 0xFEFF)
//TODO("������ ��� acute � ������ ���������");
//#define isCharNonSpacing(c) (c == 0xFEFF || (c>=0x2000 && c<=0x200F) || c == 0x2060 || c == 0x3000)
//bool __inline isCharSpace(wchar_t c)
//{
//	switch (c)
//	{
//	case ucSpace:
//	case ucNoBreakSpace:
//	case 0x2000: case 0x2001: case 0x2002: case 0x2003: case 0x2004: case 0x2005:
//	case 0x2006: case 0x2007: case 0x2008: case 0x2009: case 0x200A:
//	200A
//	}
//}

namespace VConCreateLogger
{
	static const int BUFFER_SIZE = 64;   // Must be a power of 2
	enum EventType
	{
		eCreate,
		eDelete,
	} Event;
	struct VConNewDel
	{
		CVirtualConsole* pVCon;
		EventType Event;
		DWORD Tick;
	};
	VConNewDel g_pos[BUFFER_SIZE] = {{NULL}};
	LONG g_posidx = -1;

	void Log(CVirtualConsole* pVCon, EventType Event)
	{
		// Get next message index
		LONG i = _InterlockedIncrement(&g_posidx);
		// Write a message at this index
		VConNewDel& e = g_pos[i & (BUFFER_SIZE - 1)]; // Wrap to buffer size

		e.pVCon = pVCon;
		e.Event = Event;
		e.Tick = GetTickCount(); // msg.time == 0 ������ �����
	}
}



CVirtualConsole::PARTBRUSHES CVirtualConsole::m_PartBrushes[MAX_COUNT_PART_BRUSHES] = {{0}};
char CVirtualConsole::mc_Uni2Oem[0x10000];
// MAX_SPACES == 0x400
wchar_t CVirtualConsole::ms_Spaces[MAX_SPACES];
wchar_t CVirtualConsole::ms_HorzDbl[MAX_SPACES];
wchar_t CVirtualConsole::ms_HorzSingl[MAX_SPACES];
//HMENU CVirtualConsole::mh_PopupMenu = NULL;
//HMENU CVirtualConsole::mh_TerminatePopup = NULL;
//HMENU CVirtualConsole::mh_DebugPopup = NULL;
//HMENU CVirtualConsole::mh_EditPopup = NULL;

//--> VConGroup
//CVirtualConsole* CVirtualConsole::CreateVCon(RConStartArgs *args)
//{
//	if (!args)
//	{
//		_ASSERTE(args!=NULL);
//		return NULL;
//	}
//
//	if (args->pszSpecialCmd)
//	{
//		args->ProcessNewConArg();
//	}
//
//	if (args->bForceUserDialog)
//	{
//		_ASSERTE(args->aRecreate!=cra_RecreateTab);
//		args->aRecreate = cra_CreateTab;
//
//		int nRc = gpConEmu->RecreateDlg(args);
//		if (nRc != IDC_START)
//			return NULL;
//	}
//
//	CVirtualConsole* pCon = new CVirtualConsole(args);
//
//	if (!pCon->mp_RCon->PreCreate(args))
//	{
//		delete pCon;
//		return NULL;
//	}
//
//	return pCon;
//}

CVirtualConsole::CVirtualConsole()
	: mp_RCon(NULL)
	, mp_Ghost(NULL)
	, mp_Group(NULL)
	, m_DC(NULL)
{
	VConCreateLogger::Log(this, VConCreateLogger::eCreate);
	mh_WndDC = NULL;
}

bool CVirtualConsole::Constructor(RConStartArgs *args)
{
	//mh_WndDC = NULL;
	//CreateView();
	//mp_RCon = NULL; //new CRealConsole(this);
	//mp_Ghost = NULL;
	//mp_Group = NULL;

	mp_Set = NULL; // ��������� �� ��������� ����������� �� �����������

	#ifdef __GNUC__
	HMODULE hGdi32 = GetModuleHandle(L"gdi32.dll");
	GdiAlphaBlend = (AlphaBlend_t)(hGdi32 ? GetProcAddress(hGdi32, "GdiAlphaBlend") : NULL);
	#endif

	gpConEmu->OnVConCreated(this, args);

	WARNING("��������������� ������ ����");
	SIZE_T nMinHeapSize = (1000 + (200 * 90 * 2) * 6 + MAX_PATH*2)*2 + 210*sizeof(*TextParts);
	mh_Heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, nMinHeapSize, 0);
	cinf.dwSize = 15; cinf.bVisible = TRUE;
	ZeroStruct(winSize); ZeroStruct(coord);
	TextLen = 0;
	mb_RequiredForceUpdate = true;
	mb_LastFadeFlag = false;
	mn_LastBitsPixel = 0;
	//mb_NeedBgUpdate = FALSE; mb_BgLastFade = false;
	mp_Bg = new CBackground();
	#ifdef APPDISTINCTBACKGROUND
	mp_BgInfo = args->pszWallpaper ? CBackgroundInfo::CreateBackgroundObject(args->pszWallpaper, false) : NULL;
	#endif
	//mp_BkImgData = NULL; mn_BkImgDataMax = 0; mb_BkImgChanged = FALSE; mb_BkImgExist = /*mb_BkImgDelete =*/ FALSE;
	//mp_BkEmfData = NULL; mn_BkEmfDataMax = 0; mb_BkEmfChanged = FALSE;
	//mcs_BkImgData = NULL;
	//mn_BkImgWidth = mn_BkImgHeight = 0;
	_ASSERTE(sizeof(mh_FontByIndex) == (sizeof(gpSetCls->mh_Font)+sizeof(mh_FontByIndex[0])));
	// mh_FontByIndex[MAX_FONT_STYLES] // ��������������� ��� 'Unicode CharMap'
	memmove(mh_FontByIndex, gpSetCls->mh_Font, MAX_FONT_STYLES*sizeof(mh_FontByIndex[0])); //-V512
	mh_UCharMapFont = NULL; ms_LastUCharMapFont[0] = 0;
	mh_FontByIndex[MAX_FONT_STYLES] = NULL; // ��������������� ��� 'Unicode CharMap'
	memset(&TransparentInfo, 0, sizeof(TransparentInfo));
	isFade = false; isForeground = true;
	mp_Colors = gpSet->GetColors(-1);
	memset(&m_LeftPanelView, 0, sizeof(m_LeftPanelView));
	memset(&m_RightPanelView, 0, sizeof(m_RightPanelView));
	mn_ConEmuFadeMsg = /*mn_ConEmuSettingsMsg =*/ 0;
	// ��� ���������� ��������������� � TRUE, ���� ��� ��������� Redraw ����� �������� ������ �������
	mb_LeftPanelRedraw = mb_RightPanelRedraw = FALSE;
	mn_LastDialogsCount = 0;
	memset(mrc_LastDialogs, 0, sizeof(mrc_LastDialogs));
	mn_DialogsCount = 0; mn_DialogAllFlags = 0;
	memset(mrc_Dialogs, 0, sizeof(mrc_Dialogs));
	memset(mn_DialogFlags, 0, sizeof(mn_DialogFlags));
	//InitializeCriticalSection(&csDC); ncsTDC = 0;
	//mb_PaintRequested = FALSE;
	//mb_PaintLocked = FALSE;
	//InitializeCriticalSection(&csCON); ncsTCON = 0;
	mb_InPaintCall = FALSE;
	mb_InConsoleResize = FALSE;
	nFontHeight = gpSetCls->FontHeight();
	nFontWidth = gpSetCls->FontWidth();
	nFontCharSet = gpSetCls->FontCharSet();
	nLastNormalBack = 255;
	mb_ConDataChanged = false;
	mh_TransparentRgn = NULL;
	mb_ChildWindowWasFound = false;
#ifdef _DEBUG
	mn_BackColorIdx = 2; // Green
#else
	mn_BackColorIdx = 0; // Black
#endif
	memset(&Cursor, 0, sizeof(Cursor));
	Cursor.nBlinkTime = GetCaretBlinkTime();
	TextWidth = TextHeight = Width = Height = nMaxTextWidth = nMaxTextHeight = 0;
	LastPadSize = 0;

	_ASSERTE((HDC)m_DC == NULL);
	hBgDc = NULL; bgBmpSize.X = bgBmpSize.Y = 0;

	hSelectedFont = NULL; hOldFont = NULL;
	PointersInit();
	mb_IsForceUpdate = false;
	mb_InUpdate = false;
	hBrush0 = NULL; hSelectedBrush = NULL; hOldBrush = NULL;
	isEditor = false;
	memset(&csbi, 0, sizeof(csbi)); mdw_LastError = 0;

	//nSpaceCount = 1000;
	//Spaces = (wchar_t*)Alloc(nSpaceCount,sizeof(wchar_t));
	//for (UINT i=0; i<nSpaceCount; i++) Spaces[i]=L' ';
	if (!ms_Spaces[0])
	{
		wmemset(ms_Spaces, L' ', MAX_SPACES);
		wmemset(ms_HorzDbl, ucBoxDblHorz, MAX_SPACES);
		wmemset(ms_HorzSingl, ucBoxSinglHorz, MAX_SPACES);
	}

	hOldBrush = NULL;
	hOldFont = NULL;

	RECT rcConAll;
	if (gpConEmu->WndWidth.Value && gpConEmu->WndHeight.Value)
	{
		//if (gpConEmu->wndWidth)  TextWidth = gpConEmu->wndWidth;
		//if (gpConEmu->wndHeight) TextHeight = gpConEmu->wndHeight;

		rcConAll = gpConEmu->CalcRect(CER_CONSOLE_ALL);
		if (rcConAll.right && rcConAll.bottom)
		{
			TextWidth = rcConAll.right;
			TextHeight = rcConAll.bottom;
		}
	}
	else
	{
		Assert(gpConEmu->WndWidth.Value && gpConEmu->WndHeight.Value);
	}

	ZeroStruct(m_HighlightInfo);
	m_HighlightInfo.m_Last.X = m_HighlightInfo.m_Last.Y = -1;
	m_HighlightInfo.m_Cur.X = m_HighlightInfo.m_Cur.Y = -1;

	DEBUGTEST(mb_DebugDumpDC = false);

	//if (gpSet->isShowBgImage)
	//    gpSet->LoadBackgroundFile(gpSet->sBgImage);

	if (gpSetCls->isAdvLogging != 3)
	{
		mpsz_LogScreen = NULL;
	}
	else
	{
		mn_LogScreenIdx = 0;
		//DWORD dwErr = 0;
		wchar_t szFile[MAX_PATH+64], *pszDot;
		wcscpy_c(szFile, gpConEmu->ms_ConEmuExe);

		if ((pszDot = wcsrchr(szFile, L'\\')) == NULL)
		{
			DisplayLastError(L"wcsrchr failed!");
			return false; // ������
		}

		*pszDot = 0;
		mpsz_LogScreen = (wchar_t*)calloc(pszDot - szFile + 64, 2);
		wcscpy(mpsz_LogScreen, szFile);
		wcscpy(mpsz_LogScreen+_tcslen(mpsz_LogScreen), L"\\ConEmu-VCon-%i-%i.con"/*, RCon()->GetServerPID()*/);
	}

	CreateView();
	mp_RCon = new CRealConsole();
	_ASSERTE(mp_RCon);
	return mp_RCon->Construct(this, args);
	
	//if (gpSet->isTabsOnTaskBar())
	//{
	//	mp_Ghost = CTaskBarGhost::Create(this);
	//}

	// InitDC ����� ������������ - ������� ��� �� �������
}

CVirtualConsole::~CVirtualConsole()
{
	if (!gpConEmu->isMainThread())
	{
		//_ASSERTE(gpConEmu->isMainThread());
		MBoxA(L"~CVirtualConsole() called from background thread");
	}

	OnDestroy();

	if (mp_RCon)
		mp_RCon->StopThread();

	HEAPVAL;


	m_DC.DeleteDC();


	PointersFree();

	// ���� ������ �� �����
	if (mh_Heap)
	{
		HeapDestroy(mh_Heap);
		mh_Heap = NULL;
	}

	SafeFree(mpsz_LogScreen);

	//DeleteCriticalSection(&csDC);
	//DeleteCriticalSection(&csCON);

	if (mh_TransparentRgn)
	{
		DeleteObject(mh_TransparentRgn);
		mh_TransparentRgn = NULL;
	}

	if (mh_UCharMapFont)
	{
		DeleteObject(mh_UCharMapFont);
		mh_UCharMapFont = NULL;
	}

	SafeDelete(mp_RCon);

	CVConGroup::OnVConDestroyed(this);

	//if (mh_PopupMenu) { -- static �� ��� ����������
	//	DestroyMenu(mh_PopupMenu); mh_PopupMenu = NULL;
	//}

	//MSectionLock SC;

	//if (mcs_BkImgData)
	//	SC.Lock(mcs_BkImgData, TRUE);

	//if (mp_BkImgData)
	//{
	//	free(mp_BkImgData);
	//	mp_BkImgData = NULL;
	//	mn_BkImgDataMax = 0;
	//}

	//if (mp_BkEmfData)
	//{
	//	free(mp_BkEmfData);
	//	mp_BkEmfData = NULL;
	//	mn_BkImgDataMax = 0;
	//}
	//
	//if (mcs_BkImgData)
	//{
	//	SC.Unlock();
	//	delete mcs_BkImgData;
	//	mcs_BkImgData = NULL;
	//}

	SafeDelete(mp_Bg);
	#ifdef APPDISTINCTBACKGROUND
	SafeRelease(mp_BgInfo);
	#endif

	//FreeBackgroundImage();

	VConCreateLogger::Log(this, VConCreateLogger::eDelete);
}

void CVirtualConsole::InitGhost()
{
	if (gpSet->isTabsOnTaskBar())
	{
		if (mp_Ghost)
		{
			_ASSERTE(mp_Ghost==NULL);
		}
		else if (gpConEmu->isMainThread())
		{
			mp_Ghost = CTaskBarGhost::Create(this);
		}
		else
		{
			// Ghost-���� ����� ��������� � ������� ����
			// ���� �� �������� ��� ������� ������ (��������, ��� "-new_console")
			gpConEmu->CreateGhostVCon(this);
		}
	}
}

// WM_DESTROY
void CVirtualConsole::OnDestroy()
{
	if (this && mp_Ghost)
	{
		CTaskBarGhost* p = mp_Ghost;
		mp_Ghost = NULL;
		delete p;
	}
}

CRealConsole* CVirtualConsole::RCon()
{
	if (this)
		return mp_RCon;
	_ASSERTE(this!=NULL);
	return NULL;
}

HWND CVirtualConsole::GuiWnd()
{
	if (this && mp_RCon)
		return mp_RCon->GuiWnd();
	_ASSERTE(this!=NULL);
	return NULL;
}

// ���������� �� WM_SETFOCUS � Child & Back
// �.�. ����� ������ ���� ��� � ������� ���� ConEmu ��� � ChildGUI
void CVirtualConsole::setFocus()
{
	if (!this)
		return;

	HWND hGuiWnd = GuiWnd();
	if (hGuiWnd && ::IsWindowVisible(hGuiWnd))
	{
		RCon()->GuiWndFocusRestore();
	}
	else
	{
		gpConEmu->setFocus();
	}
}

HWND CVirtualConsole::GhostWnd()
{
	if (this && mp_Ghost)
		return mp_Ghost->GhostWnd();
	return NULL;
}

bool CVirtualConsole::isVisible()
{
	return gpConEmu->isVisible(this);
}

int CVirtualConsole::GetTabCount()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return 0;
	}
	if (!mp_RCon)
	{
		return 1;
	}
	return mp_RCon->GetTabCount();
}

int CVirtualConsole::GetActiveTab()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return 0;
	}
	if (!mp_RCon)
	{
		return 0;
	}
	return mp_RCon->GetActiveTab();
}

bool CVirtualConsole::GetTab(int tabIdx, /*OUT*/ CTab* pTab)
{
	if (!this || !mp_RCon)
	{
		_ASSERTE(this && mp_RCon);
		return false;
	}

	ConEmuTab tab = {};
	if (!GetTab(tabIdx, &tab))
		return false;

	CTabID* id = new CTabID(this, *tab.Name ? tab.Name : gpConEmu->GetDefaultTitle(), tab.Type & fwt_TypeMask/*�����*/, 0/*anPID*/, tab.Pos/*anFarWindowID*/, tab.EditViewId, tab.Type);
	pTab->Init(id);
	//id->Release(); -- ???
	return true;
}

bool CVirtualConsole::GetTab(int tabIdx, /*OUT*/ ConEmuTab* pTab)
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return false;
	}
	if (!mp_RCon)
	{
		pTab->Pos = 0; pTab->Current = 1; pTab->Type = 1; pTab->Modified = 0;
		lstrcpyn(pTab->Name, gpConEmu->GetDefaultTitle(), countof(pTab->Name));
		return true;
	}
	return mp_RCon->GetTab(tabIdx, pTab);
}

void CVirtualConsole::PointersInit()
{
	mb_PointersAllocated = false;
	mpsz_ConChar = mpsz_ConCharSave = NULL;
	mpn_ConAttrEx = mpn_ConAttrExSave = NULL;
	ConCharX = ConCharDX = NULL;
	tmpOem = NULL;
	TextParts = NULL;
	BgParts = NULL;
	PolyText = NULL;
	pbLineChanged = pbBackIsPic = NULL;
	pnBackRGB = NULL;
}

void CVirtualConsole::PointersFree()
{
#ifdef SafeFree
#undef SafeFree
#endif
#define SafeFree(f) if (f) { Free(f); f = NULL; }
	HEAPVAL;
	SafeFree(mpsz_ConChar);
	SafeFree(mpsz_ConCharSave);
	SafeFree(mpn_ConAttrEx);
	SafeFree(mpn_ConAttrExSave);
	SafeFree(ConCharX);
	SafeFree(ConCharDX);
	SafeFree(tmpOem);
	SafeFree(TextParts);
	SafeFree(BgParts);
	SafeFree(PolyText);
	SafeFree(pbLineChanged);
	SafeFree(pbBackIsPic);
	SafeFree(pnBackRGB);
	HEAPVAL;
	mb_PointersAllocated = false;
}

bool CVirtualConsole::PointersAlloc()
{
	mb_PointersAllocated = false;
	HEAPVAL;

	uint nWidthHeight = (nMaxTextWidth * nMaxTextHeight);
#ifdef AllocArray
#undef AllocArray
#endif
#define AllocArray(p,t,c) p = (t*)Alloc(c,sizeof(t)); if (!p) return false;
	AllocArray(mpsz_ConChar, wchar_t, nWidthHeight+1); // ASCIIZ
	AllocArray(mpsz_ConCharSave, wchar_t, nWidthHeight+1); // ASCIIZ
	AllocArray(mpn_ConAttrEx, CharAttr, nWidthHeight);
	AllocArray(mpn_ConAttrExSave, CharAttr, nWidthHeight);
	AllocArray(ConCharX, DWORD, nWidthHeight);
	AllocArray(ConCharDX, DWORD, nMaxTextWidth); // ����� ��� TEXTPARTS
	AllocArray(tmpOem, char, nMaxTextWidth);
	AllocArray(TextParts, TEXTPARTS, (nMaxTextWidth + 1));
	AllocArray(BgParts, BGPARTS, nMaxTextWidth);
	AllocArray(PolyText, POLYTEXT, nMaxTextWidth);
	AllocArray(pbLineChanged, bool, nMaxTextHeight);
	AllocArray(pbBackIsPic, bool, nMaxTextHeight);
	AllocArray(pnBackRGB, COLORREF, nMaxTextHeight);
	HEAPVAL;

	mb_PointersAllocated = true;
	return true;
}

void CVirtualConsole::PointersZero()
{
	uint nWidthHeight = (nMaxTextWidth * nMaxTextHeight);
	//100607: ���������� ��� �� �����. ��� ������� ����� �������������� � � ������ �����!
	HEAPVAL;
	//ZeroMemory(mpsz_ConChar, nWidthHeight*sizeof(*mpsz_ConChar));
	ZeroMemory(mpsz_ConCharSave, (nWidthHeight+1)*sizeof(*mpsz_ConChar)); // ASCIIZ
	HEAPVAL;
	//ZeroMemory(mpn_ConAttrEx, nWidthHeight*sizeof(*mpn_ConAttrEx));
	ZeroMemory(mpn_ConAttrExSave, nWidthHeight*sizeof(*mpn_ConAttrExSave));
	HEAPVAL;
	//ZeroMemory(ConCharX, nWidthHeight*sizeof(*ConCharX));
	ZeroMemory(ConCharDX, nMaxTextWidth*sizeof(*ConCharDX)); // ����� ��� TEXTPARTS
	HEAPVAL;
	ZeroMemory(tmpOem, nMaxTextWidth*sizeof(*tmpOem));
	HEAPVAL;
	ZeroMemory(TextParts, (nMaxTextWidth + 1)*sizeof(*TextParts));
	ZeroMemory(BgParts, nMaxTextWidth*sizeof(*BgParts));
	ZeroMemory(PolyText, nMaxTextWidth*sizeof(*PolyText));
	HEAPVAL;
	ZeroMemory(pbLineChanged, nMaxTextHeight*sizeof(*pbLineChanged));
	ZeroMemory(pbBackIsPic, nMaxTextHeight*sizeof(*pbBackIsPic));
	ZeroMemory(pnBackRGB, nMaxTextHeight*sizeof(*pnBackRGB));
	HEAPVAL;
}


// InitDC ���������� ������ ��� ����������� ���������� (�������, �����, � �.�.) ����� ����� ����������� DC � Bitmap
bool CVirtualConsole::InitDC(bool abNoDc, bool abNoWndResize, MSectionLock *pSDC, MSectionLock *pSCON)
{
	if (!this || !mp_RCon)
	{
		_ASSERTE(mp_RCon != NULL);
		return false;
	}
	_ASSERTE(gpConEmu->isMainThread());

	MSectionLock _SCON;
	if (!(pSCON ? pSCON : &_SCON)->isLocked())
		(pSCON ? pSCON : &_SCON)->Lock(&csCON);
	BOOL lbNeedCreateBuffers = FALSE;
	uint rTextWidth = mp_RCon->TextWidth();
	uint rTextHeight = mp_RCon->TextHeight();

	if (!rTextWidth || !rTextHeight)
	{
		WARNING("���� ��� ������ - ����� ������ DC Initialization failed, ��� �� �������...");
		Assert(mp_RCon->TextWidth() && mp_RCon->TextHeight());
		return false;
	}

#ifdef _DEBUG
	// ������ - ����������� � ������� ����
	//if (mp_RCon->IsConsoleThread())
	//{
	//    //_ASSERTE(!mp_RCon->IsConsoleThread());
	//}
	//if (TextHeight == 24)
	//    TextHeight = 24;
	_ASSERT(TextHeight >= MIN_CON_HEIGHT);
#endif

	// ����� ����������� ������ ���� ��������� ��� ����������
	if (!mb_PointersAllocated ||
	        (nMaxTextWidth * nMaxTextHeight) < (rTextWidth * rTextHeight) ||
	        (nMaxTextWidth < rTextWidth) // � ��� ����� ��� TextParts & tmpOem
	  )
		lbNeedCreateBuffers = TRUE;

	if (lbNeedCreateBuffers && mb_PointersAllocated)
	{
		DEBUGSTRDRAW(L"Relocking SCON exclusively\n");
		(pSCON ? pSCON : &_SCON)->RelockExclusive();
		DEBUGSTRDRAW(L"Relocking SCON exclusively (done)\n");
		PointersFree();
	}

	// InitDC ���������� ������ ��� ������ ������������� ��� ����� �������
	mb_IsForceUpdate = true; // ������� ���������� ������ Force
	TextWidth = rTextWidth;
	TextHeight = rTextHeight;

	if (lbNeedCreateBuffers)
	{
		// ���� ����������� ������ - �� � �������, ����� "��� ���� �� ������"...
		if (nMaxTextWidth < TextWidth)
			nMaxTextWidth = TextWidth+80;

		if (nMaxTextHeight < TextHeight)
			nMaxTextHeight = TextHeight+30;

		DEBUGSTRDRAW(L"Relocking SCON exclusively\n");
		(pSCON ? pSCON : &_SCON)->RelockExclusive();
		DEBUGSTRDRAW(L"Relocking SCON exclusively (done)\n");

		if (!PointersAlloc())
		{
			WARNING("���� ��� ������ - ����� ������ DC Initialization failed, ��� �� �������...");
			return false;
		}
	}
	else
	{
		PointersZero();
	}

	if (!pSCON)
		_SCON.Unlock();
	HEAPVAL
	hSelectedFont = NULL;

	bool lbRc = false;

	// ��� ����� ����, ���� ��������� ����������� (debug) � ����� ���� ����� �� �����
	if (!abNoDc)
	{
		DEBUGSTRDRAW(L"*** Recreate DC\n");
		//MSectionLock _SDC;
		//// ���� � ���� ���� ��� ������������ - ������ �� ���������
		//(pSDC ? pSDC : &_SDC)->Lock(&csDC, TRUE, 200); // �� �� ��������, ����� �� ������� ���������

		m_DC.DeleteDC();


		Assert(gpSetCls->FontWidth() && gpSetCls->FontHeight());

		nFontHeight = gpSetCls->FontHeight();
		nFontWidth = gpSetCls->FontWidth();
		nFontCharSet = gpSetCls->FontCharSet();

		DEBUGTEST(BOOL lbWasInitialized = TextWidth && TextHeight);

		// ��������� ����� ������ � ��������
		Width = TextWidth * nFontWidth;
		Height = TextHeight * nFontHeight;

		UINT Pad = min(gpSet->nCenterConsolePad,CENTERCONSOLEPAD_MAX)*2 + max(nFontHeight,nFontWidth)*3;

		#ifdef _DEBUG
		if (Height > 2000)
		{
			_ASSERTE(Height <= 2000);
		}
		#endif

		if (ghOpWnd)
			gpConEmu->UpdateSizes();

		if (m_DC.CreateDC(Width+Pad, Height+Pad))
		{
			// ��������� ���-���
			LastPadSize = gpSet->nCenterConsolePad;
			// OK
			lbRc = true;
		}

		goto wrap;
	}

	lbRc = true;
wrap:
	// �������, ��� �� ���������
	isFontSizeChanged = false;

	return lbRc;
}

void CVirtualConsole::DumpConsole()
{
	if (!this) return;

	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	wchar_t temp[MAX_PATH+5];
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghWnd;
	ofn.lpstrFilter = _T("ConEmu dumps (*.con)\0*.con\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = temp; temp[0] = 0;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Save console dump...";
	ofn.lpstrDefExt = L"con";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (!GetSaveFileName(&ofn))
		return;

	Dump(temp);
}

bool CVirtualConsole::Dump(LPCWSTR asFile)
{
	if (!this || !mp_RCon)
		return FALSE;
	
	// ��� ������� ������ ������ ������ (hDC) � png ����
	DumpImage((HDC)m_DC, NULL, m_DC.iWidth, m_DC.iHeight, asFile);

	HANDLE hFile = CreateFile(asFile, GENERIC_WRITE, FILE_SHARE_READ,
	                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		DisplayLastError(_T("Can't create file!"));
		return FALSE;
	}

	DWORD dw;
	LPCTSTR pszTitle = gpConEmu->GetLastTitle();
	WriteFile(hFile, pszTitle, _tcslen(pszTitle)*sizeof(*pszTitle), &dw, NULL);
	wchar_t temp[100];
	swprintf(temp, _T("\r\nSize: %ix%i   Cursor: %ix%i\r\n"), TextWidth, TextHeight, Cursor.x, Cursor.y);
	WriteFile(hFile, temp, _tcslen(temp)*sizeof(wchar_t), &dw, NULL);
	WriteFile(hFile, mpsz_ConChar, TextWidth * TextHeight * sizeof(*mpsz_ConChar), &dw, NULL);
	WriteFile(hFile, mpn_ConAttrEx, TextWidth * TextHeight * sizeof(*mpn_ConAttrEx), &dw, NULL);
	WriteFile(hFile, mpsz_ConCharSave, TextWidth * TextHeight * sizeof(*mpsz_ConCharSave), &dw, NULL);
	WriteFile(hFile, mpn_ConAttrExSave, TextWidth * TextHeight * sizeof(*mpn_ConAttrExSave), &dw, NULL);

	if (mp_RCon)
	{
		mp_RCon->DumpConsole(hFile);
	}

	CloseHandle(hFile);
	return TRUE;
}

bool CVirtualConsole::LoadDumpConsole()
{
	if (!this || !mp_RCon) return false;

	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	wchar_t temp[MAX_PATH+5];
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghWnd;
	ofn.lpstrFilter = _T("ConEmu dumps (*.con)\0*.con\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = temp; temp[0] = 0;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Load console dump...";
	ofn.lpstrDefExt = L"con";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (!GetOpenFileName(&ofn)
		|| !mp_RCon->LoadDumpConsole(temp))
	{
		if (mp_RCon->GetActiveBufferType() == rbt_DumpScreen)
			mp_RCon->SetActiveBuffer(rbt_Primary);
		return false;
	}

	return true;
}

// ��� ������� ����� � ��. ����. �������
//#define isCharBorder(inChar) (inChar>=0x2013 && inChar<=0x266B)
bool CVirtualConsole::isCharBorder(wchar_t inChar)
{
	// ���� - ����� ��������� gpSet->isFixFarBorders ��� ��� ��� ���������, ����� ��������� gpSet->isEnhanceGraphics
	//if (!gpSet->isFixFarBorders)
	//	return false;
	return gpSet->isCharBorder(inChar);
	//Settings::CharRanges *pcr = gpSet->icFixFarBorderRanges;
	//for (int i = 10; --i && pcr->bUsed; pcr++) {
	//	Settings::CharRanges cr = *pcr;
	//	if (inChar>=cr.cBegin && inChar<=cr.cEnd)
	//		return true;
	//}
	//return false;
	//if (inChar>=0x2013 && inChar<=0x266B)
	//    return true;
	//else
	//    return false;
	//if (gpSet->isFixFarBorders)
	//{
	//  //if (! (inChar > 0x2500 && inChar < 0x251F))
	//  if ( !(inChar > 0x2013/*En dash*/ && inChar < 0x266B/*Beamed Eighth Notes*/) /*&& inChar!=L'}'*/ )
	//      /*if (inChar != 0x2550 && inChar != 0x2502 && inChar != 0x2551 && inChar != 0x007D &&
	//      inChar != 0x25BC && inChar != 0x2593 && inChar != 0x2591 && inChar != 0x25B2 &&
	//      inChar != 0x2562 && inChar != 0x255F && inChar != 0x255A && inChar != 0x255D &&
	//      inChar != 0x2554 && inChar != 0x2557 && inChar != 0x2500 && inChar != 0x2534 && inChar != 0x2564) // 0x2520*/
	//      return false;
	//  else
	//      return true;
	//}
	//else
	//{
	//  if (inChar < 0x01F1 || inChar > 0x0400 && inChar < 0x045F || inChar > 0x2012 && inChar < 0x203D || /*? - not sure that optimal*/ inChar > 0x2019 && inChar < 0x2303 || inChar > 0x24FF && inChar < 0x266C)
	//      return false;
	//  else
	//      return true;
	//}
}

// � ��� ������ "��������" �������, � ������� ���� ����� (���� �� ���������) ������������ ����� + �������/���������
bool CVirtualConsole::isCharBorderVertical(wchar_t inChar)
{
	//if (inChar>=0x2502 && inChar<=0x25C4 && inChar!=0x2550)
	//2009-07-12 ��� ��������� - ������� �������� ����� �� ����������� �������������� �����
	//if (inChar==ucBoxSinglVert || inChar==0x2503 || inChar==0x2506 || inChar==0x2507
	//    || (inChar>=0x250A && inChar<=0x254B) || inChar==0x254E || inChar==0x254F
	//    || (inChar>=0x2551 && inChar<=0x25C5)) // �� ������ �������� Arial Unicode MS
	TODO("ucBoxSinglHorz �������� �� �����?");

	if ((inChar != ucBoxDblHorz && (inChar >= ucBoxSinglVert && inChar <= ucBoxDblVertHorz))
		|| (inChar == ucArrowUp || inChar == ucArrowDown))
		return true;
	else
		return false;
}

bool CVirtualConsole::isCharProgress(wchar_t inChar)
{
	bool isProgress = (inChar == ucBox25 || inChar == ucBox50 || inChar == ucBox75 || inChar == ucBox100);
	return isProgress;
}

bool CVirtualConsole::isCharScroll(wchar_t inChar)
{
	bool isScrollbar = (inChar == ucBox25 || inChar == ucBox50 || inChar == ucBox75 || inChar == ucBox100
	                    || inChar == ucUpScroll || inChar == ucDnScroll);
	return isScrollbar;
}

bool CVirtualConsole::isCharNonSpacing(wchar_t inChar)
{
	// ����� ���������� �� �������, ������� ������ �������� ������ � �������� �������.
	// ��������, 0xFEFF �� ��������� ������� ������ ����������� GDI �� �����-�� ����� ����� O_O
	// �� � ��, ��� ��� "�� �������� �����" � ������� �����������. ���� ���� ��������, �� ����,
	// ������ ������������� ��� ������, �������� ��� ���� �� ����� �� ���, �.�. �� ��������
	// ���������� � �������!
	switch (inChar)
	{
		case 0x135F:
		case 0x2060:
		case 0x3000:
		case 0x3099:
		case 0x309A:
		case 0xA66F:
		case 0xA670:
		case 0xA671:
		case 0xA672:
		case 0xA67C:
		case 0xA67D:
		case 0xFEFF:
			return true;

		default:
			if (inChar>=0x0300)
			{
				if (inChar<=0x2DFF)
				{
					if ((inChar<=0x036F) // Combining/Accent/Acute/NonSpacing
						|| (inChar>=0x2000 && inChar<=0x200F)
						|| (inChar>=0x202A && inChar<=0x202E)
						|| (inChar>=0x0483 && inChar<=0x0489)
						|| (inChar>=0x07EB && inChar<=0x07F3)
						|| (inChar>=0x1B6B && inChar<=0x1B73)
						|| (inChar>=0x1DC0 && inChar<=0x1DFF)
						|| (inChar>=0x20D0 && inChar<=0x20F0)
						|| (inChar>=0x2DE0))
					{
						return true;
					}
				}
				else if (inChar>=0xFE20 && inChar<=0xFE26)
				{
					return true;
				}
			}
	}
	return false;
	
	/*
	wchar_t CharList[] = {0x135F, 0xFEFF, 0};
	__asm {
		MOV  ECX, ARRAYSIZE(CharList)
		REPZ SCAS CharList
	}
	*/
}

bool CVirtualConsole::isCharSpace(wchar_t inChar)
{
	TODO("0x0020,0x00A0,0x1680,0x180E,0x2000,0x2001,0x2002,0x2003,0x2004,0x2005,0x2006,0x2007,0x2008,0x2009,0x200A,0x200b,0x200c,0x200d,0x205F,0x2060,0x3000,0xFEFF,0x00B7,0x237D,0x2420,0x2422,0x2423");
	// ���� ������ ��� �������, ������� ����� ���������� ������ ����� (��� ������� ������)
	bool isSpace = (inChar == ucSpace || inChar == ucNoBreakSpace || inChar == 0 
		/*|| (inChar>=0x2000 && inChar<=0x200F)
		|| inChar == 0x2060 || inChar == 0x3000 || inChar == 0xFEFF*/);
	return isSpace;
}

bool CVirtualConsole::isCharRTL(wchar_t inChar)
{
	bool isRtl =
	(inChar==0x05BE)||(inChar==0x05C0)||(inChar==0x05C3)||(inChar==0x05C6)||
	((inChar>=0x05D0)&&(inChar<=0x05F4))||
	(inChar==0x0608)||(inChar==0x060B)||(inChar==0x060D)||
	((inChar>=0x061B)&&(inChar<=0x064A))||
	((inChar>=0x066D)&&(inChar<=0x066F))||
	((inChar>=0x0671)&&(inChar<=0x06D5))||
	((inChar>=0x06E5)&&(inChar<=0x06E6))||
	((inChar>=0x06EE)&&(inChar<=0x06EF))||
	((inChar>=0x06FA)&&(inChar<=0x0710))||
	((inChar>=0x0712)&&(inChar<=0x072F))||
	((inChar>=0x074D)&&(inChar<=0x07A5))||
	((inChar>=0x07B1)&&(inChar<=0x07EA))||
	((inChar>=0x07F4)&&(inChar<=0x07F5))||
	((inChar>=0x07FA)&&(inChar<=0x0815))||
	(inChar==0x081A)||(inChar==0x0824)||(inChar==0x0828)||
	((inChar>=0x0830)&&(inChar<=0x0858))||
	((inChar>=0x085E)&&(inChar<=0x08AC))||
	(inChar==0x200F)||(inChar==0xFB1D)||
	((inChar>=0xFB1F)&&(inChar<=0xFB28))||
	((inChar>=0xFB2A)&&(inChar<=0xFD3D))||
	((inChar>=0xFD50)&&(inChar<=0xFDFC))||
	((inChar>=0xFE70)&&(inChar<=0xFEFC))
	//((inChar>=0x10800)&&(inChar<=0x1091B))||
	//((inChar>=0x10920)&&(inChar<=0x10A00))||
	//((inChar>=0x10A10)&&(inChar<=0x10A33))||
	//((inChar>=0x10A40)&&(inChar<=0x10B35))||
	//((inChar>=0x10B40)&&(inChar<=0x10C48))||
	//((inChar>=0x1EE00)&&(inChar<=0x1EEBB))
	;
  return isRtl;
}


void CVirtualConsole::PaintBackgroundImage(const RECT& rcText, const COLORREF crBack)
{
	WARNING("CVirtualConsole::PaintBackgroundImage - crBack ������������");

	const int inX = rcText.left;
	const int inY = rcText.top;
	const int inX2 = rcText.right;
	const int inY2 = rcText.bottom;
	const int inWidth = inX2 - inX;
	const int inHeight = inY2 - inY;

	#ifdef _DEBUG
	BOOL lbDump = FALSE;
	if (lbDump) DumpImage(hBgDc, NULL, bgBmpSize.X, bgBmpSize.Y, L"F:\\bgtemp.png");
	#endif

	RECT rcFill1 = {0}, rcFill2 = {0};
	#if 0
	RECT rcFill3 = {0}, rcFill4 = {0};
	#endif
	int bgX = inX, bgY = inY;

	BackgroundOp op = (BackgroundOp)gpSet->bgOperation;

	if (op == eUpRight)
	{
		int xShift = max(0,((int)Width - bgBmpSize.X));
		bgX = inX - xShift; bgY = inY;

		if ((bgBmpSize.X < (int)Width) || (bgBmpSize.Y < (int)Height))
		{
			if (inY < bgBmpSize.Y)
			{
				rcFill1 = MakeRect(inX, inY, min(inX2,xShift), min(bgBmpSize.Y,inY2));
				rcFill2 = MakeRect(inX, max(inY,bgBmpSize.Y), min(inX2,xShift), inY2);
			}
			else
			{
				rcFill1 = MakeRect(inX, inY, inX2, inY2);
			}
		}
	}
	else if (op == eDownLeft)
	{
		int yShift = max(0,((int)Height - bgBmpSize.Y));
		bgX = inX; bgY = inY - yShift;

		if ((bgBmpSize.X < (int)Width) || (bgBmpSize.Y < (int)Height))
		{
			if (inY2 >= yShift)
			{
				rcFill1 = MakeRect(inX, inY, inX2, min(yShift,inY2));
				rcFill2 = MakeRect(bgBmpSize.X, max(inY,yShift), inX2, inY2);
			}
			else
			{
				rcFill1 = MakeRect(inX, inY, inX2, inY2);
			}
		}

	}
	else if (op == eDownRight)
	{
		int xShift = max(0,((int)Width - bgBmpSize.X));
		int yShift = max(0,((int)Height - bgBmpSize.Y));
		bgX = inX - xShift; bgY = inY - yShift;

		if ((bgBmpSize.X < (int)Width) || (bgBmpSize.Y < (int)Height))
		{
			rcFill1 = MakeRect(inX, inY, inX2, min(yShift,inY2));
			rcFill2 = MakeRect(inX, max(inY,yShift), min(inX2,xShift), inY2);
		}

	}
	else if ((op == eFit) || (op == eFill) || (op == eCenter))
	{
		int xShift = ((int)Width - bgBmpSize.X)/2;
		int yShift = ((int)Height - bgBmpSize.Y)/2;
		bgX = inX - xShift; bgY = inY - yShift;

		if (op == eFit)
		{
			if (xShift > 0)
			{
				rcFill1 = MakeRect(inX, inY, min(inX2,xShift), inY2);
				if (inX2 > (bgBmpSize.X + xShift))
					rcFill2 = MakeRect(xShift+bgBmpSize.X, inY, inX2, inY2);
			}
			else if (yShift > 0)
			{
				rcFill1 = MakeRect(inX, inY, inX2, min(inY2,yShift));
				if (inY2 > (bgBmpSize.Y + yShift))
					rcFill2 = MakeRect(inX, yShift+bgBmpSize.Y, inX2, inY2);
			}
		}
		else if (op == eCenter)
		{
			WARNING("OPTIMIZE!");
		#if 1
			if (inX < xShift || inY < yShift || inX2 > (bgBmpSize.X + xShift) || inY2 > (bgBmpSize.Y + yShift))
				rcFill1 = rcText;
		#else
			LPRECT prc[] = {&rcFill1, &rcFill2, &rcFill3, &rcFill4}; int iRct = 0;

			// Full?
			if (((yShift > 0) && ((yShift >= inY2) || (inY >= (bgBmpSize.Y + yShift)))) // All above or below
				|| ((xShift > 0) && ((xShift >= inX2) || (inX >= (bgBmpSize.X + xShift))))) // All leftward or rightward
			{
				*(prc[iRct++]) = MakeRect(inX, inY, inX2, min(inY2,yShift));
			}
			else
			{
				// Parts
				if (xShift > 0)
				{
					*(prc[iRct++]) = MakeRect(inX, inY, min(inX2,xShift), inY2);

					if (inX2 > (bgBmpSize.X + xShift))
					{
						if (iRct < countof(prc))
							*(prc[iRct++]) = MakeRect(xShift+bgBmpSize.X, inY, inX2, inY2);
						else
							_ASSERTE(iRct<=1);
					}
				}
				if (yShift > 0)
				{
					if (iRct < countof(prc))
						*(prc[iRct++]) = MakeRect(inX, inY, inX2, min(inY2,yShift));
					else
						_ASSERTE(iRct<=1);

					if (inY2 > (bgBmpSize.Y + yShift))
					{
						if (iRct < countof(prc))
							*(prc[iRct++]) = MakeRect(inX, yShift+bgBmpSize.Y, inX2, inY2);
						else
							_ASSERTE(iRct<=1);
					}
				}
			}
		#endif
		}
	}
	else
	{
		//if (bgBmpSize.X>inX && bgBmpSize.Y>inY)
		//{
		//	BitBlt((HDC)m_DC, inX, inY, inWidth, inHeight, hBgDc, inX, inY, SRCCOPY);
		//}

		// ������� ������ (��� ��� ��� ��������)
		if ((bgBmpSize.X < (inX+inWidth)) || (bgBmpSize.Y < (inY+inHeight)))
		{
			//if (hBrush0 == NULL)
			//{
			//	hBrush0 = CreateSolidBrush(mp_Colors[0]);
			//	SelectBrush(hBrush0);
			//}

			rcFill1 = MakeRect(max(inX,bgBmpSize.X), inY, inX+inWidth, inY+inHeight);

			//#ifndef SKIP_ALL_FILLRECT
			//if (!IsRectEmpty(&rect))
			//{
			//	FillRect((HDC)m_DC, &rect, hBrush0);
			//}
			//#endif

			if (bgBmpSize.X>inX)
			{
				//rect.left = inX; rect.top = max(inY,bgBmpSize.Y); rect.right = bgBmpSize.X;
				rcFill2 = MakeRect(inX, max(inY,bgBmpSize.Y), bgBmpSize.X, inY+inHeight);

				//#ifndef SKIP_ALL_FILLRECT
				//if (!IsRectEmpty(&rect))
				//{
				//	FillRect((HDC)m_DC, &rect, hBrush0);
				//}
				//#endif
			}
		}
	}

	for (int i = 0; i <= 1; i++)
	{
		if (((i == 0) && (op != eCenter)) || ((i != 0) && (op == eCenter)))
		{
			// Bacground part
			if (bgBmpSize.X>bgX && bgBmpSize.Y>bgY)
			{
				BitBlt((HDC)m_DC, inX, inY, inWidth, inHeight, hBgDc, bgX, bgY, SRCCOPY);
			}
		}
		else
		{
			// Fill with color#0 (if background image is not large enough)
			HBRUSH hBr = NULL;
			#if 0
			hBr = PartBrush(L' ', crBack, 0);
			#else
			if (hBrush0 == NULL)
			{
				hBrush0 = CreateSolidBrush(mp_Colors[0]);
				//SelectBrush(hBrush0);
			}
			hBr = hBrush0;
			#endif

			#ifndef SKIP_ALL_FILLRECT
			if (!IsRectEmpty(&rcFill1))
			{
				FillRect((HDC)m_DC, &rcFill1, hBrush0);
			}

			if (!IsRectEmpty(&rcFill2))
			{
				FillRect((HDC)m_DC, &rcFill2, hBrush0);
			}

			#if 0
			if (!IsRectEmpty(&rcFill3))
			{
				FillRect((HDC)m_DC, &rcFill3, hBrush0);
			}

			if (!IsRectEmpty(&rcFill4))
			{
				FillRect((HDC)m_DC, &rcFill4, hBrush0);
			}
			#endif
			#endif
		}
	}
}

void CVirtualConsole::SelectFont(CEFONT hNew)
{
	if (!hNew.IsSet())
	{
		if (hOldFont.IsSet())
		{
			m_DC.SelectObject(hOldFont);
		}

		hOldFont = NULL;
		hSelectedFont = NULL;
	}
	else if (hSelectedFont != hNew)
	{
		#ifdef _DEBUG
		if (hNew == mh_UCharMapFont)
			hNew = mh_UCharMapFont;
		#endif

		hSelectedFont = m_DC.SelectObject(hNew);

		if (!hOldFont.IsSet())
			hOldFont = hSelectedFont;

		hSelectedFont = hNew;
	}
}

void CVirtualConsole::SelectBrush(HBRUSH hNew)
{
	if (!hNew)
	{
		if (hOldBrush)
		{
			SelectObject((HDC)m_DC, hOldBrush);
		}

		hOldBrush = NULL;
	}
	else if (hSelectedBrush != hNew)
	{
		hSelectedBrush = (HBRUSH)SelectObject((HDC)m_DC, hNew);

		if (!hOldBrush)
			hOldBrush = hSelectedBrush;

		hSelectedBrush = hNew;
	}
}

bool CVirtualConsole::CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col)
{
	TODO("��������� ���� �� �����");
	return false;
	//if ((select.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) == 0)
	//    return false;
	//if (row < select.srSelection.Top || row > select.srSelection.Bottom)
	//    return false;
	//if (col < select.srSelection.Left || col > select.srSelection.Right)
	//    return false;
	//return true;
}

#ifdef MSGLOGGER
class DcDebug
{
	public:
		DcDebug(HDC* ahDcVar, HDC* ahPaintDC)
		{
			mb_Attached=FALSE; mh_OrigDc=NULL; mh_DcVar=NULL; mh_Dc=NULL;

			if (!ahDcVar || !ahPaintDC) return;

			mh_DcVar = ahDcVar;
			mh_OrigDc = *ahDcVar;
			mh_Dc = *ahPaintDC;
			*mh_DcVar = mh_Dc;
		};
		~DcDebug()
		{
			if (mb_Attached && mh_DcVar)
			{
				mb_Attached = FALSE;
				*mh_DcVar = mh_OrigDc;
			}
		};
	protected:
		BOOL mb_Attached;
		HDC mh_OrigDc, mh_Dc;
		HDC* mh_DcVar;
};
#endif

// ������������� ���������� �������� ������� (���+�����) � ����� ���� � ������
// (!) ���������� ������ ������ ����� ���� ��������� �� ���� ������ ����� ����
// atr ������������� � BYTE, �.�. ������� ����� ��� �� ����������
//void CVirtualConsole::GetCharAttr(WORD atr, BYTE& foreColorNum, BYTE& backColorNum, HFONT* pFont)
//{
//	// ��� ���������� ������ ������ ����
//    atr &= 0xFF;
//
//    // ������� ����� ������� ������� ������ � �������, � �������� ������ �� �������
//    foreColorNum = m_ForegroundColors[atr];
//    backColorNum = m_BackgroundColors[atr];
//    if (pFont) *pFont = mh_FontByIndex[atr];
//
//    if (bExtendColors) {
//        if (backColorNum == nExtendColor) {
//            backColorNum = attrBackLast; // ��� ����� �������� �� ������� ���� �� �������� ������
//            foreColorNum += 0x10;
//        } else {
//            attrBackLast = backColorNum; // �������� ������� ���� �������� ������
//        }
//    }
//}

void CVirtualConsole::CharABC(wchar_t ch, ABC *abc)
{
	BOOL lbCharABCOk;

	WARNING("�� �������� � *.bdf?");

	if (!gpSetCls->CharABC[ch].abcB)
	{
		if (isCharRTL(ch))
		{
			WARNING("��������� � RTL ��� ���������� ������, ���� ������� ����� ����������");
			gpSetCls->CharABC[ch].abcA = gpSetCls->CharABC[ch].abcC = 0;
			gpSetCls->CharABC[ch].abcB = nFontWidth;
		}
		else
		{
			if (gpSetCls->mh_Font2.IsSet() && gpSet->isFixFarBorders && isCharBorder(ch))
			{
				SelectFont(gpSetCls->mh_Font2);
			}
			else
			{
				TODO("��� ���� �� ������� �� ������ �������");
				SelectFont(gpSetCls->mh_Font[0]);
			}

			//This function succeeds only with TrueType fonts
			lbCharABCOk = GetCharABCWidths((HDC)m_DC, ch, ch, &gpSetCls->CharABC[ch]);

			if (!lbCharABCOk)
			{
				// ������ ����� �� TTF/OTF
				gpSetCls->CharABC[ch].abcB = CharWidth(ch);
				_ASSERTE(gpSetCls->CharABC[ch].abcB);

				if (!gpSetCls->CharABC[ch].abcB) gpSetCls->CharABC[ch].abcB = 1;

				gpSetCls->CharABC[ch].abcA = gpSetCls->CharABC[ch].abcC = 0;
			}
		}
	}

	*abc = gpSetCls->CharABC[ch];
}

// ���������� ������ �������, ��������� FixBorders
WORD CVirtualConsole::CharWidth(wchar_t ch)
{
	// ��������� �����, ����� �� �������� �� ������
	WORD nWidth = gpSetCls->CharWidth[ch];

	if (nWidth)
		return nWidth;

	if (gpSet->isMonospace
	        || (gpSet->isFixFarBorders && isCharBorder(ch))
	        || (gpSet->isEnhanceGraphics && isCharProgress(ch))
	  )
	{
		//2009-09-09 ��� �����������. ������ ������ ����� ����� ���� ������ ����������
		//return gpSet->BorderFontWidth();
		gpSetCls->CharWidth[ch] = nFontWidth;
		return nFontWidth;
	}

	//nWidth = nFontWidth;
	//bool isBorder = false; //, isVBorder = false;

	// �������� ��� �� ����� ������� ������ � ��� ������, ������� ����� ���� �����������
	if (gpSetCls->mh_Font2.IsSet() && gpSet->isFixFarBorders && isCharBorder(ch))
	{
		SelectFont(gpSetCls->mh_Font2);
	}
	else
	{
		TODO("��� ���� �� ������� �� ������ �������");
		SelectFont(gpSetCls->mh_Font[0]);
	}

	//SelectFont(gpSetCls->mh_Font[0]);
	SIZE sz;
	//This function succeeds only with TrueType fonts
	//#ifdef _DEBUG
	//ABC abc;
	//BOOL lb1 = GetCharABCWidths((HDC)m_DC, ch, ch, &abc);
	//#endif

	if (m_DC.GetTextExtentPoint32(&ch, 1, &sz) && sz.cx)
		nWidth = sz.cx;
	else
		nWidth = nFontWidth;

	if (!nWidth)
		nWidth = 1; // �� ������ ������, ����� ������� �� 0 �� ��������

	gpSetCls->CharWidth[ch] = nWidth;
	return nWidth;
}

char CVirtualConsole::Uni2Oem(wchar_t ch)
{
	if (mc_Uni2Oem[ch])
		return mc_Uni2Oem[ch];

	char c = '?';
	WideCharToMultiByte(CP_OEMCP, 0, &ch, 1, &c, 1, 0, 0);
	mc_Uni2Oem[ch] = c;
	return c;
}

bool CVirtualConsole::CheckChangedTextAttr()
{
	bool lbChanged = false;
	RECT conLocked;
	int nLocked = IsDcLocked(&conLocked);

	if ((isForce || m_HighlightInfo.mb_ChangeDetected) && !nLocked)
	{
		lbChanged = textChanged = attrChanged = true;
	}
	else
	{
		textChanged = 0!=memcmp(mpsz_ConChar, mpsz_ConCharSave, TextLen * sizeof(*mpsz_ConChar));
		attrChanged = 0!=memcmp(mpn_ConAttrEx, mpn_ConAttrExSave, TextLen * sizeof(*mpn_ConAttrEx));
		
		if ((lbChanged = (textChanged || attrChanged)))
		{
			if (nLocked)
			{
				// ����� ���������, ���� �� ��������� � conLocked
				if ((UINT)conLocked.right >= TextWidth || (UINT)conLocked.bottom >= TextHeight)
				{
					// ��, �� ����, ��� ������ ��� ���� ������� ��� ������� �������
					_ASSERTE((UINT)conLocked.right < TextWidth && (UINT)conLocked.bottom < TextHeight);
					LockDcRect(false);
				}
				else
				{
					wchar_t* pSpaces = NULL;
					bool lbLineChanged;
					size_t nChars = conLocked.right - conLocked.left + 1;
					if (nLocked == 1)
					{
						pSpaces = (wchar_t*)malloc((nChars+1)*sizeof(*pSpaces));
						wmemset(pSpaces, L' ', nChars);
						pSpaces[nChars] = 0;
					}

					for (int Y = conLocked.top; Y <= conLocked.bottom; Y++)
					{
						size_t nShift = Y * TextWidth + conLocked.left;
						lbLineChanged = 0!=memcmp(mpsz_ConChar+nShift, mpsz_ConCharSave+nShift, nChars * sizeof(*mpsz_ConChar));

						if (lbLineChanged)
						{
							if (nLocked == 1)
							{
								// ������������ ���������� ���������
								if (0==memcmp(mpsz_ConChar+nShift, pSpaces, nChars * sizeof(*mpsz_ConChar)))
									continue;
							}
							// ��� ����� ���������� - �����
							LockDcRect(false);
							break;
						}

						TODO("����� �� ��� � �������� ���������, �� ��� ����������� ������ ���������");
					}

					if (pSpaces)
						free(pSpaces);
				}
			}
		}
		else
		{
			lbChanged = isForce;
		}
	}

	return lbChanged;

//#ifdef MSGLOGGER
//    COORD ch;
//    if (textChanged) {
//        for (UINT i=0; i<TextLen; i++) {
//            if (mpsz_ConChar[i] != mpsz_ConCharSave[i]) {
//                ch.Y = i % TextWidth;
//                ch.X = i - TextWidth * ch.Y;
//                break;
//            }
//        }
//    }
//    if (attrChanged) {
//        for (UINT i=0; i<TextLen; i++) {
//            if (mpn_ConAttr[i] != mpn_ConAttrSave[i]) {
//                ch.Y = i % TextWidth;
//                ch.X = i - TextWidth * ch.Y;
//                break;
//            }
//        }
//    }
//#endif
//	return textChanged || attrChanged;
}

void CVirtualConsole::UpdateThumbnail(bool abNoSnapshoot /*= FALSE*/)
{
	if (!this)
		return;

	if (!mp_Ghost || !gpConEmu->Taskbar_GhostSnapshootRequired())
		return;

	// ������ ����� �� �� � ������� ����, �� ��� ���� ����������
	mp_Ghost->UpdateTabSnapshoot(FALSE, abNoSnapshoot);
}

bool CVirtualConsole::Update(bool abForce, HDC *ahDc)
{
	if (!this || !mp_RCon || !mp_RCon->ConWnd())
		return false;

	if (abForce)
		mp_RCon->ResetActiveAppSettingsId();

	isForce = abForce;

	if (!gpConEmu->isMainThread())
	{
		if (isForce)
			mb_RequiredForceUpdate = true;

		//if (mb_RequiredForceUpdate || updateText || updateCursor)
		{
			if (gpConEmu->isVisible(this))
			{
				if (gpSetCls->isAdvLogging>=3) mp_RCon->LogString("Invalidating from CVirtualConsole::Update.1");

				gpConEmu->Invalidate(this);
			}

			return true;
		}
		return false;
	}

	/* -- 2009-06-20 ���������. ������� ������ �������� ������ � MainThread
	if (!mp_RCon->IsConsoleThread()) {
	    if (!ahDc) {
	        mp_RCon->SetForceRead();
	        mp_RCon->WaitEndUpdate(1000);
	        //gpConEmu->InvalidateAll(); -- ����� �� All??? Update � ��� Invalidate �����
	        return false;
	    }
	}
	*/

	if (mb_InUpdate)
	{
		// �� ������ ���������� �� ��������
		if (!mb_InConsoleResize)
		{
			_ASSERTE(!mb_InUpdate);
		}

		return false;
	}

	MSetter inUpdate(&mb_InUpdate);
	// ������ ���������� ���������� �� CRealConsole. ����� ������� � ������� �� ��������.
	//RetrieveConsoleInfo();
	#if defined(_DEBUG) && defined(MSGLOGGER)
	DcDebug dbg(&m_DC.hDC, ahDc); // ��� ������� - ������ ����� �� ������� ����
	#endif

	HEAPVAL
	bool lRes = false;
	MSectionLock SCON; SCON.Lock(&csCON);
	//MSectionLock SDC; //&csDC, &ncsTDC);
	_ASSERTE(gpConEmu->isMainThread());

	//if (mb_PaintRequested) -- �� ������ ����. ��� ������� �������� ������ � ���������� ����
	//if (mb_PaintLocked)  // ������ ���� ����������� PaintVCon (BitBlt) - ��� ����� ���� �� ����� �������, ��� ��� ������� ���-�� ���������
	//	SDC.Lock(&csDC, TRUE, 200); // �� �� ��������, ����� �� ������� ���������

	mp_RCon->GetConsoleScreenBufferInfo(&csbi);
	// start timer before "Read Console Output*" calls, they do take time
	//gpSetCls->Performance(tPerfRead, FALSE);
	//if (gbNoDblBuffer) isForce = TRUE; // Debug, dblbuffer
	isForeground = (gpConEmu->InQuakeAnimation() || gpConEmu->isMeForeground(false))
		&& gpConEmu->isActive(this);

	if (isFade == isForeground && gpSet->isFadeInactive)
		isForce = true;

	//------------------------------------------------------------------------
	///| Read console output and cursor info... |/////////////////////////////
	//------------------------------------------------------------------------
	if (!UpdatePrepare(ahDc, NULL/*&SDC*/, &SCON))
	{
		gpConEmu->DebugStep(_T("DC initialization failed!"));
		return false;
	}

	//gpSetCls->Performance(tPerfRead, TRUE);
	gpSetCls->Performance(tPerfRender, FALSE);
	//------------------------------------------------------------------------
	///| Drawing text (if there were changes in console) |////////////////////
	//------------------------------------------------------------------------
	bool updateText, updateCursor;

	// Do we have to update changed text? isForce taked into account
	updateText = CheckChangedTextAttr();

	if (isForce)
		updateCursor = attrChanged = textChanged = true;
	// Do we have to update text under changed cursor?
	// Important: check both 'cinf.bVisible' and 'Cursor.isVisible',
	// because console may have cursor hidden and its position changed -
	// in this case last visible cursor remains shown at its old place.
	// Also, don't check foreground window here for the same reasons.
	// If position is the same then check the cursor becomes hidden.
	else if (Cursor.x != csbi.dwCursorPosition.X || Cursor.y != csbi.dwCursorPosition.Y)
		// ��������� �������. ��������� ���� ������ �����
		updateCursor = cinf.bVisible || Cursor.isVisible || Cursor.isVisiblePrevFromInfo;
	else
		updateCursor = Cursor.isVisiblePrevFromInfo && !cinf.bVisible;

	mb_DialogsChanged = CheckDialogsChanged();
	// ���� �� ������� ������ ConEmuTh - ��������� ��������� �
	// ������ ����������� ���.����� (��� �� ������ ���������� �������� ���������)
	// � �������� ������� ������� (������ �����, ������� ��������� "���" ���������)
	PolishPanelViews();

	//gpSetCls->Performance(tPerfRender, FALSE);

	// For optimization - remove highlights if they was painted
	if (!isForce)
	{
		UndoHighlights();
	}

	if (updateText /*|| updateCursor*/)
	{
		lRes = true;
		DEBUGSTRDRAW(L" +++ updateText detected in VCon\n");
		//gpSetCls->Performance(tPerfRender, FALSE);
		//------------------------------------------------------------------------
		///| Drawing modified text |//////////////////////////////////////////////
		//------------------------------------------------------------------------
		UpdateText();
		//gpSetCls->Performance(tPerfRender, TRUE);
		//HEAPVAL
		//------------------------------------------------------------------------
		///| Now, store data for further comparison |/////////////////////////////
		//------------------------------------------------------------------------
		memcpy(mpsz_ConCharSave, mpsz_ConChar, TextLen * sizeof(*mpsz_ConChar));
		memcpy(mpn_ConAttrExSave, mpn_ConAttrEx, TextLen * sizeof(*mpn_ConAttrEx));
	}

	//-- ���������� � PaintVCon
	//// ���� ���������������� ������ (ConEmuTh) - �������� ������� �������
	//// ������ ��� ����� ����� UpdateText, ������ ��� ����� ConCharX ����� ���� �������
	//if (m_LeftPanelView.bRegister || m_RightPanelView.bRegister) {
	//	if (mb_DialogsChanged) {
	//		UpdatePanelRgn(TRUE);
	//		UpdatePanelRgn(FALSE);
	//	}
	//}
	//HEAPVAL
	//------------------------------------------------------------------------
	///| Checking cursor |////////////////////////////////////////////////////
	//------------------------------------------------------------------------
	UpdateCursor(lRes);
	HEAPVAL
	//SDC.Leave();
	SCON.Unlock();

	bool bVisible = gpConEmu->isVisible(this);

	// Highlight row/col under mouse cursor
	if (bVisible && (isHighlightMouseRow() || isHighlightMouseCol()))
	{
		UpdateHighlights();
	}

	// ����� ��������� ���������� ���������� ������� (�������/��������)
	//
	if (lRes && bVisible)
	{
		if (mpsz_LogScreen && mp_RCon && mp_RCon->GetServerPID())
		{
			// ������� ������ � ���
			mn_LogScreenIdx++;
			wchar_t szLogPath[MAX_PATH]; _wsprintf(szLogPath, SKIPLEN(countof(szLogPath)) mpsz_LogScreen, mp_RCon->GetServerPID(), mn_LogScreenIdx);
			Dump(szLogPath);
		}

		// ���� ��������� - ����������� ���������� viewport'�
		if (!mb_InPaintCall)
		{
			// ������ ���������� � �������� ����
			_ASSERTE(gpConEmu->isMainThread());
			//mb_PaintRequested = TRUE;
			gpConEmu->Invalidate(this);

			if (gpSetCls->isAdvLogging>=3) mp_RCon->LogString("Invalidating from CVirtualConsole::Update.2");

			//09.06.13 � ���� ���? ������� ��������� �� ������ �� ��������?
			//UpdateWindow('ghWnd DC'); // ��� �������� ��������� � ����, � ���� ��������� ���������
			#ifdef _DEBUG
			//_ASSERTE(!gpConEmu->m_Child->mb_Invalidated);
			#endif
			//mb_PaintRequested = FALSE;
		}
	}

	m_HighlightInfo.mb_ChangeDetected = false;

	gpSetCls->Performance(tPerfRender, TRUE);
	/* ***************************************** */
	/*       Finalization, release objects       */
	/* ***************************************** */
	SelectBrush(NULL);

	if (hBrush0)    // ��������� � BlitPictureTo
	{
		DeleteObject(hBrush0); hBrush0 = NULL;
	}

	/*
	for (UINT br=0; br<m_PartBrushes.size(); br++) {
	    DeleteObject(m_PartBrushes[br].hBrush);
	}
	m_PartBrushes.clear();
	*/
	SelectFont(NULL);
	HEAPVAL
	return lRes;
}

void CVirtualConsole::PatInvertRect(HDC hPaintDC, const RECT& rect, HDC hFromDC, bool bFill)
{
	if (bFill)
	{
		BitBlt(hPaintDC, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, hFromDC, 0,0,
			    PATINVERT);
	}
	else
	{
		BitBlt(hPaintDC, rect.left, rect.top, 1, rect.bottom-rect.top, hFromDC, 0,0,
				PATINVERT);
		BitBlt(hPaintDC, rect.left, rect.top, rect.right-rect.left, 1, hFromDC, 0,0,
				PATINVERT);
		BitBlt(hPaintDC, rect.right-1, rect.top, 1, rect.bottom-rect.top, hFromDC, 0,0,
				PATINVERT);
		BitBlt(hPaintDC, rect.left, rect.bottom-1, rect.right-rect.left, 1, hFromDC, 0,0,
				PATINVERT);
	}
}

bool CVirtualConsole::isHighlightMouseRow()
{
	return m_HighlightInfo.mb_SelfSettings ? m_HighlightInfo.mb_HighlightRow : gpSet->isHighlightMouseRow;
}

bool CVirtualConsole::isHighlightMouseCol()
{
	return m_HighlightInfo.mb_SelfSettings ? m_HighlightInfo.mb_HighlightCol : gpSet->isHighlightMouseCol;
}

void CVirtualConsole::ChangeHighlightMouse(int nWhat, int nSwitch)
{
	if (!m_HighlightInfo.mb_SelfSettings)
	{
		m_HighlightInfo.mb_SelfSettings = true;
		m_HighlightInfo.mb_HighlightRow = gpSet->isHighlightMouseRow;
		m_HighlightInfo.mb_HighlightCol = gpSet->isHighlightMouseCol;
	}

	if (nWhat <= 0)
	{
		if (!m_HighlightInfo.mb_HighlightRow && !m_HighlightInfo.mb_HighlightCol)
		{
			m_HighlightInfo.mb_HighlightRow = true;
		}
		else if (m_HighlightInfo.mb_HighlightRow && !m_HighlightInfo.mb_HighlightCol)
		{
			m_HighlightInfo.mb_HighlightRow = false; m_HighlightInfo.mb_HighlightCol = true;
		}
		else if (!m_HighlightInfo.mb_HighlightRow && m_HighlightInfo.mb_HighlightCol)
		{
			m_HighlightInfo.mb_HighlightRow = m_HighlightInfo.mb_HighlightCol = true;
		}
		else
		{
			m_HighlightInfo.mb_HighlightRow = m_HighlightInfo.mb_HighlightCol = false;
		}
	}
	else
	{
		for (int i = 1; i <= 2; i++)
		{
			if (!(nWhat & i))
				continue;
			bool& bOpt = (i == 1) ? m_HighlightInfo.mb_HighlightRow : m_HighlightInfo.mb_HighlightCol;
			switch (nSwitch)
			{
			case 0:
				bOpt = false; break;
			case 1:
				bOpt = true;  break;
			case 2:
				bOpt = !bOpt; break;
			}
		}
	}

	Update(true);
}

// This is called from TrackMouse. It must NOT trigger more than one invalidate before next repaint
bool CVirtualConsole::WasHighlightRowColChanged()
{
	// Invalidate already pended?
	if (m_HighlightInfo.mb_ChangeDetected)
		return false;

	// If cursor goes out of VCon - leave row/col marks?
	COORD crPos = {-1,-1};
	if (!CalcHighlightRowCol(&crPos))
		return false;

	if ((isHighlightMouseRow() && (crPos.Y != m_HighlightInfo.m_Last.Y))
		|| (isHighlightMouseCol() && (crPos.X != m_HighlightInfo.m_Last.X)))
	{
		m_HighlightInfo.mb_ChangeDetected = true;
		return true;
	}

	return false;
}

bool CVirtualConsole::CalcHighlightRowCol(COORD* pcrPos)
{
	if (!(isHighlightMouseRow() || isHighlightMouseCol())
		|| !gpConEmu->isMeForeground()
		|| !gpConEmu->isVisible(this))
	{
		m_HighlightInfo.mb_Allowed = false;
		m_HighlightInfo.m_Cur.X = m_HighlightInfo.m_Cur.Y = -1;
		return false;
	}

	// Get SCREEN coordinates
	POINT ptCursor = {}; GetCursorPos(&ptCursor);
	RECT  rcDC = {}; GetWindowRect(mh_WndDC, &rcDC);
	if (!PtInRect(&rcDC, ptCursor))
	{
		m_HighlightInfo.mb_Allowed = false;
		m_HighlightInfo.m_Cur.X = m_HighlightInfo.m_Cur.Y = -1;
		return false;
	}

	// Get COORDs (relative to upper-left visible pos)
	COORD pos = ClientToConsole(ptCursor.x-rcDC.left, ptCursor.y-rcDC.top);
	m_HighlightInfo.m_Cur = pos;
	if (pos.X < 0 || pos.Y < 0)
	{
		m_HighlightInfo.mb_Allowed = false;
		return false;
	}
	m_HighlightInfo.mb_Allowed = true;
	if (pcrPos)
		*pcrPos = pos;
	return true;
}

void CVirtualConsole::UpdateHighlights()
{
	_ASSERTE(this && (isHighlightMouseRow() || isHighlightMouseCol()));
	_ASSERTE(gpConEmu->isVisible(this));

	ZeroStruct(m_HighlightInfo.mrc_LastRow);
	ZeroStruct(m_HighlightInfo.mrc_LastCol);
	m_HighlightInfo.m_Last.X = m_HighlightInfo.m_Last.Y = -1;

	// Get COORDs (relative to upper-left visible pos)
	COORD pos = {-1,-1};
	if (!CalcHighlightRowCol(&pos))
		return;

	int CurChar = pos.Y * TextWidth + pos.X;
	if (CurChar < 0 || CurChar>=(int)(TextWidth * TextHeight))
	{
		return; // ����� ���� ��� ���� - ��� ������ ������� ��� ����� �������� � ���������� ������� ������� ������� � ������
	}

	// And MARK! (nFontHeight or cell (nFontWidth))

	COORD pix;
	pix.X = pos.X * nFontWidth;
	pix.Y = pos.Y * nFontHeight;

	if (pos.X && ConCharX && ConCharX[CurChar-1])
		pix.X = ConCharX[CurChar-1];

	HDC hPaintDC = (HDC)m_DC;
	HBRUSH hBr = CreateSolidBrush(HILIGHT_PAT_COLOR);
	HBRUSH hOld = (HBRUSH)SelectObject(hPaintDC, hBr);

	if (isHighlightMouseRow())
	{
		RECT rect = {0, pix.Y, Width, pix.Y+nFontHeight};
		PatInvertRect(hPaintDC, rect, hPaintDC, false);
		m_HighlightInfo.m_Last.Y = pos.Y;
		m_HighlightInfo.mrc_LastRow = rect;
	}

	if (isHighlightMouseCol())
	{
		// This will be not "precise" on other rows if using proportional font...
		RECT rect = {pix.X, 0, pix.X+nFontWidth, Height};
		PatInvertRect(hPaintDC, rect, hPaintDC, false);
		m_HighlightInfo.m_Last.X = pos.X;
		m_HighlightInfo.mrc_LastCol = rect;
	}

	SelectObject(hPaintDC, hOld);
	DeleteObject(hBr);
}

void CVirtualConsole::UndoHighlights()
{
	if (IsRectEmpty(&m_HighlightInfo.mrc_LastRow) && IsRectEmpty(&m_HighlightInfo.mrc_LastCol))
		return;

	HDC hPaintDC = (HDC)m_DC;
	HBRUSH hBr = CreateSolidBrush(HILIGHT_PAT_COLOR);
	HBRUSH hOld = (HBRUSH)SelectObject(hPaintDC, hBr);

	if (!IsRectEmpty(&m_HighlightInfo.mrc_LastRow))
	{
		RECT rect = m_HighlightInfo.mrc_LastRow;
		PatInvertRect(hPaintDC, rect, hPaintDC, false);
	}

	if (!IsRectEmpty(&m_HighlightInfo.mrc_LastCol))
	{
		RECT rect = m_HighlightInfo.mrc_LastCol;
		PatInvertRect(hPaintDC, rect, hPaintDC, false);
	}

	SelectObject(hPaintDC, hOld);
	DeleteObject(hBr);
}

bool CVirtualConsole::CheckTransparent()
{
	if (!this)
	{
		_ASSERTE(this);
		return FALSE;
	}

	bool lbChanged = FALSE;
	static bool lbInCheckTransparent = false;

	if (lbInCheckTransparent)
	{
		_ASSERTE(lbInCheckTransparent==false);
		return FALSE;
	}

	bool lbHasChildWindows = false;
	if (mp_RCon)
	{
		// ���� ������ ���� � GUI ������ (notepad �� ������� ConEmu)
		if (mp_RCon->GuiWnd() != NULL)
			lbHasChildWindows = true;
		// ��� ������� �������� ��������/����������
		if (mp_RCon->isPictureView())
			lbHasChildWindows = true;
	}

	lbInCheckTransparent = true;

	// �������� �������, � ������� ������ �� ����� ��������� �������
	if (gpSet->isUserScreenTransparent)
	{
		// ���� ���� ����� �����: ��� ������� ������, �� ����� �������� �������
		if (CheckTransparentRgn(lbHasChildWindows))
		{
			lbChanged = TRUE;
		}
	}

	// ���� �������� ������ ������� ����������� PicView
	if (mb_ChildWindowWasFound != lbHasChildWindows)
	{
		mb_ChildWindowWasFound = lbHasChildWindows;
		// ����� ����������� ����� �����-������������ ����
		gpConEmu->OnTransparent();
	}


	lbInCheckTransparent = false;

	return lbChanged;
}

bool CVirtualConsole::CheckTransparentRgn(bool abHasChildWindows)
{
	bool lbRgnChanged = false;
	bool lbTransparentChanged = false;


	// ���� �������� ������, ��� ���� ������������ � ��������� �������� ���� (��� ��������)
	if (mb_ConDataChanged || !((mh_TransparentRgn != NULL) ^ abHasChildWindows))
	{
		mb_ConDataChanged = false;

		// (����)������� ������
		if (mpsz_ConChar && mpn_ConAttrEx && TextWidth && TextHeight)
		{
			MSectionLock SCON; SCON.Lock(&csCON);
			CharAttr* pnAttr = mpn_ConAttrEx;
			int nFontHeight = gpSetCls->FontHeight();
			int    nMaxRects = TextHeight*5;
			//#ifdef _DEBUG
			//nMaxRects = 5;
			//#endif
			POINT *lpAllPoints = (POINT*)Alloc(nMaxRects*4,sizeof(POINT)); // 4 ����
			//INT   *lpAllCounts = (INT*)Alloc(nMaxRects,sizeof(INT));
			HEAPVAL;
			int    nRectCount = 0;

			if (lpAllPoints /*&& lpAllCounts*/)
			{
				POINT *lpPoints = lpAllPoints;
				//INT   *lpCounts = lpAllCounts;

				if (!abHasChildWindows)
				{
					for(uint nY = 0; nY < TextHeight; nY++)
					{
						uint nX = 0;
						//int nYPix1 = nY * nFontHeight;

						while (nX < TextWidth)
						{
							// ����� ������ ���������� cell
							#if 0
							while (nX < TextWidth && !pnAttr[nX].bTransparent)
								nX++;
							#else
							WARNING("CharAttr_Transparent");
							// ������ �� ��������� �������� ������������ ������ �����, � �� � RgnDetect
							while (nX < TextWidth && !(pnAttr[nX].Flags & CharAttr_Transparent))
								nX++;
							#endif

							if (nX >= TextWidth)
								break;

							// ����� ����� ����������� �����
							uint nTranStart = nX;

							#if 0
							while (++nX < TextWidth && pnAttr[nX].bTransparent)
								;
							#else
							WARNING("CharAttr_Transparent");
							// ������ �� ��������� �������� ������������ ������ �����, � �� � RgnDetect
							while (++nX < TextWidth && (pnAttr[nX].Flags & CharAttr_Transparent))
								;
							#endif

							// ������������ ��������������� Rect
							//int nXPix = 0;

							if (nRectCount>=nMaxRects)
							{
								int nNewMaxRects = nMaxRects + max((int)TextHeight,(int)(nRectCount-nMaxRects+1));
								_ASSERTE(nNewMaxRects > nRectCount);

								HEAPVAL;
								POINT *lpTmpPoints = (POINT*)Alloc(nNewMaxRects*4,sizeof(POINT)); // 4 ����
								HEAPVAL;

								if (!lpTmpPoints)
								{
									_ASSERTE(lpTmpPoints);
									Free(lpAllPoints);
									return false;
								}

								memmove(lpTmpPoints, lpAllPoints, nMaxRects*4*sizeof(POINT));
								HEAPVAL;
								lpPoints = lpTmpPoints + (lpPoints - lpAllPoints);
								Free(lpAllPoints);
								lpAllPoints = lpTmpPoints;
								nMaxRects = nNewMaxRects;
								HEAPVAL;
							}

							nRectCount++;
	#ifdef _DEBUG

							if (nRectCount>=nMaxRects)
							{
								nRectCount = nRectCount;
							}

	#endif
							//*(lpCounts++) = 4;
							lpPoints[0] = ConsoleToClient(nTranStart, nY);
							lpPoints[1] = ConsoleToClient(nX, nY);
							// x - 1?
							//if (lpPoints[1].x > lpPoints[0].x) lpPoints[1].x--;
							lpPoints[2] = lpPoints[1]; lpPoints[2].y += nFontHeight;
							lpPoints[3] = lpPoints[0]; lpPoints[3].y = lpPoints[2].y;
							lpPoints += 4;
							nX++;
							HEAPVAL;
						}

						pnAttr += TextWidth;
					}
				}

				HEAPVAL;
				lbRgnChanged = (nRectCount != TransparentInfo.nRectCount);
				lbTransparentChanged = ((nRectCount==0) != (TransparentInfo.nRectCount==0));

				if (!lbRgnChanged && TransparentInfo.nRectCount)
				{
					_ASSERTE(TransparentInfo.pAllPoints && TransparentInfo.pAllCounts);
					lbRgnChanged = memcmp(TransparentInfo.pAllPoints, lpAllPoints, sizeof(POINT)*4*nRectCount)!=0;
				}

				HEAPVAL;

				if (lbRgnChanged)
				{
					if (mh_TransparentRgn) { DeleteObject(mh_TransparentRgn); mh_TransparentRgn = NULL; }

					INT   *lpAllCounts = NULL;

					if (nRectCount > 0)
					{
						lpAllCounts = (INT*)Alloc(nRectCount,sizeof(INT));
						INT   *lpCounts = lpAllCounts;

						for(int n = 0; n < nRectCount; n++)
							*(lpCounts++) = 4;

						mh_TransparentRgn = CreatePolyPolygonRgn(lpAllPoints, lpAllCounts, nRectCount, WINDING);
					}

					HEAPVAL;

					if (TransparentInfo.pAllCounts)
						Free(TransparentInfo.pAllCounts);

					HEAPVAL;
					TransparentInfo.pAllCounts = lpAllCounts; lpAllCounts = NULL;

					if (TransparentInfo.pAllPoints)
						Free(TransparentInfo.pAllPoints);

					HEAPVAL;
					TransparentInfo.pAllPoints = lpAllPoints; lpAllPoints = NULL;
					HEAPVAL;
					TransparentInfo.nRectCount = nRectCount;
				}

				HEAPVAL;

				//if (lpAllCounts) Free(lpAllCounts);
				if (lpAllPoints) Free(lpAllPoints);

				HEAPVAL;
			}
		}
	}

	if (lbRgnChanged && isVisible())
	{
		gpConEmu->UpdateWindowRgn();
	}

	return lbTransparentChanged;
}

bool CVirtualConsole::LoadConsoleData()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return false;
	}

	WARNING("��������� ��� �� � ������� ��������, ������ �� ��������� ������ � ������� ������");

	// ����� ����, ���� ������� ���������, �.�. CVirtualConsole::Update �� ���������� � ������� �� ����������. � ��� ���������.
	if (!mpsz_ConChar || !mpn_ConAttrEx)
	{
		// ���� ��� ������ ConEmu ������� ��������� �������� ����� '@'
		// �� ��� ����� �������� - �� ���������������� (InitDC �� ���������),
		// ��� ����� ������ � ������� ����,
		// PostMessage(ghWnd, mn_MsgInitInactiveDC, 0, apVCon) �� ���� �����������
		gpConEmu->InitInactiveDC(this);
		return false;
	}

	gpSetCls->Performance(tPerfData, FALSE);
	{
		#ifdef SHOWDEBUGSTEPS
		gpConEmu->DebugStep(L"mp_RCon->GetConsoleData");
		#endif

		mp_RCon->GetConsoleData(mpsz_ConChar, mpn_ConAttrEx, TextWidth, TextHeight); //TextLen*2);

		#ifdef SHOWDEBUGSTEPS
		gpConEmu->DebugStep(NULL);
		#endif
	}
	SMALL_RECT rcFull, rcGlyph = {0,0,-1,-1};

	const CRgnDetect* pRgn = mp_RCon->GetDetector();

	_ASSERTE(countof(mrc_Dialogs)==countof(mn_DialogFlags));
	mn_DialogsCount = pRgn->GetDetectedDialogs(countof(mrc_Dialogs), mrc_Dialogs, mn_DialogFlags);
	mn_DialogAllFlags = pRgn->GetFlags();

#if 0
	if (mn_DialogsCount == 7 && mn_LastDialogsCount == 6)
	{
		Dump(L"T:\\Dialogs.con");
	}
#endif

	if (gpSet->isExtendUCharMap)
	{
		if (pRgn && (mn_DialogAllFlags & (FR_UCHARMAP|FR_UCHARMAPGLYPH)) == (FR_UCHARMAP|FR_UCHARMAPGLYPH))
		{
			if (pRgn->GetDetectedDialogs(1, &rcFull, NULL, FR_UCHARMAP, FR_UCHARMAP)
			        && pRgn->GetDetectedDialogs(1, &rcGlyph, NULL, FR_UCHARMAPGLYPH, FR_UCHARMAPGLYPH))
			{
				wchar_t szFontName[32], *pszStart, *pszEnd;
				pszStart = mpsz_ConChar + TextWidth*(rcFull.Top+1) + rcFull.Left + 1;
				pszEnd = pszStart + 31;

				while (*pszEnd == L' ' && pszEnd >= pszStart) pszEnd--;

				if (pszEnd > pszStart)
				{
					wmemcpy(szFontName, pszStart, pszEnd-pszStart+1);
					szFontName[pszEnd-pszStart+1] = 0;

					if (!mh_UCharMapFont || lstrcmp(ms_LastUCharMapFont, szFontName))
					{
						wcscpy_c(ms_LastUCharMapFont, szFontName);

						if (mh_UCharMapFont) DeleteObject(mh_UCharMapFont);

						mh_UCharMapFont = gpSetCls->CreateOtherFont(ms_LastUCharMapFont);
					}
				}

				// ����� �������, ������ - �������� �������������� ������ ��� ������������� ����� ������
				if (mh_UCharMapFont)
				{
					for(int Y = rcGlyph.Top; Y <= rcGlyph.Bottom; Y++)
					{
						CharAttr *pAtr = mpn_ConAttrEx + (TextWidth*Y + rcGlyph.Left);

						for(int X = rcGlyph.Left; X <= rcGlyph.Right; X++, pAtr++)
						{
							pAtr->nFontIndex = MAX_FONT_STYLES;
						}
					}
				}
			}
		}
	}

	mrc_UCharMap = rcGlyph;
	gpSetCls->Performance(tPerfData, TRUE);
	return true;
}

COLORREF* CVirtualConsole::GetColors()
{
	// Update AppID if needed
	int nCurAppId = mp_RCon ? mp_RCon->GetActiveAppSettingsId() : -1;

	// Retrieve palette colors
	LPCWSTR pszPalName = mp_RCon ? mp_RCon->GetArgs().pszPalette : NULL;
	if (pszPalName && *pszPalName)
		mp_Colors = gpSet->GetPaletteColors(pszPalName, isFade);
	else
		mp_Colors = gpSet->GetColors(nCurAppId, isFade);

	return mp_Colors;
}

bool CVirtualConsole::UpdatePrepare(HDC *ahDc, MSectionLock *pSDC, MSectionLock *pSCON)
{
	MSectionLock SCON; SCON.Lock(&csCON);
	attrBackLast = 0;
	isEditor = CVConGroup::isEditor();
	isViewer = CVConGroup::isViewer();
	isFilePanel = CVConGroup::isFilePanel(true);
	isFade = !isForeground && gpSet->isFadeInactive;

	// Retrieve palette colors
	GetColors();

	if ((nFontHeight != gpSetCls->FontHeight()) || (nFontWidth != gpSetCls->FontWidth()))
		isFontSizeChanged = true;
	nFontHeight = gpSetCls->FontHeight();
	nFontWidth = gpSetCls->FontWidth();
	nFontCharSet = gpSetCls->FontCharSet();
	winSize = MakeCoord(mp_RCon->TextWidth(),mp_RCon->TextHeight());
	//csbi.dwCursorPosition.X -= csbi.srWindow.Left; -- �������������� ��������� ������������!
	csbi.dwCursorPosition.Y -= csbi.srWindow.Top;
	isCursorValid =
	    csbi.dwCursorPosition.X >= 0 && csbi.dwCursorPosition.Y >= 0 &&
	    csbi.dwCursorPosition.X < winSize.X && csbi.dwCursorPosition.Y < winSize.Y;

	if (mb_RequiredForceUpdate || mb_IsForceUpdate)
	{
		isForce = true;
		mb_RequiredForceUpdate = false;
	}

	// ������ �������������, ��� ����� �������
	BOOL lbSizeChanged = ((HDC)m_DC == NULL) || (TextWidth != (uint)winSize.X || TextHeight != (uint)winSize.Y)
		|| (LastPadSize != gpSet->nCenterConsolePad)
		|| isFontSizeChanged; // ��� ����� ������ ('Auto' �� 'Main')

	#ifdef _DEBUG
	COORD dbgWinSize = winSize;
	COORD dbgTxtSize = {TextWidth,TextHeight};
	#endif

	_ASSERTE(gpConEmu->isMainThread());

	// 120108 - ������������� DC �� ������ ��� �� ����� (isForce ������������, ��������, ��� ����� Fade)
	if (/*isForce ||*/ !mb_PointersAllocated || lbSizeChanged)
	{
		// 100627 ������� ����� InitDC, �.�. ���������
		//if (lbSizeChanged)
		//	gpConEmu->OnConsole Resize(TRUE);
		//if (pSDC && !pSDC->isLocked())  // ���� ������ ��� �� ������������� (��������� - ���������� �������)
		//	pSDC->Lock(&csDC, TRUE, 200); // �� �� ��������, ����� �� ������� ���������

		if (!InitDC(ahDc!=NULL && !isForce/*abNoDc*/ , false/*abNoWndResize*/, pSDC, pSCON))
		{
			return false;
		}

		if (lbSizeChanged)
		{
			MSetter lInConsoleResize(&mb_InConsoleResize);
			gpConEmu->OnConsoleResize(TRUE);
		}


		isForce = true; // ����� ������ ������� � DC - ��������� ������ refresh...


		#ifdef _DEBUG
		if (TextWidth == 80 && !mp_RCon->isNtvdm())
		{
			TextWidth = TextWidth;
		}
		#endif

		//if (lbSizeChanged) -- ��������� ����� ���������
		//	gpConEmu->OnConsole Resize();
	}

	// ��������� ������ �����������!
	if (mb_IsForceUpdate)
	{
		isForce = true;
		mb_IsForceUpdate = false;
	}

	// ���������� �����. ������� ����������� ���������, ��������� ���� ����� �� ahDc
	if (ahDc)
		isForce = true;

	//drawImage = (gpSet->isShowBgImage == 1 || (gpSet->isShowBgImage == 2 && !(isEditor || isViewer)) )
	//	&& gpSet->isBackgroundImageValid;
	drawImage = gpSetCls->IsBackgroundEnabled(this);
	TextLen = TextWidth * TextHeight;
	coord.X = csbi.srWindow.Left; coord.Y = csbi.srWindow.Top;

	DWORD nIndexes = gpSet->nBgImageColors;
	if (nIndexes == (DWORD)-1 && mp_RCon)
	{
		// �������� �� ����
		const CEFAR_INFO_MAPPING* pFar = mp_RCon->GetFarInfo();
		if (pFar)
		{
			// ���� ���� �������
			BYTE bgIndex = (pFar->nFarColors[col_PanelText] & 0xF0) >> 4;
			nIndexes = 1 << (DWORD)bgIndex;
		}
	}
	if (nIndexes == (DWORD)-1)
		nIndexes = BgImageColorsDefaults; // ���������
	nBgImageColors = nIndexes;

	if (drawImage)
	{
		// ��� ������ ��������, �� ��������� �� ����?
		if (mp_Bg->PrepareBackground(this, hBgDc, bgBmpSize))
		{
			isForce = true;
		}
		else
		{
			drawImage = (hBgDc!=NULL);
		}
	}

	// ����������� ������ �� ��������� ������� � mpn_ConAttrEx/mpsz_ConChar
	BOOL bConDataChanged = isForce || mp_RCon->IsConsoleDataChanged();

	if (bConDataChanged)
	{
		mb_ConDataChanged = true; // � false - �� ����������
		// ����� � �������, �.�. ������ ��� RealConsole ����� ������� ���������� ��������� ��������
		LoadConsoleData();
	}

	HEAPVAL
	// ���������� ���������� ������� (��������� � CRealConsole)
	// get cursor info
	mp_RCon->GetConsoleCursorInfo(/*hConOut(),*/ &cinf);
	HEAPVAL
	return true;
}

//enum CVirtualConsole::_PartType CVirtualConsole::GetCharType(wchar_t ch)
//{
//    enum _PartType cType = pNull;
//
//    if (ch == L' ')
//        cType = pSpace;
//    //else if (ch == L'_')
//    //  cType = pUnderscore;
//    else if (isCharBorder(ch)) {
//        if (isCharBorderVertical(ch))
//            cType = pVBorder;
//        else
//            cType = pBorder;
//    }
//    else if (isFilePanel && ch == L'}')
//        cType = pRBracket;
//    else
//        cType = pText;
//
//    return cType;
//}



// row - 0-based
//void CVirtualConsole::ParseLine(int row, wchar_t *ConCharLine, WORD *ConAttrLine)
//{
//    //UINT idx = 0;
//    struct _TextParts *pStart=TextParts, *pEnd=TextParts;
//    enum _PartType cType1, cType2;
//    UINT i1=0, i2=0;
//
//    pEnd->partType = pNull; // ����� ��������� ������
//
//    wchar_t ch1, ch2;
//    BYTE af1, ab1, af2, ab2;
//    DWORD pixels;
//    while (i1<TextWidth)
//    {
//        //GetCharAttr(ConCharLine[i1], ConAttrLine[i1], ch1, af1, ab1);
//		ch1 = ConCharLine[i1];
//		GetCharAttr(ConAttrLine[i1], af1, ab1, NULL);
//        cType1 = GetCharType(ch1);
//        if (cType1 == pRBracket) {
//            if (!(row>=2 && isCharBorderVertical(mpsz_ConChar[TextWidth+i1]))
//                && (((UINT)row)<=(TextHeight-4)))
//                cType1 = pText;
//        }
//        pixels = CharWidth(ch1);
//
//        i2 = i1+1;
//        // � ������ Force Monospace ��������� ���� �� ������ �������
//        if (!gpSet->isForceMonospace && i2 < TextWidth &&
//            (cType1 != pVBorder && cType1 != pRBracket))
//        {
//            //GetCharAttr(ConCharLine[i2], ConAttrLine[i2], ch2, af2, ab2);
//			ch2 = ConCharLine[i2];
//			GetCharAttr(ConAttrLine[i2], af2, ab2, NULL);
//            // �������� ���� �������� � ������������ �������
//            while (i2 < TextWidth && af2 == af1 && ab2 == ab1) {
//                // ���� ������ ���������� �� �������
//
//                cType2 = GetCharType(ch2);
//                if ((ch2 = ConCharLine[i2]) != ch1) {
//                    if (cType2 == pRBracket) {
//                        if (!(row>=2 && isCharBorderVertical(mpsz_ConChar[TextWidth+i2]))
//                            && (((UINT)row)<=(TextHeight-4)))
//                            cType2 = pText;
//                    }
//
//                    // � �� ������ �� ������ ������
//                    if (cType2 != cType1)
//                        break; // �� ��������� �����
//                }
//                pixels += CharWidth(ch2); // �������� ������ ������� � ��������
//                i2++; // ��������� ������
//                //GetCharAttr(ConCharLine[i2], ConAttrLine[i2], ch2, af2, ab2);
//				ch2 = ConCharLine[i2];
//				GetCharAttr(ConAttrLine[i2], af2, ab2, NULL);
//                if (cType2 == pRBracket) {
//                    if (!(row>=2 && isCharBorderVertical(mpsz_ConChar[TextWidth+i2]))
//                        && (((UINT)row)<=(TextHeight-4)))
//                        cType2 = pText;
//                }
//            }
//        }
//
//        // ��� ������� ������ ����� ��������, ���� ����� pText,pSpace,pText �� pSpace,pText �������� � ������ pText
//        if (cType1 == pText && (pEnd - pStart) >= 2) {
//            if (pEnd[-1].partType == pSpace && pEnd[-2].partType == pText &&
//                pEnd[-1].attrBack == ab1 && pEnd[-1].attrFore == af1 &&
//                pEnd[-2].attrBack == ab1 && pEnd[-2].attrFore == af1
//                )
//            {
//                pEnd -= 2;
//                pEnd->i2 = i2 - 1;
//                pEnd->iwidth = i2 - pEnd->i1;
//                pEnd->width += pEnd[1].width + pixels;
//                pEnd ++;
//                i1 = i2;
//                continue;
//            }
//        }
//        pEnd->i1 = i1; pEnd->i2 = i2 - 1; // ����� "�������"
//        pEnd->partType = cType1;
//        pEnd->attrBack = ab1; pEnd->attrFore = af1;
//        pEnd->iwidth = i2 - i1;
//        pEnd->width = pixels;
//        if (gpSet->isForceMonospace ||
//            (gpSet->isProportional && (cType1 == pVBorder || cType1 == pRBracket)))
//        {
//            pEnd->x1 = i1 * gpSetCls->FontWidth();
//        } else {
//            pEnd->x1 = -1;
//        }
//
//        pEnd ++; // ������ �� ����� ���� ������ ���������� �������� � ������, ��� ��� � ������������ ��� ��
//        i1 = i2;
//    }
//    // ���� �������� ����� ������, �����, ���� ������ �� ������ - ������� pDummy
//    pEnd->partType = pNull;
//}

void CVirtualConsole::Update_CheckAndFill()
{
#if 0
	WARNING("���� �� ������������!");
	// pointers
	wchar_t* ConCharLine = mpsz_ConChar;
	CharAttr* ConAttrLine = mpn_ConAttrEx;
	// previous
	const CharAttr* ConAttrLine2 = mpn_ConAttrExSave;
	const wchar_t* ConCharLine2 = mpsz_ConCharSave;
	// counters
	int pos = 0;
	int row = 0;
	int nMaxPos = Height - nFontHeight;
	// locals
	bool bRowChanged;

	// rows
	//BUGBUG: ������ �� ������������ ��������� ������, ���� ���� ��� ���� �� ������
	for(; pos <= nMaxPos;
	        ConCharLine += TextWidth, ConAttrLine += TextWidth,
	        ConCharLine2 += TextWidth, ConAttrLine2 += TextWidth,
	        pbLineChanged++, pbBackIsPic++, pnBackRGB++,
	        pos += nFontHeight, row++)
	{
		if (row >= (int)TextHeight)
		{
			_ASSERTE(row<(int)TextHeight);
			return;
		}

		bRowChanged = isForce
		              || (wmemcmp(ConCharLine, ConCharLine2, TextWidth) != 0)
		              || (memcmp(ConAttrLine, ConAttrLine2, TextWidth*sizeof(*ConAttrLine)) != 0);
		//// skip not changed symbols except the old cursor or selection
		//int j = 0, end = TextWidth;
		//// � ������ ���������������� ������� ��� isForce==true - ������������ ����� �������
		//// ��� ���������������� ��� ����������� ���, ��� ��� ����������� ���������� �����
		//// ����� ��������� ������ ����������� ��������, � ���������� �������� ���������
		//if (skipNotChanged)
		//{
		//    // Skip not changed head and tail symbols
		//	if (!FindChanges(j, end, ConCharLine, ConAttrLine, ConCharLine2, ConAttrLine2))
		//		continue;
		//} // if (skipNotChanged)
		*pbLineChanged = bRowChanged;

		if (bRowChanged)
		{
			if (drawImage && ISBGIMGCOLOR(ConAttrLine->nBackIdx))
			{
				*pbBackIsPic = true;
				BlitPictureTo(0, pos, Width, nFontHeight);
			}
			else
			{
				*pbBackIsPic = false;
				COLORREF cr = ConAttrLine->crBackColor;
				*pnBackRGB = cr;
				// ������ ����������, ����� �� �������������� � ���������� ��������� ��������
				HBRUSH hbr = PartBrush(L' ', cr, 0);
				RECT rect = MakeRect(0, pos, Width, pos + nFontHeight);
				FillRect((HDC)m_DC, &rect, hbr);
			}
		}
	}
#endif
}

// ������ ������ �� ������������ (���������� true, ���� ���� ������ � �� �������� �����)
// ������� ����� ���������� ������������� (���������� ��������� � DX)
bool CVirtualConsole::Update_ParseTextParts(uint row, const wchar_t* ConCharLine, const CharAttr* ConAttrLine)
{
#if 1
	return false;
#else
	bool bHasAlternateBg = false;
	int j = 0, j0 = 0;
	TEXTPARTS *pTP = TextParts;
	bool bEnhanceGraphics = gpSet->isEnhanceGraphics;
	//bool bProportional = gpSet->isMonospace == 0;
	//bool bForceMonospace = gpSet->isMonospace == 2;
	wchar_t c = 0;
	bool isUnicode = false, isProgress = false, isSpace = false, isUnicodeOrProgress = false;
	uint nPrevForeFont = 0, nForeFont;
	// ������� - ������ ����
	BGPARTS *pBG = BgParts;
	uint nColorIdx = 0;
	uint nPrevColorIdx = ConAttrLine[0].nBackIdx;
	j = j0 = 0;

	while (j < (int)TextWidth)
	{
		while (++j < (int)TextWidth)
		{
			if (nPrevColorIdx != ConAttrLine[j].nBackIdx)
			{
				nColorIdx = ConAttrLine[j].nBackIdx;
				break;
			}
		}

		pBG->i = j0;
		pBG->n = j - j0;

		if (!(pBG->bBackIsPic = (drawImage && ISBGIMGCOLOR(nPrevColorIdx))))
			pBG->nBackRGB = ConAttrLine[j0].crBackColor;

		pBG++;
		nPrevColorIdx = nColorIdx;
		j0 = j;
	}

	// *** ������ - Foreground (text) ***
	// ����� ������ ��� �� ��������� - ��������
	j = 0;
	nPrevForeFont = ConAttrLine[0].ForeFont;

	while (j < (int)TextWidth)
	{
		j0 = j;
		const CharAttr* attr = ConAttrLine+j;
		nForeFont = attr->ForeFont;
		c = ConCharLine[j];
		TODO("������ gpSet->isFixFarBorders && gpSet->isEnhanceGraphics");
		isUnicode = isCharBorder(c);
		bool isProgress = false, isSpace = false, isUnicodeOrProgress = false;

		if (isUnicode || bEnhanceGraphics)
			isProgress = isCharProgress(c); // ucBox25 / ucBox50 / ucBox75 / ucBox100

		isUnicodeOrProgress = isUnicode || isProgress;

		if (!isUnicodeOrProgress)
			isSpace = isCharSpace(c);

		TODO("���� '����������'");

		if (isProgress)
		{
			while (++j < (int)TextWidth)
			{
				if (ConCharLine[j] != c || attr->All != ConAttrLine[j].All)
					break; // �� ������ ����� ������� ��� ����� ����-������
			}
		}
		else if (isSpace)
		{
			while (++j < (int)TextWidth)
			{
				if (!isCharSpace(ConCharLine[j]))
					break; // �� ������� ������������� �������
			}
		}
		else if (isUnicode)
		{
			COLORREF crFore = attr->crForeColor;

			while (++j < (int)TextWidth)
			{
				if (!isCharBorder(ConCharLine[j]) || crFore != ConAttrLine[j].crForeColor)
					break; // �� ������� ����������� ������� ��� ����� ����� ������
			}
		}
		else
		{
			TODO("�������� �����");
			while (++j < (int)TextWidth)
			{
				attr = ConAttrLine+j;
				nForeFont = attr->ForeFont;
				c = ConCharLine[j];

				// ���� ����� ����� ���� ��� ������
				if (nForeFont != nPrevForeFont)
					break;

				isUnicode = isCharBorder(c);
				bool isProgress = false, isSpace = false, isUnicodeOrProgress = false;

				if (isUnicode || bEnhanceGraphics)
				{
					isProgress = isCharProgress(c); // ucBox25 / ucBox50 / ucBox75 / ucBox100
				}

				isUnicodeOrProgress = isUnicode || isProgress;

				if (!isUnicodeOrProgress)
					isSpace = isCharSpace(c);
			}
		}

		TODO("������� ���� j0..j");
	}

	return bHasAlternateBg;
#endif
}

// ������� ����� � �� �������� �����, ��������� ���������� � �����
void CVirtualConsole::Update_FillAlternate(uint row, uint nY)
{
}

// ����� ���������� ������ (��� ������������� Clipped)
void CVirtualConsole::Update_DrawText(uint row, uint nY)
{
}

void CVirtualConsole::UpdateText()
{
	//if (!updateText) {
	//    _ASSERTE(updateText);
	//    return;
	//}
#ifdef _DEBUG
	if (mp_RCon->IsConsoleThread())
	{
		//_ASSERTE(!mp_RCon->IsConsoleThread());
	}

	//// ������ ��� ������ ���� ���������, � ��� �� ������ ���� ����
	//BOOL lbDataValid = TRUE; uint n = 0;
	//while (n<TextLen) {
	//    if (mpsz_ConChar[n] == 0) {
	//        lbDataValid = FALSE; //break;
	//        mpsz_ConChar[n] = L'�';
	//        mpn_ConAttr[n] = 12;
	//    } else if (mpsz_ConChar[n] != L' ') {
	//        // 0 - ����� ���� ������ ��� �������. ����� ������ ����� �������, ���� �� ����, ���� �� ������
	//        if (mpn_ConAttr[n] == 0) {
	//            lbDataValid = FALSE; //break;
	//            mpn_ConAttr[n] = 12;
	//        }
	//    }
	//    n++;
	//}
	////_ASSERTE(lbDataValid);
#endif
	_ASSERTE((HDC)m_DC!=NULL);
	memmove(mh_FontByIndex, gpSetCls->mh_Font, MAX_FONT_STYLES*sizeof(mh_FontByIndex[0]));
	mh_FontByIndex[MAX_FONT_STYLES] = mh_UCharMapFont ? mh_UCharMapFont : mh_FontByIndex[0];
	SelectFont(mh_FontByIndex[0]);
	// pointers
	wchar_t* ConCharLine = NULL;
	CharAttr* ConAttrLine = NULL;
	DWORD* ConCharXLine = NULL;
	// counters
	int pos, row;
	{
		int i;
		i = 0; //TextLen - TextWidth; // TextLen = TextWidth/*80*/ * TextHeight/*25*/;
		pos = 0; //Height - nFontHeight; // Height = TextHeight * nFontHeight;
		row = 0; //TextHeight - 1;
		ConCharLine = mpsz_ConChar + i;
		ConAttrLine = mpn_ConAttrEx + i;
		ConCharXLine = ConCharX + i;
	}
	int nMaxPos = Height - nFontHeight;

	if (!ConCharLine || !ConAttrLine || !ConCharXLine)
	{
		MBoxAssert(ConCharLine && ConAttrLine && ConCharXLine);
		return;
	}


	if (/*gpSet->isForceMonospace ||*/ !drawImage)
		m_DC.SetBkMode(OPAQUE);
	else
		m_DC.SetBkMode(TRANSPARENT);

	int *nDX = (int*)malloc((TextWidth+1)*sizeof(int));

	bool skipNotChanged = !isForce;
	bool bEnhanceGraphics = gpSet->isEnhanceGraphics;
	bool bProportional = gpSet->isMonospace == 0;
	bool bForceMonospace = gpSet->isMonospace == 2;
	bool bFixFarBorders = gpSet->isFixFarBorders;
	//mh_FontByIndex[0] = gpSetCls->mh_Font; mh_FontByIndex[1] = gpSetCls->mh_FontB; mh_FontByIndex[2] = gpSetCls->mh_FontI; mh_FontByIndex[3] = gpSetCls->mh_FontBI;
	CEFONT hFont = gpSetCls->mh_Font[0];
	CEFONT hFont2 = gpSetCls->mh_Font2;

	// 120616 - Chinese - off?
	bool bFixFrameCoord = !gbIsDBCS || mp_RCon->isFar();

	_ASSERTE(((TextWidth * nFontWidth) >= Width));
#if 0
	if (((TextHeight * nFontHeight) < Height) || ((TextWidth * nFontWidth) < Width))
	{
		bool lbDelBrush = false;
		HBRUSH hBr = CreateBackBrush(false, lbDelBrush);

		if (nFontHeight && (TextHeight * nFontHeight) < Height)
		{
			RECT rcFill = {0, TextHeight * nFontHeight, Width, Height};
			FillRect((HDC)m_DC, &rcFill, hBr);
		}

		if (nFontWidth && ((TextWidth * nFontWidth) < Width))
		{
			RECT rcFill = {TextWidth * nFontWidth, 0, Width, Height};
			FillRect((HDC)m_DC, &rcFill, hBr);
		}

		if (lbDelBrush)
			DeleteObject(hBr);
	}
#endif

	//BUGBUG: ������ �� ������������ ��������� ������, ���� ���� ��� ���� �� ������
	for (; pos <= nMaxPos;
	       ConCharLine += TextWidth, ConAttrLine += TextWidth, ConCharXLine += TextWidth,
	       pos += nFontHeight, row++)
	{
		if (row >= (int)TextHeight)
		{
			//_ASSERTE(row < (int)TextHeight);
			DEBUGSTRFAIL(L"############ _ASSERTE(row < (int)TextHeight) #############\n");
			break;
		}
		//2009-09-25. ��������� (������?) ��������� ���������� �������� � ������� ������� (ASC<32)
		//            �� ����� �������� �� ��������� �������
		//{
		//	wchar_t* pszEnd = ConCharLine + TextWidth;
		//	for (wchar_t* pch = ConCharLine; pch < pszEnd; pch++) {
		//		if (((WORD)*pch) < 32) *pch = gszAnalogues[(WORD)*pch];
		//	}
		//}
		// the line
		const CharAttr* const ConAttrLine2 = mpn_ConAttrExSave + (ConAttrLine - mpn_ConAttrEx);
		const wchar_t* const ConCharLine2 = mpsz_ConCharSave + (ConCharLine - mpsz_ConChar);
		// skip not changed symbols except the old cursor or selection
		int j = 0, end = TextWidth;

		// � ������ ���������������� ������� ��� isForce==true - ������������ ����� �������
		// ��� ���������������� ��� ����������� ���, ��� ��� ����������� ���������� �����
		// ����� ��������� ������ ����������� ��������, � ���������� �������� ���������
		if (skipNotChanged)
		{
			// Skip not changed head and tail symbols
			if (!FindChanges(row, j, end, ConCharLine, ConAttrLine, ConCharLine2, ConAttrLine2))
				continue;
		} // if (skipNotChanged)

		if (Cursor.isVisiblePrev && row == Cursor.y
		        && (j <= Cursor.x && Cursor.x <= end))
			Cursor.isVisiblePrev = false;

		// *) Now draw as much as possible in a row even if some symbols are not changed.
		// More calls for the sake of fewer symbols is slower, e.g. in panel status lines.
		int j2=j+1;

		bool NextDialog = true;
		int NextDialogX = -1, CurDialogX1 = 0, CurDialogX2 = 0, CurDialogI = 0;
		DWORD CurDialogFlags = 0;

		for (; j < end; j = j2)
		{
			if (NextDialog || (j == NextDialogX))
			{
				NextDialog = false;
				CurDialogX1 = -1;
				NextDialogX = CurDialogX2 = TextWidth;
				CurDialogI = -1; CurDialogFlags = 0;
				int nMax = TextWidth-1;
				// ���� "����� �����", ����� ������� (�� Z-order) ������� ������������ �������
				for (int i = mn_DialogsCount-1; i >= 0; i--)
				{
					if (mrc_Dialogs[i].Top <= row && row <= mrc_Dialogs[i].Bottom)
					{
						int border1 = mrc_Dialogs[i].Left;
						int border2 = mrc_Dialogs[i].Right;

						// ���� ����� �������, ������� �� ���� ������, ��������� � ������� X-����������.
						if (border1 && j <= border1 && border1 < NextDialogX)
							NextDialogX = border1;
						else if (border2 < nMax && j <= border2 && border2 < NextDialogX)
							NextDialogX = border2;

						// ���� ������ (�� ������), ������� �� ���� ������, ���������� ������� X-����������.
						TODO("����������� ������� ���� ������ �� ����������, �� ��� �� �� ����������");
						if (!(mn_DialogFlags[i] & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL|FR_VIEWEREDITOR)))
						{
							if ((border1 <= j && j <= border2)
								&& (CurDialogX1 <= border1 && border2 <= CurDialogX2))
							{
								CurDialogX1 = border1;
								CurDialogX2 = border2;
								CurDialogFlags = mn_DialogFlags[i];
								_ASSERTE(CurDialogFlags!=0);
								CurDialogI = i;
							}
						}
					}
				}
			}

			TODO("OPTIMIZE: ������� ���������� �� �����");
			TODO("OPTIMIZE: ���� ������/������� ��������� � ���������� (j>0 !) �� �� ����� �������� CharWidth, isChar..., isUnicode, ...");
			const CharAttr attr = ConAttrLine[j];
			wchar_t c = ConCharLine[j];
			//BYTE attrForeIdx, attrBackIdx;
			//COLORREF attrForeClr, attrBackClr;
			bool isUnicode, isDlgBorder = false;
			if (j == NextDialogX)
			{
				NextDialog = true;
				// ��������� ��� ���, ������� �����, ��� ������ ���������� ������ ������ �����?
				if (CurDialogFlags)
				{
					// ���������� ������ � ���� �������. same as below (1)
					isUnicode = (gpSet->isFixFarBorders && isCharBorder(c/*ConCharLine[j]*/));
					isDlgBorder = (j == CurDialogX1 || j == CurDialogX2); // ����� ������ ����� ����� ��� �����;
				}
				else
				{
					isDlgBorder = isUnicode = true;
				}
			}
			else
			{
				// same as above (1)
				isUnicode = (gpSet->isFixFarBorders && isCharBorder(c/*ConCharLine[j]*/));
			}
			bool isProgress = false, isSpace = false, isUnicodeOrProgress = false, isNonSpacing = false;
			bool lbS1 = false, lbS2 = false;
			int nS11 = 0, nS12 = 0, nS21 = 0, nS22 = 0;
			//GetCharAttr(c, attr, c, attrFore, attrBack);
			//GetCharAttr(attr, attrFore, attrBack, &hFont);
			//if (GetCharAttr(c, attr, c, attrFore, attrBack))
			//    isUnicode = true;
			hFont = mh_FontByIndex[attr.nFontIndex];

			if (isUnicode || bEnhanceGraphics)
				isProgress = isCharProgress(c); // ucBox25 / ucBox50 / ucBox75 / ucBox100

			isUnicodeOrProgress = isUnicode || isProgress;

			if (!isUnicodeOrProgress)
			{ 
				isNonSpacing = isCharNonSpacing(c);
				isSpace = isCharSpace(c);
			}

			TODO("��� ���������������� (�������� � DistrubuteSpaces) �������� ��������������� �� ����������� ���������� ������");
			// ����� ��������� �������� �������� �� ���������������� �������
			HEAPVAL

			// ������������� ���������� �������� � �����
			// 120616 - Chinese - off
			if (bProportional && bFixFrameCoord && (c==ucBoxDblHorz || c==ucBoxSinglHorz))
			{
				lbS1 = true; nS11 = nS12 = j;

				while ((nS12 < end) && (ConCharLine[nS12+1] == c))
					nS12 ++;

				// ��������� ������� ���� �� �������� ����� ������
				if (nS12<end)
				{
					nS21 = nS12+1; // ��� ������ ���� �� c

					// ���� ������ "��������" ������
					while ((nS21<end) && (ConCharLine[nS21] != c) && !isCharBorder(ConCharLine[nS21]))
						nS21 ++;

					if (nS21<end && ConCharLine[nS21]==c)
					{
						lbS2 = true; nS22 = nS21;

						while ((nS22 < end) && (ConCharLine[nS22+1] == c))
							nS22 ++;
					}
				}
			} HEAPVAL

			// � ��� ��� �������� - ����� �� ����� ������
			/*else if (c==L' ' && j<end && ConCharLine[j+1]==L' ')
			{
			    lbS1 = true; nS11 = nS12 = j;
			    while ((nS12 < end) && (ConCharLine[nS12+1] == c))
			        nS12 ++;
			}*/
			//(HDC)m_DC.SetTextColor(pColors[attrFore]);
			m_DC.SetTextColor(attr.crForeColor);

			// ������������� ��������� ������������ ����� (Coord.X>0)
			if (bProportional && j && bFixFrameCoord)
			{
				HEAPVAL;
				DWORD nPrevX = ConCharXLine[j-1];
				// ����� (�� ������������ �����) � ������ ���������;
				// �������� } ��� �������� ����� ������ �������� �� ������� �������...
				// ���� ������ ��� ����� �� ��� �� ������� ����� (��� ��� �� '}')
				// ������������ �������
				//bool bBord = isDlgBorder || isCharBorderVertical(c);
				//TODO("���� �� ����� ������, ��� ������ ����� ����� ���� ������ ������!");
				//bool bFrame = false; // mpn_ConAttrEx[row*TextWidth+j].bDialogVBorder;

				if (isDlgBorder || isCharBorderVertical(c) || isCharScroll(c) || (isFilePanel && c==L'}'))
				{
					//2009-04-21 ���� (j-1) * nFontWidth;
					//ConCharXLine[j-1] = j * nFontWidth;
					bool bMayBeFrame = true;

					// ������ ����� � ���� �� ������� ������, ��������,
					// ���� �� � ���� �� X-���������� ������������ ����� (��� ����)
					TODO("������ �� ��� ������� ���������� ������� ������� ����� ���������, ����� ����� - ����� API?");
					//if (!bBord && !bFrame)
					if (!isDlgBorder)
					{
						int R;
						wchar_t prevC;
						bool bBord = false;

						for (R = (row-1); bMayBeFrame && !bBord && R>=0; R--)
						{
							prevC = mpsz_ConChar[R*TextWidth+j];
							bBord = isCharBorderVertical(prevC);
							bMayBeFrame = (bBord || isCharScroll(prevC) || (isFilePanel && prevC==L'}'));
						}

						for (R = (row+1); bMayBeFrame && !bBord && R < (int)TextHeight; R++)
						{
							prevC = mpsz_ConChar[R*TextWidth+j];
							bBord = isCharBorderVertical(prevC);
							bMayBeFrame = (bBord || isCharScroll(prevC) || (isFilePanel && prevC==L'}'));
						}
					}

					// ��� ����������� ������� ������, ������ ��������� ��� ���� ��������� ������� �������
					if (bMayBeFrame)
						ConCharXLine[j-1] = j * nFontWidth;
				}

				//else if (isFilePanel && c==L'}') {
				//    if ((row>=2 && isCharBorderVertical(mpsz_ConChar[TextWidth+j]))
				//        && (((UINT)row)<=(TextHeight-4)))
				//        //2009-04-21 ���� (j-1) * nFontWidth;
				//        ConCharXLine[j-1] = j * nFontWidth;
				//    //row = TextHeight - 1;
				//}
				HEAPVAL;

				if (nPrevX < ConCharXLine[j-1])
				{
					// ��������� ���������� ����������� �������. ��������� ���-��?
					RECT rect;
					rect.left = nPrevX;
					rect.top = pos;
					rect.right = ConCharXLine[j-1];
					rect.bottom = rect.top + nFontHeight;

					if (gbNoDblBuffer) GdiFlush();

					const CharAttr PrevAttr = ConAttrLine[j-1];

					// ���� �� ��������� ���� ���������
					if (!(drawImage && ISBGIMGCOLOR(attr.nBackIdx)))
					{
						//BYTE PrevAttrFore = attrFore, PrevAttrBack = attrBack;
						wchar_t PrevC = ConCharLine[j-1];

						//GetCharAttr(PrevC, PrevAttr, PrevC, PrevAttrFore, PrevAttrBack);
						//GetCharAttr(PrevAttr, PrevAttrFore, PrevAttrBack, NULL);
						//if (GetCharAttr(PrevC, PrevAttr, PrevC, PrevAttrFore, PrevAttrBack))
						//	isUnicode = true;

						// ���� ������� ������ - ������������ �����, � ���������� ������ - �����
						// ����� �������� ����� �� �������� �������
						if (isCharBorderVertical(c) && isCharBorder(PrevC))
						{
							//m_DC.SetBkColor(pColors[attrBack]);
							m_DC.SetBkColor(attr.crBackColor);
							wchar_t *pchBorder = (c == ucBoxDblDownLeft || c == ucBoxDblUpLeft
							                      || c == ucBoxSinglDownDblHorz || c == ucBoxSinglUpDblHorz
							                      || c == ucBoxDblDownDblHorz || c == ucBoxDblUpDblHorz
												  || c == ucBoxDblVertLeft
							                      || c == ucBoxDblVertHorz) ? ms_HorzDbl : ms_HorzSingl;
							int nCnt = ((rect.right - rect.left) / CharWidth(pchBorder[0]))+1;

							if (nCnt > MAX_SPACES)
							{
								_ASSERTE(nCnt<=MAX_SPACES);
								nCnt = MAX_SPACES;
							}

							//UINT nFlags = ETO_CLIPPED; // || ETO_OPAQUE;
							//m_DC.ExtTextOut(rect.left, rect.top, nFlags, &rect, Spaces, min(nSpaceCount, nCnt), 0);
							//if (! (drawImage && ISBGIMGCOLOR(attr.nBackIdx)))
							//	m_DC.SetBkColor(pColors[attrBack]);
							//else if (drawImage)
							//	BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
							UINT nFlags = ETO_CLIPPED | ETO_OPAQUE;

							//wmemset(Spaces, chBorder, nCnt);
							if (bFixFarBorders)
								SelectFont(hFont2);

							m_DC.SetTextColor(PrevAttr.crForeColor);
							m_DC.ExtTextOut(rect.left, rect.top, nFlags, &rect, pchBorder, nCnt, 0);
							m_DC.SetTextColor(attr.crForeColor); // �������
						}
						else
						{
							HBRUSH hbr = PartBrush(L' ', PrevAttr.crBackColor, PrevAttr.crForeColor);
							FillRect((HDC)m_DC, &rect, hbr);
						}
					}
					else if (drawImage)
					{
						// ������� ����������� �������
						RECT rcFill = {rect.left, rect.top, rect.right, rect.bottom};
						PaintBackgroundImage(rcFill, PrevAttr.crBackColor);
					} HEAPVAL

					if (gbNoDblBuffer) GdiFlush();
				}
			}

			ConCharXLine[j] = (j ? ConCharXLine[j-1] : 0)+CharWidth(c);
			HEAPVAL

			#if 0
			if (bForceMonospace)
			{
				HEAPVAL
				//m_DC.SetBkColor(pColors[attrBack]);
				m_DC.SetBkColor(attr.crBackColor);
				j2 = j + 1;

				if (bFixFarBorders && isUnicode)
					SelectFont(hFont2);
				else
					SelectFont(hFont);

				RECT rect;

				rect = MakeRect(j * nFontWidth, pos, j2 * nFontWidth, pos + nFontHeight);

				UINT nFlags = /*ETO_CLIPPED |*/ ((drawImage && ISBGIMGCOLOR(attr.nBackIdx)) ? 0 : ETO_OPAQUE);
				int nShift = 0;
				HEAPVAL

				if (!isSpace && !isUnicodeOrProgress)
				{
					ABC abc;
					//��� �� TrueType ���������� wrapper (CharWidth)
					CharABC(c, &abc);

					if (abc.abcA<0)
					{
						// ����� ������ �������� ������� �� ����������?
						nShift = -abc.abcA;
					}
					else if (abc.abcB < (UINT)nFontWidth)
					{
						int nEdge = (nFontWidth - abc.abcB - 1) >> 1;

						if (abc.abcA < nEdge)
						{
							// ������ I, i, � ��. ����� ������ - ������ ����������
							nShift = nEdge - abc.abcA;
						}
					}
				}

				HEAPVAL
				BOOL lbImgDrawn = FALSE;

				if (!(drawImage && ISBGIMGCOLOR(attr.nBackIdx)))
				{
					//m_DC.SetBkColor(pColors[attrBack]);
					m_DC.SetBkColor(attr.crBackColor);

					// � ������ ForceMonospace ������� �������� �������� �� ������ (���� ��� ��� ����������)
					// ����� �� ���������� ������ �� ���������� ��������� - ����� ������ ���������� �����
					if (nShift>0 && !isSpace && !isProgress)
					{
						HBRUSH hbr = PartBrush(L' ', attr.crBackColor, attr.crForeColor);
						FillRect(m_DC, &rect, hbr);
					}
				}
				else if (drawImage)
				{
					BlitPictureTo(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, attr.crBackColor);
					lbImgDrawn = TRUE;
				}

				if (nShift>0)
				{
					rect.left += nShift;
					// ���� ������� ��� - ��� �������� �� ������ ������� (nShift > 0)
					//rect.right += nShift;
				}

				if (gbNoDblBuffer) GdiFlush();

				if (/*isSpace ||*/ (isProgress && bEnhanceGraphics))
				{
					HBRUSH hbr = PartBrush(c, attr.crBackColor, attr.crForeColor);
					FillRect(m_DC, &rect, hbr);
				}
				else if (/*gpSet->isProportional &&*/ isSpace/*c == ' '*/)
				{
					//int nCnt = ((rect.right - rect.left) / CharWidth(L' '))+1;
					//Ext Text Out(m_DC, rect.left, rect.top, nFlags, &rect, Spaces, nCnt, 0);
					TODO("���������, ��� ����� ���� �������� ������ �� ������ ��� ������� ���������");

					if (!lbImgDrawn)
					{
						HBRUSH hbr = PartBrush(L' ', attr.crBackColor, attr.crForeColor);
						FillRect(m_DC, &rect, hbr);
					}
				}
				else
				{
					// ��� ����� Force monospace
					if (nFontCharSet == OEM_CHARSET && !isUnicode)
					{
						char cOem = Uni2Oem(c);
						m_DC.ExtTextOutA(rect.left, rect.top, nFlags, &rect, &cOem, 1, 0);
					}
					else
					{
						m_DC.ExtTextOut(rect.left, rect.top, nFlags, &rect, &c, 1, 0);
					}
				}

				if (gbNoDblBuffer) GdiFlush();

				HEAPVAL
			} // end  - if (gpSet->isForceMonospace)
			else
			#endif
			{
				wchar_t* pszDraw = NULL;
				int      nDrawLen = -1;
				bool     bDrawReplaced = false;
				RECT rect;

				//else if (/*gpSet->isProportional &&*/ (c==ucSpace || c==ucNoBreakSpace || c==0))
				if (isSpace)
				{
					j2 = j + 1; HEAPVAL;

					while (j2 < end && ConAttrLine[j2] == attr && isCharSpace(ConCharLine[j2]))
						j2++;

					DistributeSpaces(ConCharLine, ConAttrLine, ConCharXLine, j, j2, end);
				}
				else if (isNonSpacing)
				{
					j2 = j + 1; HEAPVAL
					wchar_t ch;
					//int nLastNonSpace = -1;
					while (j2 < end && ConAttrLine[j2] == attr
					        && isCharNonSpacing(ch = ConCharLine[j2]))
					{
						ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
						j2++;
					}
					SelectFont(hFont);
					HEAPVAL
				}
				else if (!isUnicodeOrProgress)
				{
					j2 = j + 1; HEAPVAL

					// ���� ����� �� ������ - � ���������������� ������� ����� ����� �������� ���� �� ������
					wchar_t ch;
					//int nLastNonSpace = -1;
					WARNING("*** �������� � ��������� �������: (!gpSet->isProportional || !isFilePanel || (ch != L'}' && ch!=L' '))");
					TODO("��� ������ �� ������ - ��������� nLastNonSpace");

					while (j2 < end && ConAttrLine[j2] == attr
							&& (ch = ConCharLine[j2], bFixFarBorders ? !isCharBorder(ch) : (!bEnhanceGraphics || !isCharProgress(ch)))
							&& !isCharNonSpacing(ch)
							&& (!bProportional || !isFilePanel || (ch != L'}' && ch!=L' '))) // ������������� ���� � ��������
					{
						ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
						j2++;
					}

					TODO("�� ������ - ��������� ������� (���� ���� ���� nLastNonSpace)");
					SelectFont(hFont);
					HEAPVAL
				}
				else
				{
					// ���� �� �������� ��� ��������, ������� �������������� "��������" �������
					// ������, ��� ����� ���� �� ������ �����, �� � ����� ���� ���, ��� ��������
					// ������������ (��������� ���������� -> gpSet->icFixFarBorderRanges)
					j2 = j + 1; HEAPVAL

					// ����� 25%, 50%, 75%, 100%
					if (bEnhanceGraphics && isProgress)
					{
						wchar_t ch;
						ch = c; // ����������� ��������� ��������� � ���������

						while (j2 < end && ConAttrLine[j2] == attr && ch == ConCharLine[j2+1])
						{
							ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
							j2++;
						}
					}
					else if (!bFixFarBorders)
					{
						// ���� ��������, ���� ��� ������ �����, �� 'Fix Far borders' ��������.
						// ����� � ���, ����� ���������� ����� ����� �� ���������� �������.
						// -- ���� ����� �������� �� ������ - ����� isUnicodeOrProgress ������ ���� false
						// -- �.�. � ��������� ��������� gpSet->isFixFarBorders
						// -- _ASSERTE(bFixFarBorders);
						// -- _ASSERTE(!isUnicodeOrProgress);
						wchar_t ch;

						while (j2 < end && ConAttrLine[j2] == attr && isCharBorder(ch = ConCharLine[j2]))
						{
							ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
							j2++;
						}
					}
					else
					{
						//wchar_t ch;
						//WARNING("��� ������������ �� ucBoxDblVert � ������ �������. �� ������ ������ � ...");
						// ��� ��������� ����� (ucBoxDblHorz) �� ���� ������
						bool bIsHorzBorder = (c == ucBoxDblHorz || c == ucBoxSinglHorz);

						if (bIsHorzBorder)
						{
							while (j2 < end && ConAttrLine[j2] == attr && c == ConCharLine[j2])
								j2++;
						}

						DistributeSpaces(ConCharLine, ConAttrLine, ConCharXLine, j, j2, end);
						int nBorderWidth = CharWidth(c);
						rect.left = j ? ConCharXLine[j-1] : 0;
						rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
						int nCnt = (rect.right - rect.left + (nBorderWidth>>1)) / nBorderWidth;

						if (nCnt > (j2 - j))
						{
							if (c==ucBoxDblHorz || c==ucBoxSinglHorz)
							{
								_ASSERTE(nCnt <= MAX_SPACES);

								if (nCnt > MAX_SPACES) nCnt = MAX_SPACES;

								bDrawReplaced = true;
								nDrawLen = nCnt;
								pszDraw = (c==ucBoxDblHorz) ? ms_HorzDbl : ms_HorzSingl;
							}
							#ifdef _DEBUG
							else
							{
								//static bool bShowAssert = true;
								//if (bShowAssert) {
								_ASSERTE(!bIsHorzBorder || (c==ucBoxDblHorz || c==ucBoxSinglHorz));
								//}
							}
							#endif
						}

						//while (j2 < end && ConAttrLine[j2] == attr &&
						//    isCharBorder(ch = ConCharLine[j2]) && ch == ConCharLine[j2+1])
						//{
						//    ConCharXLine[j2] = (j2 ? ConCharXLine[j2-1] : 0)+CharWidth(ch);
						//    j2++;
						//}
					}

					SelectFont(bFixFarBorders ? hFont2 : hFont);
					HEAPVAL
				}

				if (!pszDraw)
				{
					pszDraw = ConCharLine + j;
					nDrawLen = j2 - j;
				}

				//if (!gpSet->isProportional) {
				//	TODO("���-�� ���-��... ���� ��������� ��� ����� ���������?");
				//  rect = MakeRect(j * nFontWidth, pos, j2 * nFontWidth, pos + nFontHeight);
				//} else {
				rect.left = j ? ConCharXLine[j-1] : 0;
				rect.top = pos;
				rect.right = (TextWidth>(UINT)j2) ? ConCharXLine[j2-1] : Width;
				rect.bottom = rect.top + nFontHeight;
				//}
				HEAPVAL
				BOOL lbImgDrawn = FALSE;

				if (!(drawImage && ISBGIMGCOLOR(attr.nBackIdx)))
				{
					//m_DC.SetBkColor(pColors[attrBack]);
					m_DC.SetBkColor(attr.crBackColor);
				}
				else if (drawImage)
				{
					// ���������� ���� (BgImage) ��� ��������� ������ �� ���
					RECT rcFill = {rect.left, rect.top, rect.right, rect.bottom};
					PaintBackgroundImage(rcFill, attr.crBackColor);
					lbImgDrawn = TRUE;
				}

				WARNING("ETO_CLIPPED ����� ������ �� �������� ������, ���� ��� ����� �� �������?");
				UINT nFlags = ETO_CLIPPED | ((drawImage && ISBGIMGCOLOR(attr.nBackIdx)) ? 0 : ETO_OPAQUE);
				HEAPVAL

				if (gbNoDblBuffer) GdiFlush();

				if (isProgress && bEnhanceGraphics)
				{
					HBRUSH hbr = PartBrush(c, attr.crBackColor, attr.crForeColor);
					FillRect((HDC)m_DC, &rect, hbr);
				}
				else if (/*gpSet->isProportional &&*/ isSpace/*c == ' '*/)
				{
					//int nCnt = ((rect.right - rect.left) / CharWidth(L' '))+1;
					//Ext Text Out(m_DC, rect.left, rect.top, nFlags, &rect, Spaces, nCnt, 0);
					TODO("���������, ��� ����� ���� �������� ������ �� ������ ��� ������� ���������");

					if (!lbImgDrawn)
					{
						HBRUSH hbr = PartBrush(L' ', attr.crBackColor, attr.crForeColor);
						FillRect((HDC)m_DC, &rect, hbr);
					}
				}
				else
				{
					// � ����� ����������� ������� �������� bProportional �� ������� �����
					/*if (bProportional) {

						if (!isSpace && !isUnicodeOrProgress) {
							ABC abc;
							//��� �� TrueType ���������� wrapper (CharWidth)
							CharABC(c, &abc);

							if (abc.abcA<0) {
								// ����� ������ �������� ������� �� ����������?
								nShift = -abc.abcA;

							} else if (abc.abcB < (UINT)nFontWidth)  {
								int nEdge = (nFontWidth - abc.abcB - 1) >> 1;
								if (abc.abcA < nEdge) {
									// ������ I, i, � ��. ����� ������ - ������ ����������
									nShift = nEdge - abc.abcA;
								}
							}
						}

					} else {
					}*/
					if (nFontCharSet == OEM_CHARSET && !(bFixFarBorders && isUnicode))
					{
						if (nDrawLen > (int)TextWidth)
						{
							_ASSERTE(nDrawLen <= (int)TextWidth);
							nDrawLen = TextWidth;
						}

						WideCharToMultiByte(CP_OEMCP, 0, pszDraw, nDrawLen, tmpOem, TextWidth, 0, 0);

						if (!bProportional)
						{
							for(int idx = 0; idx < nDrawLen; idx++)
							{
								WARNING("BUGBUG: ��� ������ ����� ���������� ��� ��������� ������ OEM �������?");
								nDX[idx] = CharWidth(tmpOem[idx]);
							}
						}

						m_DC.ExtTextOutA(rect.left, rect.top, nFlags,
						            &rect, tmpOem, nDrawLen, bProportional ? 0 : nDX);
					}
					else
					{
						int nShift0 = 0, nPrevEdge = 0;
						ABC abc;

						// ��� ���������������� ������� - ��� �������� ������ ������ ����������� GDI
						if (!bProportional)
						{
							if (!bForceMonospace)
							{
								// � ���� ������ ������ �������������� � ������ ����� ������
								for (int idx = 0, n = nDrawLen; n; idx++, n--)
								{
									// ���������� ��� GDI. ������ ���������, �.�. ����� ����� ������� � ��������� ��������.
									nDX[idx] = nFontWidth; //CharWidth(pszDraw[idx]); -- ������ ����� �������
								}
							}
							else
							{
								for (int idx = 0, n = nDrawLen; n; idx++, n--)
								{
									WARNING("��� RTL ����� ����������");
									// GDI ��������� nDX �� �� ������� ���������� �������� � ������,
									// � �� ������� ��������� (��� �������), �� ���� ����� �������� �� ������,
									// ����� ����� RTL � ������� nDX ��� pszDraw � �������� �������


									wchar_t ch = pszDraw[idx];
									if (isCharSpace(ch) || isCharBorder(ch) || isCharProgress(ch))
									{
										abc.abcA = abc.abcC = 0;
										abc.abcB = nFontWidth;
									}
									else
									{
										//��� �� TrueType ���������� wrapper (CharWidth)
										CharABC(ch, &abc);
									}

									int nDrawWidth = abc.abcA + abc.abcB + 1;

									//if (!idx)
									//	nShift0 = abc.abcA;

									#ifdef _DEBUG
									if (abc.abcA!=0)
									{
										int nDbgA = 0;
									}
									if (abc.abcC!=0)
									{
										// ��� �������� ���������� - "1"
										int nDbgC = 0;
									}
									#endif

									//if (abc.abcA<0)
									//{
									//	// ����� ������ �������� ������� �� ����������?
									//	//nShift = -abc.abcA;
									//	nDX[idx] = abc.abcA + abc.abcB + abc.abcC;
									//}
									//else
									if (nDrawWidth < nFontWidth)
									{
										int nEdge = ((nFontWidth - nDrawWidth) >> 1) - nPrevEdge;

										if (idx)
										{
											nDX[idx-1] += nEdge;
											_ASSERTE(nDX[idx-1]>=-100);
										}
										else
										{
											nShift0 += nEdge;
										}

										nPrevEdge += nEdge;

										nDX[idx] = nFontWidth;

										//if (abc.abcA < nEdge)
										//{
										//	// ������ I, i, � ��. ����� ������ - ������ ����������
										//	nShift = nEdge - abc.abcA;
										//}
									}
									else
									{
										// ������ �������������� ����� ������ ��� ����������,
										// �� �.�. ���� ����� ����� Monospace - �������������
										// ���������� ������ ������
										nDX[idx] = nFontWidth; // abc.abcA + abc.abcB /*+ abc.abcC*/;
										if (nPrevEdge)
										{
											_ASSERTE(idx>0 && "Must be, cause of nPrevEdge");
											nDX[idx-1] -= nPrevEdge;
											nPrevEdge = 0;
										}
										//_ASSERTE(abc.abcC==0 && "Check what symbols can produce '!=0'");
									}
								}
							}
						}

						// nDX ��� ������ �� ������ ���������� �������, � ������ �����������
						m_DC.ExtTextOut(rect.left+nShift0, rect.top, nFlags, &rect,
						           pszDraw, nDrawLen, bProportional ? 0 : nDX);
					}
				}

				if (gbNoDblBuffer) GdiFlush();

				HEAPVAL
			}

			DUMPDC(L"F:\\vcon.png");
			//-- �� ��������� ������� � ����������������� �������� � ClearType
			//-- ������ ���������� ������ �������
			//// skip next not changed symbols again
			//if (skipNotChanged)
			//{
			//    HEAPVAL
			//	wchar_t ch;
			//    // skip the same except spaces
			//    while (j2 < end && (ch = ConCharLine[j2]) == ConCharLine2[j2] && ConAttrLine[j2] == ConAttrLine2[j2]
			//		&& (ch != ucSpace && ch != ucNoBreakSpace && ch != 0))
			//    {
			//        ++j2;
			//    }
			//}
		}
	}

	if ((pos > nMaxPos) && (pos < (int)Height))
	{
		bool lbDelBrush = false;
		HBRUSH hBr = CreateBackBrush(false, lbDelBrush);

		RECT rcFill = {0, pos, Width, Height};
		FillRect((HDC)m_DC, &rcFill, hBr);

		if (lbDelBrush)
			DeleteObject(hBr);
	}

	free(nDX);

	// Screen updated, reset until next "UpdateHighlights()" call
	m_HighlightInfo.m_Last.X = m_HighlightInfo.m_Last.Y = -1;
	ZeroStruct(m_HighlightInfo.mrc_LastRow);
	ZeroStruct(m_HighlightInfo.mrc_LastCol);

	return;
}

void CVirtualConsole::ClearPartBrushes()
{
	_ASSERTE(gpConEmu->isMainThread());

	for(UINT br=0; br<MAX_COUNT_PART_BRUSHES; br++)
	{
		DeleteObject(m_PartBrushes[br].hBrush);
	}

	memset(m_PartBrushes, 0, sizeof(m_PartBrushes));
}

HBRUSH CVirtualConsole::PartBrush(wchar_t ch, COLORREF nBackCol, COLORREF nForeCol)
{
	_ASSERTE(gpConEmu->isMainThread());
	//std::vector<PARTBRUSHES>::iterator iter = m_PartBrushes.begin();
	//while (iter != m_PartBrushes.end()) {
	PARTBRUSHES *pbr = m_PartBrushes;

	for(UINT br=0; pbr->ch && br<MAX_COUNT_PART_BRUSHES; br++, pbr++)
	{
		if (pbr->ch == ch && pbr->nBackCol == nBackCol && pbr->nForeCol == nForeCol)
		{
			_ASSERTE(pbr->hBrush);
			return pbr->hBrush;
		}
	}

	MYRGB clrBack, clrFore, clrMy;
	clrBack.color = nBackCol;
	clrFore.color = nForeCol;
	clrMy.color = clrFore.color; // 100 %
	////#define   PART_75(f,b) ((((int)f) + ((int)b)*3) / 4)
	////#define   PART_50(f,b) ((((int)f) + ((int)b)) / 2)
	////#define   PART_25(f,b) (((3*(int)f) + ((int)b)) / 4)
	////#define   PART_75(f,b) (b + 0.80*(f-b))
	////#define   PART_50(f,b) (b + 0.75*(f-b))
	////#define   PART_25(f,b) (b + 0.50*(f-b))
	// (gpSet->isPartBrushXX >> 8) ���� ��� �� ����� (0 .. 1), ��������, 0.80
#define PART_XX(f,b,p) ((f==b) ? f : ( (b + ((p*(f-b))>>8))))
#define PART_75(f,b) ((f>b) ? PART_XX(f,b,gpSet->isPartBrush75) : PART_XX(b,f,gpSet->isPartBrush25))
#define PART_50(f,b) ((f>b) ? PART_XX(f,b,gpSet->isPartBrush50) : PART_XX(b,f,gpSet->isPartBrush50))
#define PART_25(f,b) ((f>b) ? PART_XX(f,b,gpSet->isPartBrush25) : PART_XX(b,f,gpSet->isPartBrush75))

	if (ch == ucBox75 || ch == ucBox50 || ch == ucBox25)
	{
		// ��� ������ ���������. ���� 25% ������ �� ���������� �� ������������ 100% (������� ������)
		if (clrBack.color == 0)
		{
			if (gpSet->isPartBrushBlack
			        && clrFore.rgbRed > gpSet->isPartBrushBlack
			        && clrFore.rgbGreen > gpSet->isPartBrushBlack
			        && clrFore.rgbBlue > gpSet->isPartBrushBlack)
			{
				clrBack.rgbBlue = clrBack.rgbGreen = clrBack.rgbRed = gpSet->isPartBrushBlack;
			}
		}
		else if (clrFore.color == 0)
		{
			if (gpSet->isPartBrushBlack
			        && clrBack.rgbRed > gpSet->isPartBrushBlack
			        && clrBack.rgbGreen > gpSet->isPartBrushBlack
			        && clrBack.rgbBlue > gpSet->isPartBrushBlack)
			{
				clrFore.rgbBlue = clrFore.rgbGreen = clrFore.rgbRed = gpSet->isPartBrushBlack;
			}
		}
	}

	if (ch == ucBox75 /* 75% */)
	{
		clrMy.rgbRed = PART_75(clrFore.rgbRed,clrBack.rgbRed);
		clrMy.rgbGreen = PART_75(clrFore.rgbGreen,clrBack.rgbGreen);
		clrMy.rgbBlue = PART_75(clrFore.rgbBlue,clrBack.rgbBlue);
		clrMy.rgbReserved = 0;
	}
	else if (ch == ucBox50 /* 50% */)
	{
		clrMy.rgbRed = PART_50(clrFore.rgbRed,clrBack.rgbRed);
		clrMy.rgbGreen = PART_50(clrFore.rgbGreen,clrBack.rgbGreen);
		clrMy.rgbBlue = PART_50(clrFore.rgbBlue,clrBack.rgbBlue);
		clrMy.rgbReserved = 0;
	}
	else if (ch == ucBox25 /* 25% */)
	{
		clrMy.rgbRed = PART_25(clrFore.rgbRed,clrBack.rgbRed);
		clrMy.rgbGreen = PART_25(clrFore.rgbGreen,clrBack.rgbGreen);
		clrMy.rgbBlue = PART_25(clrFore.rgbBlue,clrBack.rgbBlue);
		clrMy.rgbReserved = 0;
	}
	else if (ch == ucSpace || ch == ucNoBreakSpace /* Non breaking space */ || !ch)
	{
		clrMy.color = clrBack.color;
	}

	PARTBRUSHES pb;
	pb.ch = ch; pb.nBackCol = nBackCol; pb.nForeCol = nForeCol;
	pb.hBrush = CreateSolidBrush(clrMy.color);
	//m_PartBrushes.push_back(pb);
	*pbr = pb;
	return pb.hBrush;
}

void CVirtualConsole::UpdateCursorDraw(HDC hPaintDC, RECT rcClient, COORD pos, UINT dwSize)
{
	if (!mp_RCon || !mp_RCon->isNeedCursorDraw())
		return;
	//if (mp_RCon && mp_RCon->GuiWnd())
	//{
	//	// � GUI ������ VirtualConsole ������ ��� GUI ����� � ����� ������ ��� "���������" BufferHeight
	//	if (!mp_RCon->isBufferHeight())
	//	{
	//		return;
	//	}
	//}

	Cursor.x = csbi.dwCursorPosition.X;
	Cursor.y = csbi.dwCursorPosition.Y;
	int CurChar = pos.Y * TextWidth + pos.X;

	if (CurChar < 0 || CurChar>=(int)(TextWidth * TextHeight))
	{
		return; // ����� ���� ��� ���� - ��� ������ ������� ��� ����� �������� � ���������� ������� ������� ������� � ������
	}

	if (!ConCharX)
	{
		return; // ������
	}

	COORD pix;
	pix.X = pos.X * nFontWidth;
	pix.Y = pos.Y * nFontHeight;

	if (pos.X && ConCharX[CurChar-1])
		pix.X = ConCharX[CurChar-1];

	RECT rect;
	TODO("DoubleView: ��������� ����������� �����");
	bool bActive = gpConEmu->isMeForeground()
		&& CVConGroup::isActive(this, false);

	// ��������� �� ��������� ����������� �� �����������
	mp_Set = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());

	//bool bHollowBlock = false;
	CECursorStyle curStyle = mp_Set->CursorStyle(bActive);
	int MinSize = mp_Set->CursorMinSize(bActive);

	//if ((!bForeground && mp_Set->CursorBlockInactive()) || (mp_Set->CursorType() == 2)) // Hollow-Block
	if ((curStyle == cur_Block) || (curStyle == cur_Rect))
	{
		//bHollowBlock = true;
		dwSize = 100; // percents
		rect.left = pix.X; /*Cursor.x * nFontWidth;*/
		rect.right = pix.X + nFontWidth; /*(Cursor.x+1) * nFontWidth;*/ //TODO: � ���� ������� ���������� ������� ��������!
		rect.bottom = (pos.Y+1) * nFontHeight;
		rect.top = (pos.Y * nFontHeight) /*+ 1*/;
	}
	else if (curStyle == cur_Horz) // Horizontal
	{
		if (!gpSet->isMonospace)
		{
			rect.left = pix.X; /*Cursor.x * nFontWidth;*/
			rect.right = pix.X + nFontWidth; /*(Cursor.x+1) * nFontWidth;*/ //TODO: � ���� ������� ���������� ������� ��������!
		}
		else
		{
			rect.left = pos.X * nFontWidth;
			rect.right = (pos.X+1) * nFontWidth;
		}

		//rect.top = (Cursor.y+1) * nFontHeight - MulDiv(nFontHeight, cinf.dwSize, 100);
		rect.bottom = (pos.Y+1) * nFontHeight;
		rect.top = (pos.Y * nFontHeight) /*+ 1*/;
		//if (cinf.dwSize<50)
		int nHeight = 0;

		if (dwSize)
		{
			nHeight = MulDiv(nFontHeight, dwSize, 100);

			nHeight = min(nFontHeight,max(nHeight,MinSize));
			//if (nHeight < HCURSORHEIGHT) nHeight = HCURSORHEIGHT;
		}

		//if (nHeight < HCURSORHEIGHT) nHeight = HCURSORHEIGHT;
		rect.top = max(rect.top, (rect.bottom-nHeight));
	}
	else // Vertical
	{
		_ASSERTE(curStyle == cur_Vert); // Validate type

		if (!gpSet->isMonospace)
		{
			rect.left = pix.X; /*Cursor.x * nFontWidth;*/
			//rect.right = rect.left/*Cursor.x * nFontWidth*/ //TODO: � ���� ������� ���������� ������� ��������!
			//  + klMax(1, MulDiv(nFontWidth, cinf.dwSize, 100)
			//  + (cinf.dwSize > 10 ? 1 : 0));
		}
		else
		{
			rect.left = pos.X * nFontWidth;
			//rect.right = Cursor.x * nFontWidth
			//  + klMax(1, MulDiv(nFontWidth, cinf.dwSize, 100)
			//  + (cinf.dwSize > 10 ? 1 : 0));
		}

		rect.top = pos.Y * nFontHeight;
		int nR = (!gpSet->isMonospace && ConCharX[CurChar]) // ������ �������
		         ? ConCharX[CurChar] : ((pos.X+1) * nFontWidth);
		//if (cinf.dwSize>=50)
		//  rect.right = nR;
		//else
		//  rect.right = min(nR, (rect.left+VCURSORWIDTH));
		int nWidth = 0;

		if (dwSize)
		{
			int nMaxWidth = (nR - rect.left);

			nWidth = MulDiv(nMaxWidth, dwSize, 100);

			nWidth = min(nMaxWidth,max(nWidth,MinSize));
			//if (nWidth < VCURSORWIDTH) nWidth = VCURSORWIDTH;
		}

		rect.right = min(nR, (rect.left+nWidth));
		//rect.right = rect.left/*Cursor.x * nFontWidth*/ //TODO: � ���� ������� ���������� ������� ��������!
		//      + klMax(1, MulDiv(nFontWidth, cinf.dwSize, 100)
		//      + (cinf.dwSize > 10 ? 1 : 0));
		rect.bottom = (pos.Y+1) * nFontHeight;
	}

	rect.left += rcClient.left;
	rect.right += rcClient.left;
	rect.top += rcClient.top;
	rect.bottom += rcClient.top;
	// ���� ������ �������� ����� 40% ������� - ������������� �������
	// XOR �����, ����� (��� ����� ��� ����������) ������ ���������
	// ���� ������ � ��� �� �����
	// 110131 ���� ������ �������� - �������� ��������� ������ ��� AltIns � ����
	bool bCursorColor = mp_Set->CursorColor(bActive)
		|| (dwSize >= 40 && !mp_Set->CursorBlink(bActive) && (curStyle != cur_Rect));

	// ������ � rect ����� ���������� ������ (XOR'�� ���������?)
	if (bCursorColor)
	{
		HBRUSH hBr = CreateSolidBrush(CURSOR_PAT_COLOR);
		HBRUSH hOld = (HBRUSH)SelectObject(hPaintDC, hBr);

		PatInvertRect(hPaintDC, rect, (HDC)m_DC, (curStyle != cur_Rect));

		SelectObject(hPaintDC, hOld);
		DeleteObject(hBr);
		return;
	}

	//lbDark = Cursor.foreColorNum < 5; // ���� ������
	//BOOL lbIsProgr = isCharProgress(Cursor.ch[0]);
	BOOL lbDark = FALSE;
	DWORD clr = (Cursor.ch != ucBox100 && Cursor.ch != ucBox75) ? Cursor.bgColor : Cursor.foreColor;
	BYTE R = (clr & 0xFF);
	BYTE G = (clr & 0xFF00) >> 8;
	BYTE B = (clr & 0xFF0000) >> 16;
	lbDark = (R <= 0xC0) && (G <= 0xC0) && (B <= 0xC0);
	clr = lbDark ? mp_Colors[15] : mp_Colors[0];
	HBRUSH hBr = CreateSolidBrush(clr);
	if (curStyle != cur_Rect)
	{
		FillRect(hPaintDC, &rect, hBr);
	}
	else
	{
		RECT r = {rect.left, rect.top+1, rect.left+1, rect.bottom-1};
		FillRect(hPaintDC, &r, hBr);
		r = MakeRect(rect.left+1, rect.top, rect.right-1, rect.top+1);
		FillRect(hPaintDC, &r, hBr);
		r = MakeRect(rect.right-1, rect.top+1, rect.right, rect.bottom-1);
		FillRect(hPaintDC, &r, hBr);
		r = MakeRect(rect.left+1, rect.bottom-1, rect.right-1, rect.bottom);
		FillRect(hPaintDC, &r, hBr);
	}
	DeleteObject(hBr);
}

void CVirtualConsole::UpdateCursor(bool& lRes)
{
	if (mp_RCon && mp_RCon->GuiWnd())
	{
		// � GUI ������ VirtualConsole ������ ��� GUI ����� � ����� ������ ��� "���������" BufferHeight
		if (!mp_RCon->isBufferHeight())
		{
			lRes = false;
			return;
		}
	}

	// ��������� �� ��������� ����������� �� �����������
	mp_Set = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());

	//------------------------------------------------------------------------
	///| Drawing cursor |/////////////////////////////////////////////////////
	//------------------------------------------------------------------------
	Cursor.isVisiblePrevFromInfo = cinf.bVisible;
	BOOL lbUpdateTick = FALSE;
	bool bForeground = gpConEmu->isMeForeground();
	TODO("DoubleView: ����������� �����");
	bool bConActive = CVConGroup::isActive(this, false); // � ��� ������ Active, �.�. ������ ������ ������ � �������� �������
	bool bConVisible = bConActive || gpConEmu->isVisible(this);
	bool bIsAlive = mp_RCon ? (mp_RCon->isAlive() || !mp_RCon->isFar(TRUE)) : false; // �� ������, ���� ��� "������"
	//if (bConActive) {
	//	bForeground = gpConEmu->isMeForeground();
	//}

	// ���� ������ (� �������) �����, � ��������� � ������� ������� (��� ���������)
	if (cinf.bVisible && isCursorValid)
	{
		if (!mp_Set->CursorBlink(bConActive && bForeground) /*|| !bForeground || !bConActive*/)
		{
			Cursor.isVisible = true; // ����� ������ (���� � ���������� �������), �� ������

			if (Cursor.isPrevBackground == bForeground)
			{
				lRes = true;
			}
		}
		else
		{
			// ����� ������� ������� - ��� ����� ��������, ���� ���� �������
			if ((Cursor.x != csbi.dwCursorPosition.X) || (Cursor.y != csbi.dwCursorPosition.Y))
			{
				Cursor.isVisible = bConVisible;

				if (Cursor.isVisible) lRes = true;  //force, pos changed

				lbUpdateTick = TRUE;
			}
			else
			{
				// ������� ����� �������
				if ((GetTickCount() - Cursor.nLastBlink) > Cursor.nBlinkTime)
				{
					if (gpConEmu->isRightClickingPaint())
					{
						// ������ ������ ������ �����, ������ ��������� ��������
						Cursor.isVisible = false;
					}
					else if (Cursor.isPrevBackground && bForeground)
					{
						// ����� "�����������" ������� - ����� �������� ��������, ������ ����� - ������
						Cursor.isVisible = true;
						lRes = true;
					}
					else
					{
						WARNING("DoubleView: � ���������� �� ������� ������� ���� ����������, �� ������");
						Cursor.isVisible = bConVisible && (!Cursor.isVisible || !bIsAlive);
					}

					lbUpdateTick = TRUE;
				}
			}
		}
	}
	else
	{
		// ����� - ��� ����� �������� (������ ����� � �������, ��� ���� �� ������� ������)
		Cursor.isVisible = false;
	}

	// ����� ��������� ����� � ConEmu
	if (Cursor.isVisible != Cursor.isVisiblePrev && !gpConEmu->isRightClickingPaint())
	{
		lRes = true;
		lbUpdateTick = TRUE;
	}

	if (Cursor.isVisible)
		Cursor.isPrevBackground = !bForeground;

	// �������� ����� ���������
	//Cursor.x = csbi.dwCursorPosition.X;
	//Cursor.y = csbi.dwCursorPosition.Y;
	Cursor.isVisiblePrev = Cursor.isVisible;

	if (lbUpdateTick)
		Cursor.nLastBlink = GetTickCount();
}


LPVOID CVirtualConsole::Alloc(size_t nCount, size_t nSize)
{
	HEAPVAL;
	
	size_t nWhole = nCount * nSize;
	LPVOID ptr = HeapAlloc(mh_Heap, HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, nWhole);
	
	//HEAPVAL;
	return ptr;
}

void CVirtualConsole::Free(LPVOID ptr)
{
	if (ptr && mh_Heap)
	{
		HEAPVAL;

		HeapFree(mh_Heap, 0, ptr);

		//HEAPVAL;
	}
}

bool CVirtualConsole::StretchPaint(HDC hPaintDC, int anX, int anY, int anShowWidth, int anShowHeight)
{
	if (!this)
		return false;

	if (!gpConEmu->isMainThread())
	{
		_ASSERTE(gpConEmu->isMainThread());
		return false;
	}

	if (!(HDC)m_DC && mpsz_ConChar)
	{
		Update();
	}

	bool bPaintRC = false;
	
	if ((HDC)m_DC)
	{
		SetStretchBltMode(hPaintDC, HALFTONE);
		bPaintRC = StretchBlt(hPaintDC, anX,anY,anShowWidth,anShowHeight, (HDC)m_DC, 0,0,Width,Height, SRCCOPY);
	}
	else
	{
		_ASSERTE((HDC)m_DC!=NULL);
	}

	return bPaintRC;
}

HBRUSH CVirtualConsole::CreateBackBrush(bool bGuiVisible, bool& rbNonSystem, COLORREF *pColors /*= NULL*/)
{
	HBRUSH hBr = NULL;
	
	if (bGuiVisible)
	{
		hBr = GetSysColorBrush(COLOR_APPWORKSPACE);
		rbNonSystem = false;
	}
	else
	{
		if (!pColors)
		{
			pColors = gpSet->GetColors(mp_RCon->GetActiveAppSettingsId());
		}

		// ������ ������ 0
		int nBackColorIdx = mp_RCon->GetDefaultBackColorIdx(); // Black
		//#ifdef _DEBUG
		//nBackColorIdx = 2; // Green
		//#endif
		
		hBr = CreateSolidBrush(pColors[nBackColorIdx]);
		rbNonSystem = (hBr != NULL);
	}

	return hBr;
}

// PaintRect may be specified for stretching
bool CVirtualConsole::PrintClient(HDC hPrintDc, bool bAllowRepaint, const LPRECT PaintRect)
{
	BOOL lbBltRc = FALSE;
	CVirtualConsole* pVCon = this;
	CVConGuard guard(pVCon);

	if (!pVCon)
		return false;

	if (!gpConEmu->isMainThread())
	{
		_ASSERTE(gpConEmu->isMainThread());
		return false;
	}

	if (((HDC)m_DC) != NULL)
	{
		RECT rcClient = GetDcClientRect();
		int  nPrintX = 0, nPrintY = 0;

		bool bNeedStretch = false;
		if (PaintRect != NULL)
		{
			bNeedStretch = (((rcClient.right-rcClient.left) != (PaintRect->right-PaintRect->left))
				|| ((rcClient.bottom-rcClient.top) != (PaintRect->bottom-PaintRect->top)));
		}

		if (bNeedStretch)
		{
			lbBltRc = StretchPaint(hPrintDc, PaintRect->left, PaintRect->top, PaintRect->right-PaintRect->left, PaintRect->bottom-PaintRect->top);
		}
		else
		{
			if (bAllowRepaint)
			{
				Update(mb_RequiredForceUpdate);
			}

			lbBltRc = BitBlt(
				hPrintDc, nPrintX, nPrintY, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top,
				(HDC)m_DC, 0, 0,
				SRCCOPY);
		}
	}

	UNREFERENCED_PARAMETER(lbBltRc);
	return (lbBltRc != 0);
}

// hPaintDc - ��� DC ViewWnd!!!
// rcClient - ��� �����, ���� ����� "��������" ������. ����� ���� ������������!
void CVirtualConsole::PaintVCon(HDC hPaintDc)
{
	// ���� "�����" PostUpdate
	if (gpConEmu->mp_TabBar->NeedPostUpdate())
		gpConEmu->mp_TabBar->Update();

	if (!mh_WndDC)
	{
		_ASSERTE(mh_WndDC!=NULL);
		return;
	}

	CVConGuard guard(this);

	if (!gpConEmu->isMainThread())
	{
		_ASSERTE(gpConEmu->isMainThread());
		return;
	}

	RECT rcClient = GetDcClientRect();

	_ASSERTE(hPaintDc);
	_ASSERTE(rcClient.left!=rcClient.right && rcClient.top!=rcClient.bottom);

	WARNING("TODO: Need to offset mrc_Client if padding exists");
	mrc_Client = rcClient;
	WARNING("TODO: Need to calculate mrc_Back = {0,0}-{BackWidth,BackHeight}");
	mrc_Back = mrc_Client;

	BOOL lbSimpleBlack = FALSE, lbGuiVisible = FALSE;

	if (!guard.VCon())
		lbSimpleBlack = TRUE;
	else if (!mp_RCon)
		lbSimpleBlack = TRUE;
	else if (!mp_RCon->ConWnd() && !mp_RCon->GuiWnd())
		lbSimpleBlack = TRUE;
	else if ((lbGuiVisible = mp_RCon->isGuiOverCon()))
		lbSimpleBlack = TRUE;
#ifdef _DEBUG
	else
	{
		int nConNo = gpConEmu->isVConValid(this);
		nConNo = nConNo;
	}
#endif

	if (lbSimpleBlack)
	{
		guard.VCon()->PaintVConSimple(hPaintDc, rcClient, lbGuiVisible);

		if (guard.VCon() && mp_Ghost)
			mp_Ghost->UpdateTabSnapshoot(); //CreateTabSnapshoot(hPaintDc, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
	}
	else
	{
		PaintVConNormal(hPaintDc, rcClient);

		if (mp_Ghost)
			mp_Ghost->UpdateTabSnapshoot(); //CreateTabSnapshoot(hPaintDc, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);

		// Debug rects and (scrolllock)counter
		PaintVConDebug(hPaintDc, rcClient);
	}
}

void CVirtualConsole::PaintVConSimple(HDC hPaintDc, RECT rcClient, BOOL bGuiVisible)
{
	if (!this)
	{
		_ASSERTE(FALSE && "Called with NULL pVCon");
		COLORREF crBack = gpSet->GetColors(-1, isFade)[0];
		HBRUSH hBr = CreateSolidBrush(crBack);
		FillRect(hPaintDc, &rcClient, hBr);
		DeleteObject(hBr);
		return;
	}

	// ����� ����������, ���� ����
	LockDcRect(false);

	COLORREF *pColors = GetColors();
		
	bool lbDelBrush = false;
	HBRUSH hBr = CreateBackBrush(bGuiVisible, lbDelBrush, pColors);
		
	//#ifndef SKIP_ALL_FILLRECT
	FillRect(hPaintDc, &rcClient, hBr);
	//#endif
		
	TODO("���������� �� StatusBar, ���� gpSet->isStatusBarShow");
	if (!bGuiVisible)
	{
		CEDC cePaintDc(hPaintDc);
		CEFONT hOldF = cePaintDc.SelectObject(gpSetCls->mh_Font[0]);
		LPCWSTR pszStarting = L"Initializing ConEmu.";
			
		// 120721 - ���� �������� ��������� ������ - �� ����� ������ � ���� �������?
		if (gpSet->isStatusBarShow)
			pszStarting = NULL;
		else if (gpConEmu->isProcessCreated())
			pszStarting = L"No consoles";
		else if (this && mp_RCon)
			pszStarting = mp_RCon->GetConStatus();
		else if (CRealConsole::ms_LastRConStatus[0])
			pszStarting = CRealConsole::ms_LastRConStatus;

		if (pszStarting != NULL)
		{
			UINT nFlags = ETO_CLIPPED;
			cePaintDc.SetTextColor(pColors[7]);
			cePaintDc.SetBkColor(pColors[0]);
			cePaintDc.ExtTextOut(rcClient.left, rcClient.top, nFlags, &rcClient,
				        pszStarting, _tcslen(pszStarting), 0);
			cePaintDc.SelectObject(hOldF);
		}
	}

	if (lbDelBrush)
		DeleteObject(hBr);
			
	if (mp_Ghost)
		mp_Ghost->UpdateTabSnapshoot(TRUE); //CreateTabSnapshoot(hPaintDc, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
}

void CVirtualConsole::PaintVConNormal(HDC hPaintDc, RECT rcClient)
{
	if (gpConEmu->isActive(this))
		gpSetCls->Performance(tPerfFPS, TRUE); // ��������� �� ������

	// ���������, �� ��������� �� �������� � ���������� ����
	DWORD nBits = GetDeviceCaps(hPaintDc, BITSPIXEL);

	if (mn_LastBitsPixel != nBits)
	{
		mb_RequiredForceUpdate = true;
	}

	mb_InPaintCall = TRUE;

	if (gbNoDblBuffer)
		Update(true, &hPaintDc);
	else
		Update(mb_RequiredForceUpdate);

	m_HighlightInfo.mb_ChangeDetected = false;
	mb_InPaintCall = FALSE;
	//BOOL lbExcept = FALSE;
	RECT client = rcClient;
	//PAINTSTRUCT ps;
	//HDC hPaintDc = NULL;

	//Get ClientRect('ghWnd DC', &client);

	//120807 - ������ �������� ������ ��������
#if 0
	if (!gpConEmu->isNtvdm())
	{
		// rcClient �� ��������� ��������� ������ ���������
		RECT rcFull; GetWindowRect(mh_WndDC, &rcFull);
		// ����� ������ � �������� ����� ���� �������� � �������� ���� DC
		if ((rcFull.right - rcFull.left) < (LONG)Width || (rcFull.bottom - client.top) < (LONG)Height)
		{
			WARNING("�������������. �������� PaintVCon, ������� �������� OnSize. � ����� - 100% �������� ����.");
			gpConEmu->OnSize(false); // ������ ������ �������� ����
		}
	}
#endif

	//MSectionLock S; //&csDC, &ncsTDC);
	//RECT rcUpdateRect = {0};
	//BOOL lbInval = GetUpdateRect('ghWnd DC', &rcUpdateRect, FALSE);
	//if (lbInval)
	//  hPaintDc = BeginPaint('ghWnd DC', &ps);
	//else
	//	hPaintDc = GetDC('ghWnd DC');
	// ���� ���� ������ �������� DC - ������ ���� (������/�����) ������� ������
	HBRUSH hBr = NULL;

	if (((ULONG)(client.right-client.left)) > Width)
	{
		if (!hBr) hBr = CreateSolidBrush(mp_Colors[mn_BackColorIdx]);

		RECT rcFill = MakeRect(client.left+Width, client.top, client.right, client.bottom);
#ifndef SKIP_ALL_FILLRECT
		FillRect(hPaintDc, &rcFill, hBr);
#endif
		client.right = client.left+Width;
	}

	if (((ULONG)(client.bottom-client.top)) > Height)
	{
		if (!hBr) hBr = CreateSolidBrush(mp_Colors[mn_BackColorIdx]);

		RECT rcFill = MakeRect(client.left, client.top+Height, client.right, client.bottom);
#ifndef SKIP_ALL_FILLRECT
		FillRect(hPaintDc, &rcFill, hBr);
#endif
		client.bottom = client.top+Height;
	}

	if (hBr) { DeleteObject(hBr); hBr = NULL; }

	//BOOL lbPaintLocked = FALSE;

	//if (!mb_PaintRequested)    // ���� PaintVCon ������� �� �� Update (� �������� ������, ��� ��� ����� ������� ���-�� ���������).
	//{
	//	//if (S.Lock(&csDC, 200))  // �� �� ��������, ����� �� ������� ���������
	//	//	mb_PaintLocked = lbPaintLocked = TRUE;
	//}

	//bool bFading = false;
	bool lbLeftExists = (m_LeftPanelView.hWnd && IsWindowVisible(m_LeftPanelView.hWnd));

	if (lbLeftExists && !mb_LeftPanelRedraw)
	{
		DWORD n = GetWindowLong(m_LeftPanelView.hWnd, 16*4); //-V112

		if (n != (isFade ? 2 : 1)) mb_LeftPanelRedraw = TRUE;
	}

	bool lbRightExists = (m_RightPanelView.hWnd && IsWindowVisible(m_RightPanelView.hWnd));

	if (lbRightExists && !mb_RightPanelRedraw)
	{
		DWORD n = GetWindowLong(m_RightPanelView.hWnd, 16*4); //-V112

		if (n != (isFade ? 2 : 1)) mb_RightPanelRedraw = TRUE;
	}

	if (mb_LeftPanelRedraw)
	{
		UpdatePanelView(TRUE); mb_LeftPanelRedraw = FALSE;

		if (lbLeftExists)
		{
			InvalidateRect(m_LeftPanelView.hWnd, NULL, FALSE);
		}
	}
	else if (mb_DialogsChanged)
	{
		UpdatePanelRgn(TRUE, FALSE, FALSE);
	}

	if (mb_RightPanelRedraw)
	{
		UpdatePanelView(FALSE); mb_RightPanelRedraw = FALSE;

		if (lbRightExists)
		{
			InvalidateRect(m_RightPanelView.hWnd, NULL, FALSE);
		}
	}
	else if (mb_DialogsChanged)
	{
		UpdatePanelRgn(FALSE, FALSE, FALSE);
	}

	// ����������, ����������� �������� bitmap
	if (!gbNoDblBuffer)
	{
		// ������� �����
		if (gpSetCls->isAdvLogging>=3) mp_RCon->LogString("Blitting to Display");

		BOOL lbBltRc;
		if (!m_LockDc.bLocked)
		{
			lbBltRc = BitBlt(
				hPaintDc, client.left, client.top, client.right-client.left, client.bottom-client.top,
				(HDC)m_DC, 0, 0,
				SRCCOPY);
		}
		else
		{
			RECT rc = m_LockDc.rcScreen;
			if (client.top < rc.top)
				lbBltRc = BitBlt(
					hPaintDc, client.left, client.top, client.right-client.left, rc.top-client.top,
					(HDC)m_DC, 0, 0,
					SRCCOPY);
			if (client.left < rc.left)
				lbBltRc = BitBlt(
					hPaintDc, client.left, rc.top, rc.left-client.left, client.bottom-rc.top,
					(HDC)m_DC, 0, rc.top-client.top,
					SRCCOPY);
			if (client.right > rc.right)
				lbBltRc = BitBlt(
					hPaintDc, rc.right, rc.top, client.right-rc.right, rc.bottom-rc.top,
					(HDC)m_DC, rc.right-client.left, rc.top-client.top,
					SRCCOPY);
			if (client.bottom > rc.bottom)
				lbBltRc = BitBlt(
					hPaintDc, client.left, rc.bottom, client.right-client.left, client.bottom-rc.bottom,
					(HDC)m_DC, 0, rc.bottom-client.top,
					SRCCOPY);

			TODO("����� �� ��� ������������ ��, ��� ���������� �� ����� DC ���������� ���������");
		}
		UNREFERENCED_PARAMETER(lbBltRc);

		//#ifdef _DEBUG
		//MoveToEx(hPaintDc, client.left, client.top, NULL);
		//LineTo(hPaintDc, client.right-client.left, client.bottom-client.top);
		//FillRect(hPaintDc, &rcClient, (HBRUSH)GetStockObject(WHITE_BRUSH));
		//#endif

		CheckTransparent();
	}
	else
	{
		GdiSetBatchLimit(1); // ��������� ����������� ������ ��� ������� ����
		GdiFlush();
		// ������ ����� �� �������, ��� �����������
		// ��� ��� ������� ���������, ������� ����������� ���� � ������
		_ASSERTE(gpSet->isTabs == 0);
		Update(true, &hPaintDc);
	}

	// ��������� ��������� �������� �������
	mn_LastBitsPixel = nBits;

	if (gbNoDblBuffer) GdiSetBatchLimit(0);  // ������� ����������� �����

	if (Cursor.isVisible && cinf.bVisible && isCursorValid)
	{
		if (mpsz_ConChar && mpn_ConAttrEx)
		{
			CEDC cePaintDc(hPaintDc);
			CEFONT hOldFont = cePaintDc.SelectObject(gpSetCls->mh_Font[0]);
			MSectionLock SCON; SCON.Lock(&csCON);

			int CurChar = csbi.dwCursorPosition.Y * TextWidth + csbi.dwCursorPosition.X;
			Cursor.ch = mpsz_ConChar[CurChar];
			Cursor.foreColor = mpn_ConAttrEx[CurChar].crForeColor;
			Cursor.bgColor = mpn_ConAttrEx[CurChar].crBackColor;

			UpdateCursorDraw(hPaintDc, rcClient, csbi.dwCursorPosition, cinf.dwSize);

			Cursor.isVisiblePrev = Cursor.isVisible;
			cePaintDc.SelectObject(hOldFont);
			SCON.Unlock();
		}
	}
}

void CVirtualConsole::PaintVConDebug(HDC hPaintDc, RECT rcClient)
{
	if (mp_RCon)
	{
		#ifdef DEBUGDRAW_RCONPOS
		if (GetKeyState(DEBUGDRAW_RCONPOS) & 1)
		{
			// �������������, ������������� ��������� ���� RealConsole
			HWND hConWnd = mp_RCon->hConWnd;
			RECT rcCon; GetWindowRect(hConWnd, &rcCon);
			MapWindowPoints(NULL, GetView(), (LPPOINT)&rcCon, 2);
			SelectObject(hPaintDc, GetStockObject(WHITE_PEN));
			SelectObject(hPaintDc, GetStockObject(HOLLOW_BRUSH));
			Rectangle(hPaintDc, rcCon.left, rcCon.top, rcCon.right, rcCon.bottom);
		}
		#endif

		if (gbDebugShowRects
				#ifdef DEBUGDRAW_DIALOGS
		        || (GetKeyState(DEBUGDRAW_DIALOGS) & 1)
				#endif
		  )
		{
			// �������������� ��������� ��������
			//SMALL_RECT rcFound[MAX_DETECTED_DIALOGS]; DWORD nDlgFlags[MAX_DETECTED_DIALOGS];
			//int nFound = mp_RCon->GetDetectedDialogs(MAX_DETECTED_DIALOGS, rcFound, nDlgFlags);
			const DetectedDialogs* pDlg = NULL;
			// ���� �������� PanelView - ����������� ����� ������� � ����
			//#ifdef _DEBUG
			MFileMapping<DetectedDialogs> pvMap;

			if (m_LeftPanelView.bRegister || m_RightPanelView.bRegister)
			{
				pvMap.InitName(CEPANELDLGMAPNAME, mp_RCon->GetFarPID(TRUE));
				pDlg = pvMap.Open();
			}
			//#endif

			if (!pDlg) pDlg = mp_RCon->GetDetector()->GetDetectedDialogsPtr();

			// �������
			HPEN hFrame = CreatePen(PS_SOLID, 1, RGB(255,0,255));
			HPEN hPanel = CreatePen(PS_SOLID, 2, RGB(255,255,0));
			HPEN hQSearch = CreatePen(PS_SOLID, 1, RGB(255,255,0));
			HPEN hUCharMap = CreatePen(PS_SOLID, 1, RGB(255,255,0));
			HPEN hActiveMenu = (HPEN)GetStockObject(WHITE_PEN);
			HPEN hOldPen = (HPEN)SelectObject(hPaintDc, hFrame);
			HBRUSH hOldBr = (HBRUSH)SelectObject(hPaintDc, GetStockObject(HOLLOW_BRUSH));
			HPEN hRed = CreatePen(PS_SOLID, 1, 255);
			HPEN hDash = CreatePen(PS_DOT, 1, 0xFFFFFF);
			HFONT hSmall = CreateFont(-7,0,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH,L"Small fonts");
			HFONT hOldFont = (HFONT)SelectObject(hPaintDc, hSmall);
			SetTextColor(hPaintDc, 0xFFFFFF);
			SetBkColor(hPaintDc, 0);

			for(int i = 0; i < pDlg->Count; i++)
			{
				int n = 1;
				DWORD nFlags = pDlg->DlgFlags[i];

				if (i == (MAX_DETECTED_DIALOGS-1))
					SelectObject(hPaintDc, hRed);
				else if ((nFlags & FR_ACTIVEMENUBAR) == FR_ACTIVEMENUBAR)
					SelectObject(hPaintDc, hActiveMenu);
				else if ((nFlags & FR_QSEARCH))
					SelectObject(hPaintDc, hQSearch);
				else if ((nFlags & (FR_UCHARMAP|FR_UCHARMAPGLYPH)))
					SelectObject(hPaintDc, hUCharMap);
				else if ((nFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)))
					SelectObject(hPaintDc, hPanel);
				else if ((nFlags & FR_HASBORDER))
					SelectObject(hPaintDc, hFrame);
				else
				{
					SelectObject(hPaintDc, hDash);
					n = 0;
				}

				//
				POINT pt[2];
				pt[0] = ConsoleToClient(pDlg->Rects[i].Left, pDlg->Rects[i].Top);
				pt[1] = ConsoleToClient(pDlg->Rects[i].Right+1, pDlg->Rects[i].Bottom+1);
				//MapWindowPoints(GetView(), ghWnd, pt, 2);
				Rectangle(hPaintDc, pt[0].x+n, pt[0].y+n, pt[1].x-n, pt[1].y-n);
				wchar_t szCoord[32]; _wsprintf(szCoord, SKIPLEN(countof(szCoord)) L"%ix%i", pDlg->Rects[i].Left, pDlg->Rects[i].Top);
				TextOut(hPaintDc, pt[0].x+1, pt[0].y+1, szCoord, _tcslen(szCoord));
			}

			SelectObject(hPaintDc, hOldBr);
			SelectObject(hPaintDc, hOldPen);
			SelectObject(hPaintDc, hOldFont);
			DeleteObject(hRed);
			DeleteObject(hPanel);
			DeleteObject(hQSearch);
			DeleteObject(hUCharMap);
			DeleteObject(hDash);
			DeleteObject(hFrame);
			DeleteObject(hSmall);
		}
	}


	//if (lbExcept)
	//{
	//	CRealConsole::Box(_T("Exception triggered in CVirtualConsole::PaintVCon"), MB_RETRYCANCEL);
	//}

	//  if (hPaintDc && 'ghWnd DC') {
	//if (lbInval)
	//	EndPaint('ghWnd DC', &ps);
	//else
	//	ReleaseDC('ghWnd DC', hPaintDc);
	//  }
	//gpSetCls->Performance(tPerfFPS, FALSE); // ��������
	mb_DialogsChanged = FALSE; // �������, ������ ������ �� �����

	#ifdef _DEBUG
	if ((GetKeyState(VK_SCROLL) & 1) && (GetKeyState(VK_CAPITAL) & 1))
	{
		gpConEmu->DebugStep(L"ConEmu: Sleeping in CVirtualConsole::PaintVCon for 1s");
		Sleep(1000);
		gpSetCls->PostUpdateCounters(true); // Force update "Info" counters
		gpConEmu->DebugStep(NULL);
	}
	#endif
}

void CVirtualConsole::UpdateInfo()
{
	if (!ghOpWnd || !this)
		return;

	if (!gpConEmu->isMainThread())
	{
		return;
	}

	wchar_t szSize[128];

	if (!mp_RCon)
	{
		SetDlgItemText(gpSetCls->mh_Tabs[gpSetCls->thi_Info], tConSizeChr, L"(None)");
		SetDlgItemText(gpSetCls->mh_Tabs[gpSetCls->thi_Info], tConSizePix, L"(None)");
		SetDlgItemText(gpSetCls->mh_Tabs[gpSetCls->thi_Info], tPanelLeft, L"(None)");
		SetDlgItemText(gpSetCls->mh_Tabs[gpSetCls->thi_Info], tPanelRight, L"(None)");
	}
	else
	{
		_wsprintf(szSize, SKIPLEN(countof(szSize)) _T("%ix%i"), mp_RCon->TextWidth(), mp_RCon->TextHeight());
		SetDlgItemText(gpSetCls->mh_Tabs[gpSetCls->thi_Info], tConSizeChr, szSize);
		_wsprintf(szSize, SKIPLEN(countof(szSize)) _T("%ix%i"), Width, Height);
		SetDlgItemText(gpSetCls->mh_Tabs[gpSetCls->thi_Info], tConSizePix, szSize);
		RECT rcPanel;
		RCon()->GetPanelRect(FALSE, &rcPanel);

		if (rcPanel.right>rcPanel.left)
			_wsprintf(szSize, SKIPLEN(countof(szSize)) L"(%i, %i)-(%i, %i), %ix%i", rcPanel.left+1, rcPanel.top+1, rcPanel.right+1, rcPanel.bottom+1, rcPanel.right-rcPanel.left+1, rcPanel.bottom-rcPanel.top+1);
		else
			wcscpy_c(szSize, L"<Absent>");

		SetDlgItemText(gpSetCls->mh_Tabs[gpSetCls->thi_Info], tPanelLeft, szSize);
		RCon()->GetPanelRect(TRUE, &rcPanel);

		if (rcPanel.right>rcPanel.left)
			_wsprintf(szSize, SKIPLEN(countof(szSize)) L"(%i, %i)-(%i, %i), %ix%i", rcPanel.left+1, rcPanel.top+1, rcPanel.right+1, rcPanel.bottom+1, rcPanel.right-rcPanel.left+1, rcPanel.bottom-rcPanel.top+1);
		else
			wcscpy_c(szSize, L"<Absent>");

		SetDlgItemText(gpSetCls->mh_Tabs[gpSetCls->thi_Info], tPanelRight, szSize);
	}
}

//void CVirtualConsole::Box(LPCTSTR szText)
//{
//#ifdef _DEBUG
//	_ASSERT(FALSE);
//#endif
//	MessageBox(NULL, szText, gpConEmu->GetDefaultTitle(), MB_ICONSTOP|MB_SYSTEMMODAL);
//}

RECT CVirtualConsole::GetRect()
{
	RECT rc;

	if (Width == 0 || Height == 0)    // ���� ������� ��� �� ����������� - ����������� �� ������� GUI ����
	{
		//rc = MakeRect(winSize.X, winSize.Y);
		//RECT rcWnd; Get ClientRect(ghWnd, &rcWnd);
		RECT rcCon = gpConEmu->CalcRect(CER_CONSOLE_CUR, this);
		RECT rcDC = gpConEmu->CalcRect(CER_DC, rcCon, CER_CONSOLE_CUR, this);
		rc = MakeRect(rcDC.right, rcDC.bottom);
	}
	else
	{
		rc = MakeRect(Width, Height);
	}

	return rc;
}

RECT CVirtualConsole::GetDcClientRect()
{
	BOOL lbRc;
	RECT rcClient = {};
	if (gpSet->isAlwaysShowScrollbar == 1)
	{
		// ����� "������ ��������� ����� ������"
		lbRc = ::GetClientRect(mh_WndDC, &rcClient);
	}
	else
	{
		// ������ ��������� ����� ����������� ����� ������� �������, �������
		lbRc = ::GetWindowRect(mh_WndDC, &rcClient);
		// � �������� � ���������� �����������
		rcClient.right -= rcClient.left;
		rcClient.bottom -= rcClient.top;
		rcClient.left = rcClient.top = 0;
	}
	UNREFERENCED_PARAMETER(lbRc);
	return rcClient;
}

void CVirtualConsole::OnFontChanged()
{
	if (!this) return;

	mb_RequiredForceUpdate = true;
	//ClearPartBrushes();
}

void CVirtualConsole::OnConsoleSizeChanged()
{
	// �� ���� ��� ������� ���������� ��� ������� ���� GUI ConEmu
	BOOL lbLast = mb_InPaintCall;
	mb_InPaintCall = TRUE; // ����� Invalidate ������ ��� �� ��������
	Update(mb_RequiredForceUpdate);
	mb_InPaintCall = lbLast;
}

void CVirtualConsole::OnConsoleSizeReset(USHORT sizeX, USHORT sizeY)
{
	// ��� ������ ���� ������ �� ����� �������� ����� ������� (��������, ��������� ������ �����)
	_ASSERTE(mp_RCon && ((mp_RCon->ConWnd()==NULL) || mp_RCon->mb_InCloseConsole));
	// � �� ����, DC ��� ������ ���� �� ������ ���
	if (Width==0 && Height==0)
	{
		TextWidth = sizeX;
		TextHeight = sizeY;
	}
	else
	{
		_ASSERTE((Width==0 && Height==0) || mp_RCon->mb_InCloseConsole);
	}
}

POINT CVirtualConsole::ConsoleToClient(LONG x, LONG y)
{
	POINT pt = {0,0};

	if (!this)
	{
		pt.y = y*gpSetCls->FontHeight();
		pt.x = x*gpSetCls->FontWidth();
		return pt;
	}

	pt.y = y*nFontHeight;

	if (x>0)
	{
		if (ConCharX && y >= 0 && y < (int)TextHeight && x < (int)TextWidth)
		{
			pt.x = ConCharX[y*TextWidth + x-1];

			if (x && !pt.x)
			{
				TODO("��� ������ �� �� �������� ���� - ConCharX ����� ���� �������");
				//2010-06-07
				// 	CRealConsole::RConServerThread -->
				//  CRealConsole::ServerThreadCommand -->
				//  CVirtualConsole::RegisterPanelView -->
				//  CVirtualConsole::UpdatePanelView(int abLeftPanel=1, int abOnRegister=1) -->
				//  CVirtualConsole::UpdatePanelRgn(int abLeftPanel=1, int abTestOnly=0, int abOnRegister=1) -->
				//  � ��� ��� ��� ����������. ConCharX �������� �������?
				Sleep(1);
				pt.x = ConCharX[y*TextWidth + x-1];
				_ASSERTE(pt.x || x==0);
			}
		}
		else
		{
			pt.x = x*nFontWidth;
		}
	}

	return pt;
}

// Only here we can precisely calc exact coords
COORD CVirtualConsole::ClientToConsole(LONG x, LONG y, bool StrictMonospace/*=false*/)
{
	COORD cr = {0,0};

	if (!this)
	{
		cr.Y = y/gpSetCls->FontHeight();
		cr.X = x/gpSetCls->FontWidth();
		return cr;
	}

	_ASSERTE(nFontWidth!=0 && nFontHeight!=0);

	// ������� ��������������� �������� �� �������� ������
	if (nFontHeight)
		cr.Y = y/nFontHeight;

	if (nFontWidth)
		cr.X = x/nFontWidth;

	if (!StrictMonospace)
	{
		// � ������, ���� ��������, ������� X ����������
		if (x > 0)
		{
			x++; // ����� ��������� �� ���� ������ �����

			if (ConCharX && cr.Y >= 0 && cr.Y < (int)TextHeight)
			{
				DWORD* ConCharXLine = ConCharX + cr.Y * TextWidth;

				for(uint i = 0; i < TextWidth; i++, ConCharXLine++)
				{
					if (((int)*ConCharXLine) >= x)
					{
						if (cr.X != (int)i)
						{
							#ifdef _DEBUG
							wchar_t szDbg[120]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Coord corrected from {%i-%i} to {%i-%i}",
														  cr.X, cr.Y, i, cr.Y);
							DEBUGSTRCOORD(szDbg);
							#endif

							cr.X = i;
						}

						break;
					}
				}
			}
		}
	}

	return cr;
}

// �� ����� ���� �� ������ �������, �� � ucBoxSinglHorz/ucBoxDblVertHorz
void CVirtualConsole::DistributeSpaces(wchar_t* ConCharLine, CharAttr* ConAttrLine, DWORD* ConCharXLine, const int j, const int j2, const int end)
{
	//WORD attr = ConAttrLine[j];
	//wchar_t ch, c;
	//
	//if ((c=ConCharLine[j]) == ucSpace || c == ucNoBreakSpace || c == 0)
	//{
	//	while (j2 < end && ConAttrLine[j2] == attr
	//		// ����� � ��� &nbsp; � 0x00
	//		&& ((ch=ConCharLine[j2]) == ucSpace || ch == ucNoBreakSpace || ch == 0))
	//		j2++;
	//} else
	//if ((c=ConCharLine[j]) == ucBoxSinglHorz || c == ucBoxDblVertHorz)
	//{
	//	while (j2 < end && ConAttrLine[j2] == attr
	//		&& ((ch=ConCharLine[j2]) == ucBoxSinglHorz || ch == ucBoxDblVertHorz))
	//		j2++;
	//} else
	//if (isCharProgress(c=ConCharLine[j]))
	//{
	//	while (j2 < end && ConAttrLine[j2] == attr && (ch=ConCharLine[j2]) == c)
	//		j2++;
	//}
	_ASSERTE(j2 > j && j >= 0);

	// ������ ������� "�������" (��� ������� �����) ����� ������� ��������������� ����������� ��� �����

	if (j2>=(int)TextWidth)    // ����� ������
	{
		ConCharXLine[j2-1] = Width;
	}
	else
	{
		//2009-09-09 ��� �����������. ������ ������ ����� ����� ���� ������ ����������
		//int nBordWidth = gpSet->BorderFontWidth(); if (!nBordWidth) nBordWidth = nFontWidth;
		int nBordWidth = nFontWidth;

		// ���������� ���������� ����� ������������������
		if (isCharBorderVertical(ConCharLine[j2]))
		{
			//2009-09-09 � ��� �������������� �� ����� (���� nFontWidth == nBordWidth)
			//ConCharXLine[j2-1] = (j2-1) * nFontWidth + nBordWidth; // ��� ��� [j] ������ ����?
			ConCharXLine[j2-1] = j2 * nBordWidth;
		}
		else
		{
			TODO("��� ���������������� ������� ���� ������ ���-�� �� �������!");

			//2009-09-09 � ��� �������������� �� ����� (���� nFontWidth == nBordWidth)
			//ConCharXLine[j2-1] = (j2-1) * nFontWidth + nBordWidth; // ��� ��� [j] ������ ����?
			if (!gpSet->isMonospace && j > 1)
			{
				//2009-12-31 ����� ������� �� ����������� �������!
				ConCharXLine[j2-1] = ConCharXLine[j-1] + (j2 - j) * nBordWidth;
			}
			else
			{
				ConCharXLine[j2-1] = j2 * nBordWidth;
			}
		}
	}

	if (j2 > (j + 1))
	{
		HEAPVAL
		DWORD n1 = (j ? ConCharXLine[j-1] : 0); // j? ���� j==0 �� ��� � ��� 10 (������ ������� 0�� ������� � ������)
		DWORD n3 = j2-j; // ���-�� ��������
		_ASSERTE(n3>0);
		DWORD n2 = ConCharXLine[j2-1] - n1; // ���������� �� ������� ������
		HEAPVAL

		for(int k=j, l=1; k<(j2-1); k++, l++)
		{
#ifdef _DEBUG
			DWORD nn = n1 + (n3 ? klMulDivU32(l, n2, n3) : 0);

			if (nn != ConCharXLine[k])
				ConCharXLine[k] = nn;

#endif
			ConCharXLine[k] = n1 + (n3 ? klMulDivU32(l, n2, n3) : 0);
			//n1 + (n3 ? klMulDivU32(k-j, n2, n3) : 0);
			HEAPVAL
		}
	}
}

bool CVirtualConsole::FindChanges(int row, int &j, int &end, const wchar_t* ConCharLine, const CharAttr* ConAttrLine, const wchar_t* ConCharLine2, const CharAttr* ConAttrLine2)
{
	// UCharMap - ������������ ���������, ��� ����� ����� ����������
	if (gpSet->isExtendUCharMap && mrc_UCharMap.Right > mrc_UCharMap.Left)
	{
		if (row >= mrc_UCharMap.Top && row <= mrc_UCharMap.Bottom)
			return TRUE; // ���� ������ ������� ����� ��������
	}

	// ���� ��������� � ������ ������ ���
	if (wmemcmp(ConCharLine, ConCharLine2, end) == 0
	        && memcmp(ConAttrLine, ConAttrLine2, end*sizeof(*ConAttrLine)) == 0)
		return FALSE;

	// Default: j = 0, end = TextWidth;
	return TRUE; // ���� ������ ������� ����� ��������
	// ���� �������� ����� � ������� ���������, ��
	// �� ��������� ��������� ������ ����� ����� ����� �������� ������ �������
	TODO("�������� ��� ���������� ClearType �� ����� ����� �������� ������ �������, ���� ��������� �� Transparent ��������� ������");

	if (gpSetCls->FontItalic() || gpSetCls->FontClearType())
		return TRUE;

	// *) Skip not changed tail symbols. �� ������ ���� ����� ����������
	if (gpSet->isMonospace)
	{
		TODO("OPTIMIZE:");

		while (--end >= 0 && ConCharLine[end] == ConCharLine2[end] && ConAttrLine[end] == ConAttrLine2[end])
		{
			// ���� ���� ����� ������ (������/������), ��
			// �� ��������� ��������� ������ ����� ����� ����� �������� ������ �������
			if (ConAttrLine[end].nFontIndex)
			{
				end = TextWidth;
				return TRUE;
			}
		}

		// [%] ClearType, TTF fonts (isProportional ����� ���� � ��������)
		if (end >= 0   // ���� ���� ���� �����-�� ���������
		        && end < (int)(TextWidth - 1) // �� ����� ������
		        && (ConCharLine[end+1] == ucSpace || ConCharLine[end+1] == ucNoBreakSpace || isCharProgress(ConCharLine[end+1])) // ����� ������������ ��� �������
		  )
		{
			int n = TextWidth - 1;

			while ((end < n)
			        && (ConCharLine[end+1] == ucSpace || ConCharLine[end+1] == ucNoBreakSpace || isCharProgress(ConCharLine[end+1])))
				end++;
		}

		if (end < j)
			return FALSE;

		++end;
	}

	// *) Skip not changed head symbols.
	while (j < end && ConCharLine[j] == ConCharLine2[j] && ConAttrLine[j] == ConAttrLine2[j])
	{
		// ���� ���� ����� ������ (������/������) ��
		// �� ��������� ��������� ������ ����� ����� ����� �������� ������ �������
		if (ConAttrLine[j].nFontIndex)
		{
			j = 0;
			end = TextWidth;
			return TRUE;
		}

		++j;
	}

	// [%] ClearType, proportional fonts
	if (j > 0  // ���� � ������ ������
	        && (ConCharLine[j-1] == ucSpace || ConCharLine[j-1] == ucNoBreakSpace || isCharProgress(ConCharLine[j-1]) // ���� �������
	            || (gpSetCls->FontItalic() || ConAttrLine[j-1].nFontIndex))  // ��� ������ ������/������?
	  )
	{
		if (gpSetCls->FontItalic() || ConAttrLine[j-1].nFontIndex)
		{
			j = 0; // � ��������� ���������� �������������� �� �����, � �� ��� ClearType �������...
		}
		else
		{
			while ((j > 0)
			        && (ConCharLine[j-1] == ucSpace || ConCharLine[j-1] == ucNoBreakSpace || isCharProgress(ConCharLine[j-1])))
			{
				j--;
			}
		}
	}

	if (j >= end)
	{
		// ��� ���������������� ������� ��������� ����������� ������ � ������ ������,
		// ������� ���� �� ������ ����� ������� - ������ ������ �� ��������
		return FALSE;
	}

	return TRUE;
}

HRGN CVirtualConsole::GetExclusionRgn(bool abTestOnly/*=false*/)
{
	if (!gpSet->isUserScreenTransparent)
		return NULL;

	if (mp_RCon->GetFarPID(TRUE) == 0)
		return NULL;

	// ���������� mh_TransparentRgn
	// ��� mh_TransparentRgn ����������� � CheckTransparentRgn,
	// ������� ������ ���������� � CVirtualConsole::PaintVCon
	if (abTestOnly && mh_TransparentRgn)
		return (HRGN)1;

	return mh_TransparentRgn;
}

COORD CVirtualConsole::FindOpaqueCell()
{
	COORD cr = {0,0};

	if (this)
	{
		// �� ��������� - �������
		cr.X = cr.Y = -1;

		if (mpsz_ConChar && mpn_ConAttrEx && TextWidth && TextHeight)
		{
			MSectionLock SCON; SCON.Lock(&csCON);
			CharAttr* pnAttr = mpn_ConAttrEx;

			// ����� ������� �������������
			for (uint y = 0; y < TextHeight; y++)
			{
				for (uint x = 0; x < TextWidth; x++, pnAttr++)
				{
					#if 0
					if (!pnAttr[x].bTransparent)
					#else
					if (!(pnAttr->Flags & CharAttr_Transparent))
					#endif
					{
						cr.X = x; cr.Y = y;
						return cr;
					}
				}

				//pnAttr += TextWidth;
			}
		}
	}

	return cr;
}

void CVirtualConsole::OnPanelViewSettingsChanged()
{
	if (!this) return;

	if (!mp_RCon) return;

	DWORD nPID = 0;

	if ((nPID = mp_RCon->GetFarPID(TRUE)) != 0)
	{
		CConEmuPipe pipe(nPID, 1000);

		if (pipe.Init(_T("CVirtualConsole::OnPanelViewSettingsChanged"), TRUE))
		{
			CESERVER_REQ_GUICHANGED lWindows = {sizeof(CESERVER_REQ_GUICHANGED)};
			lWindows.nGuiPID = GetCurrentProcessId();
			lWindows.hLeftView = m_LeftPanelView.hWnd;
			lWindows.hRightView = m_RightPanelView.hWnd;
			pipe.Execute(CMD_GUICHANGED, &lWindows, sizeof(lWindows));
			gpConEmu->DebugStep(NULL);
		}
	}

	//if (!mn_ConEmuSettingsMsg)
	//	mn_ConEmuSettingsMsg = RegisterWindowMessage(CONEMUMSG_PNLVIEWSETTINGS);
	//DWORD nPID = GetCurrentProcessId();
	//if (m_LeftPanelView.hWnd && IsWindow(m_LeftPanelView.hWnd))
	//{
	//	WARNING("������ �� ����� ������������, ���� ������� �������� ��� �������");
	//	PostMessage(m_LeftPanelView.hWnd, mn_ConEmuSettingsMsg, nPID, 0);
	//}
	//if (m_RightPanelView.hWnd && IsWindow(m_RightPanelView.hWnd))
	//{
	//	PostMessage(m_RightPanelView.hWnd, mn_ConEmuSettingsMsg, nPID, 0);
	//}
}

bool CVirtualConsole::IsPanelViews()
{
	if (!this) return false;

	if (m_LeftPanelView.hWnd && IsWindow(m_LeftPanelView.hWnd))
		return true;

	if (m_RightPanelView.hWnd && IsWindow(m_RightPanelView.hWnd))
		return true;

	return false;
}

bool CVirtualConsole::RegisterPanelView(PanelViewInit* ppvi)
{
	_ASSERTE(ppvi && ppvi->cbSize == sizeof(PanelViewInit));
	BOOL lbRc = FALSE;
	PanelViewInit* pp = (ppvi->bLeftPanel) ? &m_LeftPanelView : &m_RightPanelView;
	BOOL lbPrevRegistered = pp->bRegister;
	*pp = *ppvi;
	// ������� ������� ������� GUI
	gpConEmu->OnPanelViewSettingsChanged(FALSE/*abSendChanges*/);
	//COLORREF *pcrNormal = gpSet->GetColors(FALSE);
	//COLORREF *pcrFade = gpSet->GetColors(TRUE);
	//for (int i=0; i<16; i++) {
	//	// ����� FOR ����� � BitMask �� ����������
	//	ppvi->crPalette[i] = (pcrNormal[i]) & 0xFFFFFF;
	//	ppvi->crFadePalette[i] = (pcrFade[i]) & 0xFFFFFF;
	//}
	ppvi->bFadeColors = isFade;
	//memmove(&(ppvi->ThSet), &(gpSet->ThSet), sizeof(gpSet->ThSet));
	DWORD dwSetParentErr = 0;

	if (ppvi->bRegister)
	{
		//_ASSERTE(ppvi->bVisible); -- ���������. ��������� �������, ��������, ��� Ctrl+L

		// ��� ��������� ����������� - �� ���������
		if (!lbPrevRegistered)
		{
			//for (int i=0; i<20; i++) // �� ������ ������ - ������� ��� �������
			//	SetWindowLong(pp->hWnd, i*4, 0);
			HWND hViewParent = GetParent(ppvi->hWnd);

			if (hViewParent != GetView())
			{
				MBoxAssert(GetView()==ghWnd);
				// -- ������ �������� ������ �������� - �������, � Win7 ��� � �� ���������
				// -- ���� ���������� ������� ������� "Run as administrator"
				// --- �� ���-���� ���������?
				//DWORD dwSetParentErr = 0;
				HWND hRc = mp_RCon->SetOtherWindowParent((HWND)ppvi->hWnd, GetView());
				UNREFERENCED_PARAMETER(hRc);
				dwSetParentErr = GetLastError();
			}
			else
			{
				lbRc = TRUE;
			}
		}
		else
		{
			lbRc = TRUE;

			if (ppvi->bLeftPanel)
				mb_LeftPanelRedraw = TRUE;
			else
				mb_RightPanelRedraw = TRUE;
		}

		if (lbRc)
		{
			// ��������� � ������� (�� �������� ��������� ����)
			lbRc = UpdatePanelView(ppvi->bLeftPanel, TRUE);
			// �������� ���� ���������
			ppvi->bVisible = pp->bVisible;
			ppvi->WorkRect = pp->WorkRect;
			// pp->WorkRect ��� �������� ������������� "�� +1".
			ppvi->WorkRect.bottom--; ppvi->WorkRect.right--;
			// �� ������� ����� "��������" ������ ����� �����
			Update(true);

			// ���� ������� ���� �������� - �������� � ��� ������ "Apply"
			if (ghOpWnd) gpSetCls->OnPanelViewAppeared(TRUE);
		}
	}
	else
	{
		lbRc = TRUE;
	}

	return lbRc;
}

//HRGN CVirtualConsole::CreateConsoleRgn(int x1, int y1, int x2, int y2, BOOL abTestOnly)
//{
//	POINT pt[2];
//	if (abTestOnly) {
//		// ���������� �������������� ����������� ���������������, ��� ��� ���������� �� �����
//		pt[0].x = x1 << 3; pt[0].y = y1 << 3;
//		pt[1].x = x2 << 3; pt[1].y = y2 << 3;
//	} else {
//		pt[0] = ConsoleToClient(x1, y1);
//		pt[1] = ConsoleToClient(x2+1, y2+1);
//	}
//	HRGN hRgn = CreateRectRgn(pt[0].x, pt[0].y, pt[1].x, pt[1].y);
//	return hRgn;
//}

bool CVirtualConsole::CheckDialogsChanged()
{
	bool lbChanged = false;
	//SMALL_RECT rcDlg[32];
	_ASSERTE(sizeof(mrc_LastDialogs) == sizeof(mrc_Dialogs));
	//int nDlgCount = mp_RCon->GetDetectedDialogs(countof(rcDlg), rcDlg, NULL);

	if (mn_LastDialogsCount != mn_DialogsCount)
	{
		lbChanged = true;
	}
	else if (memcmp(mrc_Dialogs, mrc_LastDialogs, mn_DialogsCount*sizeof(mrc_Dialogs[0]))!=0
		|| memcmp(mn_DialogFlags, mn_LastDialogFlags, mn_DialogsCount*sizeof(mn_DialogFlags[0]))!=0)
	{
		lbChanged = true;
	}

	if (lbChanged)
	{
		//memset(mrc_LastDialogs, 0, sizeof(mrc_LastDialogs));
		memmove(mrc_LastDialogs, mrc_Dialogs, mn_DialogsCount*sizeof(mrc_Dialogs[0]));
		memmove(mn_LastDialogFlags, mn_DialogFlags, mn_DialogsCount*sizeof(mn_DialogFlags[0]));
		mn_LastDialogsCount = mn_DialogsCount;
	}

	return lbChanged;
}

const PanelViewInit* CVirtualConsole::GetPanelView(bool abLeftPanel)
{
	if (!this) return NULL;

	PanelViewInit* pp = abLeftPanel ? &m_LeftPanelView : &m_RightPanelView;

	if (!pp->hWnd || !IsWindow(pp->hWnd))
	{
		if (pp->hWnd)
			pp->hWnd = NULL;

		pp->bVisible = FALSE;
		return NULL;
	}

	return pp;
}

// ���������� TRUE, ���� ������ (��� ���� �� ����� �� ������)
bool CVirtualConsole::UpdatePanelRgn(bool abLeftPanel, bool abTestOnly, bool abOnRegister)
{
	PanelViewInit* pp = abLeftPanel ? &m_LeftPanelView : &m_RightPanelView;

	if (!pp->hWnd || !IsWindow(pp->hWnd))
	{
		if (pp->hWnd)
			pp->hWnd = NULL;

		pp->bVisible = FALSE;
		return FALSE;
	}
	else if (/*!pp->bVisible ||*/ !pp->bRegister)
	{
		if (IsWindowVisible(pp->hWnd))
			mp_RCon->ShowOtherWindow(pp->hWnd, SW_HIDE);
		return FALSE;
	}

	CRgnRects* pRgn = abLeftPanel ? &m_RgnLeftPanel : &m_RgnRightPanel;
	BOOL lbPartHidden = FALSE;
	BOOL lbPanelVisible = FALSE;
	SMALL_RECT rcDlg[MAX_DETECTED_DIALOGS]; DWORD rnDlgFlags[MAX_DETECTED_DIALOGS];
	_ASSERTE(sizeof(mrc_LastDialogs) == sizeof(rcDlg));
	const CRgnDetect* pDetect = mp_RCon->GetDetector();
	int nDlgCount = pDetect->GetDetectedDialogs(countof(rcDlg), rcDlg, rnDlgFlags);
	DWORD nDlgFlags = pDetect->GetFlags();

	if (!nDlgCount)
	{
		lbPartHidden = TRUE;
		pp->bVisible = FALSE;
		//if (!abTestOnly && !abOnRegister) {
		//	if (IsWindowVisible(pp->hWnd))
		//		mp_RCon->ShowOtherWindow(pp->hWnd, SW_HIDE);
		//}
	}
	else
	{
		//HRGN hRgn = NULL, hSubRgn = NULL, hCombine = NULL;
		m_RgnTest.Reset();
		SMALL_RECT rcPanel; DWORD nGetRc;

		if ((nDlgFlags & FR_FULLPANEL) == FR_FULLPANEL)
			nGetRc = pDetect->GetDialog(FR_FULLPANEL, &rcPanel);
		else if (pp->PanelRect.left == 0)
			nGetRc = pDetect->GetDialog(FR_LEFTPANEL, &rcPanel);
		else
			nGetRc = pDetect->GetDialog(FR_RIGHTPANEL, &rcPanel);

		if (!nGetRc)
		{
			lbPanelVisible = FALSE;
		}
		else
		{
			lbPanelVisible = TRUE;
			TODO("��� ������ �� �������� WorkRect? ��� �� ���������� ������ ����� �� ���� ������� RegisterPanel �� �������?");
			m_RgnTest.Init(&(pp->WorkRect));

			for(int i = 0; i < nDlgCount; i++)
			{
				// ���������� ������������� �������������� ����� ������
				if (rcDlg[i].Left == rcPanel.Left &&
				        rcDlg[i].Bottom == rcPanel.Bottom &&
				        rcDlg[i].Right == rcPanel.Right)
				{
					continue;
				}

				if (!IntersectSmallRect(pp->WorkRect, rcDlg[i]))
					continue;

				// ��� ��������� �������������� �������� �� hRgn
				int nCRC = m_RgnTest.DiffSmall(rcDlg+i);

				// ��������, ����� ������ ��� ����?
				if (nCRC == NULLREGION)
				{
					lbPanelVisible = FALSE; break;
				}
			}
		}

		if (abTestOnly)
		{
			// ������ ��������
		}
		else
		{
			bool lbChanged = pRgn->LoadFrom(&m_RgnTest);

			if (lbPanelVisible)
			{
				lbPartHidden = (pRgn->nRgnState == COMPLEXREGION);
				pp->bVisible = TRUE;
				_ASSERTE(pRgn->nRectCount > 0);

				if (pRgn->nRgnState == SIMPLEREGION)
				{
					//NULL ������ ����� ������ ������ (���� ����� ���� ReUsed)
					//if (!abOnRegister) {
					mp_RCon->SetOtherWindowRgn(pp->hWnd, 0, NULL, TRUE);
					//}
				}
				else if (lbChanged)
				{
					// � ��� ��� ��� ����� ������ ��������� � ������, ������� ����� �������, � �� ��������...
					MSectionLock SCON; SCON.Lock(&csCON);
					POINT ptShift = ConsoleToClient(pp->WorkRect.left, pp->WorkRect.top);
					POINT pt[2];
					RECT  rc[MAX_DETECTED_DIALOGS];
					_ASSERTE(pRgn->nRectCount<MAX_DETECTED_DIALOGS);

					for(int i = 0; i < pRgn->nRectCount; i++)
					{
						pt[0] = ConsoleToClient(pRgn->rcRect[i].left, pRgn->rcRect[i].top);
						pt[1] = ConsoleToClient(pRgn->rcRect[i].right+1, pRgn->rcRect[i].bottom+1);
						rc[i].left  = pt[0].x - ptShift.x;
						rc[i].top   = pt[0].y - ptShift.y;
						rc[i].right = pt[1].x - ptShift.x;
						rc[i].bottom= pt[1].y - ptShift.y;
					}

					mp_RCon->SetOtherWindowRgn(pp->hWnd, pRgn->nRectCount, rc, TRUE);
				}

				//if (!abOnRegister) {
				//	if (!IsWindowVisible(pp->hWnd))
				//		mp_RCon->ShowOtherWindow(pp->hWnd, SW_SHOWNA);
				//}
			}
			else
			{
				lbPartHidden = TRUE;
				pp->bVisible = FALSE;
				//if (IsWindowVisible(pp->hWnd)) {
				//	// ������� ������ � ������(!) ����
				//	mp_RCon->SetOtherWindowRgn(pp->hWnd, -1, NULL, FALSE);
				//}
			}
		}
	}

	if (!abTestOnly && !abOnRegister)
	{
		if (pp->bVisible && !IsWindowVisible(pp->hWnd))
		{
			mp_RCon->ShowOtherWindow(pp->hWnd, SW_SHOWNA);
		}
		else if (!pp->bVisible && IsWindowVisible(pp->hWnd))
		{
			// ������� ������ � ������(!) ����
			mp_RCon->SetOtherWindowRgn(pp->hWnd, -1, NULL, FALSE);
			//mp_RCon->ShowOtherWindow(pp->hWnd, SW_HIDE);
		}
	}

	return lbPanelVisible;
}

// ������ - Redraw �� �����, ������ Invalidate!
bool CVirtualConsole::UpdatePanelView(bool abLeftPanel, bool abOnRegister/*=false*/)
{
	PanelViewInit* pp = abLeftPanel ? &m_LeftPanelView : &m_RightPanelView;
	if (!IsWindow(pp->hWnd))
	{
		// ��� ��� �������� ���� ����� ��� ���������
		return false;
	}

	// ����� ������ ����, ��� ���������� ������� (��� ��� Fade, ��� �������� ������������� ������).
	//for (int i=0; i<16; i++)
	//	SetWindowLong(pp->hWnd, i*4, mp_Colors[i]);
	if (!mn_ConEmuFadeMsg)
		mn_ConEmuFadeMsg = RegisterWindowMessage(CONEMUMSG_PNLVIEWFADE);

	LONG nNewFadeValue = isFade ? 2 : 1;
	WARNING("������ �� ����� �������� �� Win7 RunAsAdmin");

	if (GetWindowLong(pp->hWnd, 16*4) != nNewFadeValue)
		PostMessage(pp->hWnd, mn_ConEmuFadeMsg, 100, nNewFadeValue);

	// ����������� �������
	POINT pt[2];
	int nTopShift = 1 + ((pp->FarPanelSettings.ShowColumnTitles) ? 1 : 0); //-V112
	int nBottomShift = 0;
	if (mp_RCon && (pp->FarPanelSettings.ShowStatusLine))
	{
		nBottomShift = mp_RCon->GetStatusLineCount(pp->PanelRect.left) + 1;
		if (nBottomShift < 2)
			nBottomShift = 2;
	}
	pp->WorkRect = MakeRect(
	                   pp->PanelRect.left+1, pp->PanelRect.top+nTopShift,
	                   pp->PanelRect.right, pp->PanelRect.bottom-nBottomShift);
	//if (mb_DialogsChanged)
	// ������� ����, �.�. ��� ����������� ������ ���������� GUI
	//UpdatePanelRgn(abLeftPanel, FALSE, abOnRegister);
	// ����� �� �������� �� ConCharX - �� ����� ��������� �� ������������������!
	pt[0] = MakePoint(pp->WorkRect.left*gpSetCls->FontWidth(), pp->WorkRect.top*gpSetCls->FontHeight());
	pt[1] = MakePoint(pp->WorkRect.right*gpSetCls->FontWidth(), pp->WorkRect.bottom*gpSetCls->FontHeight());
	//pt[0] = ConsoleToClient(pp->WorkRect.left, pp->WorkRect.top);
	//pt[1] = ConsoleToClient(pp->WorkRect.right, pp->WorkRect.bottom);
	//TODO("����������� ��������� ��� DoubleView");
	//MapWindowPoints(GetView(), ghWnd, pt, 2);
	//MoveWindow(ahView, pt[0].x,pt[0].y, pt[1].x-pt[0].x,pt[1].y-pt[0].y, FALSE);
	// �� ���������, ���� ������ ������ �� �����
	DWORD dwErr = 0; BOOL lbRc = TRUE;
	RECT rcCur; GetWindowRect(pp->hWnd, &rcCur);
	//MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcCur, 2);

	if (rcCur.left != pt[0].x || rcCur.top != pt[0].y
	        || rcCur.right != pt[1].x || rcCur.bottom != pt[1].y)
	{
		lbRc = mp_RCon->SetOtherWindowPos(pp->hWnd, HWND_TOP,
		                                  pt[0].x,pt[0].y, pt[1].x-pt[0].x,pt[1].y-pt[0].y,
		                                  SWP_ASYNCWINDOWPOS|SWP_DEFERERASE|SWP_NOREDRAW);

		if (!lbRc)
		{
			dwErr = GetLastError();
			DisplayLastError(L"Can't update position of PanelView window!", dwErr);
		}
	}

	// � ���������� -- �� �����. ���� ��� �� ��������? ����� ����� �������� - ����� � ����������
	//InvalidateRect(pp->hWnd, NULL, FALSE); -- �� �����, ��� ���������� ������� WM_PAINT
	// ������� � ��������� (��������� ��� ����������� �� ��������)
	UpdatePanelRgn(abLeftPanel, FALSE, abOnRegister);
	return TRUE;
}

void CVirtualConsole::PolishPanelViews()
{
	if (!this) return;

	//mpsz_ConChar, mpn_ConAttrEx, TextWidth, TextHeight

	if (!m_LeftPanelView.bRegister && !m_RightPanelView.bRegister)
		return;

	for(int i=0; i<=1; i++)
	{
		PanelViewInit *pp = i ? &m_RightPanelView : &m_LeftPanelView;

		if (!pp->bRegister || !pp->hWnd)
			continue; // ������ �� ����������������

		if (mb_DialogsChanged)
		{
			// ���������, ������� ������ ���� �� ��������
			if (!UpdatePanelRgn(i==0, TRUE))
				continue; // ������ ����� ���������
		}

		const CEFAR_INFO_MAPPING* pFarInfo = mp_RCon->GetFarInfo();

		if (!pFarInfo || !IsWindowVisible(pp->hWnd))
			continue; // ������ ��������� �������

		/* ���, ������ ������, ����� "���������" ��������� � ����������� ����� �������� */
		RECT rc = pp->PanelRect;

		if (rc.right >= (LONG)TextWidth || rc.bottom >= (LONG)TextHeight)
		{
			if (rc.left >= (LONG)TextWidth || rc.top >= (LONG)TextHeight)
			{
				_ASSERTE(rc.right<(LONG)TextWidth && rc.bottom<(LONG)TextHeight);
				continue;
			}

			if (pp->PanelRect.right >= (LONG)TextWidth) pp->PanelRect.right = (LONG)TextWidth-1;

			if (pp->PanelRect.bottom >= (LONG)TextHeight) pp->PanelRect.bottom = (LONG)TextHeight-1;

			MBoxAssert(rc.right<(LONG)TextWidth && rc.bottom<(LONG)TextHeight);
			rc = pp->PanelRect;
		}

		// ����� ����
		BYTE btNamesColor = pFarInfo->nFarColors[col_PanelColumnTitle];
		BYTE btPanelColor = pFarInfo->nFarColors[col_PanelBox];
		// 1. ��������� ������
		int x;
		wchar_t *pszLine = mpsz_ConChar;
		CharAttr *pAttrs = mpn_ConAttrEx;
		int nFore = btPanelColor & 0xF;
		int nBack = (btPanelColor & 0xF0) >> 4;

		for(x = rc.left+1; x < rc.right && pszLine[x] != L' '; x++)
		{
			if ((pszLine[x] == ucBoxSinglDownDblHorz || pszLine[x] == ucBoxDblDownDblHorz) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				pszLine[x] = ucBoxDblHorz;
		}

		for(x = rc.right-1; x > rc.left && pszLine[x] != L' '; x--)
		{
			if ((pszLine[x] == ucBoxSinglDownDblHorz || pszLine[x] == ucBoxDblDownDblHorz) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				pszLine[x] = ucBoxDblHorz;
		}

		// 2. ������ � ������� �������
		pszLine = mpsz_ConChar+TextWidth;
		pAttrs = mpn_ConAttrEx+TextWidth;
		int nNFore = btNamesColor & 0xF;
		int nNBack = (btNamesColor & 0xF0) >> 4;

		if ((pp->FarPanelSettings.ShowColumnTitles) && pp->tColumnTitle.nFlags) //-V112
		{
			LPCWSTR pszNameTitle = pp->tColumnTitle.sText;
			CharAttr ca; CharAttrFromConAttr(pp->tColumnTitle.bConAttr, &ca);
			int nNameLen = _tcslen(pszNameTitle);
			int nX1 = rc.left + 1;

			if ((pp->tColumnTitle.nFlags & PVI_TEXT_SKIPSORTMODE)
			        && (pp->FarPanelSettings.ShowSortModeLetter & 0x800/*FPS_SHOWSORTMODELETTER*/))
				nX1 += 2;

			int nLineLen = rc.right - nX1;

			//wmemset(pszLine+nX1, L' ', nLineLen);
			if (nNameLen > nLineLen) nNameLen = nLineLen;

			int nX3 = nX1 + ((nLineLen - nNameLen) >> 1);
			_ASSERTE(nX3<=rc.right);

			//wmemcpy(pszLine+nX3, mp_RCon->ms_NameTitle, nNameLen);
			//TODO("�������� ����� ����� � �������� (����) ��������");
			for(x = nX1; x < nX3; x++)
				if ((pszLine[x] != L' ' && pAttrs[x].nForeIdx == nNFore && pAttrs[x].nBackIdx == nNBack)
				        || ((pszLine[x] == ucBoxSinglVert || pszLine[x] == ucBoxDblVert) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				  ) pszLine[x] = L' ';

			int nX4 = min(rc.right,(nX3+nNameLen));

			for(x = nX3; x < nX4; x++, pszNameTitle++)
				if ((pAttrs[x].nForeIdx == nNFore && pAttrs[x].nBackIdx == nNBack)
				        || ((pszLine[x] == ucBoxSinglVert || pszLine[x] == ucBoxDblVert) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				  )
				{
					pszLine[x] = *pszNameTitle;
					pAttrs[x] = ca;
				}

			for(x = nX4; x < rc.right; x++)
				if ((pszLine[x] != L' ' && pAttrs[x].nForeIdx == nNFore && pAttrs[x].nBackIdx == nNBack)
				        || ((pszLine[x] == ucBoxSinglVert || pszLine[x] == ucBoxDblVert) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				  ) pszLine[x] = L' ';
		}

		// 3. �����������
		rc = pp->WorkRect;
		pszLine = mpsz_ConChar+TextWidth*(rc.bottom);
		pAttrs = mpn_ConAttrEx+TextWidth*(rc.bottom);

		if ((pp->FarPanelSettings.ShowStatusLine))
		{
			for(x = rc.left+1; x < rc.right; x++)
			{
				if ((pszLine[x] == ucBoxSinglUpHorz || pszLine[x] == ucBoxDblUpSinglHorz || pszLine[x] == ucBoxDblUpSinglHorz)
					&& pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				{
					pszLine[x] = ucBoxSinglHorz;
				}
			}
		}
	}
}

void CVirtualConsole::CharAttrFromConAttr(WORD conAttr, CharAttr* pAttr)
{
	memset(pAttr, 0, sizeof(CharAttr));
	pAttr->nForeIdx = (conAttr & 0xF);
	pAttr->nBackIdx = (conAttr & 0xF0) >> 4;
	pAttr->crForeColor = pAttr->crOrigForeColor = mp_Colors[pAttr->nForeIdx];
	pAttr->crBackColor = pAttr->crOrigBackColor = mp_Colors[pAttr->nBackIdx];
}

// ���������� ��� ��������� ������ Background (CECMD_SETBACKGROUND) �� �������
// � ��� ������� ��� �������� (��������) �������
SetBackgroundResult CVirtualConsole::SetBackgroundImageData(CESERVER_REQ_SETBACKGROUND* apImgData)
{
	if (!this) return esbr_Unexpected;

	//if (!gpConEmu->isMainThread())
	//{

	// ��� ������ �� ��������� ���� (������ ��� ������ �� �������)
	if (mp_RCon->isConsoleClosing())
		return esbr_ConEmuInShutdown;

	bool bUpdate = false;
	SetBackgroundResult rc = mp_Bg->SetPluginBackgroundImageData(apImgData, bUpdate);

	// Need force update?
	if (bUpdate)
		Update(true);

	return rc;

}

#ifdef APPDISTINCTBACKGROUND
CBackgroundInfo* CVirtualConsole::GetBackgroundObject()
{
	if (!this) return NULL;
	if (mp_BgInfo)
	{
		mp_BgInfo->AddRef();
		return mp_BgInfo;
	}
	return gpSetCls->GetBackgroundObject();
}
#endif

void CVirtualConsole::NeedBackgroundUpdate()
{
	if (this && mp_Bg)
		mp_Bg->NeedBackgroundUpdate();
}

bool CVirtualConsole::HasBackgroundImage(LONG* pnBgWidth, LONG* pnBgHeight)
{
	if (!this) return false;

	if (!mp_RCon->isFar())
		return false;

	return mp_Bg->HasPluginBackgroundImage(pnBgWidth, pnBgHeight);

	//if (!mb_BkImgExist || !(mp_BkImgData || (mp_BkEmfData && mb_BkEmfChanged))) return false;

	//// ���������� mn_BkImgXXX ����� �� ������������ �� ��������� mp_BkImgData

	//if (pnBgWidth)
	//	*pnBgWidth = mn_BkImgWidth;

	//if (pnBgHeight)
	//	*pnBgHeight = mn_BkImgHeight;

	//return (mn_BkImgWidth != 0 && mn_BkImgHeight != 0);
	////MSectionLock SBK; SBK.Lock(&csBkImgData);
	////if (mp_BkImgData)
	////{
	////	BITMAPINFOHEADER* pBmp = (BITMAPINFOHEADER*)(mp_BkImgData+1);
	////	if (pnBgWidth)
	////		*pnBgWidth = pBmp->biWidth;
	////	if (pnBgHeight)
	////		*pnBgHeight = pBmp->biHeight;
	////}
	////return mp_BkImgData;
}

void CVirtualConsole::OnTitleChanged()
{
	if (mp_Ghost)
		mp_Ghost->CheckTitle();
}

void CVirtualConsole::SavePaneSnapshoot()
{
	if (!gpConEmu->isMainThread())
	{
		PostMessage(GetView(), mn_MsgSavePaneSnapshoot, 0, 0);
		return;
	}
	
	if (mp_Ghost /*&& !gbNoDblBuffer*/)
	{
		mp_Ghost->UpdateTabSnapshoot(); //CreateTabSnapshoot((HDC)m_DC, Width, Height, TRUE);
	}
}

// ���������� ��� ������ �� CheckBox-� "Show on taskbar"
void CVirtualConsole::OnTaskbarSettingsChanged()
{
	if (gpSet->isTabsOnTaskBar())
	{
		if (!mp_Ghost)
			InitGhost();	
	}
	else
	{
		if (mp_Ghost)
		{
			delete mp_Ghost;
			mp_Ghost = NULL;
		}
	}
}

void CVirtualConsole::OnTaskbarFocus()
{
	if (mp_Ghost)
		mp_Ghost->ActivateTaskbar();
}

HDC CVirtualConsole::GetIntDC()
{
	return (HDC)m_DC;
}
