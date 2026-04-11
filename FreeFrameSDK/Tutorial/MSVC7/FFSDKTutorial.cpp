//
// FFSDKTutorial.cpp
//
// Plugin implementation file
//

#include "FFSDKTutorial.h"

#define FFPARAM_VALUE	0


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFPluginInfo PluginInfo ( 
	CFFSDKTutorial::CreateInstance,				// Create method
	"ZEBs",								// Plugin unique ID											
	"FFSDKTutorial",								// Plugin name											
	1,												// API major version number 													
	000,											// API minor version number	
	1,												// Plugin major version number
	000,											// Plugin minor version number
	FF_EFFECT,								// Plugin type
	"A simple plugin showing how to make a FreeFrame plugin using the FreeFrame SDK",						// Plugin description
	"InfoMus Lab - DIST - University of Genova"								// About
);


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

CFFSDKTutorial::CFFSDKTutorial()
: CFreeFramePlugin()
{
	// Plugin properties
	SetProcessFrameCopySupported(false);
	SetSupportedFormats(FF_RGB_24);
	SetSupportedOptimizations(FF_OPT_NONE);
	
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	// Parameters
	SetParamInfo(FFPARAM_VALUE, "Value", FF_TYPE_STANDARD, 0.500000f);
	m_Value = 0.500000f;
}

CFFSDKTutorial::~CFFSDKTutorial()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD CFFSDKTutorial::ProcessFrame(void* pFrame)
{
	if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

	for (int k = 0; k < GetFrameWidth() * GetFrameHeight() * 3; ++k) {
		if ((255 - ((BYTE*)pFrame)[k]) < m_iValue) ((BYTE*)pFrame)[k] = 255;
		else ((BYTE*)pFrame)[k] += m_iValue;
	}
	
	return FF_SUCCESS;
}

DWORD CFFSDKTutorial::GetParameter(DWORD dwIndex)
{
	DWORD dwRet;

	switch (dwIndex) {

	case FFPARAM_VALUE:
		memcpy(&dwRet, &m_Value, 4);
		return dwRet;

	default:
		return FF_FAIL;
	}
}

DWORD CFFSDKTutorial::SetParameter(const SetParameterStruct* pParam)
{
	if (pParam != NULL) {
		
		switch (pParam->ParameterNumber) {

		case FFPARAM_VALUE:
			memcpy(&m_Value, &(pParam->NewParameterValue), 4);
			m_iValue = (BYTE)(m_Value * 255.0f);
			break;

		default:
			return FF_FAIL;
		}

		return FF_SUCCESS;
	
	}

	return FF_FAIL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Factory method
////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD CFFSDKTutorial::CreateInstance(void** ppInstance)
{
	*ppInstance = new CFFSDKTutorial();
	if (*ppInstance != NULL) return FF_SUCCESS;
	return FF_FAIL;
}
