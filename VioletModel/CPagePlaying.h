#pragma once
#include "CVeCover.h"
#include "CVeLrc.h"
#include "CApp.h"

// CWndMain负责更新该元素的图片
class CPagePlaying : public Dui::CElement
{
	friend class CWindowMain;
private:
	CVeCover m_Cover{};
	CVeLyric m_Lrc{};
	Dui::CButton m_BTBack{};
	Dui::CLabel m_LATitle{};
	Dui::CLabel m_LAAlbum{};
	Dui::CLabel m_LAArtist{};

	ID2D1Bitmap1* m_pBmpCover{};
	ID2D1Bitmap1* m_pBmpBlurredCover{};
	ID2D1SolidColorBrush* m_pBrBkg{};

	void UpdateBlurredCover();

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
};