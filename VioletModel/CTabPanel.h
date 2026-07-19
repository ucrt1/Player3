#pragma once
#include "CVeBase.h"

class CTabPanel : public CVeBase
{
private:
	ID2D1SolidColorBrush* m_pBrush{};
	Dui::CLabel m_LAIcon{};
	Dui::CListView m_TAB{};

	void OnColorSchemeChanged();
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

	EckInlineNdCe auto& GetTabList() { return m_TAB; }
};