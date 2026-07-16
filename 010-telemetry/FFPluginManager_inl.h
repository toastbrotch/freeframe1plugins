//
// Copyright (c) 2004 - InfoMus Lab - DIST - University of Genova
//
// InfoMus Lab (Laboratorio di Informatica Musicale)
// DIST - University of Genova 
//
// http://www.infomus.dist.unige.it
// news://infomus.dist.unige.it
// mailto:staff@infomus.dist.unige.it
//
// Developer: Gualtiero Volpe
// mailto:volpe@infomus.dist.unige.it
//
// Last modified: 2004-11-10
//


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFFPluginManager inline methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool CFFPluginManager::IsProcessFrameCopySupported() const
{
	return m_bIsProcessFrameCopySupported;
}

inline DWORD CFFPluginManager::GetSupportedFormat() const
{
	return m_dwSupportedFormats;
}

inline DWORD CFFPluginManager::GetSupportedOptimization() const
{
	return m_dwSupportedOptimizations;
}

inline int CFFPluginManager::GetMinInputs() const
{
	return m_iMinInputs;
}

inline int CFFPluginManager::GetMaxInputs() const
{
	return m_iMaxInputs;
}

inline int CFFPluginManager::GetNumParams() const
{
	return m_NParams;
}

inline int CFFPluginManager::GetFrameWidth() const
{
	return int(m_VideoInfo.FrameWidth);
}

inline int CFFPluginManager::GetFrameHeight() const
{
	return int(m_VideoInfo.FrameHeight);
}

inline DWORD CFFPluginManager::GetFrameDepth() const
{
	return m_VideoInfo.BitDepth;
}

inline DWORD CFFPluginManager::GetFrameOrientation() const
{
	return m_VideoInfo.Orientation;
}
