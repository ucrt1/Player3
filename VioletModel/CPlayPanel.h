#pragma once
#include "CVeMiniCover.h"

class CPlayPanel : public CVeBase
{
private:
    CVeMiniCover m_Cover{};
    Dui::CLabel m_LATitle{};
    Dui::CLabel m_LAArtist{};
    Dui::CLabel m_LATime{};

    void OnPlayEvent(const PLAY_EVT_PARAM& e) noexcept;
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

    EckInlineNdCe auto& GetCoverElement() noexcept { return m_Cover; }
};