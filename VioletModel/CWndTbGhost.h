#pragma once
#include "CVioletAtlas.h"

class CWindowGhost final : public eck::CWindow
{
private:
    Dui::CDuiWindow* m_pMainWindow{};
    ComPtr<ITaskbarList4> m_pTaskbarList{};
    RefPtr<CVioletAtlas> m_pAtlas{ RefPtr<CVioletAtlas>::Make() };

    HBITMAP m_hbmLivePreviewCache{};// 实时预览位图缓存
    HBITMAP m_hbmThumbnailCache{};  // 缩略图位图缓存

    UINT m_cxPrev{}, m_cyPrev{};
public:
    ECK_CWND_CREATE_CLS_HINST(eck::WCN_DUMMY, eck::g_hInstance);

    LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

    void Initialize(
        Dui::CDuiWindow* pMainWindow,
        ITaskbarList4* pTaskbarList,
        RefPtr<CVioletAtlas> pAtlas) noexcept
    {
        m_pMainWindow = pMainWindow;
        m_pTaskbarList = pTaskbarList;
        m_pAtlas = std::move(pAtlas);
    }

    // 保留，当前禁止调用
    void InvalidateLivePreviewCache() noexcept;

    void InvalidateThumbnailCache() noexcept;
    EckInline HRESULT InvalidateDwmThumbnail() noexcept
    {
        return DwmInvalidateIconicBitmaps(Handle);
    }

    // 使用播放器当前封面更新任务栏缩略图，若尺寸设为UINT_MAX则使用上次记录的最适尺寸
    // 封面变化后必须立即更新一次
    HRESULT SetIconicThumbnail(
        UINT cxMax = UINT_MAX,
        UINT cyMax = UINT_MAX) noexcept;
};