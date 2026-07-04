#pragma once
class CPageMain : public CPage
{
private:
	Dui::CButton m_BTOpenFile{};
	Dui::CButton m_BTOpenFolder{};
	eck::CLayoutDummy m_Dummy{};
	Dui::CLabel m_LATest{};

	eck::CLinearLayoutH m_Lyt{};
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;
};