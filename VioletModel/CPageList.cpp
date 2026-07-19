#include "pch.h"
#include "CWndMain.h"
#include "Utils.h"

constexpr std::wstring_view ColumnName[]
{
    L"名称"sv,
    L"艺术家"sv,
    L"专辑"sv,
    L"时长"sv,
};
constexpr float ColumnWidth[]{ 270, 150, 150, 50 };

CPageList::LvIndex CPageList::LcaGetCount() const noexcept
{
    const auto& pList = PlCurrent();
    LvIndex idx{};
    idx.Item = pList->FlIsSearching() ?
        pList->FlGetSearchResultCount() : pList->FlGetCount();
    return idx;
}

int CPageList::LcaGetColumnCount() const noexcept
{
    return ARRAYSIZE(ColumnName);
}

void CPageList::LcaGet(const LvIndex& idx, int idxCol,
    LvProperty eProp, std::any& Data) const noexcept
{
    switch (eProp)
    {
    case LvProperty::SystemState:
        Data = (LvState)m_vItem[idx.Item].uState;
        break;
    case LvProperty::UiTextLayout:
        Data = m_vItem[idx.Item].pTextLayout[idxCol];
        break;
    case LvProperty::Image:
    {
        const auto idxImage = m_vItem[idx.Item].idxImage;
        Data = idxImage < 0 ? DefaultCoverIndex : idxImage;
    }
    break;
    case LvProperty::Text:
    {
        auto& rs = *std::any_cast<eck::CStringW*>(Data);
        const auto& e = PlCurrent()->FlAt(idx.Item);
        switch (idxCol)
        {
        case 0: rs = e.rsName;   break;
        case 1: rs = e.rsArtist; break;
        case 2: rs = e.rsAlbum;  break;
        case 3:
        {
            const auto uSecTime = e.s.uSecTime;
            if (uSecTime)
                rs.Format(L"%d:%02d", uSecTime / 60, uSecTime % 60);
            else
                rs.Clear();
        }
        break;
        }
    }
    break;
    }
}

void CPageList::LcaSet(const LvIndex& idx, int idxCol,
    LvProperty eProp, std::any& Data, BOOL bMove) noexcept
{
    switch (eProp)
    {
    case LvProperty::SystemState:
        m_vItem[idx.Item].uState = std::any_cast<LvState>(Data);
        break;
    case LvProperty::UiTextLayout:
        m_vItem[idx.Item].pTextLayout[idxCol] = std::any_cast<ComPtr<IDWriteTextLayout>>(Data);
        break;
    }
}

void CPageList::LcaColumnWidthChanged(int idxCol, float cxNew) noexcept
{
    // TODO: 考虑cxNew
    for (auto& e : m_vItem)
        e.pTextLayout[idxCol].Clear();
}

eck::CoroTask<void> CPageList::PlLoadMetadata(
    TSKPARAM_LOAD_META_DATA&& Param_) noexcept
{
    struct METADATA
    {
        Tag::SimpleData mi{};
        UINT uSecTime{};
        eck::CTrivialBuffer<UINT> CoverPixel{};
    };

    HRESULT hr;

    const auto Param{ std::move(Param_) };
    const auto UiThread{ eck::CoroCaptureUiThread() };

    std::vector<METADATA> vMetadata{ Param.vItem.Size() };

    const auto cxIlTile = m_cxIl + Param.pImageList->GetPadding();
    const auto cyIlTile = m_cyIl + Param.pImageList->GetPadding();

    // -- 检查加载需求
    BOOL bNeedUpdate{};
    EckCounter(Param.vItem.Size(), i)
    {
        auto& e = Param.pList->FlAtAbsolutely(Param.vItem[i]);
        vMetadata[i].mi.uMask = Tag::MIM_NONE;
        if (!e.s.bUpdated)
        {
            vMetadata[i].mi.uMask |= Tag::MIM_TITLE | Tag::MIM_ARTIST |
                Tag::MIM_ALBUM;
            e.s.bUpdated = TRUE;
            bNeedUpdate = TRUE;
        }
        if (m_vItem[i].idxImage < 0)
        {
            vMetadata[i].mi.uMask |= Tag::MIM_COVER;
            m_vItem[i].idxImage = 0;
            bNeedUpdate = TRUE;
        }
    }
    if (!bNeedUpdate)
        co_return;

    // -- 加载元数据
    co_await eck::CoroResumeBackground();
    Tag::SIMPLE_OPT Opt{};
    Opt.svArtistDiv = Opt.svCommDiv = {};
    Opt.uFlags = Tag::SMOF_MOVE;
    EckCounter(Param.vItem.Size(), i)
    {
        const auto& e = Param.pList->FlAtAbsolutely(Param.vItem[i]);
        auto& Meta = vMetadata[i];

        // -- 取持续时间
        UINT uSecTime;
        if (Meta.mi.uMask == Tag::MIM_COVER)
            uSecTime = 0;
        else
        {
            CBass Bass{};
            const auto h = Bass.Open(
                e.rsFile.Data(),
                BASS_STREAM_DECODE,
                BASS_STREAM_DECODE,
                BASS_STREAM_DECODE);
#ifdef _DEBUG
            if (!h)
                EckDbgPrintFormat(L"%s打开失败", e.rsFile.Data());
#endif
            uSecTime = h ? (UINT)round(Bass.GetLength()) : 0u;
            Bass.Close();
        }
        Meta.uSecTime = uSecTime;

        // -- 取元数据
        VltGetMusicInfo(e.rsFile.Data(), Meta.mi, Opt);
        const auto pCover = (Tag::Picture*)Meta.mi.GetMainCover();
        if (pCover)
        {
            ComPtr<IWICBitmapSource> pBitmap;
            ComPtr<IStream> pStream;

            hr = pCover->CreateStream(pStream.AtSelf());
            if (FAILED(hr))
                continue;

            hr = eck::WicLoadSource(
                pBitmap.AtSelf(),
                pStream.Get(),
                m_cxIl, m_cyIl,
                eck::DefaultWicPixelFormat,
                WICBitmapInterpolationModeFant);// 绝大部分都是缩小操作
            if (FAILED(hr))
                continue;

            Meta.CoverPixel.ReSize(cxIlTile * cyIlTile * sizeof(UINT));

            const WICRect rc{ 0, 0, m_cxIl, m_cyIl };
            hr = pBitmap->CopyPixels(
                &rc,
                cxIlTile * sizeof(UINT),
                Meta.CoverPixel.Size(),
                (BYTE*)Meta.CoverPixel.Data());
            if (FAILED(hr))
                Meta.CoverPixel.Clear();
        }
    }

    // -- 更新UI
    co_await UiThread;
    GetWindow().RdLockUpdate();
    EckCounter(Param.vItem.Size(), i)
    {
        auto& e = Param.pList->FlAtAbsolutely(Param.vItem[i]);
        auto& Ui = m_vItem[Param.vItem[i]];
        auto& Meta = vMetadata[i];

        if (!Meta.CoverPixel.IsEmpty())
        {
            UINT idxImage;
            hr = Param.pImageList->Add(idxImage);
            if (SUCCEEDED(hr))
            {
                hr = Param.pImageList->Upload(
                    idxImage,
                    Meta.CoverPixel.Data(),
                    cxIlTile * sizeof(UINT));
                if (SUCCEEDED(hr))
                    Ui.idxImage = (int)idxImage;
            }
        }

        if (Meta.mi.uMask != Tag::MIM_COVER)
        {
            e.rsTitle = std::move(Meta.mi.rsTitle);
            e.rsArtist = std::move(Meta.mi.slArtist.String);
            if (!e.rsArtist.IsEmpty())
                e.rsArtist.Erase(0);
            e.rsAlbum = std::move(Meta.mi.rsAlbum);
            if (Meta.mi.uMaskChecked & Tag::MIM_TITLE)
                e.rsName = e.rsTitle;
            e.s.uSecTime = Meta.uSecTime;
            e.s.bUpdated = TRUE;

            for (auto& pTl : Ui.pTextLayout)
                pTl.Clear();// TODO: 选择性无效化
        }
        m_GLList.GetController().InvalidateItem({ .Item = Param.vItem[i] });
    }
    GetWindow().RdUnlockUpdate();
}

void CPageList::PlBeginLoadMetadata(int idxList) noexcept
{
    if (idxList < 0)
        idxList = m_TBLPlayList.GetController().ItmGetSelected();
    if (idxList < 0)
        return;

    TSKPARAM_LOAD_META_DATA Param
    {
        .pList = App->ListManager().At(idxList).pList,
        .pImageList = m_vListInfo[idxList].pImageList
    };

    m_GLList.GetController().ForEachItem(
        [&](const Dui::CListView::TController::FOR_ITEM& e)
        {
            auto& Meta = Param.pList->FlAt(e.idx.Item);
            if (!Meta.s.bUpdated || m_vItem[e.idx.Item].idxImage < 0)
                if (Param.pList->FlIsSearching())
                    Param.vItem.PushBack(Param.pList->FlAtSearch(e.idx.Item));
                else
                    Param.vItem.PushBack(e.idx.Item);
        },
        [](const Dui::CListView::TController::FOR_GROUP& e) {},
        m_GLList.GetViewRect(),
        FALSE);

    if (!Param.vItem.IsEmpty())
        PlLoadMetadata(std::move(Param));
}

const RefPtr<CPlayList>& CPageList::PlCurrent() const noexcept
{
    const auto idx = m_TBLPlayList.GetController().ItmGetSelected();
    if (idx < 0)
        return {};
    return App->ListManager().At(idx).pList;
}

int CPageList::PlSearchEditContent(CPlayList* pList) noexcept
{
    GETTEXTLENGTHEX  gtl{};
    gtl.codepage = eck::CP_UTF16LE;
    gtl.flags = GTL_DEFAULT;
    GETTEXTEX gte{};
    if (gte.cb = m_EDSearchItem.GetTextLengthEx(&gtl))
    {
        m_bSearchItemEditEmpty = FALSE;
        gte.codepage = eck::CP_UTF16LE;
        gte.flags = GT_DEFAULT;
        eck::CStringW rsFilter{};
        rsFilter.ReSize((int)gte.cb);
        gte.cb = (DWORD)rsFilter.ByteSize();

        m_EDSearchItem.GetTextEx(&gte, rsFilter.Data());

        pList->FlDoSearch(rsFilter.ToStringView());
        return pList->FlGetSearchResultCount();
    }
    else
    {
        m_bSearchItemEditEmpty = TRUE;
        pList->FlExitSearch();
        return pList->FlGetCount();
    }
}

HRESULT CPageList::IlUploadDefaultCover(eck::CD2DImageList* pImageList) noexcept
{
    HRESULT hr;
    ComPtr<IWICBitmapScaler> pScaler;

    hr = eck::g_pWicFactory->CreateBitmapScaler(&pScaler);
    if (FAILED(hr))
        return hr;

    hr = pScaler->Initialize(
        GetAtlas()->CoverGetDefaultWicBitmap().Get(),
        m_cxIl, m_cyIl,
        WICBitmapInterpolationModeFant);
    if (FAILED(hr))
        return hr;

    const auto cxTile = m_cxIl + pImageList->GetPadding();
    const auto cyTile = m_cyIl + pImageList->GetPadding();

    eck::CTrivialBuffer<UINT> Buffer{};
    Buffer.ReSize(cxTile * cyTile * sizeof(UINT));

    const WICRect rc{ 0, 0, m_cxIl, m_cyIl };
    hr = pScaler->CopyPixels(
        &rc,
        cxTile * sizeof(UINT),
        Buffer.Size(),
        (BYTE*)Buffer.Data());
    if (FAILED(hr))
        return hr;

    if (!pImageList->GetCount())
    {
        UINT idxImage;
        hr = pImageList->Add(idxImage);
        if (FAILED(hr))
            return hr;
    }

    hr = pImageList->Upload(0, Buffer.Data(), cxTile * sizeof(UINT));
    return hr;
}

RefPtr<eck::CD2DImageList> CPageList::IlCreate() noexcept
{
    auto p = RefPtr<eck::CD2DImageList>::Make(
        (float)GetWindow().GetUserDpi(), m_cxIl, m_cyIl);
    p->BindRenderTarget(GetDC());
    IlUploadDefaultCover(p.Get());
    return p;
}

void CPageList::IlUpdateTilePixelSize() noexcept
{
    m_cxIl = m_cyIl = (int)LogicalToPixel(CxyListCover);
}

HRESULT CPageList::IlDpiChanged() noexcept
{
    IlUpdateTilePixelSize();
    const auto idx = m_TBLPlayList.GetController().ItmGetSelected();
    if (idx < 0)
        return;
    m_vListInfo[idx].pImageList = IlCreate();
    for (auto& e : m_vItem)
        e.idxImage = -1;
    m_GLList.SetImageList(m_vListInfo[idx].pImageList);
    m_GLList.Invalidate();// TODO: 仅无效化图标
    PlBeginLoadMetadata();
}

HRESULT CPageList::OnMenuAddFile(CPlayList* pList, int idxInsert)
{
    ComPtr<IFileOpenDialog> pfod;
    HRESULT hr = pfod.CreateInstance(CLSID_FileOpenDialog);
    if (FAILED(hr))
        return hr;
    pfod->SetTitle(L"打开音频文件");
    pfod->SetOptions(FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
    constexpr COMDLG_FILTERSPEC Filter[]
    {
        { L"音频文件(*.mp1;*.mp2;*.xm;*.mp3;*.flac;*.wma;*.wav;*.m4a;*.ogg;*.acc;*.ape;*.aiff)",
            L"*.mp1;*.mp2;*.xm;*.mp3;*.flac;*.wma;*.wav;*.m4a;*.ogg;*.acc;*.ape;*.aiff" },
        { L"所有文件",L"*.*" }
    };
    pfod->SetFileTypes(ARRAYSIZE(Filter), Filter);

    eck::PtcCurrent()->bEnableDarkModeHook = FALSE;
    pfod->Show(GetWindow().Handle);
    eck::PtcCurrent()->bEnableDarkModeHook = TRUE;
    ComPtr<IShellItemArray> psia;
    hr = pfod->GetResults(&psia);
    if (FAILED(hr))
        return hr;
    DWORD cItems;
    hr = psia->GetCount(&cItems);
    if (FAILED(hr))
        return hr;
    ComPtr<IShellItem> psi;
    PWSTR pszFile;
    EckCounter(cItems, i)
    {
        psia->GetItemAt(i, psi.AtClear());
        psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFile);
        if (pszFile)
        {
            pList->FlInsert(pszFile, idxInsert);
            if (idxInsert >= 0)
                ++idxInsert;
            CoTaskMemFree(pszFile);
        }
    }
    if (App->Player().IsRandom() && pList->PlyIsSelected())
        pList->FlShuffleRandom();
    return S_OK;
}

void CPageList::OnListSwitch() noexcept
{
    const auto idx = m_TBLPlayList.GetController().ItmGetSelected();
    if (idx < 0)
        return;
    auto& e = m_vListInfo[idx];
    if (!e.pImageList)
        e.pImageList = IlCreate();
    m_GLList.SetImageList(e.pImageList);
    m_GLList.ReCalculateItem();
    m_GLList.Invalidate();
}

void CPageList::InitializeUi() noexcept
{
    {
        m_EDSearch.TxSetProperty(TXTBIT_MULTILINE, 0, FALSE);
        m_EDSearch.Create({}, Dui::DES_VISIBLE, 0,
            0, 0, 0, CyStdEdit, this);
        m_LytPlayList.LobAddObject(
            {
                .pObject = &m_EDSearch,
                .Margins = {.b = CxPageIntPadding },
                .uFlags = eck::LF_FIX_HEIGHT
            });

        m_TBLPlayList.Create({}, Dui::DES_VISIBLE, 0,
            0, 0, CxListFileList, 0, this);
        m_TBLPlayList.GetController().MtSetBottomExtra(CyPlayPanel);
        m_TBLPlayList.SetAdapter(this);
        m_TBLPlayList.ReCalculateItem();
        m_LytPlayList.LobAddObject(
            {
                .pObject = &m_TBLPlayList,
                .uWeight = 1,
            });

        m_LytPlayList.LoSetSize({ CxListFileList, 0 });
    }
    m_Lyt.LobAddObject(
        {
            .pObject = &m_LytPlayList,
            .Margins = {.r = CxPageIntPadding },
            .uFlags = eck::LF_FIX_WIDTH
        });

    {
        ComPtr<IDWriteTextFormat> pTextFormat;
        App->GetFontFactory().NewFont(pTextFormat.AtSelf(), eck::Alignment::Center,
            eck::Alignment::Center, (float)CyFontNormal, 400);
        pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        m_BTAddFile.Create(L"添加文件", Dui::DES_VISIBLE, 0,
            0, 0, CxListPageButton, CyStdEdit, this);
        m_BTAddFile.SetTextFormat(pTextFormat.Get());
        m_BTAddFile.SetIcon(GetAtlas()->AtlasGetSubImage(AppImage::Plus));
        m_LytTopBar.LobAddObject(
            {
                .pObject = &m_BTAddFile,
                .uFlags = eck::LF_FIX
            });

        m_BTLocate.Create(L"定位当前", Dui::DES_VISIBLE, 0,
            0, 0, CxListPageButton, CyStdEdit, this);
        m_BTLocate.SetTextFormat(pTextFormat.Get());
        m_BTLocate.SetIcon(GetAtlas()->AtlasGetSubImage(AppImage::Locate));
        m_LytTopBar.LobAddObject(
            {
                .pObject = &m_BTLocate,
                .Margins = {.l = CxPageIntPadding },
                .uFlags = eck::LF_FIX
            });

        m_LytTopBar.LobAddObject(
            {
                .pObject = &m_TopBarDummySpace,
                .uWeight = 1,
            });

        m_EDSearchItem.TxSetProperty(TXTBIT_MULTILINE, 0, FALSE);
        m_EDSearchItem.Create(nullptr, Dui::DES_VISIBLE, 0,
            0, 0, 200, CyStdEdit, this);
        m_EDSearchItem.SetEventMask(ENM_CHANGE);
        m_LytTopBar.LobAddObject(
            {
                .pObject = &m_EDSearchItem,
                .uFlags = eck::LF_FIX
            });
        m_LytTopBar.LoSetSize({ 0, CyStdEdit });

        m_LytList.LobAddObject(
            {
                .pObject = &m_LytTopBar,
                .Margins = {.b = CxPageIntPadding },
                .uFlags = eck::LF_FIX_HEIGHT
            });

        App->GetFontFactory().NewFont(pTextFormat.AtSelfClear(), eck::Alignment::Near,
            eck::Alignment::Center, (float)CyFontNormal, 400, TRUE);
        pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        m_GLList.Create(nullptr, Dui::DES_VISIBLE, 0,
            0, 0, 0, 0, this);
        m_GLList.SetTextFormat(pTextFormat.Get());

        auto& Controller = m_GLList.GetController();
        Controller.SetView(Dui::CListView::View::List);
        Controller.MtSetItemHeight(CyPlayListItem);
        Controller.SetSelectionType(Dui::CListView::Selection::Multiple);

        auto& Header = m_GLList.GetHeader();
        EckCounter(ARRAYSIZE(ColumnName), i)
            Header.InsertItem(i, ColumnName[i], ColumnWidth[i]);
        Header.SetTextFormat(pTextFormat.Get());

        m_LytList.LobAddObject(
            {
                .pObject = &m_GLList,
                .uFlags = eck::LF_FILL,
                .uWeight = 1,
            });
    }
    m_Lyt.LobAddObject(
        {
            .pObject = &m_LytList,
            .Margins = {.l = CxPageIntPadding },
            .uWeight = 1,
        });

    m_GLList.GetEventChain().Connect(
        [&](UINT uMsg, WPARAM wParam, LPARAM lParam, eck::Slot&)
        {
            switch (uMsg)
            {
            case WM_LBUTTONDBLCLK:
            {
                const auto pt = EagPoint(lParam);
                Dui::CListView::TController::HT_INFO ht{ pt.x, pt.y };
                const auto idx = m_GLList.GetController().HitTest(ht);
                if (idx.Item < 0)
                    break;
                const auto pList = PlCurrent();
                App->Player().SetList(pList);
                App->Player().Play(pList->FlSearchIndexToRealIndex(idx.Item));
            }
            break;
            }
            return 0;
        });
}

LRESULT CPageList::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_NOTIFY:
    {
        if (wParam == (WPARAM)&m_TBLPlayList)
            switch (((Dui::ELENMHDR*)lParam)->uNotify)
            {
                //case Dui::TBLE_SELCHANGED:
                //{
                //    const auto* const p = (Dui::NMTBLITEMINDEX*)lParam;
                //    if (p->idx < 0)
                //        break;
                //    IlReCreate(p->idx, FALSE);
                //    const auto& e = App->ListManager().At(p->idx);
                //    e.pList->LtmEnsureLoaded();
                //    m_GLList.InvalidateCache();
                //    m_GLList.SetImageList(e.pImageList.Get());
                //    int cItem = e.pList->FlGetCount();
                //    if (m_bSearchItemEditEmpty)
                //        e.pList->FlExitSearch();
                //    else
                //        cItem = PlSearchEditContent(e.pList.get());
                //    m_GLList.SetItemCount(cItem);
                //    m_GLList.ReCalc();
                //    m_GLList.Invalidate();
                //    PlCheckVisibleItemMetadata(p->idx);
                //}
                return 0;
            }
        else if (wParam == (WPARAM)&m_GLList)
            switch (((Dui::ELENMHDR*)lParam)->uNotify)
            {
            //case Dui::LTE_SCROLLED:
            //{
            //    const auto p = (Dui::NMLTSCROLLED*)lParam;
            //    if (eck::PtcCurrent() != App->UiThreadContext())
            //        App->UiThreadContext()->Callback.EnQueueCallback(
            //            [this, idx0 = p->idxBegin, idx1 = p->idxEnd]
            //            {
            //                PlBeginLoadMetadata(idx0, idx1);
            //            });
            //    else
            //        PlBeginLoadMetadata(p->idxBegin, p->idxEnd);
            //}
            return 0;
            }
        else if (wParam == (WPARAM)&m_BTAddFile)
            switch (((Dui::ELENMHDR*)lParam)->uNotify)
            {
            case Dui::ENC_COMMAND:
            {
                const auto& pList = PlCurrent();
                if (!pList)
                    break;
                OnMenuAddFile(pList.Get(), -1);
                m_GLList.ReCalculateItem();
                m_GLList.Invalidate();
                PlBeginLoadMetadata();
            }
            return 0;
            }
        else if (wParam == (WPARAM)&m_BTLocate)
            switch (((Dui::ELENMHDR*)lParam)->uNotify)
            {
            case Dui::ENC_COMMAND:
            {
                const auto pList = PlCurrent();
                if (!pList)
                    break;
                m_GLList.GetController().EnsureVisible(TRUE, pList->PlyGetCurrentItem());
            }
            return 0;
            }
        else if (wParam == (WPARAM)&m_EDSearchItem)
            switch (((Dui::ELENMHDR*)lParam)->uNotify)
            {
            case Dui::ENC_ED_TXNOTIFY:
            {
                const auto p = (Dui::CEdit::EVT_TX*)lParam;
                if (p->iNotify != EN_CHANGE)
                    break;
                const auto pList = PlCurrent();
                if (!pList)
                    break;
                PlSearchEditContent(pList.Get());
                m_GLList.ReCalculateItem();
                m_GLList.Invalidate();
                PlBeginLoadMetadata();
            }
            return 0;
            }
    }
    return 0;

    case WM_SIZE:
        m_Lyt.Arrange(GetWidth(), GetHeight());
        PlBeginLoadMetadata(-1);
        break;

    case WM_SETFONT:
    {
        m_TBLPlayList.SetTextFormat(GetTextFormat().Get());
        m_EDSearch.SetTextFormat(GetTextFormat().Get());
        m_EDSearchItem.SetTextFormat(GetTextFormat().Get());
    }
    return 0;

    case Dui::EWM_COLORSCHEMECHANGED:
    {
        for (auto& e : m_vListInfo)
        {
            if (e.pImageList)
                IlUploadDefaultCover(e.pImageList.Get());
        }
    }
    break;
    case WM_CREATE:
    {
        InitializeUi();
        IlUpdateTilePixelSize();
        OnListSwitch();
    }
    break;
    case WM_DPICHANGED:
        IlDpiChanged();
        break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}