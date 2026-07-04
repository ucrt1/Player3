#include "pch.h"
#include "CLrGeometryRealization.h"

static constexpr D2D1_COLOR_F InterpolateColor(
    const D2D1_COLOR_F& c1, const D2D1_COLOR_F& c2, float k)
{
    return {
        c1.r + (c2.r - c1.r) * k,
        c1.g + (c2.g - c1.g) * k,
        c1.b + (c2.b - c1.b) * k,
        c1.a + (c2.a - c1.a) * k };
}

void CLrGeometryRealization::ReCreateFadeBrush()
{
    m_pBrFade.Clear();
    if (!m_bTopBtmFade)
        return;
    constexpr float CyLrcGradient = 50.f;
    const auto k = CyLrcGradient / m_cyView;
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
    Prop.endPoint = { 0.f,m_cyView };
    m_pDC->CreateLinearGradientBrush(Prop, pStopCollection.Get(), &m_pBrFade);
}

HRESULT CLrGeometryRealization::LrInit(const LRD_INIT& Opt)
{
    if (SUCCEEDED(Opt.pD2DContext->QueryInterface(m_pDC.AddrOfClear())))
    {
        m_pDC->CreateSolidColorBrush({}, m_pBrush.AddrOfClear());
        return S_OK;
    }
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}

void CLrGeometryRealization::LrBeginDraw()
{
    // 可以优化为不使用图层，但是太麻烦了不做了
    if (m_bTopBtmFade)
    {
        D2D1_LAYER_PARAMETERS1 LyParam{ D2D1::LayerParameters1() };
        LyParam.contentBounds = { 0.f,0.f,m_cxView,m_cyView };
        LyParam.opacityBrush = m_pBrFade.Get();
        m_pDC->PushLayer(LyParam, nullptr);
    }
}

void CLrGeometryRealization::LrEndDraw()
{
    if (m_bTopBtmFade)
        m_pDC->PopLayer();
}

void CLrGeometryRealization::LrItmSetCount(int cItems)
{
    m_vItem.clear();
    m_vItem.resize(cItems);
}

HRESULT CLrGeometryRealization::LrItmUpdateText(int idx,
    const Lyric::Line& Line, _Out_ LRD_TEXT_METRICS& Met)
{
    EckAssert(idx >= 0 && idx < (int)m_vItem.size());
    DWRITE_TEXT_METRICS Metrics{};
    auto& e = m_vItem[idx];
    e.bLayoutValid = TRUE;
    e.bGrValid = FALSE;
    const float cxMax = (m_cxView - m_cxyLineMargin * 2.f) / m_fMaxScale;

    eck::g_pDwFactory->CreateTextLayout(Line.pszLrc, Line.cchLrc,
        m_pTfMain.Get(), cxMax, m_cyView, e.pLayoutMain.AddrOfClear());
    e.pLayoutMain->GetMetrics(&Metrics);
    Met.cxMain = e.cxMain = Metrics.width;
    Met.cyMain = Metrics.height;

    if (Line.pszTranslation && Line.cchTranslation)
    {
        eck::g_pDwFactory->CreateTextLayout(Line.pszTranslation,
            Line.cchTranslation, m_pTfTrans.Get(),
            cxMax, m_cyView, e.pLayoutTrans.AddrOfClear());
        e.pLayoutTrans->GetMetrics(&Metrics);
        Met.cxTrans = e.cxTrans = Metrics.width;
        Met.cyTrans = Metrics.height;
    }
    else
    {
        e.pLayoutTrans.Clear();
        Met.cxTrans = e.cxTrans = 0.f;
    }
    return S_OK;
}

void CLrGeometryRealization::LrItmDraw(const LRD_DRAW& Opt)
{
    EckAssert(Opt.idx >= 0 && Opt.idx < (int)m_vItem.size());
    const auto& Item = m_vItem[Opt.idx];
    D2D1_MATRIX_3X2_F Mat0;
    m_pDC->GetTransform(&Mat0);

    D2D1_POINT_2F ptScale{};
    ptScale.y = Opt.cy / 2.f - m_cxyLineMargin;
    switch (Opt.eAlignH)
    {
    case eck::Alignment::Center:
        ptScale.x = m_cxView / 2.f;
        break;
    case eck::Alignment::Far:
        ptScale.x = m_cxView;
        break;
    }

    D2D1_MATRIX_3X2_F Mat{ Mat0 };
    const auto cyExtra = (Opt.cy - m_cxyLineMargin * 2.f) *
        (m_fMaxScale - 1.f) / 2.f;
    Mat.dx += m_cxyLineMargin;
    Mat.dy += (Opt.y + m_cxyLineMargin + cyExtra);

    if (!Item.bGrValid)
    {
        auto& Item = m_vItem[Opt.idx];
        constexpr float cyPadding[]{ 5.f,0.f };

        float x[2]{};
        switch (Opt.eAlignH)
        {
        case eck::Alignment::Center:
            x[0] = (m_cxView - m_cxView / Opt.fScale) / 2.f;
            x[1] = (m_cxView - Item.cxTrans) / 2.f;
            break;
        case eck::Alignment::Far:
            x[0] = (m_cxView - m_cxView / Opt.fScale);
            x[1] = (m_cxView - Item.cxTrans);
            break;
        }

        float xDpi, yDpi;
        m_pDC->GetDpi(&xDpi, &yDpi);
        ComPtr<ID2D1PathGeometry1> pPathGeometry;
        eck::GetTextLayoutPathGeometry(2, &Item.pLayoutMain, cyPadding, x,
            0.f, pPathGeometry.RefOf(), xDpi, TRUE);
        m_pDC->CreateFilledGeometryRealization(
            pPathGeometry.Get(),
            D2D1::ComputeFlatteningTolerance(
                D2D1::Matrix3x2F::Identity(), xDpi, yDpi, m_fMaxScale),
            Item.pGrMain.AddrOfClear());
        Item.bGrValid = TRUE;
    }

    if (Opt.eState != Dui::State::None)
    {
        D2D1_RECT_F rc{ Opt.x,Opt.y,Opt.x + Opt.cx,Opt.y + Opt.cy };
        Dui::DTB_OPT ThemeOpt;
        if ((Opt.uFlags & LRIF_AN_SEL_BKG) && Opt.eState == Dui::State::Hot)
        {
            const auto dxy = -m_cxyLineMargin * Opt.kAnSelBkg;
            eck::InflateRect(rc, dxy, dxy);
            ThemeOpt.uFlags = Dui::DTBO_NEW_OPACITY;
            ThemeOpt.fOpacity = 1.f - Opt.kAnSelBkg;
        }
        else
            ThemeOpt.uFlags = Dui::DTBO_NONE;
        m_pTheme->DrawBackground(Dui::Part::ListItem, Opt.eState, rc, &ThemeOpt);
    }

    if (Opt.uFlags & LRIF_PREV_AN)
    {
        const auto k = m_fMaxScale + 1.f - Opt.fScale;
        m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(k, k, ptScale) * Mat);
        const auto m = (Opt.fScale - 1.f) / (m_fMaxScale - 1.f);
        m_pBrush->SetColor(InterpolateColor(
            m_Color[CriNormal], m_Color[CriHiLight], 1.f - m));
    }
    else if (Opt.uFlags & LRIF_CURR_AN)
    {
        const auto k = Opt.fScale;
        m_pDC->SetTransform(D2D1::Matrix3x2F::Scale(k, k, ptScale) * Mat);
        const auto m = (Opt.fScale - 1.f) / (m_fMaxScale - 1.f);
        m_pBrush->SetColor(InterpolateColor(
            m_Color[CriNormal], m_Color[CriHiLight], m));
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
        m_pBrush->SetColor(m_Color[(Opt.uFlags & LRIF_CURR_AN) ? CriHiLight : CriNormal]);
    }
    m_pDC->DrawGeometryRealization(Item.pGrMain.Get(), m_pBrush.Get());
    m_pDC->SetTransform(Mat0);
}

HRESULT CLrGeometryRealization::LrUpdateEmptyText(const LRD_EMTRY_TEXT& Opt)
{
    m_pGrEmptyText.Clear();
    m_cxEmptyText = m_cyEmptyText = 0.f;

    HRESULT hr;
    ComPtr<IDWriteTextLayout> pLayout;

    hr = eck::g_pDwFactory->CreateTextLayout(Opt.svText.data(),
        (UINT32)Opt.svText.size(), m_pTfMain.Get(),
        m_cxView, m_cyView, &pLayout);
    if (FAILED(hr))
        return hr;
    DWRITE_TEXT_METRICS tm{};
    pLayout->GetMetrics(&tm);
    m_cxEmptyText = tm.width;
    m_cyEmptyText = tm.height;
    pLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    pLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

    float xDpi, yDpi;
    m_pDC->GetDpi(&xDpi, &yDpi);

    ComPtr<ID2D1PathGeometry1> pPathGeometry;
    eck::GetTextLayoutPathGeometry(pLayout.Get(),
        0.f, 0.f, pPathGeometry.RefOf(), xDpi);

    return m_pDC->CreateFilledGeometryRealization(
        pPathGeometry.Get(),
        D2D1::ComputeFlatteningTolerance(
            D2D1::Matrix3x2F::Identity(), xDpi, yDpi),
        &m_pGrEmptyText);
}

void CLrGeometryRealization::LrSetViewSize(float cx, float cy)
{
    m_cyView = cy;
    if (eck::FloatEqual(m_cxView, cx))
        return;
    if (cx < m_cxView)
        for (auto& e : m_vItem)
        {
            if (e.bMultiLine || std::max(e.cxMain, e.cxTrans) > cx)
            {
                e.bGrValid = FALSE;
                e.bLayoutValid = FALSE;
            }
        }
    else
        for (auto& e : m_vItem)
        {
            if (e.bMultiLine)
            {
                e.bGrValid = FALSE;
                e.bLayoutValid = FALSE;
            }
        }
    m_cxView = cx;
}

void CLrGeometryRealization::LrDpiChanged(float fNewDpi)
{
    LrInvalidate();
}

void CLrGeometryRealization::LrInvalidate()
{
    for (auto& e : m_vItem)
        e.bGrValid = FALSE;
}