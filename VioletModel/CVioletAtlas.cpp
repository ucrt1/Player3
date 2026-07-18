#include "pch.h"
#include "CVioletAtlas.h"
#include "CApp.h"

constexpr std::string_view AtlasSubImageFile[]
{
    "ArrowCross.png"sv,
    "ArrowRight1.png"sv,
    "ArrowRight3.png"sv,
    "Circle.png"sv,
    "CircleOne.png"sv,
    "Cross.png"sv,
    "CrossSolid.png"sv,
    "Effect.png"sv,
    "File.png"sv,
    "Folder.png"sv,
    "Gear.png"sv,
    "Home.png"sv,
    "Information.png"sv,
    "List.png"sv,
    "Locate.png"sv,
    "LockSolid.png"sv,
    "Lyric.png"sv,
    "Next.png"sv,
    "NextSolid.png"sv,
    "Pause.png"sv,
    "PauseSolid.png"sv,
    "PlayPageDown.png"sv,
    "PlayPageUp.png"sv,
    "Plus.png"sv,
    "Previous.png"sv,
    "PreviousSolid.png"sv,
    "Silent.png"sv,
    "Speaker.png"sv,
    "Triangle.png"sv,
    "TriangleSolid.png"sv,
    "WindowLogo.png"sv,
};
static_assert(ARRAYSIZE(AtlasSubImageFile) == CVioletAtlas::AtlasSubImageCount);

constexpr std::wstring_view SingleImageFile[]
{
    {},
};
static_assert(ARRAYSIZE(SingleImageFile) == CVioletAtlas::SingleImageCount);



HRESULT CVioletAtlas::PrepareRealization(ID2D1DeviceContext* pDC) noexcept
{
    m_pDC = pDC;
    return S_OK;
}

HRESULT CVioletAtlas::AtlasInitialize() noexcept
{
    HRESULT hr;
    auto rsPath{ eck::GetRunningPath() };
    const auto cchOld = rsPath.Size();

    rsPath.PushBack(LR"(\Skin\Atlas.png)"sv);
    hr = eck::WicLoadSource(m_pAtlasWic.AtSelfClear(), rsPath.Data());
    if (FAILED(hr))
        return hr;

    rsPath.PopBack(3);// png
    rsPath.PushBack(LR"(json)"sv);

    NTSTATUS nts;
    const auto rbMetaJson = eck::ReadInFile(rsPath.Data(), &nts);
    if (!NT_SUCCESS(nts))
        return HRESULT_FROM_NT(nts);

    Json::CDocument Doc{ rbMetaJson };
    if (!Doc.IsValid())
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    const auto ValFrame = Doc["/frmaes"];
    if (!ValFrame.IsValid())
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    EckCounter((size_t)AppImage::Detail_AtlasMaximum, i)
    {
        const auto ValRect = ValFrame[AtlasSubImageFile[i]]["/frame"];
        const auto x = ValRect["/x"].GetInt();
        const auto y = ValRect["/y"].GetInt();
        const auto cx = ValRect["/cx"].GetInt();
        const auto cy = ValRect["/cy"].GetInt();

        m_SubImage[i].rc = { x, y, x + cx, y + cy };
    }

    return S_OK;
}

HRESULT CVioletAtlas::AtlasRealize() noexcept
{
    return m_pDC->CreateBitmapFromWicBitmap(
        m_pAtlasWic.Get(), m_pAtlasD2D.AtClear());
}

Dui::CBitmap CVioletAtlas::AtlasGetSubImage(AppImage eImg) const noexcept
{
    Dui::CBitmap Bitmap{};
    const auto rc{ eck::MakeD2DRectF(m_SubImage[(size_t)eImg].rc) };
    Bitmap.Set(m_pAtlasD2D.Get(), &rc);
    return Bitmap;
}

HRESULT CVioletAtlas::CoverInitialize() noexcept
{
    HRESULT hr;

    auto rsPath{ eck::GetRunningPath() };
    rsPath.PushBack(LR"(\Skin\DefaultCover.png)"sv);
    ComPtr<IWICBitmapSource> pDefaultCover;
    hr = eck::WicLoadSource(pDefaultCover.AtSelfClear(), rsPath.Data());
    if (FAILED(hr))
        return hr;

    ComPtr<IWICBitmapScaler> pScaler;
    hr = eck::g_pWicFactory->CreateBitmapScaler(&pScaler);
    if (FAILED(hr))
        return hr;

    hr = pScaler->Initialize(
        pDefaultCover.Get(),
        CoverWidth, CoverHeight,
        WICBitmapInterpolationModeFant);
    if (FAILED(hr))
        return hr;

    m_pCoverWic = std::move(pScaler);
    return S_OK;
}

HRESULT CVioletAtlas::CoverRealize() noexcept
{
    HRESULT hr;

    D2D1_BITMAP_PROPERTIES1 Prop{};
    Prop.pixelFormat = { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED };
    Prop.dpiX = Prop.dpiY = 96.f;

    constexpr D2D1_SIZE_U Size{ CoverWidth * 2 + CoverAtlasGap, CoverHeight };

    hr = m_pDC->CreateBitmap(Size, nullptr, 0, Prop, m_pCoverD2D.AtClear());
    if (FAILED(hr))
        return hr;

    eck::UniquePtr<eck::DelVA<BYTE>> pBuffer{
        (BYTE*)eck::VAllocate((CoverWidth + CoverAtlasGap) * CoverHeight * sizeof(UINT)) };

    constexpr WICRect rc{ 0, 0, CoverWidth, CoverHeight };
    hr = m_pCoverWic->CopyPixels(&rc,
        (CoverWidth + CoverAtlasGap) * sizeof(UINT),
        CoverWidth * CoverHeight * sizeof(UINT),
        pBuffer.get());
    if (FAILED(hr))
        return hr;

    constexpr D2D1_RECT_U rcDst{ 0, 0, CoverWidth + CoverAtlasGap, CoverHeight };
    return m_pCoverD2D->CopyFromMemory(
        &rcDst,
        pBuffer.get(),
        (CoverWidth + CoverAtlasGap) * sizeof(UINT));
}

Dui::CBitmap CVioletAtlas::CoverGetCurrentImage() noexcept
{
    return CoverGetSubImage(m_bDefaultCover ?
        AppImage::DefaultCover : AppImage::CurrentCover);
}

Dui::CBitmap CVioletAtlas::CoverGetSubImage(AppImage eImg) noexcept
{
    eck::CheckBool(eImg == AppImage::DefaultCover || eImg == AppImage::CurrentCover);
    Dui::CBitmap Bitmap{};

    D2D1_RECT_F rc;
    rc.left = (eImg == AppImage::DefaultCover) ?
        0.f : float(CoverWidth + CoverAtlasGap);
    rc.top = 0.f;
    rc.right = rc.left + CoverWidth;
    rc.bottom = (float)CoverHeight;

    Bitmap.Set(m_pCoverD2D.Get(), &rc);
    return Bitmap;
}

HRESULT CVioletAtlas::CoverUpdate(IWICBitmapSource* pBitmap) noexcept
{
    if (!pBitmap)
    {
        m_bDefaultCover = TRUE;
        return S_OK;
    }

    eck::UniquePtr<eck::DelVA<BYTE>> pBuffer{
    (BYTE*)eck::VAllocate(CoverWidth * CoverHeight * sizeof(UINT)) };

    constexpr WICRect rc{ 0, 0, CoverWidth, CoverHeight };
    const auto hr = m_pCoverWic->CopyPixels(&rc,
        CoverWidth * sizeof(UINT),
        CoverWidth * CoverHeight * sizeof(UINT),
        pBuffer.get());
    if (FAILED(hr))
        return hr;

    constexpr D2D1_RECT_U rcDst{
        CoverWidth + CoverAtlasGap,
        0,
        CoverWidth * 2 + CoverAtlasGap,
        CoverHeight };
    return m_pCoverD2D->CopyFromMemory(
        &rcDst,
        pBuffer.get(),
        CoverWidth * sizeof(UINT));
}

HRESULT CVioletAtlas::SingleInitialize() noexcept
{
    return E_NOTIMPL;
}

Dui::CBitmap CVioletAtlas::SingleGetImage(AppImage eImg) noexcept
{
    return Dui::CBitmap();
}