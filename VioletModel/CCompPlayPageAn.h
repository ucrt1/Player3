#pragma once
class CCompositorPlayPage final : public Dui::CCompositorCornerMapping
{
private:
	ComPtr<ID2D1Bitmap1> m_pBitmapOverlay{};
public:
	void PostRender(Dui::COMP_RENDER_INFO& cri) noexcept override
	{
		cri.pDC->DrawBitmap(cri.pBitmap, cri.rcDst, 1.f,
			D2D1_INTERPOLATION_MODE_LINEAR, cri.rcSrc, AtMatrixD2D());
		if (m_pBitmapOverlay)
			cri.pDC->DrawBitmap(m_pBitmapOverlay.Get(), cri.rcDst, GetOpacity(),
				D2D1_INTERPOLATION_MODE_LINEAR, nullptr, AtMatrixD2D());
	}

	void SetOverlayBitmap(ID2D1Bitmap1* pBitmap)
	{
		m_pBitmapOverlay = pBitmap;
	}
};