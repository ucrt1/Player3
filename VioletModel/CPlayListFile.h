#pragma once

#pragma region 版本0
//--------For compatibility--------
struct LISTFILEHEADER_0	// 播放列表文件头
{
    CHAR cHeader[4];	// 文件起始标记，ASCII字符串"QKPL"
    int iCount;			// 项目数
    DWORD dwVer;		// 存档文件版本，QKLFVER_常量（见下）
    DWORD dwReserved;	// 保留，必须为0
};

constexpr inline int
QKLFVER_1 = 0,
QKLFVER_2 = 1;

struct LISTFILEITEM_0	// 播放列表文件项目头
{
    UINT uFlags;		// 项目标志
    DWORD dwReserved1;	// 保留，必须为0
    DWORD dwReserved2;	// 保留，必须为0
};

enum
{
    QKLIF_INVALID = 0x00000001,	// 项目无效
    QKLIF_IGNORED = 0x00000002,	// 忽略
    QKLIF_BOOKMARK = 0x00000004,// 有书签
    QKLIF_DUMMY1 = 0x00000008,	// 
    QKLIF_TIME = 0x00000010,	// 有时长字符串
    QKLIF_DUMMY2 = 0x00000020,	// 
};

/*
* LISTFILEHEADER_0
* {
*	LISTFILEITEM_0
*	名称\0
*	文件名\0
*	[仅当具有QKLIF_BOOKMARK时] :
*		书签颜色 : COLORREF
*		书签名称\0
*		书签备注\0
* 	[仅版本2] :
*		时间\0
* }
*/
#pragma endregion 版本0
#pragma region 版本1
//--------For compatibility--------
struct LISTFILEHEADER_1	// 播放列表文件头
{
    CHAR chHeader[4];	// 文件起始标记，ASCII字符串"PNPL"
    int iVer;			// 存档文件版本，PNLFVER_常量
    UINT ocbBookMark;	// 书签信息偏移
    int cItems;			// 项目数
    int cchCreator;		// 创建者署名长度
    // WCHAR szCreator[];
};

constexpr inline int
PNLFVER_0 = 0,
PNBMVER_0 = 0;

struct PLUPUREDATA// 结构稳定，不能修改
{
    UINT uSecTime{};		// 【文件】时长
    UINT uSecPlayed{};		// 【统计】播放总时间
    UINT cPlayed{};			// 【统计】播放次数
    UINT cLoop{};			// 【统计】循环次数
    ULONGLONG ftLastPlayed{};	// 【统计】上次播放时间
    ULONGLONG ftModified{};	// 【文件】修改时间
    ULONGLONG ftCreated{};	// 【文件】创建时间
    USHORT usYear{};		// 【元数据】年代
    USHORT usBitrate{};		// 【元数据】比特率
    BYTE byRating{};		// 【元数据】分级
    BYTE bIgnore : 1{};		// 项目被忽略
    BYTE bBookmark : 1{};	// 项目含书签
    BYTE bNeedUpdated : 1{};// 信息需要更新
    BYTE bMarked : 1{};		// 项目已标记
};
static_assert(sizeof(PLUPUREDATA) <= sizeof(PLDATA));

struct LISTFILEITEM_1	// 播放列表文件项目头
{
    // 下列长度均不包含结尾NULL
    int cchName;	// 名称长度
    int cchFile;	// 文件名长度
    int cchTitle;	// 标题长度
    int cchArtist;	// 艺术家长度
    int cchAlbum;	// 专辑名长度
    int cchGenre;	// 流派长度

    PLUPUREDATA s;
    // WCHAR szName[];
    // WCHAR szFile[];
    // WCHAR szTitle[];
    // WCHAR szArtist[];
    // WCHAR szAlbum[];
    // WCHAR szGenre[];
};

struct BOOKMARKHEADER	// 书签信息头
{
    int iVer;			// 书签信息版本，PNBMVER_常量
    int cBookmarks;		// 书签数
};

struct BOOKMARKITEM		// 书签信息项目头
{
    int idxItem;		// 书签所属项目的索引，取值范围为0到cItems，将显示在指定索引之前
    COLORREF cr;		// 书签颜色
    int cchName;		// 书签名称长度，不包括结尾的\0
    // WCHAR szName[];
};

/*
* LISTFILEHEADER_1
* {
* 	LISTFILEITEM_1
* }
* BOOKMARKHEADER
* {
* 	BOOKMARKITEM
* }
*/
#pragma endregion 版本1
#pragma region 版本2
//--------当前版本--------
enum :int
{
    VLLFVER_0 = 0,
    VLBMVER_0 = 0,
};
struct LISTFILEHEADER_2
{
    CHAR chHeader[4];	// 文件起始标记，ASCII字符串"VLPL"
    int iVer;			// 存档文件版本，VLLFVER_常量
    PlType eType;		// 列表类型
    UINT ocbBookMark;	// 书签信息偏移
    UINT ocbGroupList;	// 组列表偏移
    UINT ocbFlatList;	// 平面列表偏移
    int cItem;			// 项目数
    int cBookmark;		// 书签数
    int cGroup;			// 组数
    int cFlat;			// 平面项目数，目前总与cItem相同
};
struct LISTFILEITEM_2	// 实际项目
{
    // 下列长度均不包含结尾NULL
    int cchName;	// 名称长度
    int cchFile;	// 文件名长度
    int cchTitle;	// 标题长度
    int cchArtist;	// 艺术家长度
    int cchAlbum;	// 专辑名长度
    int cchGenre;	// 流派长度

    PLDATA s;
    // WCHAR szName[];
    // WCHAR szFile[];
    // WCHAR szTitle[];
    // WCHAR szArtist[];
    // WCHAR szAlbum[];
    // WCHAR szGenre[];
};
struct LISTFILE_GROUP	// 组视图
{
    int cchName;	// 组名长度
    int cItem;		// 组内项目数
    // WCHAR szName[];
};
struct LISTFILE_IDXITEM	// 平面视图和组视图的索引项
{
    int idxItem;	// 对应实际项目的索引
};
struct LISTFILE_RECENT	// 最近播放列表
{
    UINT Reserved;
};
struct LISTFILE_VIEWQUERY	// 视图查询
{
    int cchQuery;	// 查询语句长度
    // WCHAR szQuery[];
};
/*
* LISTFILEHEADER_2
* { // 与平面列表的顺序相同
* 	LISTFILEITEM_2
* }
*
* //---PlType::Normal后跟下列结构
* {	// 组视图
* 	LISTFILE_GROUP
*   {
*   	LISTFILE_IDXITEM
*   }
* }
* {	// 平面视图，目前未用，不写入该节
* 	LISTFILE_IDXITEM
* }
* {	// 书签
* 	BOOKMARKITEM
* }
*
* //---PlType::Recent后跟下列结构
* LISTFILE_RECENT
*
* //---PlType::View*后跟下列结构
* LISTFILE_VIEWQUERY
*/
#pragma endregion 版本2

// 此结构引用的字符串必须以NULL字符结尾
struct LISTFILE_STRINFO
{
    std::wstring_view svName;
    std::wstring_view svFile;
    std::wstring_view svTitle;
    std::wstring_view svArtist;
    std::wstring_view svAlbum;
    std::wstring_view svGenre;
};

class CPlayList;

class CPlayListFileReader final
{
private:
    eck::CFile m_File{};
    eck::CFileSectionMap m_Map{};
    LISTFILEHEADER_0* m_pHeader0{};
    LISTFILEHEADER_1* m_pHeader1{};
    LISTFILEHEADER_2* m_pHeader2{};
public:
    using FItemProcessor = std::function<BOOL(const LISTFILEITEM_1* pItem,
        PCWSTR pszName, PCWSTR pszFile, PCWSTR pszTitle,
        PCWSTR pszArtist, PCWSTR pszAlbum, PCWSTR pszGenre)>;
    using FBookmarkProcessor = std::function<BOOL(const BOOKMARKITEM* pItem, PCWSTR pszName)>;
    ECK_DISABLE_COPY_MOVE_DEF_CONS(CPlayListFileReader)
public:
    CPlayListFileReader(PCWSTR pszFile) { Open(pszFile); }

    BOOL Open(PCWSTR pszFile);

    void Load(CPlayList* pList);

    int GetItemCount();

    void ForBookmark(const FBookmarkProcessor& fnProcessor);
};


class CPlayListFileWriter
{
private:
    enum class Stage
    {
        OrgItem,
        Group,
        Flat,
        Bookmark,
    };
    eck::CFile m_File{};
    LISTFILEHEADER_2 m_Header{ {'V','L','P','L'},VLLFVER_0 };
    Stage m_eStage{ Stage::OrgItem };

    void WriteStringView(std::wstring_view sv)
    {
        if (sv.empty())
            m_File << L'\0';
        else
            m_File.Write(sv.data(), DWORD((sv.size() + 1) * sizeof(WCHAR)));
    }
public:
    ECK_DISABLE_COPY_MOVE(CPlayListFileWriter)
public:
    BOOL Open(PCWSTR pszFile, PlType eType);

    void AddOrgItem(const PLDATA& Item, const LISTFILE_STRINFO& StrInfo);

    void SetRecentOptions();

    void SetViewOptions(const eck::CStringW& rsQuery);

    void SetStage(Stage eStage);

    void AddGroup(const eck::CStringW& rsName,
        _In_reads_(cOrg) const int* pidxOrg, size_t cOrg);

    void AddFlat(_In_reads_(cOrg) const int* pidxOrg, size_t cOrg);

    void AddBookmark(const BOOKMARKITEM& Item, eck::CStringW& rsName);

    void BeginBookMark();

    BOOL End();
};