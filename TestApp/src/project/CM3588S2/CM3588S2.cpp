#include "project/CM3588S2/CM3588S2.h"

CM3588S2::CM3588S2() {
    // 初始化按键映射
    Key::keyMap[114] = "VOL-";   // 电源键
    Key::keyMap[115] = "VOL+";   // 音量加
    Key::keyMap[28] = "OK";  // 音量减
}

CM3588S2::CM3588S2( 
                const char* mmcDeviceSizePath, const char* pcieDeviceSizePath,     /* Storage */
                const char* tfCardDeviceSizePath,   /* Tf */
                // const char* keyEventPath, int keyTimeOut,   /* Key */
                const char* rledPath, LED_CTRL rled_status, const char* gledPath, LED_CTRL gled_status, /* Led */
                const char* hwidPath, /* BaseInfo */
                const char* audioEventName, const char* audioCardName,  /* audio */ /* audio */

                const char* fanCtrlPath, FAN_CTRL initialStatus,
                const char* interface, int scanCount,
                const std::string& rtcDevicePath)
    :   RkGenericBoard( 
                        mmcDeviceSizePath, pcieDeviceSizePath, /* Storage */
                        tfCardDeviceSizePath,   /* Tf */
                        // keyEventPath, keyTimeOut,   /* Key */
                        rledPath, rled_status, gledPath, gled_status, /* Led */
                        hwidPath, /* BaseInfo */
                        audioEventName, audioCardName,  /* audio */
                        fanCtrlPath, initialStatus, 
                        interface, scanCount,
                        rtcDevicePath) {

    CM3588S2();
        
    LogInfo("CM3588S2", "CM3588S2 with Fan initialized");
}

bool CM3588S2::getHwId(std::string& hwid) {
    int ret = adcTest("/sys/bus/iio/devices/iio:device0/in_voltage2_raw");
    if (ret < 0) {
        LogError("CM3588S2", "读取HWID失败");
        hwid = "test app read error";
        return false;
    } else if (ret > 4090 && ret <= 4096) {
        hwid = "4096";
    } else {
        hwid = std::to_string(ret);
    }
    return true;
}

bool CM3588S2::getFirmwareVersion(std::string& fwVersion) {
    fwVersion = "CM3588S2_RK3588S2_Ubuntu20.04_20251025.192202";
    return true;
}