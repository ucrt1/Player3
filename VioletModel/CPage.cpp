#include "pch.h"
#include "CPage.h"

LRESULT CPage::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (uMsg)
	{
#ifdef _DEBUG
	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU eps;
		BeginPaint(eps, wParam, lParam);
		ID2D1SolidColorBrush* pBrush;
		GetDC()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &pBrush);
		EckAssert(pBrush);
		auto rcf{ GetViewRectF() };
		constexpr float cxStroke = 2.f;
		eck::InflateRect(rcf, -cxStroke / 2.f, -cxStroke / 2.f);
		GetDC()->DrawRectangle(rcf, pBrush, cxStroke);
		pBrush->Release();
		EndPaint(eps);
	}
	return 0;
#endif
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}