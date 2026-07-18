#pragma once
#include "CPlayer.h"
#include "CPlayListMgr.h"

constexpr PCWSTR MainWndPageName[]
{
    L"主页",
    L"列表",
    L"效果",
    L"设置",
};

constexpr static float
CxTabPanel = 48,
CyPlayPanel = 90,
CxyWndLogo = 18,
CyTitleBar = 40,
DTopPageTitle = 12,
CxPageTitle = 120,
CyPageTitle = 34,
CxPageIntPadding = 10,
CxTabToPagePadding = 14,
CxListFileList = 170,
CxProgress = 350,
CyProgress = 20,
CyProgressTrack = 8,
DLeftMiniCover = 40,
DTopMiniCover = 10,
CxyMiniCover = 70,
CxyPlayPageArrow = 30,
CxPaddingPlayPanelText = 20,
DTopTitle = 18,
CyPlayPanelText = 20,
CyPaddingTitleAndArtist = 14,
CxMaxTitleAndArtist = 130,
DTopTime = 40,
CxMaxTime = 110,
CxyCircleButton = 34,
CxyCircleButtonBig = 44,
CxPaddingCircleButton = 24,
CxyCircleButtonImage = 18,
CyPageSwitchAnDelta = 60,
DRightPaddingSmallCircleBtn = 44,
DTopCtrlBtn = 18,
CyFontNormal = 14,
CyFontPageTitle = 20,
CyStdList = 32,
CyStdEdit = 37,
CxListPageButton = 110,
CxyListCover = 40,
CyPlayListItem = 46,
CyVolTB = 16,
CyVolTrack = 8,
CxVolBar = 220,
CyVolBar = 32,
CxVolLabel = 30,
CxVolBarPadding = 8,
DVolAn = 10,
DLrcTop = 50,
DLrcBottom = 135,
CxyBackBtn = 40,
CyPlayPageLabelCoverPadding = 24,
CyPlayPageLabelPadding = 4,
CyPlayPageLabel = 26,
CyFontPlayPageLabel = 18,
CxyLrcBtn = 30,
CxyLrcPadding = 8,
DWndLogoToTab = 20
;

// 所有的ID，包括窗口定时器、WM_COMMAND、控件ID等
enum
{
    VIOLET_ID_BEGIN = 0x514B,

    IDT_COMM_TICK,
    IDT_LRC_MOUSELEAVE,


    IDTBB_PREV,
    IDTBB_PLAY,
    IDTBB_NEXT,

    ELEID_PLAYPAGE_BACK,
    ELEID_VOLBAR_TRACK,

    TE_COMM_TICK = 200,
    TE_PROG = TE_COMM_TICK * 2,
    TE_LRC_MOUSELEAVE = 800,
    TE_LRC_MOUSELEAVE_FIRST = 1600,
};

enum
{
    ELEN_PLACEHOLDER = Dui::ENC_PRIVATE_BEGIN,
    ELEN_PAGE_CHANGE,		// [CTabPanel]边栏被单击时(NMLTITEMINDEX*)
    ELEN_MINICOVER_CLICK,	// [CMiniCover]封面被单击时
    ELEN_DTLRC_GET_TIME,	// [CVeDtLrc]取当前播放器时间(NM_DTL_GET_TIME*)
    ELEN_PLAYPAGE_LBTN_UP,	// [CPagePlaying]左键弹起
};

class CWindowMain;
class CApplication
{
private:
    CPlayer m_Player{};
    CPlayListManager m_ListManager{};

    eck::CDWriteFontFactory m_FontFactory{};

    eck::ThreadContext* m_ptcUiThread{};
    CWindowMain* m_pWndMain{};
public:
    CApplication();

    static void Init();

    EckInlineNdCe auto& Player() { return m_Player; }
    EckInlineNdCe auto& ListManager() { return m_ListManager; }
    EckInlineNdCe auto& GetFontFactory() { return m_FontFactory; }
    EckInlineNdCe auto& GetMainWindow() { return *m_pWndMain; }

    EckInlineNdCe auto UiThreadContext() const { return m_ptcUiThread; }

    EckInlineCe void SetMainWindow(CWindowMain* pWnd) { m_pWndMain = pWnd; }
};

extern CApplication* App;