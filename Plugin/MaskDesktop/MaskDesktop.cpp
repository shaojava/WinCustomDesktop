﻿#include "stdafx.h"
#include "MaskDesktop.h"
#include <CDEvents.h>
#include <CDAPI.h>
#include "Config.h"
#include <thread>
#include <shellapi.h>


MaskDesktop::MaskDesktop(HMODULE hModule) : 
	m_module(hModule),
	m_menuID(cd::GetMenuID())
{
	// 载入图片
	InitImg();

	// 监听事件
	cd::g_fileListWndProcEvent.AddListener(std::bind(&MaskDesktop::OnFileListWndProc, this, std::placeholders::_1, 
		std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), m_module);
	cd::g_postDrawIconEvent.AddListener(std::bind(&MaskDesktop::OnPostDrawIcon, this, std::placeholders::_1), m_module);
	cd::g_appendTrayMenuEvent.AddListener(std::bind(&MaskDesktop::OnAppendTrayMenu, this, std::placeholders::_1), m_module);
	cd::g_chooseMenuItemEvent.AddListener(std::bind(&MaskDesktop::OnChooseMenuItem, this, std::placeholders::_1), m_module);

	cd::RedrawDesktop();
}

void MaskDesktop::InitImg()
{
	CImage img;
	img.Load(g_config.m_imagePath.c_str());
	if (!m_img.IsNull())
		m_img.Destroy();
	m_img.Create(g_config.m_size, g_config.m_size, 32, CImage::createAlphaChannel);
	img.Draw(m_img.GetDC(), -5, -5, g_config.m_size + 10, g_config.m_size + 10);
	m_img.ReleaseDC();
}


bool MaskDesktop::OnFileListWndProc(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& res)
{
	if (message == WM_MOUSEMOVE)
	{
		const auto lastPos = m_curPos;
		m_curPos = MAKEPOINTS(lParam);

		RECT rect;
		if (m_curPos.x < lastPos.x)
		{
			rect.left = m_curPos.x - g_config.m_size / 2 - 1;
			rect.right = lastPos.x + g_config.m_size / 2 + 1;
		}
		else
		{
			rect.left = lastPos.x - g_config.m_size / 2 - 1;
			rect.right = m_curPos.x + g_config.m_size / 2 + 1;
		}
		if (m_curPos.y < lastPos.y)
		{
			rect.top = m_curPos.y - g_config.m_size / 2 - 1;
			rect.bottom = lastPos.y + g_config.m_size / 2 + 1;
		}
		else
		{
			rect.top = lastPos.y - g_config.m_size / 2 - 1;
			rect.bottom = m_curPos.y + g_config.m_size / 2 + 1;
		}

		cd::RedrawDesktop(&rect);
	}
	return true;
}

bool MaskDesktop::OnPostDrawIcon(HDC& hdc)
{
	if (m_img.IsNull())
		return true;

	m_img.AlphaBlend(hdc, m_curPos.x - g_config.m_size / 2, m_curPos.y - g_config.m_size / 2);

	const HBRUSH brush = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	SIZE scrSize;
	cd::GetDesktopSize(scrSize);
	RECT rect;
	rect = { 0, 0, m_curPos.x - g_config.m_size / 2 + 1, scrSize.cy };
	FillRect(hdc, &rect, brush);
	rect = { m_curPos.x - g_config.m_size / 2 + 1, 0, m_curPos.x + g_config.m_size / 2 - 1, m_curPos.y - g_config.m_size / 2 + 1 };
	FillRect(hdc, &rect, brush);
	rect = { m_curPos.x + g_config.m_size / 2 - 1, 0, scrSize.cx, scrSize.cy };
	FillRect(hdc, &rect, brush);
	rect = { m_curPos.x - g_config.m_size / 2 + 1, m_curPos.y + g_config.m_size / 2 - 1, m_curPos.x + g_config.m_size / 2 - 1, scrSize.cy };
	FillRect(hdc, &rect, brush);

	return true;
}


bool MaskDesktop::OnAppendTrayMenu(HMENU menu)
{
	AppendMenu(menu, MF_STRING, m_menuID, APPNAME);
	return true;
}

bool MaskDesktop::OnChooseMenuItem(UINT menuID)
{
	if (menuID != m_menuID)
		return true;

	std::thread([this]{
		SHELLEXECUTEINFOW info = {};
		info.cbSize = sizeof(info);
		info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
		info.lpVerb = L"open";
		info.lpFile = L"notepad.exe";
		std::wstring param = cd::GetPluginDir() + L"\\Data\\MaskDesktop.ini";
		info.lpParameters = param.c_str();
		info.nShow = SW_SHOWNORMAL;
		ShellExecuteExW(&info);

		WaitForSingleObject(info.hProcess, INFINITE);
		CloseHandle(info.hProcess);

		cd::ExecInMainThread([this]{
			Config newConfig;
			int oldSize = g_config.m_size;
			g_config = newConfig;
			if (newConfig.m_size != oldSize)
				InitImg();
		});
	}).detach();
	return false;
}
