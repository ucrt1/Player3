#include "pch.h"
#include "CWndMain.h"

LRESULT CPageMain::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (uMsg)
	{
	case WM_SIZE:
	{
		m_Lyt.Arrange(GetWidth(), GetHeight());
	}
	return 0;
	case WM_SETFONT:
		m_BTOpenFile.SetTextFormat(GetTextFormat());
		m_BTOpenFolder.SetTextFormat(GetTextFormat());
		return 0;
	case WM_CREATE:
	{
		constexpr eck::LYTMARGINS Mar{ .r = CxPageIntPadding };
		const auto pWnd = (CWindowMain*)GetWnd();
		m_BTOpenFile.Create(L"打开文件", Dui::DES_VISIBLE, 0,
			0, 0, 140, 40, this, GetWnd());
		m_BTOpenFile.SetBitmap(pWnd->RealizeImage(AppImage::File));
		m_Lyt.Add(&m_BTOpenFile, Mar, eck::LF_FIX);

		m_BTOpenFolder.Create(L"打开文件夹", Dui::DES_VISIBLE, 0,
			0, 0, 140, 40, this, GetWnd());
		m_BTOpenFolder.SetBitmap(pWnd->RealizeImage(AppImage::Folder));
		m_BTOpenFolder.SetTextFormat(GetTextFormat());
		m_Lyt.Add(&m_BTOpenFolder, Mar, eck::LF_FIX);

		m_Lyt.Add(&m_Dummy, {}, eck::LF_FIX_HEIGHT | eck::LF_FILL_WIDTH, 1);

		m_LATest.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, 400, 160, this, GetWnd());
		m_LATest.SetOnlyBitmap(TRUE);
		m_LATest.SetBitmap(pWnd->RealizeImage(AppImage::Test));
		//m_LATest.SetFullElem(TRUE);
		m_LATest.SetBackgroundMode(eck::BkImgMode::Center);
		m_Lyt.Add(&m_LATest, Mar, eck::LF_FIX_WIDTH | eck::LF_FILL_HEIGHT);
	}
	break;
	case WM_DESTROY:
		break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}