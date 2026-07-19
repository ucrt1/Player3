#pragma once
#include "CVioletAtlas.h"

class CVeBase : public Dui::CElement
{
private:
    RefPtr<CVioletAtlas> m_pAtlas{};
public:
    EckInline void SetAtlas(RefPtr<CVioletAtlas> p) noexcept { m_pAtlas = std::move(p); }
    EckInlineNdCe auto& GetAtlas() const noexcept { return m_pAtlas; }
};