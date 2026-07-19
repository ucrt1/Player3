#pragma once
enum class AppImage : BYTE
{
    // -- 图集的子图

    Detail_AtlasMinimum,

    ArrowCross,
    ArrowRight1,
    ArrowRight3,
    Circle,
    CircleOne,
    Cross,
    CrossSolid,
    Effect,
    File,
    Folder,
    Gear,
    Home,
    Information,
    List,
    Locate,
    LockSolid,
    Lyric,
    Next,
    NextSolid,
    Pause,
    PauseSolid,
    PlayPageDown,
    PlayPageUp,
    Plus,
    Previous,
    PreviousSolid,
    Silent,
    Speaker,
    Triangle,
    TriangleSolid,
    WindowLogo,

    Detail_AtlasMaximum,

    // -- 独立图像

    Detail_SingleMinimum,

    Detail_Dummy,

    Detail_SingleMaximum,

    // -- 封面，需要特殊处理

    DefaultCover,
    CurrentCover,// Placeholder
};

// 图像分为三类：
//  1. 作为小图标使用，打包在图集中
//     发布后对原始图标图像文件无依赖
//  2. 独立图像文件
//  3. 封面
//     在GPU侧保留两倍大小的纹理，分别用于默认封面和当前封面
// 
// 初始化步骤如下：
//  1. 入口调用XxxInitialize，载入WIC图像和必要元数据
//  2. 主窗口载入后调用PrepareRealization，传入D2D设备上下文
//  3. 主窗口调用XxxRealize，实现为D2D位图
// 
// 独立图像在被取用时进行懒加载，不提供Realize接口
// 所有D2D位图的DPI为96
// 
// 封面的位图由播放器持有，因为这不是库存位图，也不是GPU侧位图
// 如果当前封面更新，主窗口负责调用CoverUpdate上传到GPU
//
class CVioletAtlas
{
public:
    constexpr static UINT CoverWidth = 500;
    constexpr static UINT CoverHeight = 500;
    constexpr static UINT CoverAtlasGap = 1;

    constexpr static size_t AtlasSubImageIndexBegin = (size_t)AppImage::Detail_AtlasMinimum + 1;
    constexpr static size_t AtlasSubImageCount =
        (size_t)AppImage::Detail_AtlasMaximum - AtlasSubImageIndexBegin;

    constexpr static size_t SingleImageIndexBegin = (size_t)AppImage::Detail_SingleMinimum + 1;
    constexpr static size_t SingleImageCount =
        (size_t)AppImage::Detail_SingleMaximum - SingleImageIndexBegin;
private:
    struct SUB_IMAGE
    {
        RECT rc{};
    };

    ComPtr<ID2D1DeviceContext> m_pDC{};

    // -- 图集

    ComPtr<ID2D1Bitmap1> m_pAtlasD2D{};
    ComPtr<IWICBitmapSource> m_pAtlasWic{};
    SUB_IMAGE m_SubImage[AtlasSubImageCount]{};

    // -- 独立图像

    ComPtr<ID2D1Bitmap1> m_pSingleD2D[SingleImageCount]{};
    ComPtr<IWICBitmapSource> m_pSingleWic[SingleImageCount]{};

    // -- 封面

    // 此图像内容布局如下
    // +---------------+-----+---------------+
    // | Default Cover | Gap | Current Cover |
    // +---------------+-----+---------------+
    ComPtr<ID2D1Bitmap1> m_pCoverD2D{};
    ComPtr<IWICBitmapSource> m_pDefaultCoverWic{};// 大小为 CoverWidth * CoverHeight
    BOOL m_bDefaultCover{ TRUE };// 当前是否使用默认封面
public:
    HRESULT PrepareRealization(ID2D1DeviceContext* pDC) noexcept;

    HRESULT AtlasInitialize() noexcept;
    HRESULT AtlasRealize() noexcept;
    EckInlineNdCe auto& AtlasGetWicBitmap() const noexcept { return m_pAtlasWic; }
    Dui::CBitmap AtlasGetSubImage(AppImage eImg) const noexcept;

    HRESULT CoverInitialize() noexcept;
    HRESULT CoverRealize() noexcept;
    // 返回当前封面图像，如果没有，返回默认封面
    Dui::CBitmap CoverGetCurrentImage() noexcept;
    Dui::CBitmap CoverGetSubImage(AppImage eImg) noexcept;
    HRESULT CoverUpdate(IWICBitmapSource* pBitmap) noexcept;
    EckInlineNdCe BOOL CoverIsDefault() const noexcept { return m_bDefaultCover; }
    EckInlineNdCe auto& CoverGetDefaultWicBitmap() const noexcept { return m_pDefaultCoverWic; }

    HRESULT SingleInitialize() noexcept;
    Dui::CBitmap SingleGetImage(AppImage eImg) noexcept;
};