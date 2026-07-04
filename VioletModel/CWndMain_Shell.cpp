#include "pch.h"
#include "CWndMain.h"

static HRESULT ScaleImageForButton(
	AppIcon eImg,
	int iDpi,
	_Out_ IWICBitmapSource*& pSource)
{
	const auto cxy = eck::DpiScale(20, iDpi);
	ComPtr<IWICBitmapScaler> pScaler;
	HRESULT hr;
	if (FAILED(hr = eck::g_pWicFactory->CreateBitmapScaler(&pScaler)))
		return hr;
	return pScaler->Initialize(App->GetImg(eImg), cxy, cxy, WICBitmapInterpolationModeFant);
}

HRESULT CWindowMain::TblCreateGhostWindow(PCWSTR pszText)
{
	m_WndTbGhost.Create(pszText, WS_OVERLAPPEDWINDOW,
		WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
		-32000, -32000, 0, 0, nullptr, nullptr);
	return S_OK;
}

HRESULT CWindowMain::TblSetup()
{
	m_pTaskbarList->UnregisterTab(m_WndTbGhost.Handle);
	m_pTaskbarList->RegisterTab(m_WndTbGhost.Handle, Handle);
#pragma warning(suppress: 6387)// 可能为NULL
	m_pTaskbarList->SetTabOrder(m_WndTbGhost.Handle, nullptr);

	HICON hiPrev, hiNext;
	ComPtr<IWICBitmapSource> pBitmap;
	ScaleImageForButton(AppIcon::PrevSolid, GetWindowDpi(), pBitmap.AtSelfClear());
	eck::WicCreateIcon(hiPrev, pBitmap.Get());
	ScaleImageForButton(AppIcon::TriangleSolid, GetWindowDpi(), pBitmap.AtSelfClear());
	eck::WicCreateIcon(m_hiTbPlay, pBitmap.Get());
	ScaleImageForButton(AppIcon::PauseSolid, GetWindowDpi(), pBitmap.AtSelfClear());
	eck::WicCreateIcon(m_hiTbPause, pBitmap.Get());
	ScaleImageForButton(AppIcon::NextSolid, GetWindowDpi(), pBitmap.AtSelfClear());
	eck::WicCreateIcon(hiNext, pBitmap.Get());

	THUMBBUTTON tb[3]{};
	constexpr auto dwMask = THB_ICON | THB_TOOLTIP;
	tb[0].dwMask = dwMask;
	tb[0].hIcon = hiPrev;
	tb[0].iId = IDTBB_PREV;
	eck::TcsCopyLength(tb[0].szTip, EckArgArray(L"上一曲"));

	tb[1].dwMask = dwMask;
	tb[1].hIcon = m_hiTbPlay;
	tb[1].iId = IDTBB_PLAY;
	eck::TcsCopyLength(tb[1].szTip, EckArgArray(L"播放"));

	tb[2].dwMask = dwMask;
	tb[2].hIcon = hiNext;
	tb[2].iId = IDTBB_NEXT;
	eck::TcsCopyLength(tb[2].szTip, EckArgArray(L"下一曲"));

	const auto hr = m_pTaskbarList->ThumbBarAddButtons(
		m_WndTbGhost.Handle, ARRAYSIZE(tb), tb);
	DestroyIcon(hiNext);
	DestroyIcon(hiPrev);
	return S_OK;
}

HRESULT CWindowMain::TblUpdateToolBarIcon()
{
	HICON hiPrev, hiNext;
	ComPtr<IWICBitmapSource> pBitmap;
	ScaleImageForButton(AppIcon::PrevSolid, GetWindowDpi(), pBitmap.AtSelfClear());
	eck::WicCreateIcon(hiPrev, pBitmap.Get());
	ScaleImageForButton(AppIcon::TriangleSolid, GetWindowDpi(), pBitmap.AtSelfClear());
	eck::WicCreateIcon(m_hiTbPlay, pBitmap.Get());
	ScaleImageForButton(AppIcon::PauseSolid, GetWindowDpi(), pBitmap.AtSelfClear());
	eck::WicCreateIcon(m_hiTbPause, pBitmap.Get());
	ScaleImageForButton(AppIcon::NextSolid, GetWindowDpi(), pBitmap.AtSelfClear());
	eck::WicCreateIcon(hiNext, pBitmap.Get());

	THUMBBUTTON tb[3]{};
	constexpr auto dwMask = THB_ICON;
	tb[0].dwMask = dwMask;
	tb[0].hIcon = hiPrev;
	tb[0].iId = IDTBB_PREV;

	tb[1].dwMask = dwMask;
	tb[1].hIcon = m_hiTbPlay;
	tb[1].iId = IDTBB_PLAY;

	tb[2].dwMask = dwMask;
	tb[2].hIcon = hiNext;
	tb[2].iId = IDTBB_NEXT;

	m_pTaskbarList->ThumbBarUpdateButtons(m_WndTbGhost.Handle, ARRAYSIZE(tb), tb);
	DestroyIcon(hiNext);
	DestroyIcon(hiPrev);
	return S_OK;
}

HRESULT CWindowMain::TblCreateObjectAndInit()
{
	if (m_pTaskbarList.Get())
		return S_FALSE;
	m_pTaskbarList.CreateInstance(CLSID_TaskbarList);
	return m_pTaskbarList->HrInit();
}

BOOL CWindowMain::TblOnCommand(WPARAM wParam)
{
	if (HIWORD(wParam) != THBN_CLICKED)
		return FALSE;
	switch (LOWORD(wParam))
	{
	case IDTBB_NEXT:
		App->GetPlayer().Next();
		return TRUE;
	case IDTBB_PLAY:
		App->GetPlayer().PlayOrPause();
		return TRUE;
	case IDTBB_PREV:
		App->GetPlayer().Prev();
		return TRUE;
	}
	return FALSE;
}

HRESULT CWindowMain::TblUpdateState()
{
	HRESULT hr;
	const auto& Player = App->GetPlayer();
	if (Player.IsActive())
		hr = m_pTaskbarList->SetProgressState(
			Handle, Player.IsPaused() ? TBPF_PAUSED : TBPF_NORMAL);
	else
		hr = m_pTaskbarList->SetProgressState(Handle, TBPF_NOPROGRESS);
	if (FAILED(hr)) return hr;

	const auto bPauseIcon = (Player.IsActive() && !Player.IsPaused());
	THUMBBUTTON tb{};
	tb.dwMask = THB_ICON | THB_TOOLTIP;
	tb.iId = IDTBB_PLAY;
	tb.hIcon = bPauseIcon ? m_hiTbPause : m_hiTbPlay;
	if (bPauseIcon)
		eck::TcsCopyLength(tb.szTip, EckArgArray(L"暂停"));
	else
		eck::TcsCopyLength(tb.szTip, EckArgArray(L"播放"));
	return m_pTaskbarList->ThumbBarUpdateButtons(m_WndTbGhost.Handle, 1, &tb);
}

HRESULT CWindowMain::TblUpdateProgress()
{
	const auto& Player = App->GetPlayer();
	return m_pTaskbarList->SetProgressValue(Handle,
		ULONGLONG(Player.GetCurrentTime() * 1000.),
		ULONGLONG(Player.GetTotalTime() * 1000.));
}

HRESULT CWindowMain::TblOnTaskbarButtonCreated()
{
	HRESULT hr;
	if (FAILED(hr = TblSetup()))
		return hr;
	m_WndTbGhost.SetIconicThumbnail();
	if (FAILED(hr = TblUpdateToolBarIcon()))
		return hr;
	if (FAILED(hr = TblUpdateState()))
		return hr;
	if (FAILED(hr = TblUpdateProgress()))
		return hr;
	return S_OK;
}

HRESULT CWindowMain::SmtcInit() noexcept
{
#if VIOLET_WINRT
	HRESULT hr;
	//////////TMPTMPTMP//////////
	// HACK 更合理的创建快捷方式逻辑（要求管理员权限）
	PWSTR pszProgramPath;
	hr = SHGetKnownFolderPath(FOLDERID_CommonPrograms, 0, nullptr, &pszProgramPath);
	if (SUCCEEDED(hr))
	{
		eck::CStringW rsLink{ pszProgramPath };
		rsLink.PushBack(EckArgString(L"\\VioletModel.lnk"));
		if (!PathFileExistsW(rsLink.Data()))
			eck::CreateShortcut(rsLink.Data(),
				NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer);
		CoTaskMemFree(pszProgramPath);
	}
	//////////TMPTMPTMP//////////
	try
	{
		const auto pSmtcInterop = winrt::get_activation_factory<
			WinMedia::SystemMediaTransportControls,
			ISystemMediaTransportControlsInterop>();
		hr = pSmtcInterop->GetForWindow(Handle,
			winrt::guid_of<WinMedia::SystemMediaTransportControls>(),
			winrt::put_abi(m_Smtc));
		if (FAILED(hr)) return hr;
	}
	catch (winrt::hresult_error& e)
	{
		return e.to_abi();
	}

	m_Smtc.IsEnabled(true);
	m_Smtc.IsPlayEnabled(true);
	m_Smtc.IsPauseEnabled(true);
	m_Smtc.IsNextEnabled(true);
	m_Smtc.IsPreviousEnabled(true);

	m_SmtcEvtTokenButtonPressed = m_Smtc.ButtonPressed(
		[this](const WinMedia::SystemMediaTransportControls&,
			const WinMedia::SystemMediaTransportControlsButtonPressedEventArgs& Args)
		{
			const auto eBtn = Args.Button();
			m_ptcUiThread->Callback.EnQueueCallback([eBtn, this]
				{
					switch (eBtn)
					{
					case WinMedia::SystemMediaTransportControlsButton::Play:
						App->GetPlayer().PlayOrPause(FALSE);
						break;
					case WinMedia::SystemMediaTransportControlsButton::Pause:
						App->GetPlayer().PlayOrPause(TRUE);
						break;
					case WinMedia::SystemMediaTransportControlsButton::Next:
						App->GetPlayer().Next();
						break;
					case WinMedia::SystemMediaTransportControlsButton::Previous:
						App->GetPlayer().Prev();
						break;
					default:
						return;
					}
					SmtcUpdateState();
					SmtcUpdateTimeLinePosition();
				});
		});
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

#if VIOLET_WINRT
eck::CoroTask<> CWindowMain::SmtcpCoroUpdateDisplay()
{
	const auto mi = App->GetPlayer().GetMusicInfo();// 复制一份
	auto Token{ co_await eck::CoroGetPromiseToken() };
	co_await eck::CoroResumeBackground();

	auto u = m_Smtc.DisplayUpdater();
	u.ClearAll();
	u.Type(WinMedia::MediaPlaybackType::Music);
	auto MusicProp = u.MusicProperties();
	MusicProp.Artist(mi.slArtist.FrontData());
	MusicProp.Title(mi.rsTitle.Data());
	MusicProp.AlbumTitle(mi.rsAlbum.Data());
	if (!mi.rsGenre.IsEmpty())
	{
		auto Genres = MusicProp.Genres();
		Genres.Append(mi.rsGenre.Data());
	}
	const auto pCover = mi.GetMainCover();
	if (pCover)
	{
		using namespace winrt::Windows::Storage;
		using namespace winrt::Windows::Storage::Streams;
		auto Stream = u.Thumbnail();
		if (pCover->bLink)
		{
			auto Task{ StorageFile::GetFileFromPathAsync(
				std::get<eck::CStringW>(pCover->varPic).Data()) };
			Token.GetPromise().SetCanceller([](void* p)
				{
					((decltype(Task)*)p)->Cancel();
				}, &Task);
			auto File{ co_await Task };
			Token.GetPromise().SetCanceller(nullptr, nullptr);
			u.Thumbnail(RandomAccessStreamReference::CreateFromFile(File));
		}
		else
		{
			InMemoryRandomAccessStream InMemStream;
			DataWriter w{ InMemStream };
			const auto& rb = std::get<eck::CRefBin>(pCover->varPic);
			w.WriteBytes(winrt::array_view<const uint8_t>{ rb.Data(), (uint32_t)rb.Size() });
			auto Task{ w.StoreAsync() };
			Token.GetPromise().SetCanceller([](void* p)
				{
					((decltype(Task)*)p)->Cancel();
				}, &Task);
			co_await Task;
			Token.GetPromise().SetCanceller(nullptr, nullptr);
			w.DetachStream();
			u.Thumbnail(RandomAccessStreamReference::CreateFromStream(InMemStream));
		}
	}
	u.Update();
}
#endif// VIOLET_WINRT

HRESULT CWindowMain::SmtcUpdateDisplay() noexcept
{
#if VIOLET_WINRT
	if (m_TskSmtcUpdateDisplay.hCoroutine &&
		!m_TskSmtcUpdateDisplay.IsCompleted())
	{
		m_TskSmtcUpdateDisplay.TryCancel();
		m_TskSmtcUpdateDisplay.SyncWait();
	}
	m_TskSmtcUpdateDisplay = SmtcpCoroUpdateDisplay();
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

HRESULT CWindowMain::SmtcOnCommonTick() noexcept
{
#if VIOLET_WINRT
	const auto ullTick = NtGetTickCount64();
	if (ullTick - m_ullSmtcTimeLineLastUpdate >= 5000)
	{
		m_ullSmtcTimeLineLastUpdate = ullTick;
		return SmtcUpdateTimeLinePosition();
	}
	else
		return S_FALSE;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

HRESULT CWindowMain::SmtcUpdateTimeLineRange() noexcept
{
#if VIOLET_WINRT
	using winrt::Windows::Foundation::TimeSpan;
	m_SmtcTimeline.StartTime(TimeSpan{});
	m_SmtcTimeline.MinSeekTime(TimeSpan{});

	const auto lfSeconds = App->GetPlayer().GetTotalTime();
	const TimeSpan Dur{ std::chrono::milliseconds{ LONGLONG(lfSeconds * 1000.) } };
	m_SmtcTimeline.EndTime(Dur);
	m_SmtcTimeline.MaxSeekTime(Dur);
	m_Smtc.UpdateTimelineProperties(m_SmtcTimeline);
	m_Smtc.PlaybackStatus(WinMedia::MediaPlaybackStatus::Playing);
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

HRESULT CWindowMain::SmtcUpdateTimeLinePosition() noexcept
{
#if VIOLET_WINRT
	using winrt::Windows::Foundation::TimeSpan;
	const auto lfSeconds = App->GetPlayer().GetCurrentTime();
	const TimeSpan Pos{ std::chrono::milliseconds{ LONGLONG(lfSeconds * 1000.) } };
	m_SmtcTimeline.Position(Pos);
	m_Smtc.UpdateTimelineProperties(m_SmtcTimeline);
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

HRESULT CWindowMain::SmtcUpdateState() noexcept
{
#if VIOLET_WINRT
	const auto& Player = App->GetPlayer();
	if (!Player.IsActive())
	{
		m_Smtc.PlaybackStatus(WinMedia::MediaPlaybackStatus::Closed);
		return S_OK;
	}
	if (Player.IsPaused())
		m_Smtc.PlaybackStatus(WinMedia::MediaPlaybackStatus::Paused);
	else
		m_Smtc.PlaybackStatus(WinMedia::MediaPlaybackStatus::Playing);
	return S_OK;
#else
	return E_NOTIMPL;
#endif// VIOLET_WINRT
}

void CWindowMain::SmtcUnInit() noexcept
{
#if VIOLET_WINRT
	// 若不取消将导致内存泄漏
	m_Smtc.ButtonPressed(m_SmtcEvtTokenButtonPressed);
#endif// VIOLET_WINRT
}