#include "pch.h"
#include "CVeMiniCover.h"
#include "CWndMain.h"

constexpr static float CoverAnimationEndValue = 6.f;

void CVeMiniCover::OnColorSchemeChanged(BOOL bForceUpdateCover) noexcept
{
    if (App->Player().IsDefaultCover() || bForceUpdateCover)
        m_BitmapCover = ((CWindowMain&)GetWindow()).RealizeImage2(AppImage::DefaultCover);
    m_BitmapArrowUp = ((CWindowMain&)GetWindow()).RealizeImage2(AppImage::PlayPageUp);
}

LRESULT CVeMiniCover::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        Dui::PAINTINFO ps;
        BeginPaint(ps, wParam, lParam);
        float k;
        if (m_bAnActive)
        {
            k = m_ec.K;
        BlurDC:
            auto rcView{ GetRectInClientD2D() };
            eck::InflateRect(rcView, k, k);
            GetDC()->DrawBitmap(m_BitmapCover.Get(), rcView, 1.f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_BitmapCover.GetSourceRect());
            GetDC()->Flush();
            GetWindow().CcReserveBitmapLogical(GetWidth(), GetHeight());
            auto rcInTarget{ GetRectInClientD2D() };
            eck::OffsetRect(rcInTarget, ps.ox, ps.oy);
            GetWindow().BlurDrawDC(rcInTarget, {}, k);

            rcView = GetRectInClientD2D();
            rcView.left = (rcView.right - (float)CxyPlayPageArrow) / 2;
            rcView.right = rcView.left + (float)CxyPlayPageArrow;
            rcView.top = (rcView.bottom - (float)CxyPlayPageArrow) / 2 +
                (CoverAnimationEndValue - k) * 4.f/*箭头的行程因子*/;
            rcView.bottom = rcView.top + (float)CxyPlayPageArrow;
            GetDC()->DrawBitmap(m_BitmapArrowUp.Get(), rcView, k / CoverAnimationEndValue,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_BitmapArrowUp.GetSourceRect());
        }
        else
        {
            if (m_bHover)
            {
                k = CoverAnimationEndValue;
                goto BlurDC;
            }
            else
                GetDC()->DrawBitmap(m_BitmapCover.Get(), GetRectInClientD2D(), 1.f,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_BitmapCover.GetSourceRect());
        }

        DbgDrawFrame();
        EndPaint(ps);
    }
    return 0;
    case WM_MOUSEMOVE:
    {
        if (!m_bHover)
        {
            m_bHover = TRUE;
            m_ec.Start(m_ec.K, CoverAnimationEndValue);
            GetWindow().KctWake();
        }
    }
    return 0;
    case WM_MOUSELEAVE:
    {
        if (m_bHover)
        {
            m_bHover = FALSE;
            m_ec.Start(CoverAnimationEndValue, 0.f, m_bAnActive);
            GetWindow().KctWake();
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
            const auto& pt = EagPoint(lParam);
            if (eck::PointInRect(GetViewRect(), pt))
            {
                Dui::ELENMHDR nm{ Dui::ENC_COMMAND };
                SendNotify(&nm);
            }
        }
    }
    break;
    case Dui::EWM_COLORSCHEMECHANGED:
        OnColorSchemeChanged(FALSE);
        break;
    case WM_CREATE:
    {
        GetWindow().KctRegisterTimeLine(this);
        OnColorSchemeChanged(TRUE);
    }
    break;
    case WM_DESTROY:
    {
        m_BitmapCover = {};
        m_BitmapArrowUp = {};
        GetWindow().KctUnregisterTimeLine(this);
    }
    break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}

void CVeMiniCover::TlTick(int ms) noexcept
{
    if (!m_bAnActive)
        return;
    m_bAnActive = m_ec.Tick(ms, 200.f);
    Invalidate();
}