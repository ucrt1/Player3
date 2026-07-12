#pragma once
enum class AppImage : BYTE
{
    Add,
    Copy,
    DefaultCover,
    File,
    Folder,
    Home,
    License,
    List,
    ListPlayList,
    PlayerVolume0,
    PlayerVolume1,
    PlayerVolume2,
    PlayerVolume3,
    PlayerVolumeMute,
    PlayPageDown,
    PlayPageUp,
    Plugin,
    Settings,
    WindowLogo,
    ArrowCross,
    ArrowRight3,
    Circle,
    Next,
    Previous,
    Triangle,
    CircleOne,
    ArrowRight1,
    Lyric,
    Pause,
    NextSolid,
    PreviousSolid,
    PauseSolid,
    TriangleSolid,
    LockSolid,
    Cross,
    CrossSolid,
    Effect,
    Locate,

    Maximum
};

class CApplicationBitmapPool
{
private:
    ComPtr<IWICBitmapSource> m_pBitmap[(size_t)AppImage::Maximum]{};

    eck::CD2DImageList m_IconList{};
    BYTE m_IconIndex[(size_t)AppImage::Maximum]{};
public:
    const ComPtr<IWICBitmapSource>& GetIconWic(AppImage n) noexcept;
};