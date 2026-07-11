#pragma once
#include "CPlayList.h"

class CPlayListManager
{
private:
    struct ITEM
    {
        RefPtr<CPlayList> pList;
    };
    std::vector<ITEM> m_vPlayList{};
public:
    void LoadList(std::wstring_view svPath, BOOL bClear = TRUE) noexcept;

    EckInlineNdCe int GetCount() const noexcept { return (int)m_vPlayList.size(); }

    EckInlineNdCe auto& AtList(int idx) const noexcept { return m_vPlayList[idx].pList; }
    EckInlineNdCe auto& At(int idx) const noexcept { return m_vPlayList[idx]; }

    RefPtr<CPlayList> Add() noexcept;
};