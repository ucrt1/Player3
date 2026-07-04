#pragma once
constexpr inline DWORD BassNull{};

class CBass
{
private:
	enum class MusicType
	{
		Invalid,
		Normal,
		Mod,
		Midi
	};

	MusicType m_eMusicType = MusicType::Invalid;
	DWORD m_hStream = BassNull;
	float m_fDefSpeed = 0.f;
    float m_fVolume = 1.f;
public:
	static int GetError(PCWSTR* ppszErr = nullptr);

	static PCWSTR GetErrorMsg(int iErrCode);

	EckInline static BOOL Init(int iDevice = -1, DWORD dwFreq = 44100,
		DWORD dwFlags = 0, HWND hWnd = nullptr)
	{
		return BASS_Init(iDevice, dwFreq, dwFlags, hWnd, nullptr);
	}

	EckInline static BOOL Free()
	{
		return BASS_Free();
	}

	EckInline static DWORD GetVer()
	{
		return BASS_GetVersion();
	}

	EckInline static void VerToString(DWORD dw, eck::CStringW& rs)
	{
		const WORD wHigh = HIWORD(dw);
		const WORD wLow = LOWORD(dw);
		rs.Format(L"%d.%d.%d.%d", (int)HIBYTE(wHigh), (int)LOBYTE(wHigh),
			(int)HIBYTE(wLow), (int)LOBYTE(wLow));
	}

	~CBass();

	DWORD Open(PCWSTR pszFile,
		DWORD dwFlagsHS = BASS_SAMPLE_FX | BASS_STREAM_DECODE,
		DWORD dwFlagsHM = BASS_SAMPLE_FX | BASS_STREAM_DECODE | BASS_MUSIC_PRESCAN,
		DWORD dwFlagsHMIDI = BASS_SAMPLE_FX | BASS_STREAM_DECODE);

	EckInline BOOL Play(BOOL bReset = FALSE) const
	{
		return BASS_ChannelPlay(m_hStream, bReset);
	}

	EckInline BOOL Pause() const
	{
		return BASS_ChannelPause(m_hStream);
	}

	EckInline BOOL Stop() const
	{
		return BASS_ChannelStop(m_hStream);
	}

	EckInline BOOL SetVolume(float fVolume)
	{
        m_fVolume = fVolume;
		return SetAttr(BASS_ATTRIB_VOL, fVolume);
	}

	EckInline float GetVolume() const
	{
		return GetAttr(BASS_ATTRIB_VOL);
	}

	EckInline BOOL SetSpeed(float fScale) const
	{
		return SetAttr(BASS_ATTRIB_FREQ, fScale * m_fDefSpeed);
	}

	EckInline float GetSpeed() const
	{
		if (eck::FloatEqual(m_fDefSpeed, 0.f))
			return 0.f;
		else [[likely]]
			return GetAttr(BASS_ATTRIB_FREQ) / m_fDefSpeed;
	}

	EckInline BOOL SetPosition(double fTime) const
	{
		return BASS_ChannelSetPosition(m_hStream, BASS_ChannelSeconds2Bytes(m_hStream, fTime), BASS_POS_BYTE);
	}

	EckInline BOOL SetAttr(DWORD dwAttr, float f) const
	{
		return BASS_ChannelSetAttribute(m_hStream, dwAttr, f);
	}

	EckInline float GetAttr(DWORD dwAttr, BOOL* pb = nullptr) const
	{
		float f{};
		const auto b = BASS_ChannelGetAttribute(m_hStream, dwAttr, &f);
		if (pb)
			*pb = b;
		return f;
	}

	EckInline double GetPosition() const
	{
		return BASS_ChannelBytes2Seconds(m_hStream, BASS_ChannelGetPosition(m_hStream, BASS_POS_BYTE));
	}

	EckInline double GetLength() const
	{
		return BASS_ChannelBytes2Seconds(m_hStream, BASS_ChannelGetLength(m_hStream, BASS_POS_BYTE));
	}

	void Close();

	EckInline DWORD GetHStream() const { return m_hStream; }

	EckInline DWORD GetLevel() const
	{
		return BASS_ChannelGetLevel(m_hStream);
	}

	EckInline DWORD GetData(float* pBuf, DWORD cbBuf) const
	{
		return BASS_ChannelGetData(m_hStream, pBuf, cbBuf);
	}

	EckInline DWORD TempoCreate(DWORD dwFlags = BASS_SAMPLE_FX | BASS_FX_FREESOURCE)
	{
		m_hStream = BASS_FX_TempoCreate(m_hStream, dwFlags);
		return m_hStream;
	}

	EckInline DWORD IsActive() const
	{
		return BASS_ChannelIsActive(m_hStream);
	}

	EckInline HSYNC SetSync(DWORD dwType, QWORD ullParam, SYNCPROC pfn, void* pUser = nullptr) const
	{
		return BASS_ChannelSetSync(m_hStream, dwType, ullParam, pfn, pUser);
	}

	EckInline HFX SetFx(DWORD dwType, int iPriority) const
	{
		return BASS_ChannelSetFX(m_hStream, dwType, iPriority);
	}

	EckInline BOOL RemoveFx(HFX hFx) const
	{
		return BASS_ChannelRemoveFX(m_hStream, hFx);
	}
};