#pragma once
#include "CVioletElement.h"

class CPage : public CVioletElement
{
public:
	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept override;
};