#include "pch.h"
#include "CVeLrc.h"

enum
{
    IDT_MOUSEIDLE = 1001,
    TE_MOUSEIDLE = 500,
    T_MOUSEIDLEMAX = 4500,
};

constexpr inline float AnDurLrcSelBkg{ 100.f };         // 歌词选中背景动画时长
constexpr inline float AnDurLrcScrollExpand{ 200.f };   // 滚动展开动画时长
constexpr inline float AnDurLrcDelay{ 600.f };          // 每个项目的延迟动画时长
constexpr inline float DurMaxItemDelay{ 310.f };        // 延迟间隔

void CVeLrc::ScrAnProc(float fPos, float fPrevPos)
{
    if (IsEmpty())
        return;
    const auto iMs = m_psv->TlGetCurrentInterval();
    if (m_bEnlarging)
    {
        m_fAnValue = eck::Easing::OutCubic(m_psv->GetCurrentTime(),
            1.f, m_pRenderer->GetMaxScale() - 1.f, m_psv->GetDuration());
        if (m_psv->GetCurrentTime() >= m_psv->GetDuration())
        {
            m_bEnlarging = FALSE;
            m_idxPrevAnItem = -1;
            m_fAnValue = m_pRenderer->GetMaxScale();
        }
    }

    EckAssert(AnDurLrcScrollExpand < (float)m_psv->GetDuration());
    if (m_bScrollExpand)
    {
        if (m_bSeEnlarging)
            m_msScrollExpand += iMs;
        else
            m_msScrollExpand -= iMs;
        m_kScrollExpand = eck::Easing::Linear(m_msScrollExpand, 1.f,
            m_pRenderer->GetMaxScale() - 1.f, AnDurLrcScrollExpand);
        if (m_msScrollExpand >= AnDurLrcScrollExpand ||
            m_msScrollExpand <= 0.f)
        {
            m_bScrollExpand = FALSE;
            m_kScrollExpand = m_bSeEnlarging ? m_pRenderer->GetMaxScale() : 1.f;
            m_msScrollExpand = m_bSeEnlarging ? AnDurLrcScrollExpand : 0.f;
        }
    }

    ScrDoItemScroll(fPos);

    ItmReCalcTop();
    Invalidate();
}

void CVeLrc::ItmReCalcTop()
{
    if (IsEmpty())
    {
        m_idxTop = 0;
        return;
    }
    m_idxTop = ItmIndexFromY(0.f);
    // VLTBUG 250804
    // 先前调整过ItmIndexFromY使坐标实际上命中项目顶边减去行间距，
    // 可能导致项目顶边可见，但减去行间距后不可见
    // 追加：即使未调整此函数，仍有可能出现项目顶边不可见的情况
    // 此处命中不到任何项目的情况下，总使顶端项目为最后一个项目
    if (m_idxTop < 0)
        m_idxTop = (int)m_vItem.size() - 1;
}

void CVeLrc::MiBeginDetect()
{
    if (!m_tMouseIdle)
        SetTimer(IDT_MOUSEIDLE, TE_MOUSEIDLE);
    m_tMouseIdle = T_MOUSEIDLEMAX;
}

int CVeLrc::ItmHitTest(POINT pt)
{
    D2D1_RECT_F rc;
    for (int i = m_idxTop; i < (int)m_vItem.size(); ++i)
    {
        ItmGetRect(i, rc);
        if (eck::PtInRect(rc, pt))
            return i;
        if (rc.top > GetHeight())
            break;
    }
    return -1;
}

float CVeLrc::ItmPaint(int idx)
{
    const auto& e = m_vItem[idx];
    LRD_DRAW Opt
    {
        .idx = idx,
        .eAlignH = m_eAlignH,
        .x = e.x,
        .y = e.y,
        .cx = e.cx,
        .cy = e.cy,
        .fScale = m_fAnValue,
        .kAnSelBkg = e.kAnSelBkg,
        .kScrollExpand = m_kScrollExpand,
    };
    if (e.bAnSelBkg)
        Opt.uFlags |= LRIF_AN_SEL_BKG;
    if (m_bScrollExpand || MiIsManualScroll())
        Opt.uFlags |= LRIF_SCROLL_EXPAND;

    if (idx == m_idxPrevAnItem)
        Opt.uFlags |= LRIF_PREV_AN;
    else if (idx == m_idxCurrAnItem)
        Opt.uFlags |= LRIF_CURR_AN;

    if (e.bSel)
        if (idx == m_idxHot)
            Opt.eState = Dui::State::HotSelected;
        else
            Opt.eState = Dui::State::Selected;
    else
        if (idx == m_idxHot || e.bAnSelBkg)
            Opt.eState = Dui::State::Hot;
        else
            Opt.eState = Dui::State::None;

    m_pRenderer->LrItmDraw(Opt);

#ifdef _DEBUG
    WCHAR szDbg[eck::CchI32ToStrBufNoRadix2];
    const auto cchDbg = swprintf_s(szDbg, L"%d", idx);
    D2D1_RECT_F rcDbg;
    ItmGetRect(idx, rcDbg);
    ComPtr<ID2D1SolidColorBrush> pBrDbg;
    m_pDC->CreateSolidColorBrush(D2D1::ColorF{ (ItmIsDelaying() && ItmInDelayRange(idx)) ?
        D2D1::ColorF::Green : D2D1::ColorF::Red }, &pBrDbg);
    m_pDC->DrawTextW(szDbg, cchDbg, GetTextFormat(), rcDbg, pBrDbg.Get());
#endif
    return Opt.y + Opt.cy + m_cyLinePadding;
}

void CVeLrc::ItmGetRect(int idx, _Out_ D2D1_RECT_F& rc)
{
    const auto& e = m_vItem[idx];
    rc.left = e.x;
    rc.top = e.y;
    rc.right = e.x + e.cx;
    rc.bottom = rc.top + e.cy;
}

int CVeLrc::ItmIndexFromY(float y)
{
    if (IsEmpty())
        return -1;
    if (m_bItemAnDelay)
    {
        for (int i = (int)m_vItem.size() - 1; i >= 0; --i)
        {
            if (y > m_vItem[i].y - m_cyLinePadding)
                return i;
        }
        return 0;
    }
    else
    {
        const auto it = std::lower_bound(m_vItem.begin(), m_vItem.end(), y,
            [=](const ITEM& r, float f) { return r.y - m_cyLinePadding < f; });
        if (it == m_vItem.end())
            return -1;
        if (it == m_vItem.begin())
            return 0;
        else
            return (int)std::distance(m_vItem.begin(), it - 1);
    }
}

void CVeLrc::ItmInvalidate(int idx)
{
    D2D1_RECT_F rc;
    ItmGetRect(idx, rc);
    Invalidate(rc);
}

void CVeLrc::SeBeginExpand(BOOL bEnlarge)
{
    const auto bWake = !IsValid();
    if (!m_bScrollExpand)
    {
        m_bScrollExpand = TRUE;
        if (m_bSeEnlarging = bEnlarge)
            m_msScrollExpand = 0.f;
        else
            m_msScrollExpand = AnDurLrcScrollExpand;
    }
    if (bWake)
        GetWindow().KctWake();
}

void CVeLrc::ItmDelayPrepare(float dy)
{
    float y;
    m_bItemAnDelay = TRUE;// 马上要启动滚动条时间线，无需唤醒渲染线程
    m_bDelayScrollUp = (dy > 0.f);
    int i;
    const auto& ItemCurr = m_vItem[m_idxCurr];
    // 当前（含）以上
    y = ItmGetCurrentItemTarget() - ItemCurr.cy / 2.f
        + ItemCurr.cy + m_cyLinePadding;
    m_idxDelayBegin = 0;
    for (int i = m_idxCurr; i >= 0; --i)
    {
        auto& e = m_vItem[i];
        y -= (e.cy + m_cyLinePadding);
        e.yAnDelayDst = y;
        e.yAnDelaySrc = e.y;
        e.msDelay = e.msAnDelay = 0.f;
        if (y - m_cyLinePadding <= 0.f)
        {
            m_idxDelayBegin = i;
            if (m_bDelayScrollUp)
                m_yMinMaxDelayPos = e.yAnDelaySrc;
            break;
        }
    }
    // 当前以下
    y = ItemCurr.yAnDelayDst + ItemCurr.cy + m_cyLinePadding;
    m_idxDelayEnd = (int)m_vItem.size() - 1;
    for (i = m_idxCurr + 1; i < (int)m_vItem.size(); ++i)
    {
        auto& e = m_vItem[i];
        e.yAnDelayDst = y;
        e.yAnDelaySrc = e.y;
        e.msDelay = e.msAnDelay = 0.f;
        y += (e.cy + m_cyLinePadding);
        if (y >= GetHeight())
        {
            m_idxDelayEnd = i;
            if (!m_bDelayScrollUp)
                m_yMinMaxDelayPos = e.yAnDelaySrc;
            break;
        }
    }
}

void CVeLrc::ItmDelayComplete()
{
    m_idxDelayBegin = m_idxDelayEnd = -1;
    m_bItemAnDelay = FALSE;
}

BOOL CVeLrc::ItmIsDelayEnd(const ITEM& e)
{
    const auto cy = GetHeight();
    const auto msDelay = DurMaxItemDelay * ((m_idxDelayEnd - m_idxDelayBegin + 1) / 10.f);
    if (m_bDelayScrollUp)
        return e.msDelay >= (msDelay * ((e.yAnDelaySrc - m_yMinMaxDelayPos) / cy));
    else
        return e.msDelay >= (msDelay * ((m_yMinMaxDelayPos - e.yAnDelaySrc) / cy));
}

LRESULT CVeLrc::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        Dui::PAINTINFO ps;
        BeginPaint(ps, wParam, lParam);
        m_pRenderer->LrBeginDraw();
        for (int i = m_idxTop; i < (int)m_vItem.size(); ++i)
        {
            if (ItmPaint(i) > GetHeight())
                break;
        }
        m_pRenderer->LrEndDraw();
        DbgDrawFrame();
        EndPaint(ps);
    }
    return 0;

    case WM_MOUSEMOVE:
    {
        if (m_vItem.empty())
            break;
        const POINT pt ECK_GET_PT_LPARAM(lParam);
        int idx = ItmHitTest(pt);
        if (idx != m_idxHot)
        {
            std::swap(idx, m_idxHot);
            if (idx >= 0)
            {
                m_vItem[idx].OnKillHot();
                IsbWakeRenderThread();
            }
            if (m_idxHot >= 0)
            {
                m_vItem[m_idxHot].OnSetHot();
                IsbWakeRenderThread();
            }
        }
    }
    return 0;

    case WM_MOUSELEAVE:
    {
        if (m_vItem.empty())
            break;
        int idx = -1;
        if (idx != m_idxHot)
        {
            std::swap(idx, m_idxHot);
            if (idx >= 0)
            {
                m_vItem[idx].OnKillHot();
                IsbWakeRenderThread();
            }
        }
    }
    return 0;

    case WM_MOUSEWHEEL:
    {
        if (m_vItem.empty())
            break;
        if (!MiIsManualScroll())
            SeBeginExpand(TRUE);
        ItmDelayComplete();
        ScrManualScrolling();
        m_psv->OnMouseWheel2(-GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
        GetWindow().KctWake();
    }
    return 0;

    case WM_LBUTTONDOWN:
    {
        SetFocus();
        if (m_vItem.empty())
            break;
        const POINT pt ECK_GET_PT_LPARAM(lParam);
        int idx = ItmHitTest(pt);
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        {
            if (idx >= 0)
                m_idxMark = idx;
        }
        else if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        {
            if (m_idxMark < 0 || idx < 0)
                break;
            const int idx0 = std::min(m_idxMark, idx);
            const int idx1 = std::max(m_idxMark, idx);
            int i = 0;
            for (; i < idx0; ++i)
            {
                if (m_vItem[i].bSel)
                {
                    m_vItem[i].bSel = FALSE;
                    ItmInvalidate(i);
                }
            }
            for (; i <= idx1; ++i)
            {
                if (!m_vItem[i].bSel)
                {
                    m_vItem[i].bSel = TRUE;
                    ItmInvalidate(i);
                }
            }
            for (; i < (int)m_vItem.size(); ++i)
            {
                if (m_vItem[i].bSel)
                {
                    m_vItem[i].bSel = FALSE;
                    ItmInvalidate(i);
                }
            }
            break;
        }
        else
        {
            if (idx >= 0)
                m_idxMark = idx;
            EckCounter((int)m_vItem.size(), i)
            {
                if (m_vItem[i].bSel)
                {
                    m_vItem[i].bSel = FALSE;
                    ItmInvalidate(i);
                }
            }
        }

        if (idx >= 0)
            if (!m_vItem[idx].bSel)
            {
                m_vItem[idx].bSel = TRUE;
                ItmInvalidate(idx);
            }
    }
    return 0;

    case WM_KEYDOWN:
    {
        if (m_vItem.empty())
            break;
        if (wParam == VK_ESCAPE)
        {
            if (MiIsManualScroll())
            {
                m_tMouseIdle = 0;
                KillTimer(IDT_MOUSEIDLE);
                ScrAutoScrolling();
            }
        }
        else if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
        {
            EckCounter((int)m_vItem.size(), i)
            {
                if (!m_vItem[i].bSel)
                {
                    m_vItem[i].bSel = TRUE;
                    Invalidate();
                }
            }
        }
    }
    return 0;

    case WM_NOTIFY:
    {
        if ((Dui::CElement*)wParam == &m_SB)
        {
            switch (((Dui::ELENMHDR*)lParam)->uNotify)
            {
            case Dui::ENC_SCROLL:
            {
                if (!MiIsManualScroll())
                    SeBeginExpand(TRUE);
                ItmDelayComplete();
                ScrManualScrolling();
                m_psv->InterruptAnimation();
                ScrDoItemScroll(m_psv->GetPosition());
                ItmReCalcTop();
                Invalidate();
            }
            return TRUE;
            }
        }
    }
    return 0;

    case WM_TIMER:
    {
        if (wParam == IDT_MOUSEIDLE)
        {
            if (m_vItem.empty())
                break;
            m_tMouseIdle -= TE_MOUSEIDLE;
            if (m_tMouseIdle <= 0)
            {
                m_tMouseIdle = 0;
                KillTimer(IDT_MOUSEIDLE);
                ScrAutoScrolling();
            }
        }
    }
    return 0;

    case WM_SIZE:
    {
        m_pRenderer->LrSetViewSize(GetWidth(), GetHeight());

        ItmDelayComplete();
        ItmLayout();
        Kw::Rect rc;
        rc.left = GetWidth() - m_SB.GetWidth();
        rc.top = 0;
        rc.right = rc.left + m_SB.GetWidth();
        rc.bottom = rc.top + GetHeight();
        m_SB.SetRect(rc);
        if (!IsEmpty())
        {
            if (MiIsManualScroll())
                ScrFixItemPosition();
            else
            {
                const auto idxCurr = m_idxCurr < 0 ? 0 : m_idxCurr;
                const auto& CurrItem = m_vItem[idxCurr];
                float y = ItmGetCurrentItemTarget() - CurrItem.cy / 2.f
                    + CurrItem.cy + m_cyLinePadding;
                for (int i = idxCurr; i >= 0; --i)
                {
                    auto& e = m_vItem[i];
                    y -= (e.cy + m_cyLinePadding);
                    e.y = e.yNoDelay = y;
                }
                m_psv->SetPosition(-m_vItem.front().y);
                y = CurrItem.yNoDelay + CurrItem.cy + m_cyLinePadding;
                for (int i = idxCurr + 1; i < (int)m_vItem.size(); ++i)
                {
                    auto& e = m_vItem[i];
                    e.y = e.yNoDelay = y;
                    y += (e.cy + m_cyLinePadding);
                }
            }
            ItmReCalcTop();
        }
    }
    break;

    case WM_SETFONT:
        m_pRenderer->SetTextFormatMain(GetTextFormat());
        break;

    case WM_CREATE:
    {
        GetWindow().KctRegisterTimeLine(this);

        const LRD_INIT InitOpt{ .pD2DContext = m_pDC, };
        m_pRenderer->LrInit(InitOpt);
        m_pRenderer->SetTheme(GetTheme());

        m_SB.Create(nullptr, 0, 0,
            0, 0, GetTheme()->GetMetrics(Dui::Metrics::CxVScroll), 0,
            this);
        m_psv = m_SB.GetScrollView();
        m_psv->AddRef();
        m_psv->SetMinThumbSize(Dui::CxyMinScrollThumb);
        m_psv->SetCallback([](float fPos, float fPrevPos, LPARAM lParam)
            {
                ((CVeLrc*)lParam)->ScrAnProc(fPos, fPrevPos);
            }, (LPARAM)this);
        m_psv->SetDelta(80);
    }
    break;

    case WM_DESTROY:
        m_vItem.clear();
        break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}

HRESULT CVeLrc::LrcSetCurrentLine(int idxCurr)
{
    if (idxCurr < 0)
        return E_BOUNDS;
    if (m_idxCurr == idxCurr)
        return S_FALSE;
    if (idxCurr >= (int)m_vItem.size())
        return E_BOUNDS;
    const int idxPrev = m_idxCurr;
    m_idxCurr = idxCurr;
    if (MiIsManualScroll())
    {
        if (idxPrev >= 0)
            ItmInvalidate(idxPrev);
        if (m_idxCurr >= 0)
            ItmInvalidate(m_idxCurr);
    }
    else
    {
        m_bEnlarging = TRUE;
        m_idxPrevAnItem = idxPrev;
        m_idxCurrAnItem = m_idxCurr;
        const auto& e = m_vItem[m_idxCurr];
        const auto dy = (e.yNoDelay + e.cy / 2.f) - ItmGetCurrentItemTarget();
        m_psv->InterruptAnimation();
        m_psv->SmoothScrollDelta(dy);
        ItmDelayPrepare(dy);
        GetWindow().KctWake();
    }
    return S_OK;
}

HRESULT CVeLrc::LrcInit(RefPtr<Lyric::CLyric> pLyric)
{
    m_pLrc = pLyric;
    m_idxCurr = -1;
    m_pRenderer->LrItmSetCount(m_pLrc->MgGetLineCount());
    ItmLayout();
    ItmDelayComplete();
    if (!IsEmpty())
        ScrFixItemPosition();
    ItmReCalcTop();
    Invalidate();
    return S_OK;
}

void CVeLrc::LrcClear()
{
    SafeRelease(m_pLrc);
    m_vItem.clear();
    m_idxTop = -1;
    m_idxHot = -1;
    m_idxMark = -1;
    m_idxCurr = -1;
    m_idxPrevAnItem = -1;
    m_idxCurrAnItem = -1;
    m_fAnValue = 1.f;
    m_bEnlarging = FALSE;
    m_SB.SetVisible(FALSE);
    Invalidate();
}

void CVeLrc::SetTextFormatTrans(IDWriteTextFormat* pTf)
{
    m_pRenderer->SetTextFormatTrans(pTf);
}

void CVeLrc::TlTick(int iMs) noexcept
{
    BOOL bAn{};
    int i = (m_bDelayScrollUp ? 0 : (int)m_vItem.size() - 1);
    for (; i >= 0 && i < (int)m_vItem.size(); (m_bDelayScrollUp ? ++i : --i))
    {
        auto& e = m_vItem[i];
        if (e.bAnSelBkg)
        {
            if (e.bAnSelBkgEnlarge)
                e.msAnSelBkg -= iMs;
            else
                e.msAnSelBkg += iMs;
            e.kAnSelBkg = eck::Easing::Linear(
                e.msAnSelBkg, 0.f, 1.f, AnDurLrcSelBkg);
            if (e.kAnSelBkg >= 1.f || e.kAnSelBkg <= 0.f)
                e.bAnSelBkg = FALSE;
            else
                bAn = TRUE;
            ItmInvalidate(i);
        }
        if (m_bItemAnDelay && ItmInDelayRange(i))
            if (ItmIsDelayEnd(e))
            {
                e.msAnDelay += iMs;
                auto k = eck::Easing::OutExpo(
                    e.msAnDelay, 0.f, 1.f, AnDurLrcDelay);
                // VLTBUG 250822
                // 缓动函数内部的钳位会导致某些曲线结束位置会产生较大的跳变，
                // ECK已修改，取消了所有钳位，并且在外部应使用k作为终点条件
                if (fabs(e.y - e.yAnDelayDst) < 0.4f &&// 动画结束
                    ((m_bDelayScrollUp ? m_idxDelayBegin : m_idxDelayEnd) == i))
                {
                    // VLTBUG 250916
                    // OutExpo本身平缓区域过大，在此裁去一部分防止动画空转浪费GPU资源
                    // 必须按顺序停止动画，仅当上一动画结束时才判定当前动画完成
                    k = 1.f;
                    e.msDelay = 0.f;
                    e.msAnDelay = 0.f;
                    if (m_bDelayScrollUp)
                    {
                        EckAssert(m_idxDelayBegin == i);
                        ++m_idxDelayBegin;
                    }
                    else
                    {
                        EckAssert(m_idxDelayEnd == i);
                        --m_idxDelayEnd;
                    }
                    if (m_idxDelayEnd < m_idxDelayBegin)
                        ItmDelayComplete();
                }
                D2D1_RECT_F rc;
                ItmGetRect(i, rc);
                e.y = e.yAnDelaySrc + (e.yAnDelayDst - e.yAnDelaySrc) * k;
                rc.top = std::min(rc.top, e.y);
                rc.bottom = std::max(rc.bottom, e.y + e.cy);
                Invalidate(rc);
            }
            else
                e.msDelay += iMs;
    }
    m_bAnSelBkg = bAn;
    if (m_bItemAnDelay)
        ItmReCalcTop();
}

void CVeLrc::ScrAutoScrolling()
{
    if (m_idxCurr != m_idxCurrAnItem && m_idxCurrAnItem >= 0)
    {
        m_idxPrevAnItem = m_idxCurrAnItem;
        m_idxCurrAnItem = m_idxCurr;
        m_bEnlarging = TRUE;
    }
    const auto& CurrItem = m_idxCurr < 0 ? m_vItem.front() : m_vItem[m_idxCurr];
    const auto dy = (CurrItem.yNoDelay + CurrItem.cy / 2.f) - ItmGetCurrentItemTarget();
    m_psv->InterruptAnimation();
    m_psv->SmoothScrollDelta(dy);
    SeBeginExpand(FALSE);
    if (m_idxCurr >= 0)
        ItmDelayPrepare(dy);
    GetWindow().KctWake();
}

void CVeLrc::ItmLayout()
{
    const float cx = GetWidth(), cy = GetHeight();
    if (cx <= 0.f || cy <= 0.f)
    {
        m_SB.SetVisible(FALSE);
        return;
    }
    m_vItem.clear();
    if (!m_pLrc || !m_pLrc->MgGetLineCount())
    {
        m_SB.SetVisible(FALSE);
        return;
    }
    const auto cLrc = m_pLrc->MgGetLineCount();
    const auto yInit = (float)-m_psv->GetPosition();
    float y = yInit;
    LRD_TEXT_METRICS Metrics;

    m_vItem.resize(cLrc);

    EckCounter(cLrc, i)
    {
        auto& e = m_vItem[i];
        const auto& Lrc = m_pLrc->MgAtLine(i);

        m_pRenderer->LrItmUpdateText(i, Lrc, Metrics);

        e.cx = Metrics.cxMain;
        e.cy = Metrics.cyMain;
        if (Metrics.cxTrans)
        {
            e.cxTrans = Metrics.cxTrans;
            e.cx = std::max(e.cx, e.cxTrans);
            e.cy += (Metrics.cyTrans + m_pRenderer->GetMainToTransDistance());
        }

        switch (m_eAlignH)
        {
        case eck::Alignment::Center:
            e.x = (cx - e.cx) / 2.f;
            break;
        case eck::Alignment::Far:
            e.x = cx - e.cx;
            break;
        }
        e.cx *= m_pRenderer->GetMaxScale();
        e.cy *= m_pRenderer->GetMaxScale();
        e.cx += (m_pRenderer->GetItemMargin() * 2.f);
        e.cy += (m_pRenderer->GetItemMargin() * 2.f);
        y += (e.cy + m_cyLinePadding);
    }

    m_psv->SetMinimum(-cy / 3.f - m_vItem.front().cy);
    m_psv->SetMaximum(y - yInit - m_cyLinePadding + cy / 3.f * 2.f);
    m_psv->SetPage(cy);
    m_psv->SetViewSize(cy);
    m_SB.SetVisible(m_psv->IsVisible());
}

void CVeLrc::ScrManualScrolling()
{
    MiBeginDetect();
    m_bEnlarging = FALSE;
    m_idxPrevAnItem = -1;
    m_fAnValue = m_pRenderer->GetMaxScale();
}

void CVeLrc::ScrDoItemScroll(float fPos)
{
    float y = -fPos;
    auto& Front = m_vItem.front();
    Front.yNoDelay = y;
    if (!(m_bItemAnDelay && ItmInDelayRange(0)))
        Front.y = y;
    for (int i = 1; i < (int)m_vItem.size(); ++i)
    {
        const auto& Prev = m_vItem[i - 1];
        y += (Prev.cy + m_cyLinePadding);
        if (!(m_bItemAnDelay && ItmInDelayRange(i)))
            m_vItem[i].y = y;
        m_vItem[i].yNoDelay = y;
    }
}

void CVeLrc::ScrFixItemPosition()
{
    const auto& e = m_vItem.front();
    const auto iRealPos = (int)-e.y;
    if (iRealPos < m_psv->GetMinimum())
        ScrDoItemScroll(m_psv->GetPosition());
    else if (iRealPos > m_psv->GetMaxWithPage())
        ScrDoItemScroll(m_psv->GetPosition());
}


void CVeLrc::ITEM::OnSetHot()
{
    if (!bAnSelBkg)
    {
        bAnSelBkg = TRUE;
        msAnSelBkg = AnDurLrcSelBkg;
    }
    bAnSelBkgEnlarge = TRUE;
}

void CVeLrc::ITEM::OnKillHot()
{
    if (!bAnSelBkg)
    {
        bAnSelBkg = TRUE;
        msAnSelBkg = 0.f;
    }
    bAnSelBkgEnlarge = FALSE;
}