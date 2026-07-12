#include "pch.h"
#include "CPagePlaying.h"
#include "CApp.h"
#include "CWndMain.h"

void CPagePlaying::UpdateBlurredCover()
{
    const auto cxEle = LogicalToPixel(GetWidth());
    const auto cyEle = LogicalToPixel(GetHeight());
    if (!cxEle || !cyEle)
        return;

    // -- 取封面

    auto pWicCover = App->Player().GetCover();
    if (!pWicCover)
        pWicCover = App->GetImage(AppImage::DefaultCover);

    // -- 准备环境

    ComPtr<ID2D1Image> pOldTarget;
    GetDC()->GetTarget(&pOldTarget);
    GetDC()->SetTarget(m_pBitmapBlurredCover.Get());
    GetDC()->SetTransform(D2D1::Matrix3x2F::Identity());
    GetDC()->BeginDraw();
    GetDC()->Clear(D2D1::ColorF(D2D1::ColorF::White));// TODO:主题色

    // -- 填充计算

    UINT cx0, cy0;  // 原始大小
    float cyRgn;    // 截取区域高
    float cx, cy;   // 截取后图片大小
    D2D_POINT_2F pt;// 画出位置

    pWicCover->GetSize(&cx0, &cy0);
    cyRgn = cyEle / cxEle * (float)cx0;
    if (cyRgn < cy0)// 1. 宽较大  cxClient / cxPic = cyClient / cyRgn
    {
        cx = cxEle;
        cy = cx * cy0 / cx0;
        pt = { 0.f,(cyEle - cy) / 2 };
    }
    else// 2. 高较大  cyClient / cyPic = cxClient / cxRgn
    {
        cy = (float)cyEle;
        cx = cx0 * cy / cy0;
        pt = { (cxEle - cx) / 2,0.f };
    }

    // -- 缩放

    ComPtr<IWICBitmapScaler> pWicBitmapScaled;
    eck::g_pWicFactory->CreateBitmapScaler(&pWicBitmapScaled);
    pWicBitmapScaled->Initialize(
        pWicCover.Get(),
        (int)cx,
        (int)cy,
        WICBitmapInterpolationModeNearestNeighbor);

    ComPtr<ID2D1Bitmap1> pBitmapCover{};
    GetDC()->CreateBitmapFromWicBitmap(pWicBitmapScaled.Get(), pBitmapCover.AtClear());
    m_Cover.SetBitmap(pBitmapCover.Get());

    // -- 模糊 

    ComPtr<ID2D1Effect> pEffect;
    GetDC()->CreateEffect(CLSID_D2D1GaussianBlur, &pEffect);
    pEffect->SetInput(0, pBitmapCover.Get());
    pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, 40.f);
    pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
    pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION,
        D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED);
    PixelToLogical(pt);
    GetDC()->DrawImage(pEffect.Get(), pt);

    // -- 半透明遮罩

    GetDC()->FillRectangle(GetViewRectD2D(), GetWindow().CcSetBrushColor(
        App->GetColor(GPal::PlayPageOverlay)));

    GetDC()->EndDraw();
    GetDC()->SetTarget(pOldTarget.Get());
}

void CPagePlaying::OnPlayEvent(const PLAY_EVT_PARAM& e)
{
    switch (e.eEvent)
    {
    case PlayEvent::CommonTick:
    {
        m_Lyric.LrcSetCurrentLine(App->Player().GetCurrentLyricLine());
    }
    break;
    case PlayEvent::Play:
    {
        UpdateBlurredCover();
        Invalidate();
        const auto& mi = App->Player().GetMusicSimpleData();
        m_LATitle.SetText(mi.rsTitle.Data());
        m_LAAlbum.SetText(mi.rsAlbum.Data());
        m_LAArtist.SetText(mi.slArtist.FrontData());

        m_Lyric.LrcInitialize(App->Player().GetLyric());
    }
    break;
    case PlayEvent::Stop:
    {
        m_Lyric.LrcClear();
        SetEmptyText();
    }
    break;
    }
}

void CPagePlaying::SetEmptyText()
{
    m_LATitle.SetText(L"Violet Model");
    m_LAArtist.SetText(L"AuroraStudio");
    m_LAAlbum.SetText(L"VC++/Win32");
}

void CPagePlaying::OnColorSchemeChanged()
{}

LRESULT CPagePlaying::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        Dui::PAINTINFO ps;
        BeginPaint(ps, wParam, lParam);
        GetDC()->DrawBitmap(m_pBitmapBlurredCover.Get(), ps.rcClipInEle,
            1.f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, ps.rcClipInEle);
        EndPaint(ps);
    }
    return 0;

    case WM_SIZE:
    {
        const auto cx = GetWidth();
        const auto cy = GetHeight();
        if (m_pBitmapBlurredCover)
        {
            const auto Size = m_pBitmapBlurredCover->GetSize();
            if (!(Size.width < cx || Size.width / 2.f > cx ||
                Size.height < cy || Size.height / 2.f > cy))
                goto Update;
            GetWindow().BmCreateLogical(cx, cy, m_pBitmapBlurredCover.AtSelfClear());
        }
        else
            GetWindow().BmCreateLogical(cx, cy, m_pBitmapBlurredCover.AtSelf());
    Update:;
        UpdateBlurredCover();

        const auto cxMinGap = cx * 1.f / 20.f;
        const Kw::Rect rcLyric
        {
            cx * 10.f / 20.f,
            DLrcTop,
            cx * 18.f / 20.f,
            cy - DLrcBottom
        };
        m_Lyric.SetRect(rcLyric);

        Kw::Rect rcCover;
        rcCover.left = cx * 1.f / 10.f;
        rcCover.top = cy * 15.f / 100.f;
        const auto cxyCover = std::min(
            rcLyric.left - cxMinGap - rcCover.left,
            cy * 4.f / 10.f);
        rcCover.right = rcCover.left + cxyCover;
        rcCover.bottom = rcCover.top + cxyCover;
        m_Cover.SetRect(rcCover);

        Kw::Rect rcLabel{ rcCover };
        rcLabel.top = rcCover.bottom + CyPlayPageLabelCoverPadding;
        rcLabel.bottom = rcLabel.top + CyPlayPageLabel;
        m_LATitle.SetRect(rcLabel);
        rcLabel.top = rcLabel.bottom + CyPlayPageLabelPadding;
        rcLabel.bottom = rcLabel.top + CyPlayPageLabel;
        m_LAAlbum.SetRect(rcLabel);
        rcLabel.top = rcLabel.bottom + CyPlayPageLabelPadding;
        rcLabel.bottom = rcLabel.top + CyPlayPageLabel;
        m_LAArtist.SetRect(rcLabel);

        const auto dBackBtnMar = (CxyMiniCover - CxyBackBtn) / 2;
        Kw::Rect rcBackBtn;
        rcBackBtn.left = DLeftMiniCover + dBackBtnMar;
        rcBackBtn.top = cy - CyPlayPanel + DTopMiniCover + dBackBtnMar;
        rcBackBtn.right = rcBackBtn.left + CxyBackBtn;
        rcBackBtn.bottom = rcBackBtn.top + CxyBackBtn;
        m_BTBack.SetRect(rcBackBtn);
    }
    break;
    case WM_DPICHANGED:
    {
        const auto cx = GetWidth();
        const auto cy = GetHeight();
        GetWindow().BmCreateLogical(cx, cy, m_pBitmapBlurredCover.AtSelfClear());
        UpdateBlurredCover();
    }
    break;
    case WM_SETFONT:
    {
        const auto pTf = GetTextFormat().Get();
        m_LATitle.SetTextFormat(pTf);
        m_LAAlbum.SetTextFormat(pTf);
        m_LAArtist.SetTextFormat(pTf);
    }
    break;
    case Dui::EWM_COLORSCHEMECHANGED:
    {
        OnColorSchemeChanged();
        UpdateBlurredCover();
        Invalidate();
    }
    break;
    // FIXME
    //case WM_LBUTTONUP:
    //{
    //    if (((CWindowMain*)GetWnd())->TlIsValid())
    //    {
    //        Dui::DUINMHDR nm{ ELEN_PLAYPAGE_LBTN_UP };
    //        GenElemNotify(&nm);
    //    }
    //}
    //break;
    case WM_CREATE:
    {
        App->Player().GetEventChain().Connect(this, &CPagePlaying::OnPlayEvent);

        m_Cover.Create(nullptr, Dui::DES_VISIBLE, 0,
            50, 50, 200, 200, this);

        m_Lyric.Create(nullptr, Dui::DES_VISIBLE, 0,
            50, 260, 200, 100, this);

        m_LATitle.Create(nullptr, Dui::DES_VISIBLE, 0,
            0, 0, 0, 0, this);
        m_LATitle.SetFade(TRUE);

        m_LAAlbum.Create(nullptr, Dui::DES_VISIBLE, 0,
            0, 0, 0, 0, this);
        m_LAAlbum.SetFade(TRUE);

        m_LAArtist.Create(nullptr, Dui::DES_VISIBLE, 0,
            0, 0, 0, 0, this);
        m_LAArtist.SetFade(TRUE);

        m_BTBack.Create(nullptr, Dui::DES_VISIBLE | Dui::DES_NOTIFY_WND, 0,
            100, 100, 70, 20, this);
        m_BTBack.SetId(ELEID_PLAYPAGE_BACK);

        OnColorSchemeChanged();

        ComPtr<IDWriteTextFormat> pTfLrc;
        auto& FontFactory = App->GetFontFactory();;
        FontFactory.NewFont(pTfLrc.AtSelfClear(),
            eck::Alignment::Near, eck::Alignment::Near, 25, 700);
        m_Lyric.SetTextFormat(pTfLrc.Get());
        FontFactory.NewFont(pTfLrc.AtSelfClear(),
            eck::Alignment::Near, eck::Alignment::Near, 21, 500);
        m_Lyric.SetTextFormatTranslation(pTfLrc.Get());

        SetEmptyText();
    }
    break;
    case WM_DESTROY:
    {
        m_pBitmapBlurredCover.Clear();
    }
    break;
    }
    return __super::OnEvent(uMsg, wParam, lParam);
}