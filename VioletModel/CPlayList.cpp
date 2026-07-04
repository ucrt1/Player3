#include "pch.h"
#include "CPlayList.h"
#include "CApp.h"
#include "CPlayListFile.h"


void CPlayList::ImFixGroupIndex(int idxFlatBegin, int nDelta)
{
}

void CPlayList::SetListFile(std::wstring_view svPath, std::wstring_view svFileName)
{
	m_rsListFile = svPath;
	m_rsListFile.PushBackChar(L'\\');
	m_rsListFile.PushBack(svFileName);
	m_rsName = svFileName;
	m_rsName.PazRemoveExtension();
}

void CPlayList::ImEnsureLoaded()
{
	if (m_bNeedInit)
	{
		m_bNeedInit = FALSE;
		CPlayListFileReader r{ m_rsListFile.Data() };
		r.Load(this);
	}
}

BOOL CPlayList::PlyIsSelected() noexcept
{
	return App->GetPlayer().GetList() == this;
}

HRESULT CPlayList::InitFromListFile(PCWSTR pszFile)
{
	const auto rbFile = eck::ReadInFile(pszFile);
	// TODO
	return S_OK;
}

void CPlayList::InvalidateImage()
{
	for (auto& e : m_vFlat)
	{
		e.idxIl = 0;
		e.s.bCoverUpdated = FALSE;
	}
}

int CPlayList::FlInsert(const eck::CStringW& rsFile, int idx)
{
	idx = FlInsertEmpty(idx);
	auto& e = FlAt(idx);
	e.rsFile = rsFile;
	e.rsName.Clear();
	rsFile.PazTrimToFileName(e.rsName);
	return idx;
}

int CPlayList::FlInsertEmpty(int idx)
{
	EckAssert(idx <= (int)m_vFlat.size());
	auto& e = (idx < 0 ? m_vFlat.emplace_back() :
		*m_vFlat.emplace(m_vFlat.begin() + idx));
	if (idx < 0)
		idx = (int)m_vFlat.size() - 1;
	return idx;
}

void CPlayList::FlSchDoSearch(std::wstring_view svKeyWord)
{
	m_vSearchResult.clear();
	if (svKeyWord.empty())
		return;
	for (int i{}; const auto& e : m_vFlat)
	{
		if (e.rsName.FindI(svKeyWord) >= 0 ||
			e.rsArtist.FindI(svKeyWord) >= 0 ||
			e.rsAlbum.FindI(svKeyWord) >= 0)
			m_vSearchResult.emplace_back(i);
		++i;
	}
}

void CPlayList::FlRmShuffle() noexcept
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

void CPlayList::FlRmOnPlayItem(int idxFlat) noexcept
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

int CPlayList::GrInsertGroup(const eck::CStringW& rsName, int idx)
{
	return 0;
}

CPlayList::GROUPIDX CPlayList::GrInsert(const eck::CStringW& rsFile, int idxItem, int idxGroup)
{
	return GROUPIDX();
}