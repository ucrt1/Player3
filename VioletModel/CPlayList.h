#pragma once
// 所有索引如无特别说明，均为真实平面列表索引（而不是搜索列表等的索引）
class CPlayList final
{
private:
    struct Item
    {
        eck::CStringW rsName{};     // 名称
        eck::CStringW rsFile{};     // 文件路径

        eck::CStringW rsTitle{};    // 标题
        eck::CStringW rsArtist{};   // 艺术家
        eck::CStringW rsAlbum{};    // 唱片集
        eck::CStringW rsGenre{};    // 流派

        PLDATA s{};
        int idxSortMapping{ -1 };   // 【排序时用】映射到的项
    };

    eck::CStringW m_rsListFile{};	// 对应列表文件路径
    eck::CStringW m_rsName{};		// 列表名称

    std::vector<Item> m_vFlat{};

    eck::CTrivialBuffer<int> m_vSearchResult{};// 搜索结果，存储平面列表索引
    eck::CTrivialBuffer<int> m_vRandomMapping{};

    int m_idxCurrFlat{ -1 };
    int m_idxCurrRandom{ -1 };// m_vRandomMapping中的索引

    BOOLEAN m_bSort{};
    BOOLEAN m_bLazyInit{};
    PlType m_eType{};

public:
    static RefPtr<CPlayList> New() noexcept { return RefPtr<CPlayList>::Make(); }

    // 设置存盘文件路径和名称
    void LtmSetFile(std::wstring_view svPath, std::wstring_view svFileName) noexcept;
    // 设置列表名称，这与文件无关
    void LtmSetName(std::wstring_view svName) noexcept { m_rsName = svName; }
    EckInlineNdCe auto& LtmGetName() const noexcept { return m_rsName; }

    void LtmReserve(int cItem) noexcept { m_vFlat.reserve(cItem); }
    void LtmReserveIncrement(int cItem) noexcept { m_vFlat.reserve(m_vFlat.size() + cItem); }

    // 通常由列表管理器使用
    void LtmMarkLazyInitialize() noexcept { m_bLazyInit = TRUE; }
    void LtmEnsureLoaded() noexcept;

    // 从文件初始化列表内容，此函数不修改当前列表文件路径和名称
    HRESULT LtmInitializeFromListFile(PCWSTR pszFile) noexcept;

    EckInlineNdCe Item& FlAtAbsolutely(int idx) noexcept { return m_vFlat[idx]; }
    EckInlineNdCe Item& FlAt(int idx) noexcept
    {
        return FlIsSearching() ? FlAtAbsolutely(FlAtSearch(idx)) : FlAtAbsolutely(idx);
    }

    int FlInsert(const eck::CStringW& rsFile, int idx = -1) noexcept;
    int FlInsertEmpty(int idx = -1) noexcept;

    EckInlineNdCe int FlGetCount() const noexcept { return (int)m_vFlat.size(); }

    void FlDoSearch(std::wstring_view svKeyWord) noexcept;
    EckInlineNdCe int FlGetSearchResultCount() const noexcept
    {
        return (int)m_vSearchResult.Size();
    }
    EckInlineNdCe BOOL FlIsSearching() const noexcept { return !m_vSearchResult.IsEmpty(); }
    EckInlineNdCe int FlAtSearch(int idx) noexcept { return m_vSearchResult[idx]; }
    EckInlineCe void FlExitSearch() noexcept { m_vSearchResult.Clear(); }
    EckInlineNdCe int FlSearchIndexToRealIndex(int idx) noexcept
    {
        return FlIsSearching() ? FlAtSearch(idx) : idx;
    }

    void FlShuffleRandom() noexcept;
    EckInlineNdCe int FlRandomIndexToRealIndex(int idxRandom) const noexcept
    {
        return m_vRandomMapping[idxRandom];
    }

    // 当前列表是否被全局播放器选入
    BOOL PlyIsSelected() noexcept;

    EckInlineCe void PlySetCurrentItem(int idx) noexcept { m_idxCurrFlat = idx; }
    EckInlineNdCe int PlyGetCurrentItem() const noexcept { return m_idxCurrFlat; }

    // 播放器调用以下函数实现当前随机项目的更新
    // - 如果新项目随机索引可以通过简单计算得到，则播放器执行此计算，
    //   然后将结果传递到PlySetCurrentRandomItem（如上一曲/下一曲时）
    // - 如果无法得到项目的随机索引，则播放器将新的平面索引传递到
    //   PlySetCurrentRandomItemFromFlat，列表遍历随机映射，然后更新当前随机索引
    //   （如双击列表项目时）

    void PlySetCurrentRandomItemFromFlat(int idxFlat) noexcept;
    EckInlineCe void PlySetCurrentRandomItem(int idxRandom) noexcept { m_idxCurrRandom = idxRandom; }
    EckInlineNdCe int PlyGetCurrentRandomItem() const noexcept { return m_idxCurrRandom; }
};