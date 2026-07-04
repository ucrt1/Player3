#include "pch.h"
#include "CWndMain.h"


void CTabPanel::OnColorSchemeChanged()
{
    m_LAIcon.SetBitmap(((CWindowMain*)GetWnd())->RealizeImage(AppIcon::WindowLogo));
}

LRESULT CTabPanel::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_NOTIFY:
    {
        if (wParam == (WPARAM)&m_TAB)
        {
            switch (((Dui::DUINMHDR*)lParam)->uCode)
            {
            case Dui::TBLE_GETDISPINFO:
            {
                const auto p = (Dui::NMTBLDISPINFO*)lParam;
                const auto pWnd = (CWindowMain*)GetWnd();
                switch (p->idx)
                {
                case 0:
                    p->pszText = L"主页";
                    p->cchText = 2;
                    p->pImage = pWnd->RealizeImage(AppIcon::Home);
                    break;
                case 1:
                    p->pszText = L"列表";
                    p->cchText = 2;
                    p->pImage = pWnd->RealizeImage(AppIcon::List);
                    break;
                case 2:
                    p->pszText = L"效果";
                    p->cchText = 2;
                    p->pImage = pWnd->RealizeImage(AppIcon::Effect);
                    break;
                case 3:
                    p->pszText = L"设置";
                    p->cchText = 2;
                    p->pImage = pWnd->RealizeImage(AppIcon::Settings);
                    break;
                default:
                    p->pszText = L"...";
                    p->cchText = 3;
                    p->pImage = pWnd->RealizeImage(AppIcon::SmallLogo);
                }
            }
            return 0;

            case Dui::EE_CLICK:
            {
                auto nm{ *(Dui::NMTBLITEMINDEX*)lParam };
                nm.uCode = ELEN_PAGE_CHANGE;
                GenElemNotify(&nm);
            }
            return 0;
            }
        }
    }
    break;

    case WM_PAINT:
    {
        Dui::ELEMPAINTSTRU ps;
        BeginPaint(ps, wParam, lParam);
        m_pBrush->SetColor(App->GetColor(GPal::TabPanelBk));
        m_pDC->FillRectangle(ps.rcfClipInElem, m_pBrush);
        EndPaint(ps);
    }
    return 0;

    case WM_SIZE:
    {
        D2D1_RECT_F rc;
        rc.left = (GetWidthF() - CxyWndLogo) / 2.f;
        rc.top = rc.left + CxyWndLogo / 3.f;
        rc.right = rc.left + CxyWndLogo;
        rc.bottom = rc.top + CxyWndLogo;
        m_LAIcon.SetRect(rc);

        const auto Padding = GetTheme()->GetMetrics(Dui::Metrics::SmallPadding);
        m_TAB.SetRect({
            Padding,
            rc.bottom + DWndLogoToTab,
            GetWidthF() - Padding,
            GetHeightF() - Padding });
    }
    break;

    case Dui::EWM_COLORSCHEMECHANGED:
        OnColorSchemeChanged();
        break;

    case WM_CREATE:
        m_pDC->CreateSolidColorBrush({}, &m_pBrush);
        m_LAIcon.Create(nullptr, Dui::DES_VISIBLE, 0,
            0, 0, GetWidthF(), GetWidthF(), this, GetWnd());
        m_LAIcon.SetOnlyBitmap(TRUE);
        m_LAIcon.SetBackgroundMode(eck::BkImgMode::StretchKeepAspectRatio);
        m_LAIcon.SetFullElem(TRUE);

        m_TAB.Create(nullptr, Dui::DES_VISIBLE, 0,
            0, 0, 0, 0, this, GetWnd());
        m_TAB.SetInterpolationMode(D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
        m_TAB.SetItemCount(4);

        OnColorSchemeChanged();
        break;

    case WM_DESTROY:
        SafeRelease(m_pBrush);
        break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}