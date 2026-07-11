#include "pch.h"
#include "CVeCover.h"


LRESULT CVeCover::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        Dui::PAINTINFO ps;
        BeginPaint(ps, wParam, lParam);

        if (m_pBmp)
        {
            float f;
            const auto size = m_pBmp->GetSize();
            const float cx0 = size.width, cy0 = size.height;
            D2D1_RECT_F rcF;

            const float cxElem = GetWidth();
            const float cyElem = GetHeight();

            if (cxElem / cyElem > cx0 / cy0)// y对齐
            {
                f = cx0 * cyElem / cy0;
                rcF.left = (cxElem - f) / 2.f;
                rcF.right = rcF.left + f;
                rcF.top = 0.f;
                rcF.bottom = rcF.top + cyElem;
            }
            else// x对齐
            {
                f = cxElem * cy0 / cx0;
                rcF.left = 0.f;
                rcF.right = rcF.left + cxElem;
                rcF.top = (cyElem - f) / 2.f;
                rcF.bottom = rcF.top + f;
            }

            GetDC()->DrawBitmap(m_pBmp, &rcF);
        }

        DbgDrawFrame();
        EndPaint(ps);
    }
    return 0;

    case WM_DESTROY:
    {
        SafeRelease(m_pBmp);
    }
    break;
    }

    return __super::OnEvent(uMsg, wParam, lParam);
}