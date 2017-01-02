﻿#pragma once
#include "Singleton.h"
#include <string>
#include <vector>


namespace cd
{
	class Global final : public Singleton<Global>
	{
		DECL_SINGLETON(Global);
	public:
		HWND m_topWnd = NULL;
		HWND m_parentWnd = NULL;
		HWND m_fileListWnd = NULL;
		SIZE m_wndSize;
		SIZE m_screenSize;

		// XP有两个comctl32，所以需要获取所有comctl32句柄
		std::vector<HMODULE> m_comctlModules;

		HMODULE m_cdModule = NULL;
		std::wstring m_cdDir;

		// 正在调用BeginPaint
		bool m_isInBeginPaint = false;
		// 需要更新图标层，comctl调用RedrawWindow后为true
		bool m_needUpdateIcon = true;


		bool Init();

	private:
		Global();
		~Global() = default;
	};

	extern Global& g_global;
}
