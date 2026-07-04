#include "pch.h"
#include "CWndMain.h"


void CWndLrc::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
	switch (e.eEvent)
	{
	case PlayEvt::CommTick:
		m_Lrc.LrcSetCurrentLine(App->Player().GetCurrLrcIdx());
		break;
	case PlayEvt::Play:
	{
		Lyric::CLyric pLyric;
		App->Player().GetLrc(pLyric());
		m_Lrc.SetLyric(pLyric.Get());
	}
	break;
	}
}

LRESULT CWndLrc::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (uMsg)
	{
	case WM_NCHITTEST:
	{
		POINT pt ECK_GET_PT_LPARAM(lParam);
		ScreenToClient(Handle, &pt);
		const auto cxPadded = eck::DaGetSystemMetrics(SM_CXPADDEDBORDER, GetWindowDpi());
		const auto cxFrame = eck::DaGetSystemMetrics(SM_CXFRAME, GetWindowDpi()) + cxPadded;
		const auto cyFrame = eck::DaGetSystemMetrics(SM_CYFRAME, GetWindowDpi()) + cxPadded;
		const MARGINS m{ cxFrame,cyFrame,cxFrame,cyFrame };
		auto lResult = eck::MsgOnNcHitTest(pt, m, GetClientWidth(), GetClientHeight());
		if (lResult == HTCAPTION)
		{
			lResult = __super::OnMessage(uMsg, wParam, lParam);
			if (!EtCurrentNcHitTest() || EtCurrentNcHitTest() == &m_Lrc)
				lResult = HTCAPTION;
		}
		return lResult;
	}
	break;
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	{
		if (m_bShowBk)
			break;
		m_bShowBk = TRUE;
		m_AnFade.Start(0.0f, 1.0f, m_bAnFade);
		m_bAnFade = TRUE;
		SetTimer(Handle, IDT_LRC_MOUSELEAVE, TE_LRC_MOUSELEAVE, nullptr);
		KctWake();
	}
	break;
	case WM_SIZE:
	{
		const auto lResult = __super::OnMessage(uMsg, wParam, lParam);
		const auto cxLyt = m_Layout.LoGetSize().cx;
		m_Layout.Arrange(
			(GetClientWidthLogical() - cxLyt) / 2,
			CxyLrcPadding,
			cxLyt, CxyLrcBtn);
		m_Lrc.SetRect({
			CxyLrcPadding,
			float(m_Layout.LoGetPosition().y + m_Layout.LoGetSize().cy + CxyLrcPadding),
			GetClientWidthLogical() - CxyLrcPadding,
			GetClientHeightLogical() - CxyLrcPadding });
		return lResult;
	}
	break;
	case WM_TIMER:
	{
		switch (wParam)
		{
		case IDT_LRC_MOUSELEAVE:
		{
			if (GetCapture() == Handle)
				return 0;
			RECT rc;
			GetWindowRect(Handle, &rc);
			POINT pt;
			GetCursorPos(&pt);
			if (!PtInRect(&rc, pt))
			{
				m_bShowBk = FALSE;
				KillTimer(Handle, IDT_LRC_MOUSELEAVE);
				m_AnFade.Start(1.0f, 0.0f, m_bAnFade);
				m_bAnFade = TRUE;
				KctWake();
			}
		}
		return 0;
		}
	}
	break;
	case WM_SHOWWINDOW:
	{
		if (m_bInitShow)
			SetTimer(Handle, IDT_LRC_MOUSELEAVE, TE_LRC_MOUSELEAVE, nullptr);
	}
	break;
	case WM_CREATE:
	{
		const auto lResult = __super::OnMessage(uMsg, wParam, lParam);

		App->Player().GetSignal().Connect(this, &CWndLrc::OnPlayEvent);
		KctRegisterTimeLine(this);

		m_pVioletTheme = new CVioletTheme{};
		StRegisterAutoTheme(m_pVioletTheme);

		constexpr eck::LYTMARGINS Mar{ .r = CxyLrcPadding };
		m_BTPrev.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTPrev.SetBitmap(App->GetMainWindow().RealizeImage(AppImage::PrevSolid));
		m_BTPrev.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTPrev, Mar, eck::LF_FIX);

		m_BTPlay.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTPlay.SetBitmap(App->GetMainWindow().RealizeImage(AppImage::TriangleSolid));
		m_BTPlay.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTPlay, Mar, eck::LF_FIX);

		m_BTNext.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn * 2, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTNext.SetBitmap(App->GetMainWindow().RealizeImage(AppImage::NextSolid));
		m_BTNext.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTNext, Mar, eck::LF_FIX);

		m_BTLock.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn * 3, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTLock.SetBitmap(App->GetMainWindow().RealizeImage(AppImage::LockSolid));
		m_BTLock.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTLock, Mar, eck::LF_FIX);

		m_BTClose.Create(nullptr, Dui::DES_VISIBLE, 0,
			CxyLrcBtn * 4, 0, CxyLrcBtn, CxyLrcBtn, nullptr, this);
		m_BTClose.SetBitmap(App->GetMainWindow().RealizeImage(AppImage::CrossSolid));
		m_BTClose.SetTheme(m_pVioletTheme);
		m_Layout.Add(&m_BTClose, {}, eck::LF_FIX);

		m_Lrc.Create(nullptr, Dui::DES_VISIBLE, 0,
			0, 0, 0, 0, nullptr, this);
		ComPtr<IDWriteTextFormat> pTfLrc;
		auto& FontFactory = App->GetFontFactory();;
		FontFactory.NewFont(pTfLrc.RefOfClear(),
			eck::Alignment::Near, eck::Alignment::Near, 30, 700);
		pTfLrc->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		m_Lrc.SetTextFormat(pTfLrc.Get());
		FontFactory.NewFont(pTfLrc.RefOfClear(),
			eck::Alignment::Near, eck::Alignment::Near, 20, 500);
		pTfLrc->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		m_Lrc.SetTextFormatTrans(pTfLrc.Get());
		m_Lrc.LrcSetEmptyText(L"VioletModel - VC++/Win32"sv);

		return lResult;
	}
	break;
	case WM_DESTROY:
	{
		const auto lResult = __super::OnMessage(uMsg, wParam, lParam);
		SafeRelease(m_pVioletTheme);
		return lResult;
	}
	break;
	}
	return __super::OnMessage(uMsg, wParam, lParam);
}

LRESULT CWndLrc::OnElementNotify(Dui::CElement* pEle, Dui::ELENMHDR* pnm) noexcept
{
	if (pEle == &m_Lrc)
		switch (pnm->uNotify)
		{
		case ELEN_DTLRC_GET_TIME:
		{
			const auto p = (NM_DTL_GET_TIME*)pnm;
			p->fTime = (float)App->Player().GetBass().GetPosition();
		}
		return 0;
		}
	if (pEle == &m_BTPrev)
		App->Player().Prev();
	else if (pEle == &m_BTPlay)
		App->Player().PlayOrPause();
	else if (pEle == &m_BTNext)
		App->Player().Next();
	//else if (pEle == &m_BTLock)
	//	;
	//else if (pEle == &m_BTClose)
	//	;
	return __super::OnElementNotify(pEle, pnm);
}

LRESULT CWndLrc::OnRenderEvent(UINT uMsg, Dui::RENDER_EVENT& e) noexcept
{
	if (uMsg == Dui::RE_FILLBACKGROUND)
	{
		constexpr float MaxAlpha = 0.4f;
		if (m_bAnFade)
			BbrGet()->SetColor({ .a = m_AnFade.K * MaxAlpha });
		else if (m_bShowBk)
			BbrGet()->SetColor({ .a = MaxAlpha });
		else
			return Dui::RER_NONE;
		GetDeviceContext()->FillRectangle(e.FillBkg.rc, BbrGet());
		return Dui::RER_NONE;
	}
	return __super::OnRenderEvent(uMsg, e);
}

void CWndLrc::TlTick(int iMs) noexcept
{
	if (m_bAnFade)
	{
		m_bAnFade = m_AnFade.Tick((float)iMs, 200);
		Redraw(FALSE);
	}
}