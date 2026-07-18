#include "pch.h"
#include "CApp.h"

CApplication* App{};

CApplication::CApplication()
{
	m_ptcUiThread = eck::PtcCurrent();
	EckAssert(m_ptcUiThread);
	m_ListManager.LoadList((eck::GetRunningPath() + L"\\List").ToStringView());
}

void CApplication::Init() {}
