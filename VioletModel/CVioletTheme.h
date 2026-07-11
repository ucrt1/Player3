#pragma once
class CVioletTheme final : public Dui::CThemeCommonBase
{
private:
    ComPtr<ID2D1SolidColorBrush> m_pBrush{};
public:
    HRESULT RealizeForDC(ID2D1DeviceContext* pDC) noexcept override
    {
        pDC->CreateSolidColorBrush({}, m_pBrush.AtClear());
        return __super::RealizeForDC(pDC);
    }
    HRESULT DrawBackground(Dui::Part ePart, Dui::State eState,
        const D2D1_RECT_F& rc, _In_opt_ const Dui::DTB_OPT* pOpt) noexcept override;
};