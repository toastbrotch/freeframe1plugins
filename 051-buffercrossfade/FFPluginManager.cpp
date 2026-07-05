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

#include "FFPluginManager.h"
#include "FFPluginSDK.h"

#include <stdlib.h> 


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFFPluginManager constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFFPluginManager::CFFPluginManager()
{
	m_bIsProcessFrameCopySupported = false;
	m_dwSupportedFormats = FF_RGB_24;
	m_dwSupportedOptimizations = FF_OPT_NONE;
	m_iMinInputs = 0;
	m_iMaxInputs = 0;

	m_NParams = 0;
	m_pFirst = NULL;
	m_pLast = NULL;
	
	m_VideoInfo.BitDepth = FF_DEPTH_24;
	m_VideoInfo.FrameHeight = 0;
	m_VideoInfo.FrameWidth = 0;
	m_VideoInfo.Orientation = FF_ORIENTATION_TL;
}

CFFPluginManager::~CFFPluginManager()
{
	if (m_pFirst != NULL) {
		ParamInfo* pCurr = m_pFirst;
		ParamInfo* pNext = m_pFirst;
		while (pCurr->pNext != NULL) {
			pNext = pCurr->pNext;
			if ( (pCurr->dwType == FF_TYPE_TEXT) &&
				 (pCurr->StrDefaultValue != NULL) )
			{
				free(pCurr->StrDefaultValue);
			}
			delete pCurr;
			pCurr = pNext;
		}
	}
	m_pFirst = NULL;
	m_pLast = NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFFPluginManager methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFFPluginManager::SetProcessFrameCopySupported(bool bIsSupported)
{
	m_bIsProcessFrameCopySupported = bIsSupported;
}

void CFFPluginManager::SetSupportedFormats(DWORD dwFlags)
{
	m_dwSupportedFormats = dwFlags;
}

void CFFPluginManager::SetSupportedOptimizations(DWORD dwFlags)
{
	m_dwSupportedOptimizations = dwFlags;
}

void CFFPluginManager::SetMinInputs(int iMinInputs)
{
	m_iMinInputs = iMinInputs;
}

void CFFPluginManager::SetMaxInputs(int iMaxInputs)
{
	m_iMaxInputs = iMaxInputs;
}

void CFFPluginManager::SetParamInfo(DWORD dwIndex, const char* pchName, DWORD dwType, float fDefaultValue)
{
	ParamInfo* pInfo = new ParamInfo;
	pInfo->ID = dwIndex;
	
	bool bEndFound = false;
	for (int i = 0; i < 16; ++i) {
		if (pchName[i] == '\0') bEndFound = true;
		pInfo->Name[i] = (bEndFound) ?  0 : pchName[i];
	}
	
	pInfo->dwType = dwType;
	if (fDefaultValue > 1.0) fDefaultValue = 1.0;
	if (fDefaultValue < 0.0) fDefaultValue = 0.0;
	pInfo->DefaultValue = fDefaultValue;
	pInfo->StrDefaultValue = NULL;
	pInfo->pNext = NULL;
	if (m_pFirst == NULL) m_pFirst = pInfo; 
	if (m_pLast != NULL) m_pLast->pNext = pInfo;
	m_pLast = pInfo;
	m_NParams++;
}

void CFFPluginManager::SetParamInfo(DWORD dwIndex, const char* pchName, DWORD dwType, bool bDefaultValue)
{
	ParamInfo* pInfo = new ParamInfo;
	pInfo->ID = dwIndex;
	
	bool bEndFound = false;
	for (int i = 0; i < 16; ++i) {
		if (pchName[i] == '\0') bEndFound = true;
		pInfo->Name[i] = (bEndFound) ?  0 : pchName[i];
	}
	
	pInfo->dwType = dwType;
	pInfo->DefaultValue = bDefaultValue ? 1.0f : 0.0f;
	pInfo->StrDefaultValue = NULL;
	pInfo->pNext = NULL;
	if (m_pFirst == NULL) m_pFirst = pInfo; 
	if (m_pLast != NULL) m_pLast->pNext = pInfo;
	m_pLast = pInfo;
	m_NParams++;
}

void CFFPluginManager::SetParamInfo(DWORD dwIndex, const char* pchName, DWORD dwType, const char* pchDefaultValue)
{
	ParamInfo* pInfo = new ParamInfo;
	pInfo->ID = dwIndex;
	
	bool bEndFound = false;
	for (int i = 0; i < 16; ++i) {
		if (pchName[i] == '\0') bEndFound = true;
		pInfo->Name[i] = (bEndFound) ?  0 : pchName[i];
	}

	pInfo->dwType = dwType;
	pInfo->DefaultValue = 0;
	pInfo->StrDefaultValue = strdup(pchDefaultValue);
	pInfo->pNext = NULL;
	if (m_pFirst == NULL) m_pFirst = pInfo; 
	if (m_pLast != NULL) m_pLast->pNext = pInfo;
	m_pLast = pInfo;
	m_NParams++;
}

char* CFFPluginManager::GetParamName(DWORD dwIndex) const
{
	ParamInfo* pCurr = m_pFirst;
	bool bFound = false;
	while (pCurr != NULL) {
		if (pCurr->ID == dwIndex) {
			bFound = true;
			break;
		}
		pCurr = pCurr->pNext;
	}
	if (bFound) return pCurr->Name;
	return NULL;
}
	
DWORD CFFPluginManager::GetParamType(DWORD dwIndex) const
{
	ParamInfo* pCurr = m_pFirst;
	bool bFound = false;
	while (pCurr != NULL) {
		if (pCurr->ID == dwIndex) {
			bFound = true;
			break;
		}
		pCurr = pCurr->pNext;
	}
	if (bFound) return pCurr->dwType;
	return FF_FAIL;
}

void* CFFPluginManager::GetParamDefault(DWORD dwIndex) const
{
	ParamInfo* pCurr = m_pFirst;
	bool bFound = false;
	while (pCurr != NULL) {
		if (pCurr->ID == dwIndex) {
			bFound = true;
			break;
		}
		pCurr = pCurr->pNext;
	}
	if (bFound) {
		if (GetParamType(dwIndex) == FF_TYPE_TEXT)
			return (void*)pCurr->StrDefaultValue;
		else
			return (void*) &pCurr->DefaultValue;
	}
	return NULL;
}

void CFFPluginManager::SetVideoInfo(const VideoInfoStruct* pVideoInfo)
{
	if (pVideoInfo != NULL) {
		memcpy(&m_VideoInfo, pVideoInfo, sizeof(VideoInfoStruct));
	}
}
