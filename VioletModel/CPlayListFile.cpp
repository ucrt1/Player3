#include "pch.h"
#include "CPlayListFile.h"
#include "CPlayList.h"


BOOL CPlayListFileReader::Open(PCWSTR pszFile)
{
    if (!NT_SUCCESS(m_File.Create(pszFile, FILE_OPEN, FILE_GENERIC_READ)))
        return FALSE;
    if (!m_Map.Create(m_File.Get()))
        return FALSE;
    const auto p = m_Map.Map();

    if (memcmp(p, "VLPL", 4) == 0)
    {
        m_pHeader0 = nullptr;
        m_pHeader1 = nullptr;
        m_pHeader2 = (LISTFILEHEADER_2*)p;
        return TRUE;
    }
    else if (memcmp(p, "PNPL", 4) == 0)
    {
        m_pHeader0 = nullptr;
        m_pHeader1 = (LISTFILEHEADER_1*)p;
        m_pHeader2 = nullptr;
        return TRUE;
    }
    else if (memcmp(p, "QKPL", 4) == 0)
    {
        m_pHeader0 = (LISTFILEHEADER_0*)p;
        m_pHeader1 = nullptr;
        m_pHeader2 = nullptr;
        return TRUE;
    }
    else
        return FALSE;
}

void CPlayListFileReader::Load(CPlayList* pList)
{
    eck::CMemoryReader r{};
    if (m_pHeader2)
    {
        r.SetData(m_pHeader2, (size_t)m_File.GetSize());
        r += sizeof(LISTFILEHEADER_2);
        pList->ImReserveIncrement(m_pHeader2->cItem);

        const LISTFILEITEM_2* pItem;
        EckCounter(m_pHeader2->cItem, i)
        {
            auto& e = pList->FlAt(pList->FlInsertEmpty());
            r.SkipPointer(pItem);
            e.s = pItem->s;
            e.s.bCoverUpdated = FALSE;

            e.rsName.Assign((PCWSTR)r.Data(), pItem->cchName);
            r += eck::CchToCbW(pItem->cchName);
            e.rsFile.Assign((PCWSTR)r.Data(), pItem->cchFile);
            r += eck::CchToCbW(pItem->cchFile);
            e.rsTitle.Assign((PCWSTR)r.Data(), pItem->cchTitle);
            r += eck::CchToCbW(pItem->cchTitle);
            e.rsArtist.Assign((PCWSTR)r.Data(), pItem->cchArtist);
            r += eck::CchToCbW(pItem->cchArtist);
            e.rsAlbum.Assign((PCWSTR)r.Data(), pItem->cchAlbum);
            r += eck::CchToCbW(pItem->cchAlbum);
            e.rsGenre.Assign((PCWSTR)r.Data(), pItem->cchGenre);
            r += eck::CchToCbW(pItem->cchGenre);
        }
    }
    else if (m_pHeader1)
    {
        r.SetData(m_pHeader1, (size_t)m_File.GetSize());

        r += sizeof(LISTFILEHEADER_1);
        r += eck::CchToCbW(m_pHeader1->cchCreator);
        pList->ImReserveIncrement(m_pHeader1->cItems);

        const LISTFILEITEM_1* pItem;
        EckCounter(m_pHeader1->cItems, i)
        {
            auto& e = pList->FlAt(pList->FlInsertEmpty());
            r.SkipPointer(pItem);
            memcpy(&e.s, &pItem->s, sizeof(PLUPUREDATA));
            e.s.bMarked = pItem->s.bMarked;
            e.s.bCoverUpdated = FALSE;
            e.s.bUpdated = TRUE;

            e.rsName.Assign((PCWSTR)r.Data(), pItem->cchName);
            r += eck::CchToCbW(pItem->cchName);
            e.rsFile.Assign((PCWSTR)r.Data(), pItem->cchFile);
            r += eck::CchToCbW(pItem->cchFile);
            e.rsTitle.Assign((PCWSTR)r.Data(), pItem->cchTitle);
            r += eck::CchToCbW(pItem->cchTitle);
            e.rsArtist.Assign((PCWSTR)r.Data(), pItem->cchArtist);
            r += eck::CchToCbW(pItem->cchArtist);
            e.rsAlbum.Assign((PCWSTR)r.Data(), pItem->cchAlbum);
            r += eck::CchToCbW(pItem->cchAlbum);
            e.rsGenre.Assign((PCWSTR)r.Data(), pItem->cchGenre);
            r += eck::CchToCbW(pItem->cchGenre);
        }
    }
    else if (m_pHeader0)
    {
        r.SetData(m_pHeader0, (size_t)m_File.GetSize());

        r += sizeof(LISTFILEHEADER_0);
        pList->ImReserveIncrement(m_pHeader0->iCount);

        const LISTFILEITEM_0* pItem;
        EckCounter(m_pHeader0->iCount, i)
        {
            auto& e = pList->FlAt(pList->FlInsertEmpty());
            r.SkipPointer(pItem);
            e.s.bIgnore = eck::IsBitSet(pItem->uFlags, QKLIF_IGNORED);

            auto pText = (PCWSTR)r.Data();
            auto cchText = (int)wcslen(pText);
            e.rsName.Assign(pText, cchText);
            r += eck::CchToCbW(cchText);

            pText = (PCWSTR)r.Data();
            cchText = (int)wcslen(pText);
            e.rsFile.Assign(pText, cchText);
            r += eck::CchToCbW(cchText);

            if (eck::IsBitSet(pItem->uFlags, QKLIF_BOOKMARK))
            {
                e.s.bBookmark = TRUE;
                r += sizeof(COLORREF);
                r += ((wcslen((PCWSTR)r.Data()) + 1) * sizeof(WCHAR));
                r += ((wcslen((PCWSTR)r.Data()) + 1) * sizeof(WCHAR));
            }
            else
            {
                r += sizeof(COLORREF) + sizeof(WCHAR) * 2;
                e.s.bBookmark = FALSE;
            }

            if (m_pHeader0->dwVer == QKLFVER_2)
                r += (((int)wcslen((PCWSTR)r.Data()) + 1) * sizeof(WCHAR));
        }
    }
}

int CPlayListFileReader::GetItemCount()
{
    if (m_pHeader2)
        return m_pHeader2->cItem;
    else if (m_pHeader1)
        return m_pHeader1->cItems;
    else if (m_pHeader0)
        return m_pHeader0->iCount;
    else
        return 0;
}

void CPlayListFileReader::ForBookmark(const FBookmarkProcessor& fnProcessor)
{
    eck::CMemoryReader r{};
    PCWSTR pszName;
    if (m_pHeader1)
    {
        r.SetData(m_pHeader1, (size_t)m_File.GetSize());

        r += m_pHeader1->ocbBookMark;
        const BOOKMARKHEADER* pHeader;
        const BOOKMARKITEM* pItem;
        r.SkipPointer(pHeader);
        EckCounter(pHeader->cBookmarks, i)
        {
            r.SkipPointer(pItem);
            pszName = (PCWSTR)r.Data();
            r += ((pItem->cchName + 1) * sizeof(WCHAR));
            fnProcessor(pItem, pszName);
        }
    }
    else if (m_pHeader0)
    {
        r.SetData(m_pHeader0, (size_t)m_File.GetSize());

        r += sizeof(LISTFILEHEADER_0);
        BOOKMARKITEM Item;
        const LISTFILEITEM_0* pItem;
        EckCounter(m_pHeader0->iCount, i)
        {
            r.SkipPointer(pItem);

            r += ((wcslen((PCWSTR)r.Data()) + 1) * sizeof(WCHAR));
            r += ((wcslen((PCWSTR)r.Data()) + 1) * sizeof(WCHAR));

            if (eck::IsBitSet(pItem->uFlags, QKLIF_BOOKMARK))
            {
                r >> Item.cr;

                pszName = (PCWSTR)r.Data();
                Item.cchName = (int)wcslen(pszName);
                r += ((Item.cchName + 1) * sizeof(WCHAR));

                Item.idxItem = i;

                fnProcessor(&Item, pszName);

                r += ((wcslen((PCWSTR)r.Data()) + 1) * sizeof(WCHAR));
            }
            else
                r += sizeof(COLORREF) + sizeof(WCHAR) * 2;

            if (m_pHeader0->dwVer == QKLFVER_2)
                r += ((wcslen((PCWSTR)r.Data()) + 1) * sizeof(WCHAR));
        }
    }
    else
        EckDbgBreak();
}





BOOL CPlayListFileWriter::Open(PCWSTR pszFile, PlType eType)
{
    if (!NT_SUCCESS(m_File.Create(pszFile, FILE_OVERWRITE_IF, FILE_GENERIC_WRITE)))
        return FALSE;
    m_File.SeekToBegin();
    m_File += sizeof(LISTFILEHEADER_1);
    return TRUE;
}

void CPlayListFileWriter::AddOrgItem(const PLDATA& Item, const LISTFILE_STRINFO& StrInfo)
{
    ++m_Header.cItem;
    LISTFILEITEM_2 Item2;
    Item2.cchName = (int)StrInfo.svName.size();
    Item2.cchFile = (int)StrInfo.svFile.size();
    Item2.cchTitle = (int)StrInfo.svTitle.size();
    Item2.cchArtist = (int)StrInfo.svArtist.size();
    Item2.cchAlbum = (int)StrInfo.svAlbum.size();
    Item2.cchGenre = (int)StrInfo.svGenre.size();
    Item2.s = Item;
    m_File << Item2;
    WriteStringView(StrInfo.svName);
    WriteStringView(StrInfo.svFile);
    WriteStringView(StrInfo.svTitle);
    WriteStringView(StrInfo.svArtist);
    WriteStringView(StrInfo.svAlbum);
    WriteStringView(StrInfo.svGenre);
}

void CPlayListFileWriter::SetRecentOptions()
{
}

void CPlayListFileWriter::SetViewOptions(const eck::CStringW& rsQuery)
{
}

void CPlayListFileWriter::SetStage(Stage eStage)
{
    m_eStage = eStage;
}

void CPlayListFileWriter::AddGroup(const eck::CStringW& rsName,
    _In_reads_(cOrg) const int* pidxOrg, size_t cOrg)
{
    LISTFILE_GROUP Group;
    Group.cchName = (int)rsName.Size();
    Group.cItem = (int)cOrg;
    m_File << Group;
    WriteStringView(rsName.ToStringView());
    m_File.Write(pidxOrg, DWORD(cOrg * sizeof(int)));
}

void CPlayListFileWriter::AddFlat(
    _In_reads_(cOrg) const int* pidxOrg, size_t cOrg)
{
    m_File.Write(pidxOrg, DWORD(cOrg * sizeof(int)));
}

void CPlayListFileWriter::BeginBookMark()
{
    m_Header.ocbBookMark = (UINT)m_File.GetPosition();
    m_File += sizeof(BOOKMARKHEADER);
}

void CPlayListFileWriter::AddBookmark(const BOOKMARKITEM& Item, eck::CStringW& rsName)
{
    auto Item2{ Item };
    Item2.cchName = (int)rsName.Size();
    m_File << Item2;
    WriteStringView(rsName.ToStringView());
}

BOOL CPlayListFileWriter::End()
{
    auto dwCurr = m_File.GetPosition();
    m_File.SeekToBegin() << m_Header;
    m_File.Seek(dwCurr).End();
    m_File.Clear();
    m_Header = { {'V','L','P','L'},VLLFVER_0 };
    return 0;
}