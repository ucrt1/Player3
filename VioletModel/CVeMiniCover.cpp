#include "pch.h"
#include "CVeMiniCover.h"
#include "CWndMain.h"

constexpr static float CoverAnEndValue = 6.f;

void CVeMiniCover::OnColorSchemeChanged(BOOL bForceUpdateCover)
{
	if (App->GetPlayer().IsDefaultCover() ||
		bForceUpdateCover)
	{
		if (m_pBmp)
			m_pBmp->Release();
		m_pBmp = ((CWindowMain*)GetWnd())->RealizeImage(AppIcon::DefaultCover);
		m_pBmp->AddRef();
	}

	if (m_pBmpCoverUp)
		m_pBmpCoverUp->Release();
	m_pBmpCoverUp = ((CWindowMain*)GetWnd())->RealizeImage(AppIcon::PlayPageUp);
	m_pBmpCoverUp->AddRef();
}

LRESULT CVeMiniCover::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		Dui::ELEMPAINTSTRU ps;
		BeginPaint(ps, wParam, lParam);
		float fValue;
		if (m_pec->IsActive())
		{
			fValue = m_pec->GetCurrentValue();
		BlurDC:
			auto rcView{ GetViewRectF() };
			eck::InflateRect(rcView, fValue, fValue);
			m_pDC->DrawBitmap(m_pBmp, rcView);
			m_pDC->Flush();
			GetWnd()->CacheReserveLogSize(GetWidthF(), GetHeightF());
			auto rcInTarget{ GetRectInClientF() };
			eck::OffsetRect(rcInTarget, ps.ox, ps.oy);
			GetWnd()->BlurDrawDC(rcInTarget, {}, fValue);

			rcView = GetViewRectF();
			rcView.left = (rcView.right - (float)CxyPlayPageArrow) / 2;
			rcView.right = rcView.left + (float)CxyPlayPageArrow;
			rcView.top = (rcView.bottom - (float)CxyPlayPageArrow) / 2 +
				(CoverAnEndValue - fValue) * 4.f/*箭头的行程因子*/;
			rcView.bottom = rcView.top + (float)CxyPlayPageArrow;
			m_pDC->DrawBitmap(m_pBmpCoverUp, rcView, fValue / CoverAnEndValue);
		}
		else
		{
			if (m_bHover)
			{
				fValue = CoverAnEndValue;
				goto BlurDC;
			}
			else
				m_pDC->DrawBitmap(m_pBmp, GetViewRectF());
		}

		ECK_DUI_DBG_DRAW_FRAME;
		EndPaint(ps);
	}
	return 0;
	case WM_MOUSEMOVE:
	{
		if (!m_bHover)
		{
			m_bHover = TRUE;
			m_pec->Begin(0.f, CoverAnEndValue);
			GetWnd()->KctWake();
		}
	}
	return 0;
	case WM_MOUSELEAVE:
	{
		if (m_bHover)
		{
			m_bHover = FALSE;
			m_pec->Begin(CoverAnEndValue, 0.f);
			GetWnd()->KctWake();
		}
	}
	return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	{
		m_bLBtnDown = TRUE;
		SetCapture();
	}
	break;
	case WM_LBUTTONUP:
	{
		if (m_bLBtnDown)
		{
			m_bLBtnDown = FALSE;
			ReleaseCapture();
			const POINT pt ECK_GET_PT_LPARAM(lParam);
			if (eck::PtInRect(GetViewRectF(), pt))
			{
				VEN_MINICOVER_CLICK n{};
				n.uCode = ELEN_MINICOVER_CLICK;
				GenElemNotify(&n);
			}
		}
	}
	break;
	case Dui::EWM_COLORSCHEMECHANGED:
		OnColorSchemeChanged(FALSE);
		break;
	case WM_CREATE:
	{
		m_pec = new eck::CEasingCurve{};
		InitEasingCurve(m_pec);
		m_pec->SetProcedure(eck::Easing::OutCubic);
		m_pec->SetDuration(200.f);
		m_pec->SetCallback([](float fCurrValue, float fOldValue, LPARAM lParam)
			{
				const auto p = (CVeMiniCover*)lParam;
				p->Invalidate();
			});
		OnColorSchemeChanged(TRUE);
	}
	break;

	case WM_DESTROY:
	{
		SafeRelease(m_pec);
		SafeRelease(m_pBmp);
		SafeRelease(m_pBmpCoverUp);
	}
	break;
	}
	return __super::OnEvent(uMsg, wParam, lParam);
}