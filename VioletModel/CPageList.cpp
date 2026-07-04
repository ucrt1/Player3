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

eck::CoroTask<void> CPageList::PlMdTskLoad(TSKPARAM_LOAD_META_DATA&& Param_)
{
    const auto Param{ std::move(Param_) };
    const auto pList = Param.pList.get();
    struct TMP
    {
        Tag::MUSICINFO mi{};
        ComPtr<IWICBitmap> pWicBitmap{};
        ComPtr<ID2D1Bitmap1> pD2DBitmap{};
        UINT uSecTime{};
    };
    auto UiThread{ eck::CoroCaptureUiThread() };

    std::vector<TMP> vTmp(Param.vItem.size());
    pList->TskIncRef();
    EckCounter(Param.vItem.size(), i)
    {
        auto& e = pList->FlAtAbs(Param.vItem[i]);
        vTmp[i].mi.uMask = Tag::MIM_NONE;
        if (!e.s.bUpdated)
        {
            vTmp[i].mi.uMask |= Tag::MIM_TITLE | Tag::MIM_ARTIST |
                Tag::MIM_ALBUM;
            e.s.bUpdated = TRUE;
        }
        if (!e.s.bCoverUpdated)
        {
            vTmp[i].mi.uMask |= Tag::MIM_COVER;
            e.s.bCoverUpdated = TRUE;
        }
    }
    auto ptc = eck::PtcCurrent();
    EckAssert(eck::PtcCurrent() == App->UiThreadContext());

    co_await eck::CoroResumeBackground();
    Tag::SIMPLE_OPT Opt{};
    Opt.svArtistDiv = Opt.svCommDiv = {};
    Opt.uFlags = Tag::SMOF_MOVE;
    EckCounter(Param.vItem.size(), i)
    {
        const auto idxFlat = Param.vItem[i];
        const auto& e = pList->FlAtAbs(idxFlat);
        auto& f = vTmp[i];
        UINT uSecTime;
        if (f.mi.uMask == Tag::MIM_COVER)
            uSecTime = 0;
        else
        {
            CBass Bass{};
            const auto h = Bass.Open(e.rsFile.Data(), BASS_STREAM_DECODE,
                BASS_STREAM_DECODE, BASS_STREAM_DECODE);
#ifdef _DEBUG
            if (!h)
                EckDbgPrintFmt(L"%s打开失败", e.rsFile.Data());
#endif
            uSecTime = h ? (UINT)round(Bass.GetLength()) : 0u;
            Bass.Close();
        }

        VltGetMusicInfo(e.rsFile.Data(), f.mi, Opt);

        f.uSecTime = uSecTime;
        const auto pCover = (Tag::MUSICPIC*)f.mi.GetMainCover();
        if (pCover)
        {
            ComPtr<IStream> pStream;
            pCover->CreateStream(pStream.RefOf());
            eck::CreateWicBitmap(f.pWicBitmap.RefOf(), pStream.Get(),
                m_cxIl, m_cyIl, eck::DefaultWicPixelFormat,
                WICBitmapInterpolationModeHighQualityCubic);
            if (f.pWicBitmap.Get())
            {
                D2D1_BITMAP_PROPERTIES1 BmpProp{};
                BmpProp.pixelFormat = D2D1_PIXEL_FORMAT(
                    DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
                m_pDC->GetDpi(&BmpProp.dpiX, &BmpProp.dpiY);
                m_pDC->CreateBitmapFromWicBitmap(f.pWicBitmap.Get(),
                    BmpProp, &f.pD2DBitmap);
            }
        }
    }
    co_await UiThread;
    ECK_DUILOCK;

    EckCounter(Param.vItem.size(), i)
    {
        auto& e = pList->FlAtAbs(Param.vItem[i]);
        auto& f = vTmp[i];
        if (f.pD2DBitmap.Get())
        {
            const auto idxImg = Param.pIl->AddImage(f.pD2DBitmap.Get());
            if (idxImg >= 0)
                e.idxIl = idxImg;
        }
        if (f.mi.uMask != Tag::MIM_COVER)
        {
            e.rsTitle = std::move(f.mi.rsTitle);
            e.rsArtist = std::move(f.mi.slArtist.Str);
            if (!e.rsArtist.IsEmpty())
                e.rsArtist.Erase(0);
            e.rsAlbum = std::move(f.mi.rsAlbum);
            if (f.mi.uMaskChecked & Tag::MIM_TITLE)
                e.rsName = e.rsTitle;
            e.s.uSecTime = f.uSecTime;
            e.s.bUpdated = TRUE;
        }
    }
    for (int i = Param.idxBeginDisplay; i <= Param.idxEndDisplay; ++i)
        if (i < m_GLList.GetItemCount())
            m_GLList.InvalidateCache(i);
    m_GLList.RedrawItem(Param.idxBeginDisplay, Param.idxEndDisplay);
    pList->TskDecRef();
}

void CPageList::PlMdBeginLoad(int idxBegin, int idxEnd, int idxList)
{
    if (idxList < 0)
        idxList = m_TBLPlayList.GetCurrentSelection();
    if (idxList < 0)
        return;
    const auto& e = App->GetListMgr().At(idxList);
    TSKPARAM_LOAD_META_DATA Param
    {
        .pList = e.pList,
        .idxBeginDisplay = idxBegin,
        .idxEndDisplay = idxEnd,
        .pIl = e.pImageList
    };
    for (int i = idxBegin; i <= idxEnd; ++i)
    {
        auto& f = e.pList->FlAt(i);
        if (f.s.bUpdated && f.s.bCoverUpdated)
            continue;
        if (e.pList->FlSchIsActive())
            Param.vItem.emplace_back(e.pList->FlSchAt(i));
        else
            Param.vItem.emplace_back(i);
    }
    if (!Param.vItem.empty())
        PlMdTskLoad(std::move(Param));
}

void CPageList::PlMdCheckVisibleItem(int idxList)
{
    int idx0, idx1;
    m_GLList.GetVisibleRange(idx0, idx1);
    PlMdBeginLoad(idx0, idx1, idxList);
}

CPlayList* CPageList::PlCurrent()
{
    const auto idx = m_TBLPlayList.GetCurrentSelection();
    if (idx < 0)
        return nullptr;
    return App->GetListMgr().AtList(idx).get();
}
std::shared_ptr<CPlayList> CPageList::PlCurrentShared()
{
    const auto idx = m_TBLPlayList.GetCurrentSelection();
    if (idx < 0)
        return {};
    return App->GetListMgr().AtList(idx);
}

int CPageList::PlSearchEditContent(CPlayList* pList)
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
        eck::CRefStrW rsFilter{};
        rsFilter.ReSize((int)gte.cb);
        gte.cb = (DWORD)rsFilter.ByteSize();

        m_EDSearchItem.GetTextEx(&gte, rsFilter.Data());

        pList->FlSchDoSearch(rsFilter.ToStringView());
        return pList->FlSchGetCount();
    }
    else
    {
        m_bSearchItemEditEmpty = TRUE;
        pList->FlSchCancel();
        return pList->FlGetCount();
    }
}

void CPageList::IlUpdateDefaultCover()
{
    SafeRelease(m_pBmpDefCover);
    ComPtr<IWICBitmap> pDefCover;
    eck::ScaleWicBitmap(App->GetImg(AppIcon::DefaultCover), pDefCover.RefOf(),
        m_cxIl, m_cyIl, WICBitmapInterpolationModeHighQualityCubic);
    D2D1_BITMAP_PROPERTIES1 BmpProp{};
    BmpProp.pixelFormat = D2D1_PIXEL_FORMAT(
        DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    m_pDC->GetDpi(&BmpProp.dpiX, &BmpProp.dpiY);
    m_pDC->CreateBitmapFromWicBitmap(pDefCover.Get(), BmpProp, &m_pBmpDefCover);
}

void CPageList::IlReCreate(int idx, BOOL bForce)
{
    if (idx < 0)
        return;
    auto& e = App->GetListMgr().At(idx);
    if (e.pImageList.Get() && !bForce)
        return;
    e.pImageList.Attach(new eck::CD2DImageList{ CxyListCover,CxyListCover });
    e.pImageList->BindRenderTarget(m_pDC);
    e.pImageList->AddImage(m_pBmpDefCover);
}

HRESULT CPageList::OnMenuAddFile(CPlayList* pList, int idxInsert)
{
    ComPtr<IFileOpenDialog> pfod;
    HRESULT hr = pfod.CreateInstance(CLSID_FileOpenDialog);
    if (FAILED(hr))
        return hr;
    pfod->SetTitle(L"打开音频文件");
    pfod->SetOptions(FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
    constexpr COMDLG_FILTERSPEC FilterSpec[]
    {
        { L"音频文件(*.mp1;*.mp2;*.xm;*.mp3;*.flac;*.wma;*.wav;*.m4a;*.ogg;*.acc;*.ape;*.aiff)",
            L"*.mp1;*.mp2;*.xm;*.mp3;*.flac;*.wma;*.wav;*.m4a;*.ogg;*.acc;*.ape;*.aiff" },
        { L"所有文件",L"*.*" }
    };
    pfod->SetFileTypes(ARRAYSIZE(FilterSpec), FilterSpec);

    eck::PtcCurrent()->bEnableDarkModeHook = FALSE;
    pfod->Show(GetWnd()->Handle);
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
    ECK_DUILOCK;
    EckCounter(cItems, i)
    {
        psia->GetItemAt(i, psi.AddrOfClear());
        psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFile);
        if (pszFile)
        {
            pList->FlInsert(pszFile, idxInsert);
            if (idxInsert >= 0)
                ++idxInsert;
            CoTaskMemFree(pszFile);
        }
    }
    if (App->GetPlayer().IsRandom() &&
        App->GetPlayer().GetList() == pList)
        pList->FlRmShuffle();
    return S_OK;
}

LRESULT CPageList::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_NOTIFY:
    {
        if (wParam == (WPARAM)&m_TBLPlayList)
            switch (((Dui::DUINMHDR*)lParam)->uCode)
            {
            case Dui::TBLE_GETDISPINFO:
            {
                const auto p = (Dui::NMTBLDISPINFO*)lParam;
                if (p->uMask & eck::DIM_TEXT)
                {
                    const auto pList = App->GetListMgr().AtList(p->idx);
                    const auto& rsName = pList->GetName();
                    p->cchText = rsName.CopyTo((PWSTR)p->pszText, p->cchText);
                }
                if (p->uMask & eck::DIM_IMAGE)
                    p->pImage = ((CWindowMain*)GetWnd())->RealizeImage(AppIcon::List);
            }
            return 0;
            case Dui::TBLE_SELCHANGED:
            {
                const auto* const p = (Dui::NMTBLITEMINDEX*)lParam;
                if (p->idx < 0)
                    break;
                IlReCreate(p->idx, FALSE);
                const auto& e = App->GetListMgr().At(p->idx);
                e.pList->ImEnsureLoaded();
                m_GLList.InvalidateCache();
                m_GLList.SetImageList(e.pImageList.Get());
                int cItem = e.pList->FlGetCount();
                if (m_bSearchItemEditEmpty)
                    e.pList->FlSchCancel();
                else
                    cItem = PlSearchEditContent(e.pList.get());
                m_GLList.SetItemCount(cItem);
                m_GLList.ReCalc();
                m_GLList.Invalidate();
                PlMdCheckVisibleItem(p->idx);
            }
            return 0;
            }
        else if (wParam == (WPARAM)&m_GLList)
            switch (((Dui::DUINMHDR*)lParam)->uCode)
            {
            case Dui::LEE_GETDISPINFO:
            {
                const auto p = (Dui::NMLEDISPINFO*)lParam;
                if (p->bItem)
                {
                    const auto pList = PlCurrent();
                    if (!pList)
                        return 0;
                    const auto& e = pList->FlAt(p->Item.idx);
                    if (p->uMask & eck::DIM_TEXT)
                    {
                        switch (p->Item.idxSub)
                        {
                        case 0:
                            p->Item.pszText = e.rsName.Data();
                            p->Item.cchText = e.rsName.Size();
                            break;
                        case 1:
                            p->Item.pszText = e.rsArtist.Data();
                            p->Item.cchText = e.rsArtist.Size();
                            break;
                        case 2:
                            p->Item.pszText = e.rsAlbum.Data();
                            p->Item.cchText = e.rsAlbum.Size();
                            break;
                        case 3:
                        {
                            m_rsDispInfoBuf.Format(L"%02u:%02u",
                                e.s.uSecTime / 60u, e.s.uSecTime % 60);
                            p->Item.pszText = m_rsDispInfoBuf.Data();
                            p->Item.cchText = m_rsDispInfoBuf.Size();
                        }
                        break;
                        }
                    }
                    if (p->Item.idxSub)
                        p->Item.idxImg = -1;
                    else
                        p->Item.idxImg = e.idxIl;
                }
            }
            return 0;
            case Dui::HEE_GETDISPINFO:
            {
                const auto p = (Dui::NMHEDISPINFO*)lParam;
                p->pszText = ColumnName[p->idx].data();
                p->cchText = (int)ColumnName[p->idx].size();
            }
            return 0;
            case Dui::LTE_SCROLLED:
            {
                const auto p = (Dui::NMLTSCROLLED*)lParam;
                if (eck::PtcCurrent() != App->UiThreadContext())
                    App->UiThreadContext()->Callback.EnQueueCallback(
                        [this, idx0 = p->idxBegin, idx1 = p->idxEnd]
                        {
                            PlMdBeginLoad(idx0, idx1);
                        });
                else
                    PlMdBeginLoad(p->idxBegin, p->idxEnd);
            }
            return 0;
            }
        else if (wParam == (WPARAM)&m_BTAddFile)
            switch (((Dui::DUINMHDR*)lParam)->uCode)
            {
            case Dui::EE_COMMAND:
            {
                const auto pList = PlCurrent();
                if (!pList)
                    break;
                OnMenuAddFile(pList, -1);
                m_GLList.SetItemCount(pList->FlGetCount());
                m_GLList.ReCalc();
                m_GLList.Invalidate();
                PlMdCheckVisibleItem(-1);
            }
            return 0;
            }
        else if (wParam == (WPARAM)&m_BTLocate)
            switch (((Dui::DUINMHDR*)lParam)->uCode)
            {
            case Dui::EE_COMMAND:
            {
                const auto pList = PlCurrent();
                if (!pList)
                    break;
                m_GLList.EnsureVisible(TRUE, pList->PlyGetCurrentItem());
            }
            return 0;
            }
        else if (wParam == (WPARAM)&m_EDSearchItem)
            switch (((Dui::DUINMHDR*)lParam)->uCode)
            {
            case Dui::EDE_TXNOTIFY:
            {
                const auto p = (Dui::NMEDTXNOTIFY*)lParam;
                if (p->iNotify != EN_CHANGE)
                    break;
                const auto pList = PlCurrent();
                if (!pList)
                    break;
                m_GLList.SetItemCount(PlSearchEditContent(pList));
                m_GLList.InvalidateCache();
                m_GLList.ReCalc();
                m_GLList.Invalidate();
                PlMdCheckVisibleItem(-1);
            }
            return 0;
            }
    }
    return 0;

    case WM_SIZE:
        m_Lyt.Arrange(GetWidthF(), GetHeightF());
        PlMdCheckVisibleItem(-1);
        break;

    case WM_SETFONT:
    {
        m_TBLPlayList.SetTextFormat(GetTextFormat());
        m_EDSearch.SetTextFormat(GetTextFormat());
        m_EDSearchItem.SetTextFormat(GetTextFormat());
    }
    return 0;

    case Dui::EWM_COLORSCHEMECHANGED:
    {
        IlUpdateDefaultCover();
        const auto idx = m_TBLPlayList.GetCurrentSelection();
        if (idx < 0)
            break;
        auto& Lm = App->GetListMgr();
        EckCounter(Lm.GetCount(), i)
        {
            const auto pIl = Lm.At(idx).pImageList.Get();
            if (!pIl)
                continue;
            pIl->ReplaceImage(0, m_pBmpDefCover);
        }
    }
    break;
    case WM_CREATE:
    {
        m_cxIl = (int)Log2PhyF(CxyListCover);
        m_cyIl = m_cxIl;
        IlUpdateDefaultCover();
        {
            m_EDSearch.TxSetProperty(TXTBIT_MULTILINE, 0, FALSE);
            m_EDSearch.Create(nullptr, Dui::DES_VISIBLE, 0,
                0, 0, 0, CyStdEdit, this);
            m_LytPlayList.Add(&m_EDSearch, { .b = CxPageIntPadding },
                eck::LF_FIX_HEIGHT);

            m_TBLPlayList.Create(nullptr, Dui::DES_VISIBLE, 0,
                0, 0, CxListFileList, 0, this, GetWnd());
            m_TBLPlayList.SetItemCount(App->GetListMgr().GetCount());
            m_TBLPlayList.SetBottomExtraSpace(CyPlayPanel);
            m_TBLPlayList.ReCalc();
            m_LytPlayList.Add(&m_TBLPlayList, {}, 0, 1);
            m_LytPlayList.LoSetSize({ CxListFileList, 0 });
        }
        m_Lyt.Add(&m_LytPlayList, { .r = CxPageIntPadding },
            eck::LF_FIX_WIDTH);

        {
            ComPtr<IDWriteTextFormat> pTextFormat;
            App->GetFontFactory().NewFont(pTextFormat.RefOfClear(), eck::Alignment::Center,
                eck::Alignment::Center, (float)CyFontNormal, 400);
            pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            m_BTAddFile.Create(L"添加文件", Dui::DES_VISIBLE, 0,
                0, 0, CxListPageButton, CyStdEdit, this);
            m_BTAddFile.SetTextFormat(pTextFormat.Get());
            m_BTAddFile.SetBitmap(((CWindowMain*)GetWnd())->RealizeImage(AppIcon::Add));
            m_LytTopBar.Add(&m_BTAddFile, {}, eck::LF_FIX);

            m_BTLocate.Create(L"定位当前", Dui::DES_VISIBLE, 0,
                0, 0, CxListPageButton, CyStdEdit, this);
            m_BTLocate.SetTextFormat(pTextFormat.Get());
            m_BTLocate.SetBitmap(((CWindowMain*)GetWnd())->RealizeImage(AppIcon::Locate));
            m_LytTopBar.Add(&m_BTLocate, { .l = CxPageIntPadding }, eck::LF_FIX);

            m_LytTopBar.Add(&m_TopBarDummySpace, {}, eck::LF_FILL, 1);

            m_EDSearchItem.TxSetProperty(TXTBIT_MULTILINE, 0, FALSE);
            m_EDSearchItem.Create(nullptr, Dui::DES_VISIBLE, 0,
                0, 0, 200, CyStdEdit, this);
            m_EDSearchItem.SetEventMask(ENM_CHANGE);
            m_LytTopBar.Add(&m_EDSearchItem, {}, eck::LF_FIX);
            m_LytTopBar.LoSetSize({ 0, CyStdEdit });

            m_LytList.Add(&m_LytTopBar, { .b = CxPageIntPadding },
                eck::LF_FIX_HEIGHT);

            App->GetFontFactory().NewFont(pTextFormat.RefOfClear(), eck::Alignment::Near,
                eck::Alignment::Center, (float)CyFontNormal, 400, TRUE);
            pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            m_GLList.Create(nullptr, Dui::DES_VISIBLE, 0,
                0, 0, 0, 0, this);
            m_GLList.SetView(Dui::CListTemplate::Type::Report);
            constexpr float Width[]{ 270, 150, 150, 50 };
            m_GLList.SetColumnCount(4, Width);
            m_GLList.SetTopExtraSpace(Dui::CListTemplate::CyDefHeader);
            m_GLList.SetBottomExtraSpace(CyPlayPanel);
            m_GLList.SetItemHeight(CyPlayListItem);
            m_GLList.SetSingleSel(FALSE);
            m_GLList.SetTextFormat(pTextFormat.Get());
            m_GLList.GetHeader().SetTextFormat(pTextFormat.Get());
            m_GLList.SetDbgIndex(TRUE);
            m_LytList.Add(&m_GLList, {}, eck::LF_FILL, 1);
        }
        m_Lyt.Add(&m_LytList, { .r = CxPageIntPadding }, 0, 1);

        m_GLList.GetSignal().Connect(
            [&](UINT uMsg, WPARAM wParam, LPARAM lParam, eck::SlotCtx& Ctx)
            {
                switch (uMsg)
                {
                case WM_LBUTTONDBLCLK:
                {
                    POINT pt ECK_GET_PT_LPARAM(lParam);
                    Dui::LE_HITTEST ht{ pt };
                    const auto idx = m_GLList.HitTest(ht);
                    if (idx < 0)
                        break;
                    const auto pList = PlCurrent();
                    App->GetPlayer().SetList(pList);
                    App->GetPlayer().Play(pList->FlSchGetRealIndex(idx));
                }
                break;
                }
                return 0;
            });
        if (App->GetListMgr().GetCount())
        {
            IlReCreate(0, TRUE);
            m_GLList.SetImageList(App->GetListMgr().At(0).pImageList.Get());
            m_TBLPlayList.SelectItemForClick(0);
            PlMdCheckVisibleItem(0);
        }
    }
    break;
    case WM_DPICHANGED:
    {
        m_cxIl = (int)Log2PhyF(CxyListCover);
        m_cyIl = m_cxIl;
        IlUpdateDefaultCover();
        ((CWindowMain*)GetWnd())->ThreadCtx()->Callback.EnQueueCallback([this]
            {
                ECK_DUILOCK;
                App->GetListMgr().InvalidateImageList();
                const auto idx = m_TBLPlayList.GetCurrentSelection();
                if (idx < 0)
                    return;
                IlReCreate(idx, TRUE);
                const auto& e = App->GetListMgr().At(idx);
                m_GLList.SetImageList(e.pImageList.Get());
                m_GLList.Invalidate();
                PlMdCheckVisibleItem(-1);
            });
    }
    break;
    case WM_DESTROY:
        SafeRelease(m_pBmpDefCover);
        break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}