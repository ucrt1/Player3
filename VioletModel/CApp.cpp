#include "pch.h"
#include "CApp.h"

constexpr static D2D1_COLOR_F PalLight[]
{
	Dui::StMakeBackgroundColorLight(0.5f),

	Dui::StMakeBackgroundColorLight(0.5f),
	Dui::StMakeBackgroundColorLight(0.3f),
	Dui::StMakeForegroundColorLight(0.2f),

	Dui::StMakeBackgroundColorLight(0.5f),
	Dui::StMakeForegroundColorLight(0.2f),

	Dui::StMakeForegroundColorLight(0.15f),
	Dui::StMakeForegroundColorLight(0.2f),
	Dui::StMakeForegroundColorLight(0.3f),

	Dui::StMakeBackgroundColorLight(0.5f),

	Dui::StMakeForegroundColorLight(0.6f),

	Dui::StMakeForegroundColorLight(0.6f),
	Dui::StMakeForegroundColorLight(1.0f),
};
constexpr static D2D1_COLOR_F PalDark[]
{
	Dui::StMakeBackgroundColorDark(0.5f),

	Dui::StMakeBackgroundColorDark(0.5f),
	Dui::StMakeBackgroundColorDark(0.3f),
	Dui::StMakeForegroundColorDark(0.2f),

	Dui::StMakeBackgroundColorDark(0.5f),
	Dui::StMakeForegroundColorDark(0.2f),

	Dui::StMakeForegroundColorDark(0.4f),
	Dui::StMakeForegroundColorDark(0.5f),
	Dui::StMakeForegroundColorDark(0.6f),

	Dui::StMakeBackgroundColorDark(0.5f),

	Dui::StMakeForegroundColorDark(0.6f),

	Dui::StMakeForegroundColorDark(0.6f),
	Dui::StMakeForegroundColorDark(1.0f),
};


CApplication* App{};

IWICBitmapSource* CApplication::InvertSkin(IWICBitmapSource* pBmp)
{
	constexpr D2D1_RENDER_TARGET_PROPERTIES Prop
	{
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		{ DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
		96.f,96.f,
		D2D1_RENDER_TARGET_USAGE_NONE,
		D2D1_FEATURE_LEVEL_DEFAULT
	};
	ID2D1RenderTarget* pRT;
	ID2D1Bitmap* pD2dBitmap;

	ID2D1DeviceContext* pDC;
	ID2D1Effect* pEffect;
	IWICBitmap* pNewBitmap;

	UINT cx, cy;

	pBmp->GetSize(&cx, &cy);
	eck::g_pWicFactory->CreateBitmap(cx, cy, eck::DefaultWicPixelFormat,
		WICBitmapCacheOnLoad, &pNewBitmap);
	eck::g_pD2DFactory->CreateWicBitmapRenderTarget(pNewBitmap, &Prop, &pRT);
	pRT->CreateBitmapFromWicBitmap(pBmp, &pD2dBitmap);

	pRT->QueryInterface(&pDC);
	pDC->CreateEffect(CLSID_D2D1ColorMatrix, &pEffect);
	EckAssert(pEffect);
	pEffect->SetInput(0, pD2dBitmap);
	pEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX,
		D2D1::Matrix5x4F(
			-1, 0, 0, 0, 0,
			-1, 0, 0, 0, 0,
			-1, 0, 0, 0, 0,
			1, 0.999f, 0.999f, 0.999f, 0));

	pDC->BeginDraw();
	pDC->Clear({});
	pDC->DrawImage(pEffect);
	pDC->EndDraw();

	pEffect->Release();
	pD2dBitmap->Release();
	pDC->Release();
	pRT->Release();
	return pNewBitmap;
}

void CApplication::LoadSkin(BOOL bLoadAll)
{
	auto rsPath{ eck::GetRunningPath() };
	rsPath.PushBack(LR"(\Skin\)");
	const auto pszFileName = rsPath.PushBack(48);
	const auto iEnd = bLoadAll ? ARRAYSIZE(ApplicationBitmapFile) : (size_t)AppImage::Priv_InvertEnd;
	for (size_t i{}; i < iEnd; ++i)
	{
		wcscpy(pszFileName, ApplicationBitmapFile[i]);
		if (FAILED(eck::WicLoadSource(m_pBitmap[i].AtSelfClear(), rsPath.Data())))
		{
            MessageBoxW(
				nullptr,
				eck::Format(L"缺少资源文件：%s", ApplicationBitmapFile[i]).Data(),
				L"VioletModel",
				MB_ICONERROR);
			abort();
		}
	}
}

CApplication::CApplication()
{
	m_ptcUiThread = eck::PtcCurrent();
	EckAssert(m_ptcUiThread);
	m_ListManager.LoadList();
	LoadSkin(TRUE);
}

void CApplication::Init() {}

const D2D1_COLOR_F& CApplication::GetColor(GPal n) const
{
	return m_bDarkMode ? PalDark[size_t(n)] : PalLight[size_t(n)];
}

void CApplication::SetDarkMode(BOOL bDarkMode)
{
	if (m_bDarkMode == bDarkMode)
		return;
	m_bDarkMode = bDarkMode;
	if (m_bDarkMode)
		for (size_t i{}; i < (size_t)AppImage::Priv_InvertEnd; ++i)
			m_pBitmap[i] = InvertSkin(m_pBitmap[i].Get());
	else
		LoadSkin(FALSE);
}