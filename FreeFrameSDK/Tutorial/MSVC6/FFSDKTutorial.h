//
// FFSDKTutorial.h
//
// Plugin header file
//

#ifndef FFSDKTUTORIAL_STANDARD
#define FFSDKTUTORIAL_STANDARD


#include "FFPluginSDK.h"


class CFFSDKTutorial : public CFreeFramePlugin {

public:

	~CFFSDKTutorial();


	///////////////////////////////////////////////////
	// FreeFrame plugin methods
	///////////////////////////////////////////////////
	
	DWORD	SetParameter(const SetParameterStruct* pParam);		
	DWORD	GetParameter(DWORD dwIndex);					
	DWORD	ProcessFrame(void* pFrame);
	

	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////

	static DWORD __stdcall CreateInstance(void** ppInstance);


private:
	
	CFFSDKTutorial();
	
	// Parameters
	float m_Value;

	int m_iValue;
};


#endif
