#include "pch.h"
#include "CPlayList.h"
#include "CApp.h"
#include "CPlayListFile.h"

void CPlayList::LtmSetFile(std::wstring_view svPath, std::wstring_view svFileName) noexcept
{
	m_rsListFile = svPath;
	m_rsListFile.PazAddBackslash();
	m_rsListFile.PushBack(svFileName);
	m_rsName = svFileName;
	m_rsName.PazRemoveExtension();
}

void CPlayList::LtmEnsureLoaded() noexcept
{
	if (m_bLazyInit)
	{
		m_bLazyInit = FALSE;
        LtmInitializeFromListFile(m_rsListFile.Data());
	}
}

HRESULT CPlayList::LtmInitializeFromListFile(PCWSTR pszFile) noexcept
{
	CPlayListFileReader r{ pszFile };
	r.Load(this);
	return S_OK;
}

int CPlayList::FlInsert(const eck::CStringW& rsFile, int idx) noexcept
{
	idx = FlInsertEmpty(idx);
	auto& e = FlAt(idx);
	e.rsFile = rsFile;
	e.rsName.Clear();
	rsFile.PazTrimToFileName(e.rsName);
	return idx;
}

int CPlayList::FlInsertEmpty(int idx) noexcept
{
	EckAssert(idx <= (int)m_vFlat.size());
	auto& e = (idx < 0 ? m_vFlat.emplace_back() :
		*m_vFlat.emplace(m_vFlat.begin() + idx));
	if (idx < 0)
		idx = (int)m_vFlat.size() - 1;
	return idx;
}

void CPlayList::FlDoSearch(std::wstring_view svKeyWord) noexcept
{
	m_vSearchResult.Clear();
	if (svKeyWord.empty())
		return;
	for (int i{}; const auto& e : m_vFlat)
	{
		if (e.rsName.FindI(svKeyWord) >= 0 ||
			e.rsArtist.FindI(svKeyWord) >= 0 ||
			e.rsAlbum.FindI(svKeyWord) >= 0)
			m_vSearchResult.PushBack(i);
		++i;
	}
}

void CPlayList::FlShuffleRandom() noexcept
{
    m_vRandomMapping.ReSize(FlGetCount());
	for (int i{}; auto& e : m_vRandomMapping)
		e = i++;

	eck::CPcg32 Pcg{};
	const auto c = FlGetCount();
    for (int i = c - 1; i > 0; --i)
	{
		const auto j = Pcg.Next(0, i);
		if (i != j)
            std::swap(m_vRandomMapping[i], m_vRandomMapping[j]);
	}
}

BOOL CPlayList::PlyIsSelected() noexcept
{
	return App->Player().GetList().Get() == this;
}

void CPlayList::PlySetCurrentRandomItemFromFlat(int idxFlat) noexcept
{
	if (m_idxCurrFlat < 0)
		return;
	const auto c = FlGetCount();
	for (int i{}; i < c; ++i)
	{
		if (m_vRandomMapping[i] == idxFlat)
		{
            m_idxCurrRandom = i;
			break;
		}
    }
}