#pragma once
class CVeCover : public Dui::CElement
{
private:
	ID2D1Bitmap1* m_pBmp{};
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;

	void SetBitmap(ID2D1Bitmap1* pBmp)
	{ 
		ECK_DUILOCK;
		std::swap(m_pBmp, pBmp);
		if (m_pBmp)
			m_pBmp->AddRef();
		if (pBmp)
			pBmp->Release();
	}
};

