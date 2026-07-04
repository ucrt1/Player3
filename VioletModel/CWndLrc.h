#pragma once
#include "CVioletTheme.h"
#include "CApp.h"
#include "CVeDtLrc.h"


class CWndLrc : public Dui::CDuiWindow, public eck::ITimeLine
{
private:
	Dui::CButton m_BTPrev{}, m_BTPlay{}, m_BTNext{}, m_BTLock{}, m_BTClose{};
	eck::CLinearLayoutH m_Layout{};
	CVeDtLrc m_Lrc{};
	CVioletTheme* m_pVioletTheme{};
	BOOLEAN m_bInitShow{ TRUE };

	BOOLEAN m_bLock{};
	BOOLEAN m_bShowBk{ TRUE };
	BOOLEAN m_bAnFade{};
	eck::EasingCurve<eck::Easing::FOutCubic> m_AnFade{};

	void OnPlayEvent(const PLAY_EVT_PARAM& e);
public:
	LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

	LRESULT OnElementNotify(Dui::CElement* pEle, Dui::ELENMHDR* pnm) noexcept override;

	LRESULT OnRenderEvent(UINT uMsg, Dui::RENDER_EVENT& e) noexcept override;

	void TlTick(int iMs) noexcept override;
	BOOL TlIsValid() noexcept override { return m_bAnFade; }
	int TlGetCurrentInterval() noexcept override { return 0; }
};