#pragma once
#include "CVeCover.h"
#include "CVeLrc.h"
#include "CVeBase.h"

class CPagePlaying : public CVeBase
{
private:
    CVeCover m_Cover{};
    CVeLyric m_Lyric{};
    Dui::CButton m_BTBack{};
    Dui::CLabel m_LATitle{};
    Dui::CLabel m_LAAlbum{};
    Dui::CLabel m_LAArtist{};

    ComPtr<ID2D1Bitmap1> m_pBitmapBlurredCover{};

    void OnPlayEvent(const PLAY_EVT_PARAM& e);

    void SetEmptyText();

    void OnColorSchemeChanged();
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

    void SetLabelTextFormatTitle(IDWriteTextFormat* pTf)
    {
        m_LATitle.SetTextFormat(pTf);
    }

    void SetLabelTextFormat(IDWriteTextFormat* pTf)
    {
        m_LAAlbum.SetTextFormat(pTf);
        m_LAArtist.SetTextFormat(pTf);
    }

    void UpdateBlurredCover();
};