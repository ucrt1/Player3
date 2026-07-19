#pragma once
class CPageList : public CPage, public Dui::CListView::IAdapter
{
private:
    using LvIndex = eck::UiBasic::Lc::Index;
    using LvProperty = eck::UiBasic::Lc::Property;
    using LvState = eck::UiBasic::Lc::TState;

    constexpr static int DefaultCoverIndex{};

    enum class Column
    {
        Title,
        Artist,
        Album,
        Duration,

        Maximum,
    };

    struct LIST_INFO
    {
        RefPtr<eck::CD2DImageList> pImageList;
    };
    struct ITEM_INFO
    {
        int idxImage{};// -1 = 需要更新
        LvState uState{};
        ComPtr<IDWriteTextLayout> pTextLayout[(size_t)Column::Maximum]{};
    };

    struct TSKPARAM_LOAD_META_DATA
    {
        RefPtr<CPlayList> pList;
        RefPtr<eck::CD2DImageList> pImageList;
        eck::CTrivialBuffer<int> vItem;
    };;

    Dui::CEdit m_EDSearch{};
    Dui::CListView m_TBLPlayList{};
    eck::CLinearLayoutV m_LytPlayList{};

    Dui::CButton m_BTAddFile{};
    Dui::CButton m_BTLocate{};
    eck::CLayoutDummy m_TopBarDummySpace{};
    Dui::CEdit m_EDSearchItem{};
    eck::CLinearLayoutH m_LytTopBar{};
    Dui::CListView m_GLList{};
    eck::CLinearLayoutV m_LytList{};

    eck::CLinearLayoutH m_Lyt{};

    int m_cxIl{}, m_cyIl{};

    std::vector<LIST_INFO> m_vListInfo{};
    std::vector<ITEM_INFO> m_vItem{};

    BOOL m_bSearchItemEditEmpty{};


    LvIndex LcaGetCount() const noexcept override;
    int LcaGetColumnCount() const noexcept override;
    void LcaGet(const LvIndex& idx, int idxCol,
        LvProperty eProp, std::any& Data) const noexcept override;
    void LcaSet(const LvIndex& idx, int idxCol,
        LvProperty eProp, std::any& Data, BOOL bMove = FALSE) noexcept override;
    void LcaColumnWidthChanged(int idxCol, float cxNew) noexcept override;

    eck::CoroTask<void> PlLoadMetadata(TSKPARAM_LOAD_META_DATA&& Param) noexcept;
    void PlBeginLoadMetadata(int idxList = -1) noexcept;

    const RefPtr<CPlayList>& PlCurrent() const noexcept;

    // 使用搜索编辑框内容搜索列表
    // 返回项目数
    int PlSearchEditContent(CPlayList* pList) noexcept;

    // 上传默认封面到图像列表的第0个磁贴
    HRESULT IlUploadDefaultCover(eck::CD2DImageList* pImageList) noexcept;
    RefPtr<eck::CD2DImageList> IlCreate() noexcept;
    void IlUpdateTilePixelSize() noexcept;
    HRESULT IlDpiChanged() noexcept;

    HRESULT OnMenuAddFile(CPlayList* pList, int idxInsert = -1) noexcept;

    void OnListSwitch() noexcept;

    void InitializeUi() noexcept;
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;
};