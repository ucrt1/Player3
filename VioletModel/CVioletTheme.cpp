#include "pch.h"
#include "CVioletTheme.h"
#include "CApp.h"

HRESULT CVioletTheme::DrawBackground(Dui::Part ePart, Dui::State eState,
    const D2D1_RECT_F& rc, _In_opt_ const Dui::DTB_OPT* pOpt) noexcept
{
    D2D1_ELLIPSE Ell;
    switch (ePart)
    {
    case Dui::Part::CircleButton:
    {
        Ell.point = D2D1::Point2F(rc.left + rc.right / 2, rc.top + rc.bottom / 2);
        Ell.radiusX = rc.right / 2;
        Ell.radiusY = rc.bottom / 2;
        switch (eState)
        {
        case Dui::State::Normal:
            m_pBrush->SetColor(App->GetColor(GPal::PlayBtnBkNormal));
            break;
        case Dui::State::Hot:
            m_pBrush->SetColor(App->GetColor(GPal::PlayBtnBkHot));
            break;
        case Dui::State::Selected:
            m_pBrush->SetColor(App->GetColor(GPal::PlayBtnBkSelected));
            break;
        default:
            goto DoDef;
        }
        GetDC()->FillEllipse(Ell, m_pBrush.Get());
    }
    return S_OK;

    case Dui::Part::Button:
    {
        if (eState == Dui::State::Normal)
            return S_OK;
    }
    break;

    case Dui::Part::ScrollThumb:
    {
        if (eState == Dui::State::Normal)// 颜色调明显一点，不然看不清
        {
            m_pBrush->SetColor(App->GetColor(GPal::ScrollBarThumb));
            GetDC()->FillRectangle(rc, m_pBrush.Get());
            return S_OK;
        }
    }
    break;
    }
DoDef:
    return __super::DrawBackground(ePart, eState, rc, pOpt);
}