#pragma once
#include "LrDef.h"

enum : BYTE
{
    LRIF_NONE = 0,
    LRIF_AN_SEL_BKG = 1 << 0,
    LRIF_PREV_AN = 1 << 1,
    LRIF_CURR_AN = 1 << 2,
    LRIF_SCROLL_EXPAND = 1 << 3,
};
struct LRD_DRAW
{
    int idx;

    BYTE uFlags;			// LRIF_*
    eck::Alignment eAlignH;
    Dui::State eState;      // 背景状态，设为State::None表示不绘制背景

    float x;
    float y;
    float cx;
    float cy;

    float fScale;           // 当前缩放，对于常规情况必须设为1
    float kAnSelBkg;		// 0~1，指定LRIF_AN_SEL_BKG时有效
    float kScrollExpand;	// 0~1，指定LRIF_SCROLL_EXPAND时有效
};

struct LRD_EMTRY_TEXT
{
    std::wstring_view svText;
};

class CLyricRendererBase : public eck::CRefObj<CLyricRendererBase>
{
public:
    enum : size_t
    {
        CriNormal,
        CriHiLight,
        CriMax
    };
protected:
    ComPtr<IDWriteTextFormat> m_pTfMain{};
    ComPtr<IDWriteTextFormat> m_pTfTrans{};

    ComPtr<Dui::ITheme> m_pTheme{};

    D2D1_COLOR_F m_Color[CriMax]{};

    float m_cxView{};
    float m_cyView{};
    float m_fMaxScale{ 1.1f };      // 当前项目的最大缩放比例
    float m_cxyLineMargin{ 14.f };  // 项目文本与边框间距
    float m_dMainToTrans{ 5.f };    // 正文与翻译间距

    BOOLEAN m_bTopBtmFade{};		// 指示是否启用顶部和底部渐变
public:
    virtual ~CLyricRendererBase() = default;

    void SetTheme(Dui::ITheme* pTheme) { m_pTheme = pTheme; }
    void SetTextFormatMain(IDWriteTextFormat* pTf) { m_pTfMain = pTf; }
    void SetTextFormatTrans(IDWriteTextFormat* pTf) { m_pTfTrans = pTf; }

    void SetColor(size_t idx, const D2D1_COLOR_F& cr) { m_Color[idx] = cr; }
    void SetColor(_In_reads_(CriMax) const D2D1_COLOR_F* pcr)
    {
        for (size_t i = 0; i < CriMax; ++i)
            m_Color[i] = pcr[i];
    }

    void SetMaxScale(float f) { m_fMaxScale = f; }
    float GetMaxScale() const { return m_fMaxScale; }
    void SetItemMargin(float f) { m_cxyLineMargin = f; }
    float GetItemMargin() const { return m_cxyLineMargin; }
    void SetMainToTransDistance(float f) { m_dMainToTrans = f; }
    float GetMainToTransDistance() const { return m_dMainToTrans; }

    virtual HRESULT LrInit(const LRD_INIT& Opt) = 0;
    virtual void LrBeginDraw() = 0;
    virtual void LrEndDraw() = 0;
    virtual void LrItmSetCount(int cItems) = 0;
    virtual HRESULT LrItmUpdateText(int idx, const Lyric::Line& Line,
        _Out_ LRD_TEXT_METRICS& Met) = 0;
    virtual void LrItmDraw(const LRD_DRAW& Opt) = 0;
    virtual HRESULT LrUpdateEmptyText(const LRD_EMTRY_TEXT& Opt) = 0;
    virtual void LrSetViewSize(float cx, float cy) = 0;
    virtual void LrDpiChanged(float fNewDpi) = 0;
    virtual void LrInvalidate() = 0;
};