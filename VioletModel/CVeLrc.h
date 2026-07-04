#pragma once
#include "CLyricRendererBase.h"

class CVeLrc : public Dui::CElement, public eck::CFixedTimeLine
{
    // Isb = Item Select Background
    // Mi = Mouse Idle
    // Se = Scroll Expand
    // Itm = Item
    // Scr = Scroll
public:
    enum : size_t
    {
        CriNormal,
        CriHiLight,
        CriMax
    };
private:
    struct ITEM
    {
        // 不要修改前两个字段的位置
        ComPtr<IDWriteTextLayout> pLayout{};
        ComPtr<IDWriteTextLayout> pLayoutTrans{};
        ComPtr<ID2D1GeometryRealization> pGr{};
        float x{};
        float y{};
        float cy{};
        float cx{};
        float cxTrans{};
        float yNoDelay{};	// 没有延迟的情况下歌词行的Y坐标

        float kAnSelBkg{};	// 0~1
        float msAnSelBkg{};	// 0~AnDurLrcSelBkg
        float yAnDelayDst{};// 目标位置
        float yAnDelaySrc{};// 起始位置
        float msAnDelay{};	// 0~AnDurLrcDelay，延迟动画曲线用
        float msDelay{};	// 0~DurMaxItemDelay，已延迟时间，达到最大值时开始进行移动动画
        BITBOOL bSel : 1{};				// 选中
        BITBOOL bCacheValid : 1{};		// 前三个字段是否有效
        BITBOOL bAnSelBkg : 1{};		// 正在运行项目热点背景动画
        BITBOOL bAnSelBkgEnlarge : 1{};	// 正在扩大，即鼠标已移入

        void OnSetHot();
        void OnKillHot();
    };

    Dui::CScrollBar m_SB{};
    eck::CInertialScrollView* m_psv{};

    CLyricRendererBase* m_pRenderer{};

    Lyric::CLyric* m_pLrc{};
    std::vector<ITEM> m_vItem{};
    int m_idxTop{ -1 };
    int m_idxHot{ -1 };
    int m_idxMark{ -1 };
    int m_idxCurr{ -1 };

    int m_idxPrevAnItem{ -1 },
        m_idxCurrAnItem{ -1 };
    float m_fAnValue{ 1.f };

    float m_msScrollExpand{};
    float m_kScrollExpand{};

    int m_tMouseIdle{};

    int m_idxDelayBegin{ -1 };
    int m_idxDelayEnd{ -1 };

    float m_yMinMaxDelayPos{};
    float m_msItemAnDelay{ 200.f };		// 当前行发生更改时歌词行之间开始动画的延迟
    float m_cyLinePadding{ 10.f };		// 项目间距
    eck::Alignment m_eAlignH{ eck::Alignment::Near };	// 水平对齐

    BOOLEAN m_bAnSelBkg{};		// 正在运行项目热点背景动画
    BOOLEAN m_bEnlarging{};		// 正在放大当前歌词行
    BOOLEAN m_bScrollExpand{};	// 由于用户滚动操作，所有歌词行都正在放大或缩小
    BOOLEAN m_bSeEnlarging{};	// 指示ScrollExpand的操作是否为放大
    BOOLEAN m_bEnableItemAnDelay{};	// 启用项目动画延迟
    BOOLEAN m_bItemAnDelay{};		// 当前正在运行项目动画延迟
    BOOLEAN m_bDelayScrollUp{};		// 指示项目是否向上运动


    void ScrAnProc(float fPos, float fPrevPos);

    void ItmReCalcTop();
    // 返回项目底边加行间距，WM_PAINT使用此值判断重画终止位置
    float ItmPaint(int idx);
    void ItmGetRect(int idx, _Out_ D2D1_RECT_F& rc);
    int ItmIndexFromY(float y);
    void ItmLayout();
    // 取当前项目中线应该在的坐标
    EckInlineNdCe float ItmGetCurrentItemTarget() const { return GetHeightF() / 3.f; }

    void ItmInvalidate(int idx);

    void ScrAutoScrolling();
    void ScrManualScrolling();
    void ScrDoItemScroll(float fPos);
    void ScrFixItemPosition();

    void IsbWakeRenderThread()
    {
        if (!m_bAnSelBkg)
        {
            m_bAnSelBkg = TRUE;
            GetWnd()->KctWake();
        }
    }

    void MiBeginDetect();
    EckInlineNdCe BOOL MiIsManualScroll() { return m_tMouseIdle > 0; }

    void SeBeginExpand(BOOL bEnlarge);

    void ItmDelayPrepare(float dy);
    void ItmDelayComplete();
    EckInlineNdCe int ItmInDelayRange(int idx) const { return idx >= m_idxDelayBegin && idx <= m_idxDelayEnd; }
    EckInlineNdCe BOOL ItmIsDelaying() const
    {
        return m_bItemAnDelay && m_idxDelayBegin >= 0 &&
            m_idxDelayEnd >= m_idxDelayBegin;
    }

    BOOL ItmIsDelayEnd(const ITEM& e);
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

    // 仅在创建控件前调用一次
    void LrcSetRenderer(CLyricRendererBase* pRenderer)
    {
        EckAssert(!CElem::IsValid() && !m_pRenderer);
        m_pRenderer = pRenderer;
        m_pRenderer->AddRef();
    }

    void LrcSetColor(_In_reads_(CriMax) const D2D1_COLOR_F* pcr) { m_pRenderer->SetColor(pcr); }

    // 调用方启动定时器检查当前时间对应的歌词行，每次检查后将结果传递到此方法
    HRESULT LrcSetCurrentLine(int idxCurr);

    HRESULT LrcInit(Lyric::CLyric* pLyric);

    void LrcClear();

    void SetTextFormatTrans(IDWriteTextFormat* pTf);

    int ItmHitTest(POINT pt);

    void TlTick(int iMs) noexcept override;
    BOOL TlIsValid() noexcept override { return m_bAnSelBkg || m_bItemAnDelay; }

    EckInlineNdCe BOOL IsEmpty() const { return m_vItem.empty(); }

    EckInlineNdCe auto& GetScrollBar() { return m_SB; }
};