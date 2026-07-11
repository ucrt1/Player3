#include "pch.h"
#include "CLrGeometryRealization.h"
#include "CVeLrc.h"

static constexpr D2D1_COLOR_F InterpolateColor(
    const D2D1_COLOR_F& c1, const D2D1_COLOR_F& c2, float k) noexcept
{
    return {
        c1.r + (c2.r - c1.r) * k,
        c1.g + (c2.g - c1.g) * k,
        c1.b + (c2.b - c1.b) * k,
        c1.a + (c2.a - c1.a) * k };
}

void CLyricRendererD2D::ReCreateFadeBrush() noexcept
{
    m_pBrFade.Clear();
    if (!(GetFlags() & LRCF_TOP_BOTTOM_FADE))
        return;
    constexpr float CyLrcGradient = 50.f;
    const auto k = CyLrcGradient / GetViewHeight();
    const D2D1_GRADIENT_STOP Stop[]
    {
        {},
        { k,{.a = 1.f } },
        { 1.f - k,{.a = 1.f } },
        { 1.f },
    };
    ComPtr<ID2D1GradientStopCollection> pStopCollection;
    m_pDC->CreateGradientStopCollection(EckArgArray(Stop), &pStopCollection);
    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES Prop;
    Prop.startPoint = {};
    Prop.endPoint = { 0.f, GetViewHeight() };
    m_pDC->CreateLinearGradientBrush(Prop, pStopCollection.Get(), &m_pBrFade);
}

HRESULT CLyricRendererD2D::LrInitialize(Dui::CElement* pEle) noexcept
{
    const auto hr = __super::LrInitialize(pEle);
    if (FAILED(hr))
        return hr;
    pEle->GetDC()->QueryInterface(&m_pDC);
    return S_OK;
}

void CLyricRendererD2D::LrBeginDraw() noexcept
{
    // 可以优化为不使用图层，但是太麻烦了不做了
    if (GetFlags() & LRCF_TOP_BOTTOM_FADE)
    {
        D2D1_LAYER_PARAMETERS1 LyParam{ D2D1::LayerParameters1() };
        LyParam.contentBounds = { 0.f, 0.f, GetViewWidth(), GetViewHeight() };
        LyParam.opacityBrush = m_pBrFade.Get();
        m_pDC->PushLayer(LyParam, nullptr);
    }
}

void CLyricRendererD2D::LrEndDraw() noexcept
{
    if (GetFlags() & LRCF_TOP_BOTTOM_FADE)
        m_pDC->PopLayer();
}

void CLyricRendererD2D::LrSetItemCount(int cItems) noexcept
{
    m_vItem.clear();
    m_vItem.resize(cItems);
}

void CLyricRendererD2D::LrDrawItem(const LRD_DRAW& Opt) noexcept
{
    EckAssert(Opt.idx >= 0 && Opt.idx < (int)m_vItem.size());
    const auto& Item = m_vItem[Opt.idx];
    D2D1_MATRIX_3X2_F Mat0;
    m_pDC->GetTransform(&Mat0);

    const auto cxyLineMargin = GetHostElement()->GetTheme()->
        GetMetric(CVeLyric::IdMeItemMargin, CVeLyric::DefaultItemMargin);
    const auto fMaxScale = GetHostElement()->GetTheme()->
        GetMetric(CVeLyric::IdMeMaximumScale, CVeLyric::DefaultMaximumScale);

    D2D1_POINT_2F ptScale{};
    ptScale.y = Opt.cy / 2.f - cxyLineMargin;
    switch (Opt.eAlignH)
    {
    case eck::Alignment::Center:
        ptScale.x = GetViewWidth() / 2.f;
        break;
    case eck::Alignment::Far:
        ptScale.x = GetViewWidth();
        break;
    }

    D2D1_MATRIX_3X2_F Mat{ Mat0 };
    const auto cyExtra = (Opt.cy - cxyLineMargin * 2.f) *
        (fMaxScale - 1.f) / 2.f;
    Mat.dx += cxyLineMargin;
    Mat.dy += (Opt.y + cxyLineMargin + cyExtra);

    if (!Item.bCacheValid)
    {
        auto& Item = m_vItem[Opt.idx];
        Item.bCacheValid = TRUE;
        Item.cxMax = std::max(Opt.cxMain, Opt.cxTranslation);
        {
            DWRITE_TEXT_METRICS tm;
            if (Opt.pTlMain)
            {
                Opt.pTlMain->GetMetrics(&tm);
                Item.bMultiLine = (tm.lineCount > 1);
            }
            if (!Item.bMultiLine && Opt.pTlTranslation)
            {
                Opt.pTlTranslation->GetMetrics(&tm);
                Item.bMultiLine = (tm.lineCount > 1);
            }
        }

        float x[2]{};
        switch (Opt.eAlignH)
        {
        case eck::Alignment::Center:
            x[0] = (GetViewWidth() - Opt.cxMain) / 2.f;
            x[1] = (GetViewWidth() - Opt.cxTranslation) / 2.f;
            break;
        case eck::Alignment::Far:
            x[0] = GetViewWidth() - Opt.cxMain;
            x[1] = GetViewWidth() - Opt.cxTranslation;
            break;
        }

        float xDpi, yDpi;
        m_pDC->GetDpi(&xDpi, &yDpi);

        ComPtr<ID2D1PathGeometry1> pPathGeometry;
        IDWriteTextLayout* const pTl[]{ Opt.pTlMain.Get(), Opt.pTlTranslation.Get() };
        constexpr float cyPadding[]{ 5.f,0.f };
        eck::GetTextLayoutPathGeometry(
            EckArgArrayR(pTl), cyPadding,
            x, 0.f,
            pPathGeometry.AtSelf(),
            xDpi,
            TRUE);

        m_pDC->CreateFilledGeometryRealization(
            pPathGeometry.Get(),
            D2D1::ComputeFlatteningTolerance(
                D2D1::Matrix3x2F::Identity(), xDpi, yDpi, fMaxScale),
            Item.pGrMain.AtClear());
    }

    const auto pEle = eck::DbgDynamicCast<CVeLyric*>(GetHostElement());
    if (Opt.ss)
    {
        D2D1_RECT_F rc{ Opt.x, Opt.y, Opt.x + Opt.cx, Opt.y + Opt.cy };
        if ((Opt.uFlags & LRIF_AN_SEL_BKG) && (Opt.ss == CVeLyric::SsHot))
        {
            const auto dxy = -cxyLineMargin * Opt.kAnSelBkg;
            eck::InflateRect(rc, dxy, dxy);
            Dui::SimpleStyle ssNull{ pEle->TmSimpleStyle(Opt.ss) };
            ssNull.bBackArgb = ssNull.bBorderArgb = FALSE;
            ssNull.CrBack = ssNull.CrBorder = Dui::IdTmInvalid;

            const auto ss = Dui::TmSsLerp(
                pEle->GetTheme().Get(),
                pEle->TmSimpleStyle(Opt.ss),
                ssNull,
                1.f - Opt.kAnSelBkg);
            pEle->GetTheme()->Draw(
                pEle,
                &ss,
                Dui::IdPtNormal,
                rc,
                Opt.prcClip);
        }
        else
            pEle->GetTheme()->Draw(
                pEle,
                &pEle->TmSimpleStyle(Opt.ss),
                Dui::IdPtNormal,
                rc,
                Opt.prcClip);
    }

    if (Opt.uFlags & LRIF_PREV_AN)
    {
        const auto k = fMaxScale + 1.f - Opt.fScale;
        m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(k, k, ptScale) * Mat);
        const auto m = (Opt.fScale - 1.f) / (fMaxScale - 1.f);

        auto argb = Dui::TmSsLerpColor(
            pEle->GetTheme().Get(), FALSE,
            CVeLyric::IdCrText, CVeLyric::IdCrTextActive,
            1.f - m);
        if (!argb)
            argb = 0;
        pEle->GetWindow().CcSetBrushColor(eck::ArgbToD2DColorF(*argb));
    }
    else if (Opt.uFlags & LRIF_CURR_AN)
    {
        const auto k = Opt.fScale;
        m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(k, k, ptScale) * Mat);
        const auto m = (Opt.fScale - 1.f) / (fMaxScale - 1.f);
        auto argb = Dui::TmSsLerpColor(
            pEle->GetTheme().Get(), FALSE,
            CVeLyric::IdCrText, CVeLyric::IdCrTextActive,
            m);
        if (!argb)
            argb = 0;
        pEle->GetWindow().CcSetBrushColor(eck::ArgbToD2DColorF(*argb));
    }
    else
    {
        if (Opt.uFlags & LRIF_SCROLL_EXPAND)
        {
            m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(
                Opt.kScrollExpand, Opt.kScrollExpand, ptScale) * Mat);
        }
        else
            m_pDC->SetTransform(Mat);
        pEle->GetWindow().CcSetBrushColor(pEle->GetTheme()->GetColorD2D(
            (Opt.uFlags & LRIF_CURR_AN) ? CVeLyric::IdCrTextActive : CVeLyric::IdCrText));
    }
    m_pDC->DrawGeometryRealization(Item.pGrMain.Get(), pEle->GetWindow().CcGetBrush());
    m_pDC->SetTransform(Mat0);
}

void CLyricRendererD2D::LrSetViewSize(float cx, float cy) noexcept
{
    if (cx < GetViewWidth())
        for (auto& e : m_vItem)
        {
            if (e.bMultiLine || e.cxMax > cx)
                e.bCacheValid = FALSE;
        }
    else
        for (auto& e : m_vItem)
        {
            if (e.bMultiLine)
                e.bCacheValid = FALSE;
        }
    __super::LrSetViewSize(cx, cy);
}

void CLyricRendererD2D::LrDpiChanged(float fNewDpi) noexcept
{
    LrInvalidate();
}

void CLyricRendererD2D::LrInvalidate() noexcept
{
    for (auto& e : m_vItem)
        e.bCacheValid = FALSE;
}