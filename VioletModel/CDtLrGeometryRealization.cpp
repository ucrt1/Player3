#include "pch.h"
#include "CDtLrGeometryRealization.h"

void CDtLrGeometryRealization::DrawTextGeometry(ID2D1GeometryRealization* pGrS,
    ID2D1GeometryRealization* pGrF, float dx, float dy, size_t idxFill)
{
    D2D1_MATRIX_3X2_F Mat0;
    GetDC()->GetTransform(&Mat0);
    auto Mat{ Mat0 };
    Mat.dx += dx;
    Mat.dy += dy;
    if (pGrF && m_bShadow)
    {
        Mat.dx += m_dxyShadow;
        Mat.dy += m_dxyShadow;
        GetDC()->SetTransform(Mat);
        m_pBrush->SetColor(m_Color[CriShadow]);
        GetDC()->DrawGeometryRealization(pGrF, m_pBrush.Get());
        Mat.dx -= m_dxyShadow;
        Mat.dy -= m_dxyShadow;
    }
    GetDC()->SetTransform(Mat);
    if (pGrS)
    {
        m_pBrush->SetColor(m_Color[CriBorder]);
        GetDC()->DrawGeometryRealization(pGrS, m_pBrush.Get());
    }
    if (pGrF)
    {
        m_pBrush->SetColor(m_Color[idxFill]);
        GetDC()->DrawGeometryRealization(pGrF, m_pBrush.Get());
    }
    GetDC()->SetTransform(Mat0);
}

HRESULT CDtLrGeometryRealization::LrInitialize(const LRD_INIT& Opt)
{
    if (SUCCEEDED(Opt.pD2DContext->QueryInterface(GetDC().AtClear())))
    {
        GetDC()->CreateSolidColorBrush({}, m_pBrush.AtClear());
        return S_OK;
    }
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}

void CDtLrGeometryRealization::LrDraw(const LDRD_DRAW& Opt)
{
    auto& e = m_Line[Opt.idxLine];

    const float cxMaxHalf = m_cxView / 2.f;
    float dx;
    if (e.bTooLong)
    {
        dx = (Opt.fCurrTime - e.fTime) * e.size.width / e.fDuration;
        if (dx < cxMaxHalf)
            dx = 0.f;
        else if (dx > e.size.width - cxMaxHalf)
            dx = m_cxView - e.size.width;
        else
            dx = cxMaxHalf - dx;
    }
    else
        switch (m_eAlign[Opt.idxLine])
        {
        case eck::Alignment::Near:	dx = 0.f; break;
        case eck::Alignment::Center:dx = (m_cxView - e.size.width) / 2.f; break;
        case eck::Alignment::Far:	dx = m_cxView - e.size.width; break;
        default: ECK_UNREACHABLE;
        }
    const auto y = Opt.idxLine ? m_cyFirstLine + m_cyLinePadding : 0.f;
    DrawTextGeometry(e.pGrS.Get(), e.pGrF.Get(), y,
        Opt.idxLine ? m_cyFirstLine + m_cyLinePadding : 0.f,
        Opt.bHiLight ? CriHiLight : CriNormal);

    float cy = e.size.height;
    if (e.pTlTranslation.Get())
    {
        const float yNew = y + cy;
        if (e.bTooLongTrans)
        {
            dx = (Opt.fCurrTime - e.fTime) * e.sizeTrans.width / e.fDuration;
            if (dx < cxMaxHalf)
                dx = 0;
            else if (dx > e.sizeTrans.width - cxMaxHalf)
                dx = m_cxView - e.sizeTrans.width;
            else
                dx = cxMaxHalf - dx;
        }
        else
            switch (m_eAlign[Opt.idxLine])
            {
            case eck::Alignment::Near:	dx = 0.f; break;
            case eck::Alignment::Center:dx = (m_cxView - e.sizeTrans.width) / 2.f; break;
            case eck::Alignment::Far:	dx = m_cxView - e.sizeTrans.width; break;
            default: ECK_UNREACHABLE;
            }
        DrawTextGeometry(e.pGrSTrans.Get(), e.pGrFTrans.Get(), dx, yNew,
            Opt.bHiLight ? CriHiLight : CriNormal);
        cy += e.sizeTrans.height;
    }
}

HRESULT CDtLrGeometryRealization::LrUpdateText(int idx, const Lyric::Line& Line,
    _Out_ LRD_TEXT_METRICS& Met)
{
    auto& e = m_Line[idx];
    const auto bOldTooLong = m_bTooLong;
    float xDpi, yDpi;
    GetDC()->GetDpi(&xDpi, &yDpi);
    const float fTolerance = D2D1::ComputeFlatteningTolerance(
        D2D1::Matrix3x2F::Identity(), xDpi, yDpi, 1.f);

    if (Line.cchLrc)
        eck::g_pDwFactory->CreateTextLayout(Line.pszLrc, Line.cchLrc,
            m_pTfMain.Get(), m_cxView, m_cyView, e.pTlMain.AtClear());
    else
        eck::g_pDwFactory->CreateTextLayout(EckArgString(L"♪♪♪"),
            m_pTfMain.Get(), m_cxView, m_cyView, e.pTlMain.AtClear());
    DWRITE_TEXT_METRICS tm;
    e.pTlMain->GetMetrics(&tm);
    e.bTooLong = (tm.width > m_cxView);
    e.size = { tm.width,tm.height };
    Met.cxMain = e.size.width;
    Met.cyMain = e.size.height;

    ComPtr<ID2D1PathGeometry1> pPath;
    eck::GetTextLayoutPathGeometry(e.pTlMain.Get(),
        0, 0, pPath.RefOfClear(), xDpi);

    GetDC()->CreateFilledGeometryRealization(pPath.Get(), fTolerance, e.pGrF.AtClear());
    GetDC()->CreateStrokedGeometryRealization(pPath.Get(), fTolerance,
        m_cxOutline, nullptr, e.pGrS.AtClear());
    e.pTlTranslation.Clear();
    e.pGrFTrans.Clear();
    e.pGrSTrans.Clear();
    if (Line.cchTranslation)
    {
        eck::g_pDwFactory->CreateTextLayout(Line.pszTranslation, Line.cchTranslation,
            m_pTfTranslation.Get(), m_cxView, m_cyView, &e.pTlTranslation);
        e.pTlTranslation->GetMetrics(&tm);
        e.bTooLongTrans = (tm.width > m_cxView);
        e.sizeTrans = { tm.width,tm.height };
        Met.cxTranslation = e.sizeTrans.width;
        Met.cyTranslation = e.sizeTrans.height;

        eck::GetTextLayoutPathGeometry(e.pTlTranslation.Get(),
            0, 0, pPath.RefOfClear(), xDpi);

        GetDC()->CreateFilledGeometryRealization(pPath.Get(), fTolerance, e.pGrFTrans.AtClear());
        GetDC()->CreateStrokedGeometryRealization(pPath.Get(), fTolerance,
            m_cxOutline, nullptr, e.pGrSTrans.AtClear());
    }
    else
        Met.cxTranslation = Met.cyTranslation = 0.f;
    if (idx == 0)
    {
        m_cyFirstLine = e.size.height;
        if (e.pTlTranslation.Get())
            m_cyFirstLine += (e.sizeTrans.height + m_cyLinePadding);
    }
    m_bTooLong = !!e.bTooLong || !!e.bTooLongTrans;
    return S_OK;
}

void CDtLrGeometryRealization::LrSetViewSize(float cx, float cy)
{
    m_cxView = cx;
    m_cyView = cy;
}

void CDtLrGeometryRealization::LrDpiChanged(float fNewDpi)
{
}

void CDtLrGeometryRealization::LrInvalidate(BOOL bSecondLine)
{
}
