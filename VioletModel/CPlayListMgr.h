#pragma once
#include "CPlayList.h"

class CPlayListMgr
{
private:
	struct ITEM
	{
		std::shared_ptr<CPlayList> pList;
		RefPtr<eck::CD2DImageList> pImageList;	// 供UI使用
	};
	std::vector<ITEM> m_vPlayList{};
public:
	void LoadList();

	EckInlineNdCe int GetCount() const { return (int)m_vPlayList.size(); }

	EckInlineNdCe auto& AtList(int idx) { return m_vPlayList[idx].pList; }
	EckInlineNdCe auto& At(int idx) { return m_vPlayList[idx]; }

	std::shared_ptr<CPlayList> Add();

	void InvalidateImageList();
};