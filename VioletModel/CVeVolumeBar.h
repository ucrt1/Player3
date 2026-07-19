#pragma once
#include "CVeBase.h"

class CVeVolumeBar : public CVeBase, public eck::ITimeLine
{
private:
    Dui::CLabel m_LAVol{};
    Dui::CTrackBar m_TrackBar{};

    Dui::CCompositor2DAffineTransform m_PageAn{};
    eck::EasingCurve<eck::Easing::FOutCubic> m_ecShowing{};

    int m_msLastInterval{};
    BOOLEAN m_bShow{};
    BOOLEAN m_bAnimating{};
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

    void ShowAnimation();

    void OnVolumeChanged(float fVol);

    void TlTick(int ms) noexcept override;
    BOOL TlIsValid() noexcept override { return m_bAnimating; }
    int TlGetCurrentInterval() noexcept override { return m_msLastInterval; }
};