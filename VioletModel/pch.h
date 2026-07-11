#pragma once
#define _CRTDBG_MAP_ALLOC	1
#define ECK_OPT_NO_YYJSON	1
#define ECK_OPT_NO_PUGIXML	1
#define VIOLET_WINRT		0

#include "eck\PchInclude.h"
#include "eck\SystemHelper.h"
#include "eck\CLinearLayout.h"
#include "eck\CLayoutDummy.h"
#include "eck\DuiBuiltinCompositor.h"
#include "eck\CDuiButton.h"
#include "eck\CDuiEdit.h"
#include "eck\CDuiTrackBar.h"
#include "eck\CDuiTitleBar.h"
#include "eck\CDuiLabel.h"
#include "eck\CDuiListView.h"
#include "eck\MediaTagFlac.h"
#include "eck\MediaTagID3v1.h"
#include "eck\MediaTagID3v2.h"
#include "eck\MediaTagApe.h"
#include "eck\Lyric.h"
#include "eck\CDWriteFontFactory.h"
#include "eck\CFileEnumerator.h"
#include "eck\CoroutineHelper.h"
#include "eck\ShellHelper.h"
#include "eck\DwLayoutPath.h"
#include "eck\CTrivialBuffer.h"
#include "eck\ImageHelper.h"
#include "eck\RefPtr.h"

#if VIOLET_WINRT
//#include "eck\WinRtDCompInterop.h"
//#include "eck\BackdropEffectChain.h"
//
//#include <DispatcherQueue.h>
//#include <winrt/Windows.Foundation.Collections.h>
//#include <winrt/Windows.System.h>
//#include <winrt/Windows.Graphics.Effects.h>
//#include <winrt/Windows.UI.Composition.Effects.h>

#include <winrt/Windows.Media.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include <SystemMediaTransportControlsInterop.h>

namespace WinMedia = winrt::Windows::Media;
#endif

#include <lmcons.h>
#include <d2d1_2.h>

using eck::PCVOID;
using eck::PCBYTE;
using eck::SafeRelease;
using eck::ComPtr;
using eck::RefPtr;
using eck::RcPtr;

namespace Dui = eck::Dui;
namespace Tag = eck::MediaTag;
namespace Lyric = eck::Lyric;
namespace Kw = eck::Kw;
using namespace std::literals;

using Dui::EagPoint;

#include "Bass\bass.h"
#include "Bass\bass_fx.h"
#include "Bass\bassmidi.h"

#include "CBass.h"

struct PLDATA// 结构稳定，不能修改
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
	BYTE bMarked : 1{};		// 项目已标记
	BYTE bUpdated : 1{};	// 信息已更新
	BYTE bCoverUpdated : 1{};	// 封面已更新
};

enum class PlType : UINT
{
	Normal,			// 普通列表
	Recent,			// 最近播放
	ViewList,		// 从其他列表导出的视图
	ViewMediaLib,	// 媒体库视图
};
