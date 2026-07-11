#pragma once

struct LRD_TEXT_METRICS
{
    float cxMain{};
    float cyMain{};
    float cxTranslation{};
    float cyTranslation{};
};

enum : BYTE
{
    LRIF_NONE = 0,
    LRIF_AN_SEL_BKG = 1 << 0,
    LRIF_PREV_AN = 1 << 1,
    LRIF_CURR_AN = 1 << 2,
    LRIF_SCROLL_EXPAND = 1 << 3,

    LRCF_TOP_BOTTOM_FADE = 1 << 0,
};

// 如无特殊说明，坐标相对元素
struct LRD_DRAW
{
    int idx{};

    BYTE uFlags{};      // LRIF_*
    eck::Alignment eAlignH{};
    BYTE ss{};

    float x;
    float y;
    float cx;
    float cy;

    float cxMain;
    float cxTranslation;

    float fScale;       // 当前缩放，对于常规情况必须设为1
    float kAnSelBkg;    // 0~1，指定LRIF_AN_SEL_BKG时有效
    float kScrollExpand;// 0~1，指定LRIF_SCROLL_EXPAND时有效

    ComPtr<IDWriteTextLayout> pTlMain{};
    ComPtr<IDWriteTextLayout> pTlTranslation{};

    const D2D1_RECT_F* prcClip{};// 相对客户区
};

struct LRD_EMTRY_TEXT
{
    std::wstring_view svText;
};

class CLyricRendererBase
{
private:
    // 渲染器通常使用宿主元素主题的Draw方法
    // 如果不使用，渲染结果也应该尽量与Ss对齐
    Dui::CElement* m_pEle{};

    float m_cxView{};
    float m_cyView{};

    UINT m_uFlags{};// LRCF_*
public:
    virtual ~CLyricRendererBase() = default;

    // 宿主构造渲染器完毕后立即调用
    virtual HRESULT LrInitialize(Dui::CElement* pEle) noexcept
    {
        m_pEle = pEle;
        return S_OK;
    }

    // 绘画通知
    virtual void LrBeginDraw() noexcept = 0;
    virtual void LrEndDraw() noexcept = 0;

    // 当元素尺寸改变时调用此方法
    virtual void LrSetViewSize(float cx, float cy) noexcept
    {
        m_cxView = cx;
        m_cyView = cy;
    }

    virtual void LrSetItemCount(int cItems) noexcept = 0;
    virtual void LrDrawItem(const LRD_DRAW& Opt) noexcept = 0;

    virtual void LrDpiChanged(float fNewDpi) noexcept = 0;

    // 清除所有缓存
    virtual void LrInvalidate() noexcept = 0;

    virtual void LrSetFlags(UINT uFlags) noexcept { m_uFlags = uFlags; }

    EckInlineNdCe float GetViewWidth() const noexcept { return m_cxView; }
    EckInlineNdCe float GetViewHeight() const noexcept { return m_cyView; }
    EckInlineNdCe UINT GetFlags() const noexcept { return m_uFlags; }
    EckInlineNdCe Dui::CElement* GetHostElement() const noexcept { return m_pEle; }
};