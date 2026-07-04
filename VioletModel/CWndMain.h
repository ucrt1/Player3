#pragma once
#include "CApp.h"
#include "CPage.h"
#include "CPageMain.h"
#include "CPageList.h"
#include "CPageEffect.h"
#include "CPageOptions.h"
#include "CPagePlaying.h"
#include "CTabPanel.h"
#include "CPlayPanel.h"
#include "CCompPlayPageAn.h"
#include "CWndTbGhost.h"
#include "CVeVolumeBar.h"
#include "CVioletTheme.h"
#include "CWndLrc.h"

class CWindowMain final : public Dui::CDuiWindow, public eck::ITimeLine
{
    friend class CWndTbGhost;
public:
    enum class Page : BYTE
    {
        Main,
        List,
        Effect,
        Options,
        Max
    };
    constexpr static double ProgBarScale = 100.;
private:
    Dui::CTitleBar m_TitleBar{};
    Dui::CLabel m_LAPageTitle{};

    CTabPanel m_TabPanel{};
    CPlayPanel m_PlayPanel{};

    CPageMain m_PageMain{};
    CPageList m_PageList{};
    CPageEffect m_PageEffect{};
    CPageOptions m_PageOptions{};
    Dui::CElement m_NormalPageContainer{};// 所有下层页面的父级，动画时用

    CPagePlaying m_PagePlaying{};

    Dui::CTrackBar m_TBProgress{};

    Dui::CButton m_BTPrev{}, m_BTPlay{}, m_BTNext{},
        m_BTAutoNext{}, m_BTLrc{}, m_BTVol{};

    CVeVolumeBar m_VolBar{};

    ID2D1Bitmap1* m_vBmpRealization[(size_t)AppIcon::Max]{};

    ID2D1Bitmap1* m_pBmpCover{};

    CPage* m_vPage[(size_t)Page::Max]
    {
        &m_PageMain,
        &m_PageList,
        &m_PageEffect,
        &m_PageOptions
    };

    CPage* m_pAnPage{};
    eck::EasingCurve<eck::Easing::FOutCubic> m_ecPage{};

    BOOLEAN m_bPageAnUpToDown{};
    Page m_eCurrPage{};

    BOOLEAN m_bPPAnReverse{};// TRUE = 小到大，FALSE = 大到小
    BOOLEAN m_bPPAnActive{};
    BOOLEAN m_bPPCornerAnActive[4]{};
    eck::EasingCurve<eck::Easing::FOutExpo> m_PlayPageAn;
    eck::EasingCurve<eck::Easing::FOutExpo> m_PPCornerAn[4]{};
    D2D1_RECT_F m_rcPPMini{};
    D2D1_RECT_F m_rcPPLarge{};
    CCompPlayPageAn m_CompPlayPageAn{};

    Dui::CCompositor2DAffineTransform m_CompNormalPageAn{};

    eck::ThreadContext* m_ptcUiThread{};

    HICON m_hiTbPlay{}, m_hiTbPause{};
    ComPtr<ITaskbarList4> m_pTaskbarList{};
    CWndTbGhost m_WndTbGhost{ *this };

    CWndLrc m_WndLrc{};

    CVioletTheme* m_pVioletTheme{ new CVioletTheme{} };

#if VIOLET_WINRT
    WinMedia::SystemMediaTransportControls m_Smtc{ nullptr };
    WinMedia::SystemMediaTransportControlsTimelineProperties m_SmtcTimeline{};
    eck::CoroTask<> m_TskSmtcUpdateDisplay{};	// winrt无法在UI线程等待任务，因此将其在此协程中运行
    ULONGLONG m_ullSmtcTimeLineLastUpdate{};	// 上次更新时间线的时间戳，5s一更新
    winrt::event_token m_SmtcEvtTokenButtonPressed{};	// 反初始化时使用
#endif

    int m_msProgTimer{};
private:
    void ClearRes();

    BOOL OnCreate(HWND hWnd, CREATESTRUCT* pcs);

    void PageShow(Page ePage, BOOL bAnimate);
    void PageClearAnimation();

    void OnPlayEvent(const PLAY_EVT_PARAM& e);

    void UpdateButtonImageSize();

    void PpaPrepare();
    void PpaEnd();
    void PpaTick(int ms) noexcept;

    void RePosPalyPanelControl();

    void OnCoverUpdate();

    void OnColorSchemeChanged();

    void InvalidateRealizedImage();

    HRESULT TblCreateGhostWindow(PCWSTR pszText);
    HRESULT TblSetup();
    HRESULT TblUpdateToolBarIcon();
    HRESULT TblCreateObjectAndInit();
    BOOL TblOnCommand(WPARAM wParam);
    HRESULT TblUpdateState();
    HRESULT TblUpdateProgress();
    HRESULT TblOnTaskbarButtonCreated();

    HRESULT SmtcInit() noexcept;
#if VIOLET_WINRT
    eck::CoroTask<> SmtcpCoroUpdateDisplay();
#endif
    HRESULT SmtcUpdateDisplay() noexcept;
    HRESULT SmtcOnCommonTick() noexcept;
    HRESULT SmtcUpdateTimeLineRange() noexcept;
    HRESULT SmtcUpdateTimeLinePosition() noexcept;
    HRESULT SmtcUpdateState() noexcept;
    void SmtcUnInit() noexcept;
public:
    HWND Create(PCWSTR pszText, DWORD dwStyle, DWORD dwExStyle,
        int x, int y, int cx, int cy, HWND hParent, HMENU hMenu, PCVOID pData = nullptr) noexcept override;

    LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

    LRESULT OnElementNotify(Dui::CElement* pEle, Dui::ELENMHDR* pnm) noexcept override;

    ID2D1Bitmap1* RealizeImage(AppIcon n);

    void TlTick(int iMs) noexcept override;
    BOOL TlIsValid() noexcept override { return m_bPPAnActive; }
    int TlGetCurrentInterval() noexcept override { return 0; }

    EckInlineNdCe auto ThreadCtx() const noexcept { return m_ptcUiThread; }
    EckInlineNdCe auto GetVioletTheme() const noexcept { return m_pVioletTheme; }

    void LwShow(BOOL bShow);
    BOOL LwIsShowing();
};