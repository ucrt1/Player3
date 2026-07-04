#pragma once
#include "CPlayList.h"

enum class PlayEvt
{
	Play,
	Pause,
	Resume,
	Stop,
	End,
	CommTick,
	ListChanged,
};

struct PLAY_EVT_PARAM
{
	PlayEvt eEvent;
};

enum class PlayErr
{
	Ok,
	NoPlayList,		// 没有播放列表
	ErrBass,		// Bass报告了错误
	ErrHResult,		// HRESULT错误
	UnexpectedPlayingState,// CPlayer::PlayOrPause使用，Bass播放状态异常
	NoCurrItem,		// 当前未播放任何项
	ListEnd,		// 已播放到列表末尾
};

enum class AutoNextMode : BYTE
{
	Min,

	ListLoop = Min,	// 列表循环
	List,		// 列表播放
	Radom,		// 随机播放
	SingleLoop,	// 单曲循环
	Single,		// 单曲播放

	Max,
};
EckInlineNdCe AutoNextMode operator++(AutoNextMode& eMode) noexcept
{
	return eMode = AutoNextMode((eck::UnderlyingType_T<AutoNextMode>)eMode + 1);
}


class CPlayer final
{
private:
	eck::CEventChain<eck::NoIntercept_T, void, const PLAY_EVT_PARAM&> m_Sig{};
	CBass m_Bass{};

	ComPtr<IWICBitmapSource> m_pBmpCover{};
	CPlayList* m_pPlayList{};

	Tag::SimpleData m_MusicInfo{};
	RefPtr<Lyric::CLyric> m_pLrc{};

	int m_idxCurrLrc = -1;
	int m_idxLastLrc = -1;

	double m_lfCurrTime{};// 秒
	double m_lfTotalTime{};// 秒

	DWORD m_dwLastHrOrBassErr{};

	AutoNextMode m_eAutoNextMode{};

	BITBOOL m_bActive : 1{};
	BITBOOL m_bPaused : 1{};	// 是否暂停
	BITBOOL m_bDefCover : 1{ TRUE };	// 是否使用默认封面


	PlayErr PlayWorker(CPlayList::ITEM& e);
	PlayErr PlayIndex(int idx);
	PlayErr PlayIndex(int idxGroup, int idxItem);

	void OnPlayEvent(const PLAY_EVT_PARAM& e);

	BOOL LrcUpdatePosition();

public:
	CPlayer()
	{
		// 保证这是第一个槽
		m_Sig.Connect(this, &CPlayer::OnPlayEvent);
	}

	EckInlineNdCe auto& GetSignal() noexcept { return m_Sig; }
	void SetList(CPlayList* pPlayList) noexcept;
	EckInlineNdCe CPlayList* GetList() const noexcept { return m_pPlayList; }
	EckInlineNdCe BOOL IsActive() const noexcept { return m_bActive; }
	// 秒
	EckInlineNdCe double GetCurrentTime() const noexcept { return m_lfCurrTime; }
	// 秒
	EckInlineNdCe double GetTotalTime() const noexcept { return m_lfTotalTime; }
	EckInlineNdCe auto& GetBass() noexcept { return m_Bass; }

	PlayErr Play(int idx);
	PlayErr Play(int idxGroup, int idxItem);
	PlayErr PlayOrPause();
	PlayErr PlayOrPause(BOOL bPause)
	{
		if (bPause == m_bPaused)
			return PlayErr::Ok;
		return PlayOrPause();
	}
	PlayErr Stop(BOOL bNoGap = FALSE);
	PlayErr Next(BOOL bNoLoop = FALSE);
	PlayErr Prev();
	PlayErr AutoNext();

	// 秒
	void SetPosition(double lfPos);

	auto& GetCover() const{return m_pBmpCover; }

	AutoNextMode NextAutoNextMode();
    EckInlineNdCe BOOL IsRandom() const noexcept { return m_eAutoNextMode == AutoNextMode::Radom; }

	EckInlineNdCe auto& GetMusicInfo() const noexcept { return m_MusicInfo; }
	EckInlineNdCe DWORD GetLastHrOrBassErr() const noexcept { return m_dwLastHrOrBassErr; }
	EckInlineNdCe BOOL IsPaused() const noexcept { return m_bPaused; }
	EckInlineNdCe int GetCurrLrcIdx() const noexcept { return m_idxCurrLrc; }
	EckInlineNd auto& GetLrc() const noexcept { return m_pLrc; }
	EckInlineCe void SetAutoNextMode(AutoNextMode eMode) noexcept { m_eAutoNextMode = eMode; }
	EckInlineNdCe AutoNextMode GetAutoNextMode() const noexcept { return m_eAutoNextMode; }
	EckInlineNdCe BOOL IsDefaultCover() const noexcept { return m_bDefCover; }
};