#include "pch.h"
#include "CVeVolumeBar.h"
#include "CApp.h"

LRESULT CVeVolumeBar::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        Dui::PAINTINFO ps;
        BeginPaint(ps, wParam, lParam);
        GetDC()->FillRectangle(ps.rcClip,
            GetWindow().CcSetBrushColor(App->GetColor(GPal::VolBarBk)));
        GetDC()->DrawRectangle(GetRectInClientD2D(),
            GetWindow().CcSetBrushColor(App->GetColor(GPal::VolBarBorder)));
        EndPaint(ps);
    }
    return 0;
    case WM_SETFONT:
        m_LAVol.SetTextFormat(GetTextFormat().Get());
        break;
    case WM_CREATE:
    {
        GetWindow().KctRegisterTimeLine(this);
        const auto cx = GetWidth();
        const auto cy = GetHeight();
        m_LAVol.Create(L"100"sv, Dui::DES_VISIBLE | Dui::DES_PARENT_COMP, 0,
            CxVolBarPadding, 0, CxVolLabel, cy, this);
        const auto x = CxVolBarPadding * 2 + CxVolLabel;
        m_TrackBar.Create({}, Dui::DES_VISIBLE |
            Dui::DES_PARENT_COMP | Dui::DES_NOTIFY_WND, 0,
            x, 0, cx - x - CxVolBarPadding, cy, this, nullptr, ELEID_VOLBAR_TRACK);
        m_TrackBar.SetRange(0, 200);
        m_TrackBar.SetTrackPosition(100);
        m_TrackBar.SetTrackSize(CyVolTrack);
    }
    break;
    case WM_DESTROY:
        GetWindow().KctUnregisterTimeLine(this);
        break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}

void CVeVolumeBar::ShowAnimation()
{
    ECKBOOLNOT(m_bShow);
    SetStyle(GetStyle() | Dui::DES_VISIBLE);
    SetCompositor(&m_PageAn);
    if (m_bShow)
        m_ecShowing.Start(0.f, 1.f);
    else
        m_ecShowing.Start(1.f, 0.f);
    GetWindow().KctWake();
}

void CVeVolumeBar::OnVolumeChanged(float fVol)
{
    WCHAR szVol[eck::TcvIntBufferSize<int>()];
    PWCH pEnd;
    eck::TcvFromInt(EckArgString(szVol), (int)fVol, 10, TRUE, &pEnd);
    m_LAVol.SetText(std::wstring_view{ szVol, pEnd });
    m_LAVol.Invalidate();
}

void CVeVolumeBar::TlTick(int ms) noexcept
{
    m_msLastInterval = ms;
    m_bAnimating = m_ecShowing.Tick((float)ms, 300.f);
    m_PageAn.SetOpacity(m_ecShowing.K);
    m_PageAn.SetMatrix(D2D1::Matrix3x2F::Translation(0.f, (1.f - m_ecShowing.K) * DVolAn));

    auto rc = GetWholeRectInClient();
    CompUpdateCompositedRect();
    const auto rcNew = GetWholeRectInClient();
    eck::UnionRect(rc, rc, rcNew);
    GetWindow().RdInvalidate(rc, FALSE);

    if (!m_bAnimating)
    {
        SetCompositor(nullptr);
        if (!m_bShow)
            SetStyle(GetStyle() & ~Dui::DES_VISIBLE);
    }
}