#include "pch.h"
#include "CPlayPanel.h"
#include "CApp.h"


void CPlayPanel::OnPlayEvent(const PLAY_EVT_PARAM& e) noexcept
{
    switch (e.eEvent)
    {
    case PlayEvent::CommonTick:
    {
        const auto& Player = App->Player();
        const auto lfCurrTime = Player.GetCurrentTime();
        const auto lfTotalTime = Player.GetTotalTime();
        m_LATime.SetText(eck::Format(L"%02d:%02d/%02d:%02d",
            int(lfCurrTime / 60), int(lfCurrTime) % 60,
            int(lfTotalTime / 60), int(lfTotalTime) % 60).Data());
        m_LATime.Invalidate();
    }
    break;
    case PlayEvent::Play:
    {
        const auto& mi = App->Player().GetMusicSimpleData();
        m_LATitle.SetText(mi.rsTitle.Data());
        m_LAArtist.SetText(mi.slArtist.FrontData());
    }
    break;
    }
}

LRESULT CPlayPanel::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        Dui::PAINTINFO ps;
        BeginPaint(ps, wParam, lParam);
        GetDC()->FillRectangle(ps.rcClipInEle, GetWindow().CcSetBrushColor(
            App->GetColor(GPal::PlayPanelBk)));
        EndPaint(ps);
    }
    return 0;

    case WM_SETFONT:
    {
        m_LAArtist.SetTextFormat(GetTextFormat().Get());
        m_LATime.SetTextFormat(GetTextFormat().Get());
    }
    break;

    case Dui::EWM_COLORSCHEMECHANGED:
    {
        // FIXME
        //D2D1_COLOR_F cr;
        //GetTheme()->GetSysColor(Dui::SysColor::Text, cr);
        //m_LATitle.SetColor(cr);
        //m_LATitle.UpdateFadeColor();
        //m_LAArtist.SetColor(cr);
        //m_LAArtist.UpdateFadeColor();
    }
    break;

    case WM_CREATE:
    {
        App->Player().GetEventChain().Connect(this, &CPlayPanel::OnPlayEvent);

        const auto pWnd = &GetWindow();

        ComPtr<IDWriteTextFormat> pTfTitle;
        App->GetFontFactory().NewFont(pTfTitle.AtSelf(), eck::Alignment::Near,
            eck::Alignment::Center, (float)CyFontNormal, 700);
        pTfTitle->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        float x = DLeftMiniCover;
        m_Cover.Create(nullptr, Dui::DES_VISIBLE | Dui::DES_NOTIFY_WND, 0,
            x, DTopMiniCover, CxyMiniCover, CxyMiniCover, this, pWnd);
        x += (CxyMiniCover + CxPaddingPlayPanelText);

        D2D1_COLOR_F crText;
        GetTheme()->GetSysColor(Dui::SysColor::Text, crText);
        float y = DTopTitle;
        m_LATitle.Create(L"Violet", Dui::DES_VISIBLE, 0,
            x, y, CxMaxTitleAndArtist, CyPlayPanelText, this, pWnd);
        m_LATitle.SetColor(crText);
        m_LATitle.SetFade(TRUE);
        m_LATitle.SetTextFormat(pTfTitle.Get());
        y += (CyPlayPanelText + CyPaddingTitleAndArtist);

        m_LAArtist.Create(L"Player", Dui::DES_VISIBLE, 0,
            x, y, CxMaxTitleAndArtist, CyPlayPanelText, this, pWnd);
        m_LAArtist.SetColor(crText);
        m_LAArtist.SetFade(TRUE);
        x += (CxMaxTitleAndArtist + CxPaddingPlayPanelText);

        m_LATime.Create(L"00:00/00:00", /*Dui::DES_VISIBLE*/0, 0,
            x, DTopTime, CxMaxTime, CyPlayPanelText, this, pWnd);
    }
    break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}