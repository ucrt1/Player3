#pragma once
class CPageList : public CPage
{
    // Pl = Play List
    // Md = Metadata
    // Il = Image List
private:
    constexpr static int DefaultCoverIndex{};
    struct LIST_INFO
    {
        RefPtr<eck::CD2DImageList> pIl;
    };

    struct TSKPARAM_LOAD_META_DATA
    {
        RefPtr<CPlayList> pList;
        int idxBeginDisplay{};
        int idxEndDisplay{};
        RefPtr<eck::CD2DImageList> pIl;
        std::vector<int> vItem;
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

    eck::CStringW m_rsDispInfoBuf{};

    int m_cxIl{}, m_cyIl{};
    ID2D1Bitmap1* m_pBmpDefCover{};
    std::vector<LIST_INFO> m_vListInfo{};

    BOOL m_bSearchItemEditEmpty{};

    eck::CoroTask<void> PlLoadMetadata(TSKPARAM_LOAD_META_DATA&& Param);
    void PlBeginLoadMetadata(int idxBegin, int idxEnd, int idxList = -1);
    void PlCheckVisibleItemMetadata(int idxList);

    const RefPtr<CPlayList>& PlCurrent() const noexcept;

    // 使用搜索编辑框内容搜索列表
    // 返回项目数
    int PlSearchEditContent(CPlayList* pList);

    void IlUpdateDefaultCover();
    void IlReCreate(int idx, BOOL bForce);

    HRESULT OnMenuAddFile(CPlayList* pList, int idxInsert = -1);
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;
};