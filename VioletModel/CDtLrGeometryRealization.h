#pragma once
#include "CDtLyricRendererBase.h"

class CDtLrGeometryRealization : public CDtLyricRendererBase
{
private:
    struct LINE
    {
        ComPtr<IDWriteTextLayout> pLayout;
        ComPtr<IDWriteTextLayout> pLayoutTrans;
        ComPtr<ID2D1GeometryRealization> pGrS;
        ComPtr<ID2D1GeometryRealization> pGrSTrans;
        ComPtr<ID2D1GeometryRealization> pGrF;
        ComPtr<ID2D1GeometryRealization> pGrFTrans;
        D2D1_SIZE_F size;
        D2D1_SIZE_F sizeTrans;
        float fTime;
        float fDuration;
        BOOLEAN bTooLong;
        BOOLEAN bTooLongTrans;
        BOOLEAN bScrolling;
    };
    struct STATIC_LINE
    {
        eck::CStringW rsText;
        ID2D1GeometryRealization* pGrF;
        ID2D1GeometryRealization* pGrS;
        float cx;
        BOOL bValid;
    };

    LINE m_Line[2]{};
    STATIC_LINE m_StaticLine{};

    ComPtr<ID2D1DeviceContext1> m_pDC{};
    ComPtr<ID2D1SolidColorBrush> m_pBrush{};

    ComPtr<ID2D1GeometryRealization> m_pGrEmptyText{};
    float m_cxEmptyText{};
    float m_cyEmptyText{};

    float m_cyFirstLine{};

    void DrawTextGeometry(ID2D1GeometryRealization* pGrS, ID2D1GeometryRealization* pGrF,
        float dx, float dy, size_t idxFill);
public:
    HRESULT LrInit(const LRD_INIT& Opt) override;
    void LrBeginDraw() override {}
    void LrEndDraw() override {}
    void LrDraw(const LDRD_DRAW& Opt) override;
    HRESULT LrUpdateText(int idx, const Lyric::Line& Line,
        _Out_ LRD_TEXT_METRICS& Met) override;
    void LrSetViewSize(float cx, float cy) override;
    void LrDpiChanged(float fNewDpi) override;
    void LrInvalidate(BOOL bSecondLine) override;
};