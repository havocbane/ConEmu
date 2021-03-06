
/*
Copyright (c) 2010-2011 Maximus5
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


#ifdef _DEBUG
//  �����������������, ����� ����� ����� �������� ������� �������� MessageBox, ����� ����������� ����������
//  #define SHOW_STARTED_MSGBOX
//  #define SHOW_WRITING_RECTS
#endif


#include <windows.h>

#include "../common/common.hpp"
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginW1761.hpp" // ���������� �� 995 �������� SynchoApi
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "ConEmuLn.h"
#include "../common/FarVersion.h"

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))


#if defined(__GNUC__)
extern "C" {
	BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
	void WINAPI SetStartupInfoW(void *aInfo);
	void WINAPI OnConEmuLoaded(struct ConEmuLoadedArg* pConEmuInfo);
	void WINAPI GetPluginInfoWcmn(void *piv);
};
#endif

HMODULE ghPluginModule = NULL; // ConEmuLn.dll - ��� ������
//wchar_t* gszRootKey = NULL;
FarVersion gFarVersion = {};
static RegisterBackground_t gfRegisterBackground = NULL;
bool gbInStartPlugin = false;
bool gbSetStartupInfoOk = false;
ConEmuLnSettings gSettings[] = {
	{L"PluginEnabled", (LPBYTE)&gbBackgroundEnabled, REG_BINARY, 1},
	{L"LinesColor", (LPBYTE)&gcrLinesColor, REG_DWORD, 4},
	{L"HilightType", (LPBYTE)&giHilightType, REG_DWORD, 4},
	{L"HilightPlugins", (LPBYTE)&gbHilightPlugins, REG_BINARY, 1},
	{L"HilightPlugBack", (LPBYTE)&gcrHilightPlugBack, REG_DWORD, 4},
	{NULL}
};

BOOL gbBackgroundEnabled = FALSE;
COLORREF gcrLinesColor = RGB(0,0,0xA8); // ���� ������� ������
int giHilightType = 0; // 0 - �����, 1 - ������
BOOL gbHilightPlugins = FALSE;
COLORREF gcrHilightPlugBack = RGB(0xA8,0,0); // ���� ������� ��������



// minimal(?) FAR version 2.0 alpha build FAR_X_VER
int WINAPI GetMinFarVersionW(void)
{
	// ACTL_SYNCHRO required
	return MAKEFARVERSION(2,0,max(1007,FAR_X_VER));
}

void WINAPI GetPluginInfoWcmn(void *piv)
{
	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(GetPluginInfoW)(piv);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(GetPluginInfoW)(piv);
	else
		FUNC_X(GetPluginInfoW)(piv);
}


BOOL gbInfoW_OK = FALSE;


void WINAPI ExitFARW(void);
void WINAPI ExitFARW3(void*);
int WINAPI ConfigureW(int ItemNumber);
int WINAPI ConfigureW3(void*);

#include "../common/SetExport.h"
ExportFunc Far3Func[] =
{
	{"ExitFARW", (void*)ExitFARW, (void*)ExitFARW3},
	{"ConfigureW", (void*)ConfigureW, (void*)ConfigureW3},
	{NULL}
};

BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			ghPluginModule = (HMODULE)hModule;
			//ghWorkingModule = (u64)hModule;
			HeapInitialize();

#ifdef SHOW_STARTED_MSGBOX
			if (!IsDebuggerPresent())
				MessageBoxA(NULL, "ConEmuLn*.dll loaded", "ConEmuLn plugin", 0);
#endif

			bool lbExportsChanged = false;
			if (LoadFarVersion())
			{
				if (gFarVersion.dwVerMajor == 3)
				{
					lbExportsChanged = ChangeExports( Far3Func, ghPluginModule );
					if (!lbExportsChanged)
					{
						_ASSERTE(lbExportsChanged);
					}
				}
			}
		}
		break;
		case DLL_PROCESS_DETACH:
			HeapDeinitialize();
			break;
	}

	return TRUE;
}

#if defined(CRTSTARTUP)
extern "C" {
	BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
};

BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
#endif




BOOL LoadFarVersion()
{
	wchar_t ErrText[512]; ErrText[0] = 0;
	BOOL lbRc = LoadFarVersion(gFarVersion, ErrText);

	if (!lbRc)
	{
		gFarVersion.dwVerMajor = 2;
		gFarVersion.dwVerMinor = 0;
		gFarVersion.dwBuild = FAR_X_VER;
	}

	return lbRc;
}

void WINAPI SetStartupInfoW(void *aInfo)
{
	gbSetStartupInfoOk = true;

	if (!gFarVersion.dwVerMajor) LoadFarVersion();

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(SetStartupInfoW)(aInfo);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(SetStartupInfoW)(aInfo);
	else
		FUNC_X(SetStartupInfoW)(aInfo);

	//_ASSERTE(gszRootKey!=NULL && *gszRootKey!=0);
	gbInfoW_OK = TRUE;
	StartPlugin(FALSE);
}

int WINAPI PaintConEmuBackground(struct PaintBackgroundArg* pBk)
{
	DWORD nPanelBackIdx = (pBk->nFarColors[col_PanelText] & 0xF0) >> 4;

	if (pBk->dwLevel == 0)
	{
		if (pBk->LeftPanel.bVisible)
		{
			COLORREF crPanel = pBk->crPalette[nPanelBackIdx];

			if (pBk->LeftPanel.bPlugin && gbHilightPlugins)
				crPanel = gcrHilightPlugBack;

			HBRUSH hBr = CreateSolidBrush(crPanel);
			FillRect(pBk->hdc, &pBk->rcDcLeft, hBr);
			DeleteObject(hBr);
		}

		if (pBk->RightPanel.bVisible)
		{
			COLORREF crPanel = pBk->crPalette[nPanelBackIdx];

			if (pBk->RightPanel.bPlugin && gbHilightPlugins)
				crPanel = gcrHilightPlugBack;

			HBRUSH hBr = CreateSolidBrush(crPanel);
			FillRect(pBk->hdc, &pBk->rcDcRight, hBr);
			DeleteObject(hBr);
		}
	}

	if ((pBk->LeftPanel.bVisible || pBk->RightPanel.bVisible) /*&& pBk->MainFont.nFontHeight>0*/)
	{
		HPEN hPen = CreatePen(PS_SOLID, 1, gcrLinesColor);
		HPEN hOldPen = (HPEN)SelectObject(pBk->hdc, hPen);
		HBRUSH hBrush = CreateSolidBrush(gcrLinesColor);
		int nCellHeight = 12;

		if (pBk->LeftPanel.bVisible)
			nCellHeight = pBk->rcDcLeft.bottom / (pBk->LeftPanel.rcPanelRect.bottom - pBk->LeftPanel.rcPanelRect.top + 1);
		else
			nCellHeight = pBk->rcDcRight.bottom / (pBk->RightPanel.rcPanelRect.bottom - pBk->RightPanel.rcPanelRect.top + 1);

		int nY1 = (pBk->LeftPanel.bVisible) ? pBk->rcDcLeft.top : pBk->rcDcRight.top;
		int nY2 = (pBk->rcDcLeft.bottom >= pBk->rcDcRight.bottom) ? pBk->rcDcLeft.bottom : pBk->rcDcRight.bottom;
		int nX1 = (pBk->LeftPanel.bVisible) ? 0 : pBk->rcDcRight.left;
		int nX2 = (pBk->RightPanel.bVisible) ? pBk->rcDcRight.right : pBk->rcDcLeft.right;

		bool bDrawStipe = true;
		
		for(int Y = nY1; Y < nY2; Y += nCellHeight)
		{
			if (giHilightType == 0)
			{
				MoveToEx(pBk->hdc, nX1, Y, NULL);
				LineTo(pBk->hdc, nX2, Y);
			}
			else if (giHilightType == 1)
			{
				if (bDrawStipe)
				{
					bDrawStipe = false;
					RECT rc = {nX1, Y - nCellHeight + 1, nX2, Y};
					FillRect(pBk->hdc, &rc, hBrush);
				}
				else
				{
					bDrawStipe = true;
				}
			}
		}

		SelectObject(pBk->hdc, hOldPen);
		DeleteObject(hPen);
		DeleteObject(hBrush);
	}

	return TRUE;
}

void WINAPI OnConEmuLoaded(struct ConEmuLoadedArg* pConEmuInfo)
{
	gfRegisterBackground = pConEmuInfo->RegisterBackground;
	ghPluginModule = pConEmuInfo->hPlugin;

	if (gfRegisterBackground && gbBackgroundEnabled)
	{
		StartPlugin(FALSE/*������� ��������� �� �������*/);
	}
}

void SettingsLoad()
{
	if (!gbSetStartupInfoOk)
	{
		_ASSERTE(gbSetStartupInfoOk);
		return;
	}

	if (gFarVersion.dwVerMajor == 1)
		SettingsLoadA();
	else if (gFarVersion.dwBuild >= FAR_Y2_VER)
		FUNC_Y2(SettingsLoadW)();
	else if (gFarVersion.dwBuild >= FAR_Y1_VER)
		FUNC_Y1(SettingsLoadW)();
	else
		FUNC_X(SettingsLoadW)();
}

void SettingsLoadReg(LPCWSTR pszRegKey)
{
	HKEY hkey = NULL;

	if (!RegOpenKeyExW(HKEY_CURRENT_USER, pszRegKey, 0, KEY_READ, &hkey))
	{
		DWORD nVal, nType, nSize; BYTE cVal;

		for (ConEmuLnSettings *p = gSettings; p->pszValueName; p++)
		{
			if (p->nValueType == REG_BINARY)
			{
				_ASSERTE(p->nValueSize == 1);
				if (!RegQueryValueExW(hkey, p->pszValueName, 0, &(nType = REG_BINARY), &cVal, &(nSize = sizeof(cVal))))
					*((BOOL*)p->pValue) = (cVal != 0);
			}
			else if (p->nValueType == REG_DWORD)
			{
				_ASSERTE(p->nValueSize == 4);
				if (!RegQueryValueExW(hkey, p->pszValueName, 0, &(nType = REG_DWORD), (LPBYTE)&nVal, &(nSize = sizeof(nVal))))
					*((DWORD*)p->pValue) = nVal;
			}
		}

		//if (!RegQueryValueExW(hkey, L"PluginEnabled", 0, &(nType = REG_BINARY), &cVal, &(nSize = sizeof(cVal))))
		//	gbBackgroundEnabled = (cVal != 0);

		//if (!RegQueryValueExW(hkey, L"LinesColor", 0, &(nType = REG_DWORD), (LPBYTE)&nVal, &(nSize = sizeof(nVal))))
		//	gcrLinesColor = nVal;

		//if (!RegQueryValueExW(hkey, L"HilightPlugins", 0, &(nType = REG_BINARY), &cVal, &(nSize = sizeof(cVal))))
		//	gbHilightPlugins = (cVal != 0);

		//if (!RegQueryValueExW(hkey, L"HilightPlugBack", 0, &(nType = REG_DWORD), (LPBYTE)&nVal, &(nSize = sizeof(nVal))))
		//	gcrHilightPlugBack = nVal;

		RegCloseKey(hkey);
	}
}

void SettingsSave()
{
	if (gFarVersion.dwVerMajor == 1)
		SettingsSaveA();
	else if (gFarVersion.dwBuild >= FAR_Y2_VER)
		FUNC_Y2(SettingsSaveW)();
	else if (gFarVersion.dwBuild >= FAR_Y1_VER)
		FUNC_Y1(SettingsSaveW)();
	else
		FUNC_X(SettingsSaveW)();
}

void SettingsSaveReg(LPCWSTR pszRegKey)
{
	HKEY hkey = NULL;

	if (!RegCreateKeyExW(HKEY_CURRENT_USER, pszRegKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, NULL))
	{
		BYTE cVal;

		for (ConEmuLnSettings *p = gSettings; p->pszValueName; p++)
		{
			if (p->nValueType == REG_BINARY)
			{
				_ASSERTE(p->nValueSize == 1);
				cVal = (BYTE)*(BOOL*)p->pValue;
				RegSetValueExW(hkey, p->pszValueName, 0, REG_BINARY, &cVal, sizeof(cVal));
			}
			else if (p->nValueType == REG_DWORD)
			{
				_ASSERTE(p->nValueSize == 4);
				RegSetValueExW(hkey, p->pszValueName, 0, REG_DWORD, p->pValue, p->nValueSize);
			}
		}

		//if (!RegQueryValueExW(hkey, L"PluginEnabled", 0, &(nType = REG_BINARY), &cVal, &(nSize = sizeof(cVal))))
		//	gbBackgroundEnabled = (cVal != 0);

		//if (!RegQueryValueExW(hkey, L"LinesColor", 0, &(nType = REG_DWORD), (LPBYTE)&nVal, &(nSize = sizeof(nVal))))
		//	gcrLinesColor = nVal;

		//if (!RegQueryValueExW(hkey, L"HilightPlugins", 0, &(nType = REG_BINARY), &cVal, &(nSize = sizeof(cVal))))
		//	gbHilightPlugins = (cVal != 0);

		//if (!RegQueryValueExW(hkey, L"HilightPlugBack", 0, &(nType = REG_DWORD), (LPBYTE)&nVal, &(nSize = sizeof(nVal))))
		//	gcrHilightPlugBack = nVal;

		RegCloseKey(hkey);
	}
}

void StartPlugin(BOOL bConfigure)
{
	if (gbInStartPlugin)
	{
		// ��������� ������� ���� �� ������
		_ASSERTE(gbInStartPlugin==false);
		return;
	}

	gbInStartPlugin = true;

	if (!bConfigure)
	{
		SettingsLoad();
	}

	static bool bWasRegistered = false;

	if (gfRegisterBackground)
	{
		if (gbBackgroundEnabled)
		{
			if (bConfigure && bWasRegistered)
			{
				RegisterBackgroundArg upd = {sizeof(RegisterBackgroundArg), rbc_Redraw};
				gfRegisterBackground(&upd);
			}
			else
			{
				RegisterBackgroundArg reg = {sizeof(RegisterBackgroundArg), rbc_Register, ghPluginModule};
				reg.PaintConEmuBackground = ::PaintConEmuBackground;
				reg.dwPlaces = pbp_Panels;
				reg.dwSuggestedLevel = 100;
				gfRegisterBackground(&reg);
				bWasRegistered = true;
			}
		}
		else
		{
			RegisterBackgroundArg unreg = {sizeof(RegisterBackgroundArg), rbc_Unregister, ghPluginModule};
			gfRegisterBackground(&unreg);
			bWasRegistered = false;
		}
	}
	else
	{
		bWasRegistered = false;
	}

	// ������� ���� �������
	gbInStartPlugin = false;
}

void ExitPlugin(void)
{
	if (gfRegisterBackground != NULL)
	{
		RegisterBackgroundArg inf = {sizeof(RegisterBackgroundArg), rbc_Unregister, ghPluginModule};
		gfRegisterBackground(&inf);
	}

	gbSetStartupInfoOk = false;

	//if (gszRootKey)
	//{
	//	free(gszRootKey);
	//}
}

void   WINAPI ExitFARW(void)
{
	ExitPlugin();

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(ExitFARW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
}

void WINAPI ExitFARW3(void*)
{
	ExitPlugin();

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(ExitFARW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
}



LPCWSTR GetMsgW(int aiMsg)
{
	if (gFarVersion.dwVerMajor==1)
		return L"";
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(GetMsgW)(aiMsg);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(GetMsgW)(aiMsg);
	else
		return FUNC_X(GetMsgW)(aiMsg);
}


int WINAPI ConfigureW(int ItemNumber)
{
	if (gFarVersion.dwVerMajor==1)
		return false;
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(ConfigureW)(ItemNumber);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(ConfigureW)(ItemNumber);
	else
		return FUNC_X(ConfigureW)(ItemNumber);
}

int WINAPI ConfigureW3(void*)
{
	if (gFarVersion.dwVerMajor==1)
		return false;
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(ConfigureW)(0);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(ConfigureW)(0);
	else
		return FUNC_X(ConfigureW)(0);
}

HANDLE OpenPluginWcmn(int OpenFrom,INT_PTR Item)
{
	HANDLE hResult = (gFarVersion.dwVerMajor >= 3) ? NULL : INVALID_HANDLE_VALUE;

	if (!gbInfoW_OK)
		return hResult;

	ConfigureW(0);

	return hResult;
}

HANDLE WINAPI OpenW(const void *Info)
{
	HANDLE hResult = NULL;

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		hResult = FUNC_Y2(OpenW)(Info);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		hResult = FUNC_Y1(OpenW)(Info);
	else
	{
		_ASSERTE(FALSE && "Must not called in Far2");
	}

	return hResult;
}
