#ifndef __CM3588V2_CMD3588V2_H__
#define __CM3588V2_CMD3588V2_H__

#include "hardware/RkGenericBoard.h"
#include "util/Log.h"

class CM3588V2_CMD3588V2 : public RkGenericBoard {
public:
    CM3588V2_CMD3588V2();
    CM3588V2_CMD3588V2( 
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

    std::string board_name = "CM3588V2_CMD3588V2";
    std::string get_board_name() override;
};

#endif