#include "pch.h"
#include "CVeLrc.h"
#include "CLrGeometryRealization.h"

enum
{
    IDT_MOUSEIDLE = 1001,
    TE_MOUSEIDLE = 500,
    T_MOUSEIDLEMAX = 4500,
};

constexpr inline float DurationSelectionBack{ 100.f };  // 歌词选中背景动画时长
constexpr inline float DurationScrollExpand{ 200.f };   // 滚动展开动画时长
constexpr inline float DurationDelay{ 600.f };          // 每个项目的延迟动画时长
constexpr inline float DurationMaxItemDelay{ 310.f };   // 延迟间隔

void CVeLyric::ScrAnimationCallback(const Dui::IScrollController::SCC_CALLBACK_DATA& Data) noexcept
{
    if (IsEmpty())
        return;

    const auto fMaxScale = GetTheme()->GetMetric(IdMeMaximumScale, DefaultMaximumScale);
    auto& sv = m_SB.GetScrollView();// HACK TlGetCurrentInterval改为const
    if (m_bEnlarging)
    {
        m_fAnValue = eck::Easing::OutCubic(
            sv.GetCurrentTime(),
            1.f,
            fMaxScale - 1.f,
            sv.GetDuration());
        if (sv.GetCurrentTime() >= sv.GetDuration())
        {
            m_bEnlarging = FALSE;
            m_idxPrevAnItem = -1;
            m_fAnValue = fMaxScale;
        }
    }

    EckAssert(DurationScrollExpand < (float)sv.GetDuration());
    if (m_bScrollExpand)
    {
        const auto ms = sv.TlGetCurrentInterval();
        if (m_bSeEnlarging)
            m_msScrollExpand += ms;
        else
            m_msScrollExpand -= ms;

        m_kScrollExpand = eck::Easing::Linear(
            m_msScrollExpand,
            1.f,
            fMaxScale - 1.f,
            DurationScrollExpand);

        if (m_msScrollExpand >= DurationScrollExpand ||
            m_msScrollExpand <= 0.f)
        {
            m_bScrollExpand = FALSE;
            m_kScrollExpand = m_bSeEnlarging ? fMaxScale : 1.f;
            m_msScrollExpand = m_bSeEnlarging ? DurationScrollExpand : 0.f;
        }
    }

    ScrDoItemScroll(Data.fPos);

    ItmReCalculateTop();
    Invalidate();
}

void CVeLyric::ItmReCalculateTop() noexcept
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

void CVeLyric::MiBeginDetect() noexcept
{
    if (!m_tMouseIdle)
        SetTimer(IDT_MOUSEIDLE, TE_MOUSEIDLE);
    m_tMouseIdle = T_MOUSEIDLEMAX;
}

int CVeLyric::ItmHitTest(Kw::Vec2 pt) noexcept
{
    for (int i = m_idxTop; i < (int)m_vItem.size(); ++i)
    {
        const auto rc = ItmGetRect(i);
        if (eck::PointInRect(rc, pt))
            return i;
        if (rc.top > GetHeight())
            break;
    }
    return -1;
}

float CVeLyric::ItmPaint(int idx) noexcept
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
        .cxMain = e.cxMain,
        .cxTranslation = e.cxTranslation,
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
        Opt.ss = SsSelected;
    else
        if (idx == m_idxHot || e.bAnSelBkg)
            Opt.ss = SsHot;
        else
            Opt.ss = SsNormal;

    m_pRenderer->LrDrawItem(Opt);

#ifdef _DEBUG
    {
        WCHAR szDbg[eck::TcvIntBufferSize<int>()];
        PWCH pEnd;
        eck::TcvFromInt(EckArgString(szDbg), idx, 10, TRUE, &pEnd);
        const auto cchDbg = int(pEnd - szDbg);

        GetDC()->DrawTextW(
            szDbg, cchDbg,
            GetTextFormat().Get(),
            ItmGetRect(idx),
            GetWindow().CcSetBrushColor(D2D1::ColorF{
                (ItmIsDelaying() && ItmInDelayRange(idx)) ?
                D2D1::ColorF::Green : D2D1::ColorF::Red }));
    }
#endif
    return Opt.y + Opt.cy + m_cyLinePadding;
}

int CVeLyric::ItmIndexFromY(float y) const noexcept
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

void CVeLyric::ItmInvalidate(int idx) noexcept
{
    Invalidate(ItmGetRect(idx));
}

void CVeLyric::MiBeginScrollExpand(BOOL bEnlarge) noexcept
{
    const auto bWake = !IsValid();
    if (!m_bScrollExpand)
    {
        m_bScrollExpand = TRUE;
        if (m_bSeEnlarging = bEnlarge)
            m_msScrollExpand = 0.f;
        else
            m_msScrollExpand = DurationScrollExpand;
    }
    if (bWake)
        GetWindow().KctWake();
}

void CVeLyric::ItmDelayPrepare(float dy) noexcept
{
    float y;
    m_bItemAnDelay = TRUE;// 马上要启动滚动条时间线，无需唤醒渲染线程
    m_bDelayScrollUp = (dy > 0.f);
    int i;
    const auto& Curr = m_vItem[m_idxCurr];
    // 当前（含）以上
    y = ItmGetCurrentItemTarget() - Curr.cy / 2.f + Curr.cy + m_cyLinePadding;
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
    y = Curr.yAnDelayDst + Curr.cy + m_cyLinePadding;
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

void CVeLyric::ItmDelayComplete() noexcept
{
    m_idxDelayBegin = m_idxDelayEnd = -1;
    m_bItemAnDelay = FALSE;
}

BOOL CVeLyric::ItmIsDelayEnd(const ITEM& e) noexcept
{
    const auto cy = GetHeight();
    const auto msDelay = DurationMaxItemDelay * ((m_idxDelayEnd - m_idxDelayBegin + 1) / 10.f);
    if (m_bDelayScrollUp)
        return e.msDelay >= (msDelay * ((e.yAnDelaySrc - m_yMinMaxDelayPos) / cy));
    else
        return e.msDelay >= (msDelay * ((m_yMinMaxDelayPos - e.yAnDelaySrc) / cy));
}

LRESULT CVeLyric::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
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
        const auto& pt = EagPoint(lParam);
        int idx = ItmHitTest(pt);
        if (idx != m_idxHot)
        {
            std::swap(idx, m_idxHot);
            if (idx >= 0)
                m_vItem[idx].OnKillHot();
            if (m_idxHot >= 0)
                m_vItem[m_idxHot].OnSetHot();
            if (!m_bAnSelBkg && (idx >= 0 || m_idxHot >= 0))
            {
                m_bAnSelBkg = TRUE;
                GetWindow().KctWake();
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
                if (!m_bAnSelBkg)
                {
                    m_bAnSelBkg = TRUE;
                    GetWindow().KctWake();
                }
            }
        }
    }
    return 0;

    case WM_MOUSEWHEEL:
    {
        if (m_vItem.empty())
            break;
        if (!MiIsManualScroll())
            MiBeginScrollExpand(TRUE);
        ItmDelayComplete();
        ScrManualScrolling();
        m_SB.GetScrollView().OnMouseWheel2(-GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
        GetWindow().KctWake();
    }
    return 0;

    case WM_LBUTTONDOWN:
    {
        SetFocus();
        if (m_vItem.empty())
            break;
        const auto& pt = EagPoint(lParam);
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
                    MiBeginScrollExpand(TRUE);
                ItmDelayComplete();
                ScrManualScrolling();
                m_SB.GetScrollView().InterruptAnimation();
                ScrDoItemScroll(m_SB.GetTrackPosition());
                ItmReCalculateTop();
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
                m_SB.SetTrackPosition(-m_vItem.front().y);
                y = CurrItem.yNoDelay + CurrItem.cy + m_cyLinePadding;
                for (int i = idxCurr + 1; i < (int)m_vItem.size(); ++i)
                {
                    auto& e = m_vItem[i];
                    e.y = e.yNoDelay = y;
                    y += (e.cy + m_cyLinePadding);
                }
            }
            ItmReCalculateTop();
        }
    }
    break;

    case WM_SETFONT:
        // TODO: 无效化缓存
        break;

    case WM_CREATE:
    {
        GetWindow().KctRegisterTimeLine(this);

        m_pRenderer = std::make_unique<CLyricRendererD2D>();
        m_pRenderer->LrInitialize(this);

        m_SB.Create(nullptr, 0, 0, 0, 0, 0, 0, this);
        m_SB.SccSetCallback([](const Dui::IScrollController::SCC_CALLBACK_DATA& Data)
            {
                ((CVeLyric*)Data.pUser)->ScrAnimationCallback(Data);
            }, this);
    }
    break;

    case WM_DESTROY:
        m_vItem.clear();
        break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}

HRESULT CVeLyric::LrcSetCurrentLine(int idxCurr) noexcept
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
        m_SB.GetScrollView().InterruptAnimation();
        m_SB.GetScrollView().SmoothScrollDelta(dy);
        ItmDelayPrepare(dy);
        GetWindow().KctWake();
    }
    return S_OK;
}

HRESULT CVeLyric::LrcInitialize(RefPtr<Lyric::CLyric> pLyric) noexcept
{
    m_pLrc = pLyric;
    m_idxCurr = -1;
    m_pRenderer->LrSetItemCount(m_pLrc->MgGetLineCount());
    ItmLayout();
    ItmDelayComplete();
    if (!IsEmpty())
        ScrFixItemPosition();
    ItmReCalculateTop();
    Invalidate();
    return S_OK;
}

void CVeLyric::LrcClear() noexcept
{
    m_pLrc.Clear();
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

void CVeLyric::SetTextFormatTranslation(IDWriteTextFormat* pTf) noexcept
{
    m_pTfTranslation = pTf;
}

void CVeLyric::TlTick(int iMs) noexcept
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
                e.msAnSelBkg, 0.f, 1.f, DurationSelectionBack);
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
                    e.msAnDelay, 0.f, 1.f, DurationDelay);
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
                auto rc = ItmGetRect(i);
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
        ItmReCalculateTop();
}

void CVeLyric::ScrAutoScrolling() noexcept
{
    if (m_idxCurr != m_idxCurrAnItem && m_idxCurrAnItem >= 0)
    {
        m_idxPrevAnItem = m_idxCurrAnItem;
        m_idxCurrAnItem = m_idxCurr;
        m_bEnlarging = TRUE;
    }
    const auto& CurrItem = m_idxCurr < 0 ? m_vItem.front() : m_vItem[m_idxCurr];
    const auto dy = (CurrItem.yNoDelay + CurrItem.cy / 2.f) - ItmGetCurrentItemTarget();
    m_SB.GetScrollView().InterruptAnimation();
    m_SB.GetScrollView().SmoothScrollDelta(dy);
    MiBeginScrollExpand(FALSE);
    if (m_idxCurr >= 0)
        ItmDelayPrepare(dy);
    GetWindow().KctWake();
}

void CVeLyric::ItmLayout() noexcept
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
    const auto yInit = (float)-m_SB.GetTrackPosition();
    DWRITE_TEXT_METRICS tm;

    float y = yInit;
    float cxItem, cyItem;

    const auto fMaxScale = GetTheme()->GetMetric(IdMeMaximumScale, DefaultMaximumScale);
    const auto cxyItemMargin = GetTheme()->GetMetric(IdMeItemMargin, DefaultItemMargin);
    const auto dPadding = GetTheme()->GetMetric(
        IdMeMainToTranslationDistance, DefaultMainToTranslationDistance);
    const float cxMax = (cx - cxyItemMargin * 2.f) / fMaxScale;

    m_vItem.resize(cLrc);

    EckCounter(cLrc, i)
    {
        auto& e = m_vItem[i];
        const auto& Line = m_pLrc->MgAtLine(i);

        e.bCacheValid = TRUE;
        e.y = y;
        e.cx = 0.f;
        e.cy = 0.f;

        // -- Main

        eck::g_pDwFactory->CreateTextLayout(
            Line.pszLrc, Line.cchLrc,
            GetTextFormat().Get(),
            cxMax, cy,
            e.pTlMain.AtClear());
        if (e.pTlMain)
        {
            e.pTlMain->GetMetrics(&tm);
            e.cx = std::max(e.cx, tm.width);
            e.cy += tm.height;
        }

        // -- Translation

        if (Line.pszTranslation && Line.cchTranslation)
        {
            eck::g_pDwFactory->CreateTextLayout(
                Line.pszTranslation, Line.cchTranslation,
                m_pTfTranslation.Get(),
                cxMax, cy,
                e.pTlTranslation.AtClear());
            if (e.pTlTranslation)
            {
                e.pTlTranslation->GetMetrics(&tm);
                e.cx = std::max(e.cx, tm.width);
                e.cy += (tm.height + dPadding);
            }
        }

        // --

        e.cx *= fMaxScale;
        e.cy *= fMaxScale;
        e.cx += (cxyItemMargin * 2.f);
        e.cy += (cxyItemMargin * 2.f);

        switch (m_eAlignH)
        {
        case eck::Alignment::Near:
            e.x = 0.f;
            break;
        case eck::Alignment::Center:
            e.x = (cx - e.cx) / 2.f;
            break;
        case eck::Alignment::Far:
            e.x = cx - e.cx;
            break;
        }
        y += (e.cy + m_cyLinePadding);
    }

    m_SB.SetRange(
        -cy / 3.f - m_vItem.front().cy,
        y - yInit - m_cyLinePadding + cy / 3.f * 2.f);
    m_SB.SetPage(cy);
    m_SB.SetVisible(m_SB.GetScrollView().IsVisible());
}

void CVeLyric::ScrManualScrolling() noexcept
{
    MiBeginDetect();
    m_bEnlarging = FALSE;
    m_idxPrevAnItem = -1;
    m_fAnValue = GetTheme()->GetMetric(IdMeMaximumScale, DefaultMaximumScale);
}

void CVeLyric::ScrDoItemScroll(float fPos) noexcept
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

void CVeLyric::ScrFixItemPosition() noexcept
{
    const auto& e = m_vItem.front();
    const auto iRealPos = (int)-e.y;
    if (iRealPos < m_SB.GetMinimum())
        ScrDoItemScroll(m_SB.GetTrackPosition());
    else if (iRealPos > m_SB.GetScrollView().GetMaxWithPage())
        ScrDoItemScroll(m_SB.GetTrackPosition());
}


void CVeLyric::ITEM::OnSetHot() noexcept
{
    if (!bAnSelBkg)
    {
        bAnSelBkg = TRUE;
        msAnSelBkg = DurationSelectionBack;
    }
    bAnSelBkgEnlarge = TRUE;
}

void CVeLyric::ITEM::OnKillHot() noexcept
{
    if (!bAnSelBkg)
    {
        bAnSelBkg = TRUE;
        msAnSelBkg = 0.f;
    }
    bAnSelBkgEnlarge = FALSE;
}