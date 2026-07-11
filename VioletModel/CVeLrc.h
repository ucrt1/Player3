#pragma once
#include "CLyricRendererBase.h"

class CVeLyric : public Dui::CElement, public eck::ITimeLine
{
    // Mi = Mouse Idle
    // Itm = Item
    // Scr = Scroll
public:
    const static inline UINT IdCrText = Dui::TmNextResourceId();
    const static inline UINT IdCrTextActive = Dui::TmNextResourceId();

    // 当前项目的最大缩放比例
    const static inline UINT IdMeMaximumScale = Dui::TmNextResourceId();
    // 项目文本与边框间距
    const static inline UINT IdMeItemMargin = Dui::TmNextResourceId();
    // 正文与翻译间距
    const static inline UINT IdMeMainToTranslationDistance = Dui::TmNextResourceId();

    constexpr static float DefaultMaximumScale{ 1.1f };
    constexpr static float DefaultItemMargin{ 14.f };
    constexpr static float DefaultMainToTranslationDistance{ 6.f };

    enum : BYTE
    {
        SsNormal,
        SsHot,
        SsSelected,

        SsMaximum,
    };
private:
    struct ITEM
    {
        ComPtr<IDWriteTextLayout> pTlMain{};
        ComPtr<IDWriteTextLayout> pTlTranslation{};

        float x{};
        float y{};
        float cy{};
        float cx{};

        float cxMain{};
        float cxTranslation{};

        float yNoDelay{};   // 没有延迟的情况下歌词行的Y坐标

        float kAnSelBkg{};  // 0~1
        float msAnSelBkg{}; // 0~DurationSelectionBack
        float yAnDelayDst{};// 目标位置
        float yAnDelaySrc{};// 起始位置
        float msAnDelay{};  // 0~DurationDelay，延迟动画曲线用
        float msDelay{};    // 0~DurationMaxItemDelay，已延迟时间，达到最大值时开始进行移动动画
        BITBOOL bSel : 1{};             // 选中
        BITBOOL bCacheValid : 1{};      // 前三个字段是否有效
        BITBOOL bAnSelBkg : 1{};        // 正在运行项目热点背景动画
        BITBOOL bAnSelBkgEnlarge : 1{}; // 正在扩大，即鼠标已移入

        void OnSetHot();
        void OnKillHot();
    };

    Dui::CScrollBar m_SB{};

    std::unique_ptr<CLyricRendererBase> m_pRenderer{};
    ComPtr<IDWriteTextFormat> m_pTfTranslation{};

    RefPtr<Lyric::CLyric> m_pLrc{};
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
    float m_msItemAnDelay{ 200.f }; // 当前行发生更改时歌词行之间开始动画的延迟
    float m_cyLinePadding{ 10.f };  // 项目间距

    int m_msLastInterval{};

    eck::Alignment m_eAlignH{ eck::Alignment::Near };// 水平对齐

    BOOLEAN m_bAnSelBkg{};          // 正在运行项目热点背景动画
    BOOLEAN m_bEnlarging{};         // 正在放大当前歌词行
    BOOLEAN m_bScrollExpand{};      // 由于用户滚动操作，所有歌词行都正在放大或缩小
    BOOLEAN m_bSeEnlarging{};       // 指示ScrollExpand的操作是否为放大
    BOOLEAN m_bEnableItemAnDelay{}; // 启用项目动画延迟
    BOOLEAN m_bItemAnDelay{};       // 当前正在运行项目动画延迟
    BOOLEAN m_bDelayScrollUp{};     // 指示项目是否向上运动

    // 忽略前景颜色，因为他们由IdCrText*指定
    Dui::SimpleStyle m_Style[SsMaximum]
    {
        { Dui::IdTmInvalid, Dui::IdTmInvalid, Dui::IdTmInvalid },
        { Dui::IdTmInvalid, Dui::IdCrBackHot, Dui::IdTmInvalid },
        { Dui::IdTmInvalid, Dui::IdCrBackPressed, Dui::IdTmInvalid },
    };


    void ItmReCalculateTop() noexcept;
    // 返回项目底边加行间距，WM_PAINT使用此值判断重画终止位置
    float ItmPaint(int idx) noexcept;
    int ItmIndexFromY(float y) const noexcept;
    void ItmLayout() noexcept;
    D2D1_RECT_F ItmGetRect(int idx) const noexcept
    {
        const auto& e = m_vItem[idx];
        return { e.x, e.y, e.x + e.cx, e.y + e.cy };
    }
    // 取当前项目中线所在Y坐标
    EckInlineNdCe float ItmGetCurrentItemTarget() const noexcept { return GetHeight() / 3.f; }

    void ItmInvalidate(int idx) noexcept;

    void ScrAnimationCallback(const Dui::IScrollController::SCC_CALLBACK_DATA& Data) noexcept;
    void ScrAutoScrolling() noexcept;
    void ScrManualScrolling() noexcept;
    void ScrDoItemScroll(float fPos) noexcept;
    void ScrFixItemPosition() noexcept;

    void MiBeginDetect() noexcept;
    EckInlineNdCe BOOL MiIsManualScroll() const noexcept { return m_tMouseIdle > 0; }
    void MiBeginScrollExpand(BOOL bEnlarge) noexcept;

    void ItmDelayPrepare(float dy) noexcept;
    void ItmDelayComplete() noexcept;
    EckInlineNdCe int ItmInDelayRange(int idx) const noexcept
    {
        return idx >= m_idxDelayBegin && idx <= m_idxDelayEnd;
    }
    EckInlineNdCe BOOL ItmIsDelaying() const noexcept
    {
        return m_bItemAnDelay && m_idxDelayBegin >= 0 &&
            m_idxDelayEnd >= m_idxDelayBegin;
    }

    BOOL ItmIsDelayEnd(const ITEM& e) noexcept;
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

    // 调用方启动定时器检查当前时间对应的歌词行，每次检查后将结果传递到此方法
    HRESULT LrcSetCurrentLine(int idxCurr) noexcept;
    HRESULT LrcInitialize(RefPtr<Lyric::CLyric> pLyric) noexcept;
    void LrcClear() noexcept;

    void SetTextFormatTranslation(IDWriteTextFormat* pTf) noexcept;

    int ItmHitTest(Kw::Vec2 pt) noexcept;

    void TlTick(int iMs) noexcept override;
    BOOL TlIsValid() noexcept override { return m_bAnSelBkg || m_bItemAnDelay; }
    int TlGetCurrentInterval() noexcept override { return m_msLastInterval; }

    EckInlineNdCe BOOL IsEmpty() const noexcept { return m_vItem.empty(); }

    EckInlineNdCe auto& GetScrollBar() noexcept { return m_SB; }

    EckInlineNdCe auto& TmSimpleStyle(UINT ss) const noexcept { return m_Style[ss]; }
    EckInlineNdCe auto& TmSimpleStyle(UINT ss) noexcept { return m_Style[ss]; }
};

class CThemeLyric : public Dui::CThemeBase
{
private:

public:

};