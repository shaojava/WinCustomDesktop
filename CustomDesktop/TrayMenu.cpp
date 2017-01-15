﻿#include "stdafx.h"
#include "TrayMenu.h"
#include "resource.h"
#include "Global.h"
#include <CDAPI.h>
#include <CDEvents.h>


namespace cd
{
	TrayMenu::TrayMenu() :
		m_trayData(),
		m_managePluginMenuID(GetMenuID()),
		m_exitMenuID(GetMenuID())
	{
		m_trayData.cbSize = sizeof(m_trayData);
		m_trayData.hWnd = g_global.m_fileListWnd;
		m_trayData.uID = 100;
		m_trayData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		m_trayData.uCallbackMessage = GetFileListMsgID();
		m_trayData.hIcon = LoadIcon(g_global.m_cdModule, MAKEINTRESOURCE(IDI_TRAY));
		_tcscpy_s(m_trayData.szTip, _T("CustomDesktop"));

		Init();
	}

	TrayMenu::~TrayMenu()
	{
		Uninit();
	}

	bool TrayMenu::Init()
	{
		Shell_NotifyIcon(NIM_ADD, &m_trayData);
		g_fileListWndProcEvent.AddListener(std::bind(&TrayMenu::OnFileListWndProc, this, std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4));
		g_chooseMenuItemEvent.AddListener(std::bind(&TrayMenu::OnChooseMenuItem, this, std::placeholders::_1));
		return true;
	}

	bool TrayMenu::Uninit()
	{
		Shell_NotifyIcon(NIM_DELETE, &m_trayData);
		return true;
	}


	bool TrayMenu::OnFileListWndProc(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& res)
	{
		if (message == m_trayData.uCallbackMessage) // 托盘右键
		{
			if (lParam != WM_RBUTTONUP)
				return false;

			HMENU menu = CreatePopupMenu();
			g_appendTrayMenuEvent(menu);
			if (GetMenuItemCount(menu) > 0)
				AppendMenu(menu, MF_SEPARATOR, 0, NULL);
			AppendMenu(menu, MF_STRING, m_managePluginMenuID, _T("插件管理"));
			AppendMenu(menu, MF_STRING, m_exitMenuID, _T("退出"));

			POINT pos;
			GetCursorPos(&pos);
			// https://msdn.microsoft.com/zh-cn/library/windows/desktop/ms648002(v=vs.85).aspx
			SetForegroundWindow(g_global.m_topWnd);
			TrackPopupMenu(menu, 0, pos.x, pos.y, 0, g_global.m_fileListWnd, NULL);
			PostMessage(g_global.m_topWnd, WM_NULL, 0, 0);

			DestroyMenu(menu);
			return false;
		}
		else if (message == WM_COMMAND && HIWORD(wParam) == 0) // 选择菜单项
		{
			return g_chooseMenuItemEvent(LOWORD(wParam));
		}
		return true;
	}

	bool TrayMenu::OnChooseMenuItem(UINT menuID)
	{
		if (menuID == m_managePluginMenuID) // 插件管理
		{
			ShellExecuteW(NULL, L"open", (g_global.m_cdDir + L"CDConfig.exe").c_str(), NULL, g_global.m_cdDir.c_str(), SW_SHOWNORMAL);
			return false;
		}
		else if (menuID == m_exitMenuID) // 退出
		{
			SendMessage(g_global.m_fileListWnd, WM_PREUNLOAD, NULL, NULL);
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)FreeLibrary, g_global.m_cdModule, 0, NULL);
			return false;
		}
		return true;
	}
}
