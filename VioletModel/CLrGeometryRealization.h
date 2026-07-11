#pragma once
#include "CLyricRendererBase.h"

class CLyricRendererD2D final : public CLyricRendererBase
{
private:
    struct ITEM
    {
        ComPtr<ID2D1GeometryRealization> pGrMain{};
        float cxMax{};
        BOOLEAN bMultiLine{};
        BOOLEAN bCacheValid{};
    };

    ComPtr<ID2D1DeviceContext1> m_pDC{};
    ComPtr<ID2D1LinearGradientBrush> m_pBrFade{};

    ComPtr<ID2D1GeometryRealization> m_pGrEmptyText{};
    float m_cxEmptyText{};
    float m_cyEmptyText{};

    std::vector<ITEM> m_vItem{};

    void ReCreateFadeBrush();
public:
    HRESULT LrInitialize(Dui::CElement* pEle) noexcept override;
    void LrBeginDraw() noexcept override;
    void LrEndDraw() noexcept override;
    void LrSetItemCount(int cItems) noexcept override;
    void LrDrawItem(const LRD_DRAW& Opt) noexcept override;
    void LrSetViewSize(float cx, float cy) noexcept override;
    void LrDpiChanged(float fNewDpi) noexcept override;
    void LrInvalidate() noexcept override;
};