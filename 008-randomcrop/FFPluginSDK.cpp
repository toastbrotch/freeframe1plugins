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

#include "FFPluginSDK.h"
#include <stdio.h>


// Buffer used by the default implementation of getParameterDisplay
static char s_DisplayValue[5];


////////////////////////////////////////////////////////
// CFreeFramePlugin constructor and destructor
////////////////////////////////////////////////////////

CFreeFramePlugin::CFreeFramePlugin()
: CFFPluginManager()
{
}

CFreeFramePlugin::~CFreeFramePlugin() 
{
}


////////////////////////////////////////////////////////
// Default implementation of CFreeFramePlugin methods
////////////////////////////////////////////////////////

char* CFreeFramePlugin::GetParameterDisplay(DWORD dwIndex) 
{	
	DWORD dwType = m_pPlugin->GetParamType(dwIndex);
	DWORD dwValue = m_pPlugin->GetParameter(dwIndex);
	if ((dwValue != FF_FAIL) && (dwType != FF_FAIL)) {
		if (dwType == FF_TYPE_TEXT)
			return (char*)dwValue;
		else {
			float fValue;
			memcpy(&fValue, &dwValue, 4);
			memset(s_DisplayValue, 0, 5);
			sprintf(s_DisplayValue, "%f", fValue);
			return s_DisplayValue;
		}
	}
	return NULL;
}			

DWORD CFreeFramePlugin::SetParameter(const SetParameterStruct* pParam) 
{
	return FF_FAIL;
}		

DWORD CFreeFramePlugin::GetParameter(DWORD dwIndex) 
{ 
	return FF_FAIL;
}					

DWORD CFreeFramePlugin::ProcessFrame(void* pFrame) 
{
	return FF_FAIL;
}

DWORD CFreeFramePlugin::ProcessFrameCopy(ProcessFrameCopyStruct* pFrameData) 
{
	return FF_FAIL;
}

DWORD CFreeFramePlugin::GetInputStatus(DWORD dwIndex)
{
	if (dwIndex >= (DWORD)GetMaxInputs()) return FF_FAIL;
	return FF_INPUT_INUSE;
}
