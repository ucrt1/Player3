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
        ComPtr<eck::CD2DImageList> pIl;
    };

    struct TSKPARAM_LOAD_META_DATA
    {
        std::shared_ptr<CPlayList> pList;
        int idxBeginDisplay{};
        int idxEndDisplay{};
        ComPtr<eck::CD2DImageList> pIl;
        std::vector<int> vItem;
    };;

    Dui::CEdit m_EDSearch{};
    Dui::CTabList m_TBLPlayList{};
    eck::CLinearLayoutV m_LytPlayList{};

    Dui::CButton m_BTAddFile{};
    Dui::CButton m_BTLocate{};
    eck::CLayoutDummy m_TopBarDummySpace{};
    Dui::CEdit m_EDSearchItem{};
    eck::CLinearLayoutH m_LytTopBar{};
    Dui::CList m_GLList{};
    eck::CLinearLayoutV m_LytList{};

    eck::CLinearLayoutH m_Lyt{};

    eck::CRefStrW m_rsDispInfoBuf{};

    int m_cxIl{}, m_cyIl{};
    ID2D1Bitmap1* m_pBmpDefCover{};
    std::vector<LIST_INFO> m_vListInfo{};

    BOOL m_bSearchItemEditEmpty{};

    eck::CoroTask<void> PlMdTskLoad(TSKPARAM_LOAD_META_DATA&& Param);
    void PlMdBeginLoad(int idxBegin, int idxEnd, int idxList = -1);
    void PlMdCheckVisibleItem(int idxList);

    CPlayList* PlCurrent();
    std::shared_ptr<CPlayList> PlCurrentShared();

    // 使用搜索编辑框内容搜索列表
    // 返回项目数
    int PlSearchEditContent(CPlayList* pList);

    void IlUpdateDefaultCover();
    void IlReCreate(int idx, BOOL bForce);

    HRESULT OnMenuAddFile(CPlayList* pList, int idxInsert = -1);
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;
};