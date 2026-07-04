#pragma once
#include "LrDef.h"

struct LDRD_DRAW
{
    int idxLine;
    float fCurrTime;
    BOOLEAN bHiLight;
};

class CDtLyricRendererBase : public eck::CRefObj<CDtLyricRendererBase>
{
public:
    enum : size_t
    {
        CriNormal,
        CriHiLight,
        CriTransNormal,
        CriTransHiLight,
        CriShadow,
        CriBorder,
        CriMax
    };
protected:
    ComPtr<IDWriteTextFormat> m_pTfMain{};
    ComPtr<IDWriteTextFormat> m_pTfTrans{};

    D2D1_COLOR_F m_Color[CriMax]{};

    float m_cxView{};
    float m_cyView{};

    float m_cxOutline{ 2.f };
    float m_cyLinePadding{ 4.f };
    float m_dxyShadow{ 3.f };

    BOOLEAN m_bStaticLine{};		// 绘制静态行，当无歌词时设置为TRUE
    BOOLEAN m_bTooLong{};			// 歌词太长，需要滚动显示

    BOOLEAN m_bAutoWrap{};			// 自动换行
    BOOLEAN m_bShowTrans{ TRUE };	// 显示翻译
    BOOLEAN m_bShadow{ TRUE };		// 显示阴影
    eck::Alignment m_eAlign[2]{ eck::Alignment::Near,eck::Alignment::Far };
public:
    void SetTextFormatMain(IDWriteTextFormat* pTf) { m_pTfMain = pTf; }
    void SetTextFormatTrans(IDWriteTextFormat* pTf) { m_pTfTrans = pTf; }

    void SetColor(size_t idx, const D2D1_COLOR_F& cr) { m_Color[idx] = cr; }
    void SetColor(_In_reads_(CriMax) const D2D1_COLOR_F* pcr)
    {
        for (size_t i = 0; i < CriMax; ++i)
            m_Color[i] = pcr[i];
    }

    virtual HRESULT LrInit(const LRD_INIT& Opt) = 0;
    virtual void LrBeginDraw() = 0;
    virtual void LrEndDraw() = 0;
    virtual void LrDraw(const LDRD_DRAW& Opt) = 0;
    virtual HRESULT LrUpdateText(int idx, const Lyric::Line& Line,
        _Out_ LRD_TEXT_METRICS& Met) = 0;
    virtual void LrSetViewSize(float cx, float cy) = 0;
    virtual void LrDpiChanged(float fNewDpi) = 0;
    virtual void LrInvalidate(BOOL bSecondLine) = 0;
};