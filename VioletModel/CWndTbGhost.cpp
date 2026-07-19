#include "pch.h"
#include "CWndMain.h"


LRESULT CWindowGhost::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        constexpr BOOL b = TRUE;
        DwmSetWindowAttribute(Handle, DWMWA_HAS_ICONIC_BITMAP, &b, sizeof(b));
        DwmSetWindowAttribute(Handle, DWMWA_FORCE_ICONIC_REPRESENTATION, &b, sizeof(b));

        const auto ptc = eck::PtcCurrent();
        eck::CheckBool(!ptc->hGhost);
        ptc->hGhost = Handle;
    }
    return 0;

    case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
    {
        if (!m_hbmLivePreviewCache)
        {
            BITMAPINFO bi{};
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = bi.bmiHeader.biHeight = 1;
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            DWORD* pdwBits;
            // TODO: 不使用DIB
            m_hbmLivePreviewCache = CreateDIBSection(nullptr,
                &bi, DIB_RGB_COLORS, (void**)&pdwBits, nullptr, 0);
            *pdwBits = 0x01000000;// 透明度为0将显示为完全不透明
        }
        DwmSetIconicLivePreviewBitmap(Handle, m_hbmLivePreviewCache, nullptr, 0);
    }
    return 0;

    case WM_DWMSENDICONICTHUMBNAIL:
        SetIconicThumbnail(HIWORD(lParam), LOWORD(lParam));
        return 0;

    case WM_ACTIVATE:
    {
        m_pTaskbarList->SetTabActive(Handle, m_pMainWindow->Handle, 0);
        if (IsIconic(m_pMainWindow->Handle))
            m_pMainWindow->Show(SW_RESTORE);
        SetForegroundWindow(m_pMainWindow->Handle);
    }
    return 0;

    case WM_COMMAND:
    case WM_SYSCOMMAND:
        return m_pMainWindow->SendMessageW(uMsg, wParam, lParam);

    case WM_DESTROY:
        m_pTaskbarList->UnregisterTab(Handle);
        InvalidateLivePreviewCache();
        InvalidateThumbnailCache();
        return 0;
    }
    return __super::OnMessage(uMsg, wParam, lParam);
}

void CWindowGhost::InvalidateLivePreviewCache() noexcept
{
    if (m_hbmLivePreviewCache)
    {
        DeleteObject(m_hbmLivePreviewCache);
        m_hbmLivePreviewCache = nullptr;
    }
}

void CWindowGhost::InvalidateThumbnailCache() noexcept
{
    if (m_hbmThumbnailCache)
    {
        DeleteObject(m_hbmThumbnailCache);
        m_hbmThumbnailCache = nullptr;
    }
}

HRESULT CWindowGhost::SetIconicThumbnail(UINT cxMax, UINT cyMax) noexcept
{
    HRESULT hr;
    if (cxMax == UINT_MAX || cyMax == UINT_MAX)
    {
        const auto iDpi = m_pMainWindow->GetWindowDpi();
        // 第一次调用最适尺寸未知，使用120作为默认值
        // 等到窗口第一次接收WM_DWMSENDICONICTHUMBNAIL时更新为正确的尺寸
        cxMax = (m_cxPrev ? m_cxPrev : eck::DpiScale(120, iDpi));
        cyMax = (m_cyPrev ? m_cyPrev : eck::DpiScale(120, iDpi));
    }
    m_cxPrev = cxMax;
    m_cyPrev = cyMax;

    if (m_hbmThumbnailCache)
    {
        BITMAP bmp;
        GetObjectW(m_hbmThumbnailCache, sizeof(bmp), &bmp);
        if (bmp.bmWidth <= (int)cxMax && bmp.bmHeight <= (int)cyMax)
            return DwmSetIconicThumbnail(Handle, m_hbmThumbnailCache, 0);
        else
            InvalidateThumbnailCache();
    }

    auto pCover = App->Player().GetCover();
    if (!pCover.Get())
        pCover = m_pAtlas->CoverGetDefaultWicBitmap();
    UINT cx, cy, cx0, cy0;
    pCover->GetSize(&cx0, &cy0);
    if ((float)cxMax / (float)cyMax > (float)cx0 / (float)cy0)// y对齐
    {
        cy = cyMax;
        cx = cx0 * cy / cy0;
    }
    else// x对齐
    {
        cx = cxMax;
        cy = cx * cy0 / cx0;
    }

    ComPtr<IWICBitmapScaler> pScaler;
    if (FAILED(hr = eck::g_pWicFactory->CreateBitmapScaler(&pScaler)))
        return hr;
    hr = pScaler->Initialize(pCover.Get(), cx, cy, WICBitmapInterpolationModeFant);
    if (FAILED(hr))
        return hr;
    hr = eck::WicCreateDibSection(m_hbmThumbnailCache, pScaler.Get());
    if (FAILED(hr))
        return hr;
    return DwmSetIconicThumbnail(Handle, m_hbmThumbnailCache, 0);
}