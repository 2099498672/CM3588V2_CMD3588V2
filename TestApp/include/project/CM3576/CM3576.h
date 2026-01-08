#ifndef __CM3576_H__
#define __CM3576_H__

#include "hardware/RkGenericBoard.h"
#include "util/Log.h"

class CM3576 : public RkGenericBoard {
public:
    CM3576();
    CM3576( 
            const char* mmcDeviceSizePath, const char* pcieDeviceSizePath,     /* Storage */
            const char* tfCardDeviceSizePath,   /* Tf */
            // const char* keyEventPath, int keyTimeOut,   /* Key */
            const char* rledPath, LED_CTRL rled_status, const char* gledPath, LED_CTRL gled_status, /* Led */
            const char* hwidPath, /* BaseInfo */
            const char* audioEventName,  const char* audioCardName,  /* audio *//* audio */

            const char* fanCtrlPath, FAN_CTRL initialStatus, 
            const char* interface, int scanCount,
            const std::string& rtcDevicePath);


    bool getFirmwareVersion(std::string& fwVersion) override;
    bool getAppVersion(std::string& appVersion) override;
};

#endif