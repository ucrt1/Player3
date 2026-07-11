#pragma once
struct NM_DTL_GET_TIME : Dui::ELENMHDR
{
    float fTime;
};

class CVeDtLrc : public Dui::CElement, public eck::ITimeLine
{
public:
    constexpr static int c_InvalidCacheIdx = std::numeric_limits<int>::min();
private:
    enum : size_t
    {
        BriMain,
        BriTrans,
        BriMainHiLight,
        BriTransHiLight,
        BriBorder,
        BriShadow,

        BriMax
    };

    Lyric::CLyric* m_pLrc{};
    int m_idxCurr{ -1 };
public:
    LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

    void TlTick(int iMs) noexcept override;
    BOOL TlIsValid() noexcept override { return FALSE; }

    HRESULT LrcSetCurrentLine(int idx);
    void LrcSetEmptyText(std::wstring_view svEmptyText);

    void SetTextFormatTranslation(IDWriteTextFormat* pTf);
    EckInlineNdCe auto GetTextFormatTrans() const { return (IDWriteTextFormat*)0; }

    void SetLyric(Lyric::CLyric* pLrc);
    EckInlineNdCe auto GetLyric() const { return m_pLrc; }
};