#include "pch.h"
#include "CApp.h"

constexpr PCWSTR ApplicationBitmapFile[]
{
    LR"(Add.png)",
    LR"(Copy.png)",
    LR"(DefaultCover.png)",
    LR"(File.png)",
    LR"(Folder.png)",
    LR"(Home.png)",
    LR"(License.png)",
    LR"(List.png)",
    LR"(ListPlayList.png)",
    LR"(PlayerVolume0.png)",
    LR"(PlayerVolume1.png)",
    LR"(PlayerVolume2.png)",
    LR"(PlayerVolume3.png)",
    LR"(PlayerVolumeMute.png)",
    LR"(PlayPageDown.png)",
    LR"(PlayPageUp.png)",
    LR"(Plugin.png)",
    LR"(Settings.png)",
    LR"(WindowLogo.png)",
    LR"(ArrowCross.png)",
    LR"(ArrowRight3.png)",
    LR"(Circle.png)",
    LR"(Next.png)",
    LR"(Previous.png)",
    LR"(Triangle.png)",
    LR"(CircleOne.png)",
    LR"(ArrowRight1.png)",
    LR"(Lyric.png)",
    LR"(Pause.png)",
    LR"(NextSolid.png)",
    LR"(PreviousSolid.png)",
    LR"(PauseSolid.png)",
    LR"(TriangleSolid.png)",
    LR"(LockSolid.png)",
    LR"(Cross.png)",
    LR"(CrossSolid.png)",
    LR"(Effect.png)",
    LR"(Locate.png)",
};

static_assert(ARRAYSIZE(ApplicationBitmapFile) == (size_t)AppImage::Maximum);


const ComPtr<IWICBitmapSource>& CApplicationBitmapPool::GetIconWic(AppImage n) noexcept
{
    if (m_pBitmap[(size_t)n])
        return m_pBitmap[(size_t)n];
    const auto i = (size_t)n;
    auto rsPath{ eck::GetRunningPath() };
    rsPath.PushBack(LR"(\Skin\)");
    rsPath.PushBack(ApplicationBitmapFile[i]);
    const auto pszFileName = rsPath.PushBack(48);

    if (FAILED(eck::WicLoadSource(m_pBitmap[i].AtSelfClear(), rsPath.Data())))
    {
        MessageBoxW(
            nullptr,
            eck::Format(L"缺少资源文件：%s", ApplicationBitmapFile[i]).Data(),
            L"VioletModel",
            MB_ICONERROR);
        abort();
    }
    return m_pBitmap[i];
}