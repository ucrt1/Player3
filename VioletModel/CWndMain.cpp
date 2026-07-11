#include "pch.h"

#include "CWndMain.h"

const static UINT MsgTaskbarButtonCreated{ RegisterWindowMessageW(L"TaskbarButtonCreated") };

EckInlineNdCe AppImage AutoNextModeToGImg(AutoNextMode eMode) noexcept
{
    switch (eMode)
    {
    case AutoNextMode::ListLoop: return AppImage::Circle;
    case AutoNextMode::List: return AppImage::ArrowRight3;
    case AutoNextMode::Random: return AppImage::ArrowCross;
    case AutoNextMode::SingleLoop: return AppImage::CircleOne;
    case AutoNextMode::Single: return AppImage::ArrowRight1;
    }
    ECK_UNREACHABLE;
}

void CWindowMain::ClearRes()
{
    for (auto& e : m_vBmpRealization)
        SafeRelease(e);
    SafeRelease(m_pBmpCover);
}

BOOL CWindowMain::OnCreate(HWND hWnd, CREATESTRUCT* pcs)
{
    App->SetDarkMode(ShouldAppsUseDarkMode());
    CBass::Init();
    App->Player().GetEventChain().Connect(this, &CWindowMain::OnPlayEvent);

    KctRegisterTimeLine(this);

    MARGINS m{};// 不能使用-1，否则会绘制标准标题栏
    m.cxLeftWidth = 65536 * 4;
    DwmExtendFrameIntoClientArea(hWnd, &m);
    eck::EnableWindowMica(hWnd);

    SmtcInit();

    BlurInitialize();
    BlurSetUseLayer(TRUE);

    ComPtr<IDWriteTextFormat> pTfPageTitle, pTfLeft, pTfCenter;
    App->GetFontFactory().NewFont(pTfPageTitle.AtSelf(), eck::Alignment::Near,
        eck::Alignment::Center, (float)CyFontPageTitle, 600);
    pTfPageTitle->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    App->GetFontFactory().NewFont(pTfLeft.AtSelf(), eck::Alignment::Near,
        eck::Alignment::Center, (float)CyFontNormal);
    pTfLeft->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    App->GetFontFactory().NewFont(pTfCenter.AtSelf(), eck::Alignment::Center,
        eck::Alignment::Center, (float)CyFontNormal);
    pTfCenter->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    m_NormalPageContainer.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, 0, 0, nullptr, this);
    const auto pNormalParent = &m_NormalPageContainer;
    // 左侧选择夹
    m_TabPanel.Create(nullptr, Dui::DES_VISIBLE | Dui::DES_NOTIFY_WND, 0,
        0, 0, 0, 0, pNormalParent, this);
    // 标题
    m_LAPageTitle.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, 0, 0, pNormalParent, this);
    m_LAPageTitle.SetTextFormat(pTfPageTitle.Get());
    // 页 主页
    m_PageMain.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, 0, 0, pNormalParent, this);
    m_PageMain.SetTextFormat(pTfCenter.Get());
    // 页 列表
    m_PageList.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, 0, 0, pNormalParent, this);
    m_PageList.SetTextFormat(pTfLeft.Get());
    // 页 效果
    m_PageEffect.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, 0, 0, pNormalParent, this);
    m_PageEffect.SetTextFormat(pTfLeft.Get());
    // 页 设置
    m_PageOptions.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, 0, 0, pNormalParent, this);
    m_PageOptions.SetTextFormat(pTfLeft.Get());
    // 底部播放控制栏
    m_PlayPanel.Create(nullptr, Dui::DES_VISIBLE/* | Dui::DES_BLUR_BACK*/, 0,
        0, 0, 0, 0, pNormalParent, this);
    m_PlayPanel.SetTextFormat(pTfLeft.Get());
    // 页 播放
    ComPtr<IDWriteTextFormat> pTfPP;
    m_PagePlaying.Create(nullptr, 0, 0,
        0, 0, 0, 0, nullptr, this);
    m_PagePlaying.SetTextFormat(pTfLeft.Get());
    App->GetFontFactory().NewFont(pTfPP.AtSelfClear(), eck::Alignment::Near,
        eck::Alignment::Center, (float)CyFontPlayPageLabel, 600);
    pTfPP->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    m_PagePlaying.SetLabelTextFormatTitle(pTfPP.Get());
    App->GetFontFactory().NewFont(pTfPP.AtSelfClear(), eck::Alignment::Near,
        eck::Alignment::Center, (float)CyFontPlayPageLabel);
    pTfPP->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    m_PagePlaying.SetLabelTextFormat(pTfPP.Get());
    // 进度条
    m_TBProgress.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, CxProgress, CyProgress, nullptr, this);
    m_TBProgress.SetRange(0, 100);
    m_TBProgress.SetTrackPosition(50);
    m_TBProgress.SetTrackSize(CyProgressTrack);
    m_TBProgress.SetThinTrack(TRUE);
    // 按钮 上一曲
    m_BTPrev.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
    // 按钮 播放/暂停
    m_BTPlay.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, CxyCircleButtonBig, CxyCircleButtonBig, nullptr, this);
    // 按钮 下一曲
    m_BTNext.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
    // 按钮 播放模式
    m_BTAutoNext.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
    m_BTAutoNext.GetEventChain().Connect(
        [](UINT uMsg, WPARAM, LPARAM, eck::Slot&) -> LRESULT
        {
            if (uMsg == WM_RBUTTONDOWN)
            {
                const auto pList = App->Player().GetList();
                if (pList)
                    pList->FlShuffleRandom();
            }
            return 0;
        });
    // 按钮 歌词
    m_BTLrc.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
    // 按钮 音量
    m_BTVol.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, CxyCircleButton, CxyCircleButton, nullptr, this);
    // 标题栏
    m_TitleBar.Create(nullptr, Dui::DES_VISIBLE, 0,
        0, 0, 0, 0, nullptr, this);
    // 音量条
    m_VolBar.Create(nullptr, 0, 0,
        0, 0, CxVolBar, CyVolBar, nullptr, this);
    m_VolBar.SetTextFormat(pTfCenter.Get());
    //
    UpdateButtonImageSize();
    m_PagePlaying.UpdateBlurredCover();
    OnCoverUpdate();

    OnColorSchemeChanged();
    PageShow(Page::List, FALSE);
    m_TabPanel.GetTabList().SelectItemForClick(1);
    return TRUE;
}

void CWindowMain::PageShow(Page ePage, BOOL bAnimate)
{
    __assume(ePage < Page::Max);
    const int idxShow = (int)ePage;
    const int idxShowLast = (int)m_eCurrPage;
    PageClearAnimation();
    if (idxShow == idxShowLast)
        return;
    m_eCurrPage = ePage;

    const auto bAlreadyVisible =
        (m_vPage[idxShow]->GetStyle() & Dui::DES_VISIBLE);

    m_LAPageTitle.SetText(MainWndPageName[idxShow]);
    m_LAPageTitle.Invalidate();
    m_vPage[idxShow]->SetVisible(TRUE);
    for (int i = 0; i < idxShow; ++i)
        m_vPage[i]->SetVisible(FALSE);
    for (int i = idxShow + 1; i < (int)Page::Max; ++i)
        m_vPage[i]->SetVisible(FALSE);

    if (bAnimate)
    {
        if (bAlreadyVisible)
            return;// 已经显示，不需要动画
        m_ecPage.Start(0, (float)CyPageSwitchAnDelta, !!m_pAnPage);
        m_bPageAnUpToDown = (idxShow < idxShowLast);
        m_pAnPage = m_vPage[idxShow];
        KctWake();
    }
}

void CWindowMain::PageClearAnimation()
{
    if (!m_pAnPage)
        return;
    m_pAnPage = nullptr;
}

HWND CWindowMain::Create(PCWSTR pszText, DWORD dwStyle, DWORD dwExStyle,
    int x, int y, int cx, int cy, HWND hParent, HMENU hMenu, PCVOID pData) noexcept
{
    TblCreateGhostWindow(pszText);
    TblCreateObjectAndInit();
    const auto hWnd = __super::Create(pszText, dwStyle, dwExStyle,
        x, y, cx, cy, hParent, hMenu, pData);
    TblOnTaskbarButtonCreated();
    return hWnd;
}

void CWindowMain::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
    switch (e.eEvent)
    {
    case PlayEvent::CommonTick:
    {
        m_msProgTimer += TE_COMM_TICK;
        if (m_msProgTimer >= TE_PROG)
        {
            m_msProgTimer = 0;
            m_TBProgress.SetTrackPosition(float(App->Player().GetCurrentTime() * ProgBarScale));
            m_TBProgress.Invalidate();
            TblUpdateProgress();
        }
        SmtcOnCommonTick();
    }
    break;
    case PlayEvent::Play:
    {
        m_msProgTimer = 0;
        OnCoverUpdate();
        SmtcUpdateTimeLineRange();
        SmtcUpdateDisplay();
        TblUpdateProgress();
        if (m_PagePlaying.GetStyle() & Dui::DES_VISIBLE)
            m_PagePlaying.Invalidate();
        m_TBProgress.SetRange(0.f, float(App->Player().GetTotalTime() * ProgBarScale));
        m_TBProgress.SetTrackPosition(0.f);
        m_TBProgress.Invalidate();
        m_PlayPanel.Invalidate();
        m_WndTbGhost.InvalidateThumbnailCache();
        m_WndTbGhost.InvalidateDwmThumbnail();
        m_WndTbGhost.SetIconicThumbnail();
    }
    [[fallthrough]];
    case PlayEvent::Resume:
    {
        SetTimer(Handle, IDT_COMM_TICK, TE_COMM_TICK, nullptr);
        m_BTPlay.SetIcon(RealizeImage2(AppImage::Pause));
        m_BTPlay.Invalidate();
        TblUpdateState();
        SmtcUpdateState();
    }
    break;
    case PlayEvent::Stop:
    {
        m_TBProgress.SetTrackPosition(0.f);
        m_TBProgress.Invalidate();
        TblUpdateProgress();
    }
    [[fallthrough]];
    case PlayEvent::Pause:
    {
        KillTimer(Handle, IDT_COMM_TICK);
        m_BTPlay.SetIcon(RealizeImage2(AppImage::Triangle));
        m_BTPlay.Invalidate();
        TblUpdateState();
        SmtcUpdateState();
    }
    break;
    }
}

LRESULT CWindowMain::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    if (uMsg == MsgTaskbarButtonCreated)
    {
        if (m_pTaskbarList.Get())
            TblOnTaskbarButtonCreated();
        return 0;
    }
    switch (uMsg)
    {
    case WM_TIMER:
        if (wParam == IDT_COMM_TICK)
            App->Player().GetEventChain().Emit({ PlayEvent::CommonTick });
        break;
    case WM_SIZE:
    {
        const auto lResult = __super::OnMessage(uMsg, wParam, lParam);
        PageClearAnimation();
        const auto cxClient = GetClientWidthLogical();
        const auto cyClient = GetClientHeightLogical();
        m_NormalPageContainer.SetRect({ 0,0,cxClient,cyClient });
        m_TitleBar.SetRect({ 0,0,cxClient,CyTitleBar });
        m_TabPanel.SetRect({ 0,0,CxTabPanel,cyClient - CyPlayPanel });

        const auto yPlayPanel = cyClient - CyPlayPanel;
        m_PlayPanel.SetRect({ 0,cyClient - CyPlayPanel,cxClient,cyClient });

        m_LAPageTitle.SetRect({
            CxTabPanel + CxTabToPagePadding,
            DTopPageTitle,
            CxTabPanel + CxTabToPagePadding + CxPageTitle,
            DTopPageTitle + CyPageTitle });

        m_PagePlaying.SetRect({ 0,0,cxClient,cyClient });
        m_rcPPLarge = { 0.f,0.f,(float)cxClient,(float)cyClient };
        m_rcPPMini.left = DLeftMiniCover;
        m_rcPPMini.top = float(cyClient - CyPlayPanel + DTopMiniCover);
        m_rcPPMini.right = m_rcPPMini.left + (float)CxyMiniCover;
        m_rcPPMini.bottom = m_rcPPMini.top + (float)CxyMiniCover;
        for (auto& e : m_vPage)
            e->SetRect({
                CxTabPanel + CxTabToPagePadding,
                CyPageTitle + DTopPageTitle + CxPageIntPadding,
                cxClient,
                cyClient - CxTabToPagePadding });
            RePosPalyPanelControl();
        OnCoverUpdate();
        return lResult;
    }

    case WM_NCCALCSIZE:
    {
        const auto cxFrame = eck::DaGetSystemMetrics(SM_CXFRAME, GetWindowDpi());
        const auto cyFrame = eck::DaGetSystemMetrics(SM_CYFRAME, GetWindowDpi());
        const auto cxPadded = eck::DaGetSystemMetrics(SM_CXPADDEDBORDER, GetWindowDpi());
        return eck::MsgOnNcCalculateSize(wParam, lParam,
            { cxFrame + cxPadded,cxFrame + cxPadded,0,cyFrame + cxPadded });
    }
    break;

    case WM_COMMAND:
        if (TblOnCommand(wParam))
            return 0;
        break;

    case WM_CREATE:
        __super::OnMessage(uMsg, wParam, lParam);
        return HANDLE_WM_CREATE(Handle, wParam, lParam, OnCreate);
    case WM_DESTROY:
        KillTimer(Handle, IDT_COMM_TICK);
        __super::OnMessage(uMsg, wParam, lParam);
        m_WndTbGhost.Destroy();
        SmtcUnInit();
        ClearRes();
        PostQuitMessage(0);
        return 0;
    case WM_SYSCOLORCHANGE:
        eck::MsgOnSystemColorChangeMainWindow(Handle, wParam, lParam);
        break;
    case WM_SETTINGCHANGE:
    {
        if (eck::MsgOnSettingChangeMainWindow(Handle, wParam, lParam, TRUE))
        {
            InvalidateRealizedImage();
            const auto bDark = ShouldAppsUseDarkMode();
            App->SetDarkMode(bDark);
            OnColorSchemeChanged();
            TblUpdateToolBarIcon();
            m_WndTbGhost.InvalidateDwmThumbnail();
            m_WndTbGhost.InvalidateThumbnailCache();
            m_WndTbGhost.SetIconicThumbnail();
            Redraw();
        }
    }
    break;
    case WM_DPICHANGED:
        SetUserDpi(LOWORD(wParam));
        break;
    case WM_DWMCOLORIZATIONCOLORCHANGED:
        //StUpdateColorizationColor();
        break;
    case WM_SETTEXT:
    {
        const auto lResult = __super::OnMessage(uMsg, wParam, lParam);
        if (lResult)
            m_WndTbGhost.SetText((PCWSTR)lParam);
        return lResult;
    }
    break;
    }
    return __super::OnMessage(uMsg, wParam, lParam);
}

LRESULT CWindowMain::OnElementNotify(Dui::CElement* pEle, Dui::ELENMHDR* pnm) noexcept
{
    switch (pnm->uNotify)
    {
    case ELEN_PAGE_CHANGE:
    {
        const auto* const p = (Dui::NMLTITEMINDEX*)lParam;
        PageShow((Page)p->idx, TRUE);
    }
    return 0;

    case Dui::ENC_POSCHANGED:
    {
        if (pEle == &m_TBProgress)
        {
            App->Player().SetPosition(
                m_TBProgress.GetTrackPosition() / ProgBarScale);
            return 0;
        }
        else if (pEle->GetId() == ELEID_VOLBAR_TRACK)
        {
            const auto f = ((Dui::CTrackBar*)pEle)->GetTrackPosition();
            App->Player().GetBass().SetVolume(f / 100.f);
            m_VolBar.OnVolumeChanged(f);
        }
    }
    return 0;

    case ELEN_MINICOVER_CLICK:
    {
        PpaPrepare();
        KctWake();
    }
    return 0;
    case ELEN_PLAYPAGE_LBTN_UP:
    {
        if (m_bPPAnActive)
        {
            PpaPrepare();
            KctWake();
        }
    }
    return 0;

    case Dui::ENC_COMMAND:
    {
        if (pEle == &m_BTPlay)
            App->Player().PlayOrPause();
        else if (pEle == &m_BTPrev)
            App->Player().Previous();
        else if (pEle == &m_BTNext)
            App->Player().Next();
        //else if (pEle == &m_BTLrc)
        else if (pEle == &m_BTAutoNext)
        {
            const auto r = App->Player().NextAutoNextMode();
            m_BTAutoNext.SetIcon(RealizeImage2(AutoNextModeToGImg(r)));
            m_BTAutoNext.Invalidate();
        }
        else if (pEle == &m_BTVol)
        {
            const auto x = GetClientWidthLogical() - CxVolBar - CxVolBarPadding;
            const auto y = m_BTVol.GetOffsetInClient().y - CyVolBar;
            m_VolBar.SetPosition(x, y);
            m_VolBar.ShowAnimation();
        }
        else if (pEle->GetId() == ELEID_PLAYPAGE_BACK)
        {
            PpaPrepare();
            KctWake();
        }
    }
    break;
    }
    return __super::OnElementNotify(pEle, pnm);
}

ID2D1Bitmap1* CWindowMain::RealizeImage(AppImage n)
{
    if (!m_vBmpRealization[(size_t)n])
    {
        GetDeviceContext()->CreateBitmapFromWicBitmap(
            App->GetImage(n).Get(),
            (const D2D1_BITMAP_PROPERTIES1*)nullptr,
            &m_vBmpRealization[(size_t)n]);
    }
    return m_vBmpRealization[(size_t)n];
}

Dui::CBitmap CWindowMain::RealizeImage2(AppImage n)
{
    Dui::CBitmap Bitmap;
    Bitmap.Set(RealizeImage(n));
    return Bitmap;
}

void CWindowMain::TlTick(int iMs) noexcept
{
    if (m_bPPAnActive)
        PpaTick(iMs);
    if (m_pAnPage)
    {
        const auto bActive = m_ecPage.Tick((float)iMs, 250.f);

        const auto x = m_pAnPage->GetRect().left;
        constexpr float yNormal = CyPageTitle + DTopPageTitle + CxPageIntPadding;
        if (m_bPageAnUpToDown)
            m_pAnPage->SetPosition(x, yNormal +
                CyPageSwitchAnDelta - m_ecPage.K);
        else
            m_pAnPage->SetPosition(x, yNormal -
                CyPageSwitchAnDelta + m_ecPage.K);
        if (!bActive)
            m_pAnPage = nullptr;
    }
}

//void CWindowMain::LwShow(BOOL bShow)
//{
//    if (bShow)
//    {
//        if (m_WndLrc.IsValid())
//            m_WndLrc.Show(SW_SHOWNOACTIVATE);
//        else
//        {
//            m_WndLrc.SetPresentMode(Dui::PresentMode::UpdateLayeredWindow);
//            m_WndLrc.SetTransparent(TRUE);
//            m_WndLrc.SetUserDpi(GetUserDpi());
//            m_WndLrc.Create(L"VioletModel - Lyrics", WS_POPUP | WS_VISIBLE,
//                WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
//                300, 400, 500, 300, Handle, nullptr);
//        }
//        m_WndLrc.Redraw();
//    }
//    else
//        if (m_WndLrc.IsValid())
//            m_WndLrc.Show(SW_HIDE);
//}
//
//BOOL CWindowMain::LwIsShowing()
//{
//    return m_WndLrc.IsValid() && m_WndLrc.IsVisible();
//}

void CWindowMain::UpdateButtonImageSize()
{
    constexpr D2D1_SIZE_F Size{ CxyCircleButtonImage, CxyCircleButtonImage };
    //m_BTPrev.SetImageSize(Size);
    //m_BTPlay.SetImageSize(Size);
    //m_BTNext.SetImageSize(Size);
    //m_BTLrc.SetImageSize(Size);
    //m_BTVol.SetImageSize(Size);
}

void CWindowMain::PpaPrepare()
{
    ECKBOOLNOT(m_bPPAnReverse);
    const auto kBegin = m_bPPAnReverse ? 0.f : 1.f;
    const auto kEnd = m_bPPAnReverse ? 1.f : 0.f;
    m_PlayPageAn.Start(kBegin, kEnd, m_bPPAnActive);
    EckCounter(4, i)
    {
        m_PPCornerAn[i].Start(kBegin, kEnd, m_bPPCornerAnActive[i]);
        m_bPPCornerAnActive[i] = TRUE;
    }
    m_bPPAnActive = TRUE;
    if (!m_bPPAnReverse)
    {
        m_NormalPageContainer.SetVisible(TRUE);
        m_PlayPanel.m_Cover.SetVisible(TRUE);
        m_PlayPanel.SetVisible(TRUE);
    }
    m_PagePlaying.SetVisible(TRUE);
    KctWake();
}

void CWindowMain::PpaEnd()
{
    m_bPPAnActive = FALSE;
    ZeroMemory(m_bPPCornerAnActive, sizeof(m_bPPCornerAnActive));
    m_PagePlaying.SetCompositor(nullptr);
    m_NormalPageContainer.SetCompositor(nullptr);
    m_NormalPageContainer.SetStyle(m_NormalPageContainer.GetStyle() &
        ~Dui::DES_BASE_BEGIN_END_PAINT);
    if (m_bPPAnReverse)
    {
        m_NormalPageContainer.SetVisible(FALSE);
        m_PlayPanel.SetVisible(FALSE);
    }
    else
        m_PagePlaying.SetVisible(FALSE);
}

void CWindowMain::PpaTick(int ms) noexcept
{
    Redraw(FALSE);
    constexpr float MinDistance = 0.4f;

    constexpr float MaxPPAnDuration = 700.f;
    constexpr float DurOverlayOpacity = 150.f;
    constexpr float Duration[]
    {
        MaxPPAnDuration,
        MaxPPAnDuration * 5 / 6,
        MaxPPAnDuration * 5 / 6,
        MaxPPAnDuration * 4 / 6,
    };
    constexpr float DurationR[]
    {
        MaxPPAnDuration * 4 / 6,
        MaxPPAnDuration * 8 / 9,
        MaxPPAnDuration * 5 / 6,
        MaxPPAnDuration,
    };
    if (!(m_bPPAnActive = m_PlayPageAn.Tick((float)ms, MaxPPAnDuration)))
    {
        Redraw(FALSE);
        PpaEnd();
        return;
    }
    // 页面动画更新
    const auto kOverlay = std::clamp(m_PlayPageAn.Time / DurOverlayOpacity, 0.f, 1.f);
    m_CompPlayPageAn.SetOpacity(m_bPPAnReverse ? (1.f - kOverlay) : kOverlay);

    const auto kScale = 1.f - m_PlayPageAn.K * 0.2f;
    const auto xRef = GetClientWidthLogical() / 2.f;
    const auto yRef = GetClientHeightLogical() / 2.f;
    m_CompNormalPageAn.SetMatrix(
        D2D1::Matrix3x2F::Translation(xRef, yRef) *
        D2D1::Matrix3x2F::Scale(kScale, kScale) *
        D2D1::Matrix3x2F::Translation(-xRef, -yRef));
    m_CompNormalPageAn.SetOpacity(1.f - m_PlayPageAn.K);

    D2D1_POINT_2F pt[4];
    const D2D1_POINT_2F ptMini[]
    {
        { m_rcPPMini.left, m_rcPPMini.top },
        { m_rcPPMini.right, m_rcPPMini.top },
        { m_rcPPMini.left, m_rcPPMini.bottom },
        { m_rcPPMini.right, m_rcPPMini.bottom },
    };
    const D2D1_POINT_2F ptLarge[]
    {
        { m_rcPPLarge.left, m_rcPPLarge.top },
        { m_rcPPLarge.right, m_rcPPLarge.top },
        { m_rcPPLarge.left, m_rcPPLarge.bottom },
        { m_rcPPLarge.right, m_rcPPLarge.bottom },
    };
    const auto pDur = m_bPPAnReverse ? DurationR : Duration;
    BOOL bStillRunning{};
    EckCounter(4, i)
    {
        m_bPPCornerAnActive[i] = m_PPCornerAn[i].Tick((float)ms, pDur[i]);
        eck::CalculatePointFromLineScale(ptMini[i].x, ptMini[i].y,
            ptLarge[i].x, ptLarge[i].y, m_PPCornerAn[i].K, pt[i].x, pt[i].y);
        if (m_bPPAnReverse)
        {
            if (fabs(ptLarge[i].x - pt[i].x) > MinDistance ||
                fabs(ptLarge[i].y - pt[i].y) > MinDistance)
                bStillRunning = TRUE;
        }
        else
        {
            if (fabs(ptMini[i].x - pt[i].x) > MinDistance ||
                fabs(ptMini[i].y - pt[i].y) > MinDistance)
                bStillRunning = TRUE;
        }
    }

    eck::CalculateDistortMatrix(m_rcPPLarge, pt,
        *(D2D1_MATRIX_4X4_F*)m_CompPlayPageAn.AtMatrix());
    eck::CalculateInverseDistortMatrix(m_rcPPLarge, pt,
        *(D2D1_MATRIX_4X4_F*)m_CompPlayPageAn.AtMatrixR());
    if (!m_PagePlaying.GetCompositor())
        m_PagePlaying.SetCompositor(&m_CompPlayPageAn);
    if (!m_NormalPageContainer.GetCompositor())
    {
        m_NormalPageContainer.SetCompositor(&m_CompNormalPageAn);
        m_NormalPageContainer.SetStyle(Dui::DES_BASE_BEGIN_END_PAINT |
            m_NormalPageContainer.GetStyle());
    }
    m_PagePlaying.CompUpdateCompositedRect();
    m_NormalPageContainer.CompUpdateCompositedRect();
    if (!bStillRunning)
        PpaEnd();
    Redraw(FALSE);
}

void CWindowMain::RePosPalyPanelControl()
{
    const auto cxClient = GetClientWidthLogical();
    const auto cyClient = GetClientHeightLogical();
    float x, y;
    // 移动右侧按钮
    x = cxClient - DRightPaddingSmallCircleBtn - CxyCircleButton;
    y = cyClient - CyPlayPanel + (CyPlayPanel - CxyCircleButton) / 2;
    m_BTVol.SetPosition(x, y);
    x -= (CxyCircleButton + CxPaddingCircleButton);
    m_BTLrc.SetPosition(x, y);
    x -= (CxyCircleButton + CxPaddingCircleButton);
    m_BTAutoNext.SetPosition(x, y);
    // 移动中间按钮
    x = (cxClient - (CxyCircleButton * 2 + CxyCircleButtonBig +
        CxPaddingCircleButton * 2)) / 2;
    y = cyClient - CyPlayPanel + DTopCtrlBtn;

    m_BTPrev.SetPosition(x, y);
    x += (CxyCircleButton + CxPaddingCircleButton);
    m_BTPlay.SetPosition(x, y + (  CxyCircleButton- CxyCircleButtonBig) / 2);
    x += (CxyCircleButtonBig + CxPaddingCircleButton);
    m_BTNext.SetPosition(x, y);
    // 移动进度条
    m_TBProgress.SetPosition((cxClient - CxProgress) / 2.f, cyClient - CyProgress - 6.f);
}

void CWindowMain::OnCoverUpdate()
{
    const auto pBmp = m_PagePlaying.m_pBmpCover;
    if (pBmp)
    {
        m_CompPlayPageAn.SetOverlayBitmap(pBmp);
        m_PlayPanel.m_Cover.SetCoverBitmap(pBmp);
    }
}

void CWindowMain::OnColorSchemeChanged()
{
    m_BTPrev.SetIcon(RealizeImage2(AppImage::Prev));
    m_BTPlay.SetIcon(RealizeImage2(AppImage::Triangle));
    m_BTNext.SetIcon(RealizeImage2(AppImage::Next));
    m_BTAutoNext.SetIcon(RealizeImage2(
        AutoNextModeToGImg(App->Player().GetAutoNextMode())));
    m_BTLrc.SetIcon(RealizeImage2(AppImage::Lrc));
    m_BTVol.SetIcon(RealizeImage2(AppImage::PlayerVolume3));
}

void CWindowMain::InvalidateRealizedImage()
{
    for (auto& e : m_vBmpRealization)
        SafeRelease(e);
}