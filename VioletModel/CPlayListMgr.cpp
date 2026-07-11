#include "pch.h"
#include "CPlayListMgr.h"

void CPlayListManager::LoadList(std::wstring_view svPath, BOOL bClear = TRUE) noexcept
{
    auto FnCallback = [&](eck::CFileEnumerator::TDefault& e) noexcept
        {
            const auto pList = Add();
            pList->LtmSetFile(
                svPath,
                std::wstring_view{ e.FileName, e.FileNameLength / sizeof(WCHAR) });
            pList->LtmMarkLazyInitialize();
        };
    eck::CFileEnumerator ef{};
    ef.Open(svPath.data());
    ef.Enumerate(FnCallback, L"*.VltList"sv);
    ef.Enumerate(FnCallback, L"*.PNList"sv);
    ef.Enumerate(FnCallback, L"*.QKList"sv);
}

RefPtr<CPlayList> CPlayListManager::Add() noexcept
{
    return m_vPlayList.emplace_back(CPlayList::New()).pList;
}