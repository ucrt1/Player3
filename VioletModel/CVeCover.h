#pragma once
#include "CVeBase.h"

class CVeCover : public CVeBase
{
private:
	ID2D1Bitmap1* m_pBmp{};
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

	void SetBitmap(ID2D1Bitmap1* pBmp)
	{ 
		std::swap(m_pBmp, pBmp);
		if (m_pBmp)
			m_pBmp->AddRef();
		if (pBmp)
			pBmp->Release();
	}
};