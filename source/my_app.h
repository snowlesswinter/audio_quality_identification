#ifndef _MY_APP_H_
#define _MY_APP_H_

#include "mfc_predefine.h"
#include "resource/resource.h"        // main symbols

class AudioQualityIdentificationApp : public CWinApp
{
public:
    AudioQualityIdentificationApp();

public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();

    DECLARE_MESSAGE_MAP()
};

extern AudioQualityIdentificationApp theApp;

#endif  // _MY_APP_H_