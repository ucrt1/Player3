#include "pch.h"
#include "CVeVolumeBar.h"
#include "CApp.h"

LRESULT CVeVolumeBar::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        Dui::ELEMPAINTSTRU ps;
        BeginPaint(ps, wParam, lParam);
        m_pBrush->SetColor(App->GetColor(GPal::VolBarBk));
        m_pDC->FillRectangle(ps.rcfClipInElem, m_pBrush);

        m_pBrush->SetColor(App->GetColor(GPal::VolBarBorder));
        m_pDC->DrawRectangle(GetViewRectF(), m_pBrush);
        EndPaint(ps);
    }
    return 0;
    case WM_SETFONT:
        m_LAVol.SetTextFormat(GetTextFormat());
        break;
    case WM_CREATE:
    {
        InitEasingCurve(&m_ecShowing);
        m_ecShowing.SetDuration(300);
        m_ecShowing.SetProcedure(eck::Easing::OutCubic);
        m_ecShowing.SetCallback([](float fCurrValue, float fOldValue, LPARAM lParam)
            {
                const auto p = (CVeVolumeBar*)lParam;
                p->m_PageAn.Opacity = fCurrValue;
                p->m_PageAn.Dy = (1.f - fCurrValue) * (float)DVolAn;
                  const D2D1_RECT_F rcOld = p->GetWholeRectInClient();
                p->CompReCalcCompositedRect();
                p->Invalidate(FALSE);
                p->GetWindow().IrUnion(rcOld);
                if (p->m_ecShowing.IsStop())
                {
                    p->SetCompositor(nullptr);
                    if (!p->m_bShow)
                        p->SetStyle(p->GetStyle() & ~Dui::DES_VISIBLE);
                }
            });

        m_PageAn.InitAsTranslationOpacity();

        m_pDC->CreateSolidColorBrush({}, &m_pBrush);
        const auto cx = GetWidth();
        const auto cy = GetHeight();
        m_LAVol.Create(L"100", Dui::DES_VISIBLE | Dui::DES_PARENT_COMP, 0,
            CxVolBarPadding, 0, CxVolLabel, cy, this);
        const auto x = CxVolBarPadding * 2 + CxVolLabel;
        m_TrackBar.Create(nullptr, Dui::DES_VISIBLE |
            Dui::DES_PARENT_COMP | Dui::DES_NOTIFY_TO_WND, 0,
            x, 0, cx - x - CxVolBarPadding, cy, this, nullptr, ELEID_VOLBAR_TRACK);
        m_TrackBar.SetRange(0, 200);
        m_TrackBar.SetTrackPos(100);
        m_TrackBar.SetTrackSize(CyVolTrack);
        m_TrackBar.SetGenEventWhenDragging(TRUE);
    }
    break;
    case WM_DESTROY:
        GetWindow().UnregisterTimeLine(&m_ecShowing);
        SafeRelease(m_pBrush);
        break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}

void CVeVolumeBar::ShowAnimation()
{
    ECK_DUILOCK;
    ECKBOOLNOT(m_bShow);
    SetStyle(GetStyle() | Dui::DES_VISIBLE);
    SetCompositor(&m_PageAn);
    if (m_bShow)
        m_ecShowing.Begin(0.f, 1.f);
    else
        m_ecShowing.Begin(1.f, 0.f);
    GetWindow().KctWake();
}

void CVeVolumeBar::OnVolumeChanged(float fVol)
{
    WCHAR szVol[eck::CchI32ToStrBufNoRadix2];
    swprintf(szVol, L"%d", int(fVol));
    m_LAVol.SetText(szVol);
    m_LAVol.Invalidate();
}