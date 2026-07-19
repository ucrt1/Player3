#pragma once
#include "CVeBase.h"

class CVeMiniCover : public CVeBase, public eck::ITimeLine
{
private:
    Dui::CBitmap m_BitmapCover{};
    Dui::CBitmap m_BitmapArrowUp{};
    eck::EasingCurve<eck::Easing::FOutCubic> m_ec{};

    int m_msLastInterval{};

    BOOLEAN m_bHover{};
    BOOLEAN m_bLBtnDown{};
    BOOLEAN m_bAnActive{};

    void OnColorSchemeChanged(BOOL bForceUpdateCover) noexcept;
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

    void SetCoverBitmap(const Dui::CBitmap& Bitmap) noexcept { m_BitmapCover = Bitmap; }

    void TlTick(int ms) noexcept override;
    BOOL TlIsValid() noexcept override { return m_bAnActive; }
    int TlGetCurrentInterval() noexcept override { return m_msLastInterval; }
};