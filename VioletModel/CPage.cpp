#include "pch.h"
#include "CPage.h"

LRESULT CPage::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
#ifdef _DEBUG
		Dui::PAINTINFO eps;
		BeginPaint(eps, wParam, lParam);
		ID2D1SolidColorBrush* pBrush;
		GetDC()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &pBrush);
		EckAssert(pBrush);
		auto rc{ GetRectInClientD2D() };
		constexpr float cxStroke = 2.f;
		eck::InflateRect(rc, -cxStroke / 2.f, -cxStroke / 2.f);
		GetDC()->DrawRectangle(rc, pBrush, cxStroke);
		pBrush->Release();
		EndPaint(eps);
#endif
	}
	return 0;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}