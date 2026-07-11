#include "pch.h"

#include "CWndMain.h"

#include "eck\AutoLink.h"

#include "CPlayList.h"

#if defined _WIN64
#pragma comment(lib, R"(Bass\bass_x64.lib)")
#pragma comment(lib, R"(Bass\bass_fx_x64.lib)")
#pragma comment(lib, R"(Bass\bassmidi_x64.lib)")
#elif defined _WIN32
#pragma comment(lib, R"(Bass\bass.lib)")
#pragma comment(lib, R"(Bass\bass_fx.lib)")
#pragma comment(lib, R"(Bass\bassmidi.lib)")
#endif
#pragma comment(lib, "RuntimeObject.lib")

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR pszCmdLine, _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

    if (const auto hr = CoInitialize(nullptr); FAILED(hr))
    {
        EckDbgPrintFormatMessage(hr);
        MessageBoxW(
            nullptr, 
            eck::Format(L"CoInitialize failed: 0x%08X", hr).Data(),
            L"",
            MB_ICONERROR);
        return 0;
    }

    UINT uErr;
    if (const auto r = eck::Initialize(hInstance, nullptr, &uErr);
        r != eck::StartupStatus::Ok)
    {
        EckDbgPrintFormatMessage(uErr);
        MessageBoxW(
            nullptr, 
            eck::Format(LR"(Init failed: %d(0x%08X))", r, uErr).Data(),
            L"",
            MB_ICONERROR);
        return 0;
    }

    eck::PtcCurrent()->UpdateDefaultColor();

    App = new CApp{};
    CApp::Init();
    //#ifdef _DEBUG
    App->ListManager().Add()->LtmSetName(L"测试列表"sv);
    //#endif

    const auto pWnd = new CWindowMain{};
    App->SetMainWindow(pWnd);
    const auto hMon = eck::GetOwnerMonitor(nullptr);
    const auto iDpi = eck::GetMonitorDpi(hMon);
    SIZE Size{ 940, 620 };
    eck::DpiScale(Size, iDpi);
    const auto pt = eck::CalculateCenterWindowPosition(nullptr, Size.cx, Size.cy, FALSE);
    pWnd->SetUserDpi(iDpi);
    pWnd->SetPresentMode(Dui::PresentMode::DCompositionSurface);
    pWnd->SetTransparent(TRUE);
    //pWnd->SetDrawDirtyRect(1);
    pWnd->Create(L"示例Win32程序", WS_POPUP | WS_VISIBLE | WS_CAPTION |
        WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX, 0,
        pt.x, pt.y, Size.cx, Size.cy, nullptr, 0);
    pWnd->Show(SW_SHOW);

    //pWnd->LwShow(TRUE);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (!eck::PreTranslateMessage(msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    delete pWnd;
    delete App;
    eck::ThreadUninitialize();
    eck::Uninitialize();
    CoUninitialize();
#ifdef _DEBUG
    if (eck::g_pDxgiDebug)
        eck::g_pDxgiDebug->ReportLiveObjects(
            DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
#endif
    return (int)msg.wParam;
}