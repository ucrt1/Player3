#include "pch.h"
#include "CApp.h"
#include "CPlayList.h"
#include "Utils.h"


CPlayer::CPlayer() noexcept
{
    // 保证这是第一个槽
    m_EventChain.Connect(this, &CPlayer::OnPlayEvent);
}

PlayResult CPlayer::PlayWorker(int idx) noexcept
{
    Stop(TRUE);
    GetList()->PlySetCurrentItem(idx);

    const auto& e = m_pPlayList->FlAtAbsolutely(idx);
    m_bActive = TRUE;
    m_bPaused = FALSE;
    if (!m_Bass.Open(e.rsFile.Data()))
    {
        m_dwLastHrOrBassErr = CBass::GetError();
        m_bActive = FALSE;
        return PlayResult::Bass;
    }
    m_Bass.TempoCreate();
    m_Bass.Play(TRUE);
    m_lfCurrTime = 0;
    m_lfTotalTime = m_Bass.GetLength();
    m_Bass.SetSync(BASS_SYNC_END | BASS_SYNC_ONETIME | BASS_SYNC_MIXTIME, 0,
        [](DWORD, DWORD, DWORD, PVOID pUser)
        {
            ((eck::ThreadContext*)pUser)->Callback.EnQueueCallback([]
                {
                    App->Player().m_EventChain.Emit({ PlayEvent::End });
                });
        }, eck::PtcCurrent());
    m_MusicInfo.uMask = Tag::MIM_ALL;
    Tag::SIMPLE_OPT Opt{};
    Opt.svArtistDiv = {};
    Opt.svCommDiv = {};
    Opt.uFlags = Tag::SMOF_MOVE;
    VltGetMusicInfo(e.rsFile.Data(), m_MusicInfo, Opt);

    const auto pCover = m_MusicInfo.GetMainCover();
    if (pCover)
    {
        m_bDefCover = FALSE;
        if (pCover->IsLink())
            m_dwLastHrOrBassErr = eck::WicLoadSource(
                m_pBitmapCover.AtSelfClear(), pCover->GetPath().Data());
        else
        {
            const auto pStream = new eck::CStreamView{ pCover->GetData() };
            m_dwLastHrOrBassErr = eck::WicLoadSource(m_pBitmapCover.AtSelfClear(), pStream);
            pStream->Release();
        }
        if (FAILED(m_dwLastHrOrBassErr))
            goto UseDefCover;
    }
    else
    {
    UseDefCover:
        m_bDefCover = TRUE;
        m_pBitmapCover = App->GetImage(AppImage::DefaultCover);
        m_pBitmapCover->AddRef();
    }

    m_pLyric = RefPtr<Lyric::CLyric>::Make();

    auto rsLrcPath{ e.rsFile };
    rsLrcPath.PazRenameExtension(EckArgString(L".lrc"));
    m_pLyric->MgAddDividerString(L" / "sv, {});
    m_pLyric->MgAddDividerString(L" 「"sv, L"」"sv);
    m_pLyric->MgSetDuration((float)m_lfTotalTime);
    if (NT_SUCCESS(m_pLyric->LoadTextFile(rsLrcPath.Data())) &&
        !m_pLyric->IsTextEmpty())
        m_pLyric->ParseLrc();
    if (!m_pLyric->MgGetLineCount() && !m_MusicInfo.rsLrc.IsEmpty())
    {
        m_pLyric->LoadTextMove(std::move(m_MusicInfo.rsLrc));
        m_pLyric->ParseLrc();
    }

    GetEventChain().Emit({ PlayEvent::Play });
    return PlayResult::Ok;
}

void CPlayer::OnPlayEvent(const PLAY_EVT_PARAM& e) noexcept
{
    switch (e.eEvent)
    {
    case PlayEvent::CommonTick:
        m_lfCurrTime = m_Bass.GetPosition();
        UpdateLyricLine();
        break;
    case PlayEvent::End:
        AutoNext();
        break;
    }
}

BOOL CPlayer::UpdateLyricLine() noexcept
{
    if (!m_pLyric)
        return FALSE;
    const auto idxNew = m_pLyric->MgTimeToLine((float)m_lfCurrTime, m_idxCurrLrc);
    if (m_idxCurrLrc != idxNew)
    {
        m_idxCurrLrc = idxNew;
        return TRUE;
    }
    else
        return FALSE;
}

PlayResult CPlayer::Play(int idx) noexcept
{
    if (!m_pPlayList)
        return PlayResult::NoPlayList;
    if (IsRandom())
        GetList()->PlySetCurrentRandomItemFromFlat(idx);
    return PlayWorker(idx);
}

PlayResult CPlayer::PlayOrPause() noexcept
{
    if (!m_pPlayList)
        return PlayResult::NoPlayList;
    if (m_bActive)
    {
        switch (m_Bass.IsActive())
        {
        case BASS_ACTIVE_PLAYING:
            m_Bass.Pause();
            m_bPaused = TRUE;
            GetEventChain().Emit({ PlayEvent::Pause });
            return PlayResult::Ok;
        case BASS_ACTIVE_PAUSED:
            m_Bass.Play();
            m_bPaused = FALSE;
            GetEventChain().Emit({ PlayEvent::Resume });
            return PlayResult::Ok;
        }
        return PlayResult::UnexpectedState;
    }
    else
    {
        if (GetList()->FlGetCount())
            PlayWorker(0);
    }
    return PlayResult::Ok;
}

PlayResult CPlayer::Stop(BOOL bNoGap) noexcept
{
    if (!m_pPlayList)
        return PlayResult::NoPlayList;
    m_Bass.Stop();
    m_Bass.Close();
    m_idxCurrLrc = m_idxLastLrc = -1;
    if (!bNoGap)
    {
        m_bDefCover = TRUE;
        m_bActive = FALSE;
        m_EventChain.Emit({ PlayEvent::Stop });
        GetList()->PlySetCurrentItem(-1);
    }
    return PlayResult::Ok;
}

PlayResult CPlayer::Next(BOOL bNoLoop) noexcept
{
    if (!m_pPlayList)
        return PlayResult::NoPlayList;
    int idxItem, idxGroup;
    if (IsRandom())
        idxItem = GetList()->PlyGetCurrentRandomItem();
    else
        idxItem = GetList()->PlyGetCurrentItem();
    if (idxItem < 0)
        return PlayResult::NoPlayList;
    ++idxItem;
    if (idxItem >= GetList()->FlGetCount())
    {
        if (bNoLoop)
            return PlayResult::ListEnd;
        idxItem = 0;
    }
    if (IsRandom())
    {
        GetList()->PlySetCurrentRandomItem(idxItem);
        return PlayWorker(GetList()->FlRandomIndexToRealIndex(idxItem));
    }
    return PlayWorker(idxItem);
}

PlayResult CPlayer::Previous() noexcept
{
    if (!m_pPlayList)
        return PlayResult::NoPlayList;
    int idxItem, idxGroup;
    if (m_eAutoNext == AutoNextMode::Random)
        idxItem = GetList()->PlyGetCurrentRandomItem();
    else
        idxItem = GetList()->PlyGetCurrentItem();
    if (idxItem < 0)
        return PlayResult::NoPlayList;
    if (idxItem <= 0)
        idxItem = GetList()->FlGetCount() - 1;
    else
        --idxItem;
    if (IsRandom())
    {
        GetList()->PlySetCurrentRandomItem(idxItem);
        return PlayWorker(GetList()->FlRandomIndexToRealIndex(idxItem));
    }
    return PlayWorker(idxItem);
}

PlayResult CPlayer::AutoNext() noexcept
{
    switch (m_eAutoNext)
    {
    case AutoNextMode::ListLoop:
    case AutoNextMode::Random:
        return Next();
    case AutoNextMode::List:
    {
        const auto r = Next(TRUE);
        if (r == PlayResult::ListEnd)
            return Stop();
        return r;
    }
    case AutoNextMode::SingleLoop:
        m_Bass.Play(TRUE);
        return PlayResult::Ok;
    case AutoNextMode::Single:
        return Stop();
    }
    return PlayResult::Ok;
}

void CPlayer::SetPosition(double lfPos) noexcept
{
    m_Bass.SetPosition(lfPos);
    m_lfCurrTime = m_Bass.GetPosition();
}

void CPlayer::SetList(RefPtr<CPlayList> pPlayList) noexcept
{
    if (m_pPlayList.Get() == pPlayList.Get())
        return;
    m_pPlayList = std::move(pPlayList);
    if (m_pPlayList)
    {
        if (m_eAutoNext == AutoNextMode::Random)
            m_pPlayList->FlShuffleRandom();
    }
    GetEventChain().Emit({ PlayEvent::ListChanged });
}

AutoNextMode CPlayer::NextAutoNextMode() noexcept
{
    if (++m_eAutoNext >= AutoNextMode::Maximum)
    {
        m_eAutoNext = AutoNextMode::Minimum;
        ++m_eAutoNext;
    }
    if (GetList())
        if (IsRandom())
        {
            GetList()->FlShuffleRandom();
            const auto idx = GetList()->PlyGetCurrentItem();
            if (idx >= 0)
                GetList()->PlySetCurrentRandomItemFromFlat(idx);
        }
    return m_eAutoNext;
}