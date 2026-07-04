#include "pch.h"
#include "CVeDtLrc.h"
#include "CApp.h"


LRESULT CVeDtLrc::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        Dui::ELEMPAINTSTRU ps;
        BeginPaint(ps, wParam, lParam);
        //if (m_idxCurr >= 0)
        //{
        //	float y{}, cy;
        //	if (m_idxCurr % 2)
        //	{
        //		cy = DrawLrcLine(m_idxCurr, y, FALSE);
        //		if (m_idxCurr + 1 < GetLyric()->MgGetLineCount())
        //			DrawLrcLine(m_idxCurr + 1, y + cy + m_cyLinePadding, TRUE);
        //	}
        //	else
        //	{
        //		int idx = m_idxCurr + 1;
        //		if (idx >= GetLyric()->MgGetLineCount())
        //			idx = m_idxCurr - 1;

        //		if (idx >= 0 && idx < GetLyric()->MgGetLineCount())
        //			cy = DrawLrcLine(idx, y, FALSE);
        //		else
        //			cy = 0.f;
        //		DrawLrcLine(m_idxCurr, y + cy + m_cyLinePadding, TRUE);
        //	}
        //}
        //else
        //	DrawStaticLine(0.f);

        ECK_DUI_DBG_DRAW_FRAME;
        EndPaint(ps);
    }
    return 0;
    case WM_CREATE:
    {
        //GetWindow().RegisterTimeLine(this);

        //m_pDC->QueryInterface(&m_pDC1);
        //InvalidateCache();

        //ComPtr<ID2D1SolidColorBrush> pBr;
        //m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBr);
        //m_pBrush[BriBorder] = pBr.Detach();

        //m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBr);
        //m_pBrush[BriMain] = pBr.Detach();

        //m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pBr);
        //m_pBrush[BriMainHiLight] = pBr.Detach();

        //m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &pBr);
        //m_pBrush[BriTrans] = pBr.Detach();

        //m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &pBr);
        //m_pBrush[BriTransHiLight] = pBr.Detach();

        //m_pDC1->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray, 0.4f), &pBr);
        //m_pBrush[BriShadow] = pBr.Detach();
    }
    break;
    case WM_DESTROY:
    {

        SafeRelease(m_pLrc);
    }
    break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}

void CVeDtLrc::TlTick(int iMs) noexcept
{

}

HRESULT CVeDtLrc::LrcSetCurrentLine(int idx)
{
    if (m_idxCurr == idx)
        return S_FALSE;
    m_idxCurr = idx;
    //InvalidateCache();
    Invalidate();
    return S_OK;
}

void CVeDtLrc::LrcSetEmptyText(std::wstring_view svEmptyText)
{

}

void CVeDtLrc::SetTextFormatTrans(IDWriteTextFormat* pTf)
{

}

void CVeDtLrc::SetLyric(Lyric::CLyric* pLrc)
{
    ECK_DUILOCK;
    m_idxCurr = -1;
    std::swap(m_pLrc, pLrc);
    if (m_pLrc)
        m_pLrc->AddRef();
    if (pLrc)
        pLrc->Release();
}