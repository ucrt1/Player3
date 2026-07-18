#pragma once
#include "CPlayList.h"

enum class PlayEvent
{
    Play,
    Pause,
    Resume,
    Stop,
    End,
    CommonTick,
    ListChanged,
};

struct PLAY_EVT_PARAM
{
    PlayEvent eEvent;
};

enum class PlayResult
{
    Ok,
    NoPlayList, // 没有播放列表
    Bass,       // Bass报告错误
    HResult,    // HRESULT错误
    UnexpectedState,// CPlayer::PlayOrPause使用，Bass播放状态异常
    NoCurrItem, // 当前未播放任何项
    ListEnd,    // 已播放到列表末尾
};

enum class AutoNextMode : BYTE
{
    Minimum,

    ListLoop,   // 列表循环
    List,		// 列表播放
    Random,		// 随机播放
    SingleLoop,	// 单曲循环
    Single,		// 单曲播放

    Maximum,
};
EckInlineNdCe AutoNextMode operator++(AutoNextMode& eMode) noexcept
{
    return eMode = AutoNextMode((eck::UnderlyingType_T<AutoNextMode>)eMode + 1);
}


class CPlayer final
{
private:
    eck::CEventChain<eck::NoIntercept_T, void, const PLAY_EVT_PARAM&> m_EventChain{};

    CBass m_Bass{};

    ComPtr<IWICBitmapSource> m_pBitmapCover{};
    RefPtr<CPlayList> m_pPlayList{};

    Tag::SimpleData m_MusicInfo{};

    RefPtr<Lyric::CLyric> m_pLyric{};
    int m_idxCurrLrc = -1;
    int m_idxLastLrc = -1;

    double m_lfCurrTime{};// 秒
    double m_lfTotalTime{};// 秒

    DWORD m_dwLastHrOrBassErr{};

    AutoNextMode m_eAutoNext{};

    BOOLEAN m_bActive{};
    BOOLEAN m_bPaused{};// 是否暂停


    PlayResult PlayWorker(int idx) noexcept;

    void OnPlayEvent(const PLAY_EVT_PARAM& e) noexcept;

    BOOL UpdateLyricLine() noexcept;
public:
    CPlayer() noexcept;

    PlayResult Play(int idx) noexcept;
    PlayResult PlayOrPause() noexcept;
    PlayResult PlayOrPause(BOOL bPause) noexcept
    {
        if (bPause == m_bPaused)
            return PlayResult::Ok;
        return PlayOrPause();
    }
    PlayResult Stop(BOOL bNoGap = FALSE) noexcept;
    PlayResult Next(BOOL bNoLoop = FALSE) noexcept;
    PlayResult Previous() noexcept;
    PlayResult AutoNext() noexcept;

    EckInlineNdCe BOOL IsPaused() const noexcept { return m_bPaused; }
    EckInlineNdCe BOOL IsActive() const noexcept { return m_bActive; }

    // 秒
    EckInlineNdCe double GetCurrentTime() const noexcept { return m_lfCurrTime; }
    // 秒
    EckInlineNdCe double GetTotalTime() const noexcept { return m_lfTotalTime; }
    // 秒
    void SetPosition(double lfPos) noexcept;

    void SetList(RefPtr<CPlayList> pPlayList) noexcept;
    EckInlineNdCe auto& GetList() const noexcept { return m_pPlayList; }

    // 可能返回nullptr
    EckInlineNdCe auto& GetCover() const noexcept { return m_pBitmapCover; }
    EckInlineNdCe auto& GetEventChain() noexcept { return m_EventChain; }
    EckInlineNdCe auto& GetBass() noexcept { return m_Bass; }
    EckInlineNdCe auto& GetLyric() const noexcept { return m_pLyric; }
    EckInlineNdCe auto& GetMusicSimpleData() const noexcept { return m_MusicInfo; }

    AutoNextMode NextAutoNextMode();
    EckInlineNdCe BOOL IsRandom() const noexcept { return m_eAutoNext == AutoNextMode::Random; }
    EckInlineCe void SetAutoNextMode(AutoNextMode eMode) noexcept { m_eAutoNext = eMode; }
    EckInlineNdCe AutoNextMode GetAutoNextMode() const noexcept { return m_eAutoNext; }

    EckInlineNdCe DWORD GetLastHResultOrBassError() const noexcept { return m_dwLastHrOrBassErr; }

    EckInlineNdCe int GetCurrentLyricLine() const noexcept { return m_idxCurrLrc; }
};