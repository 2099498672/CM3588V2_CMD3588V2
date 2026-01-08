#include "project/CM3576/CM3576.h"
#include "util/Log.h"

CM3576::CM3576() {
    
}

CM3576::CM3576( 
                const char* mmcDeviceSizePath, const char* pcieDeviceSizePath,      /* Storage */
                const char* tfCardDeviceSizePath,   /* Tf */
                // const char* keyEventPath, int keyTimeOut,   /* Key */
                const char* rledPath, LED_CTRL rled_status, const char* gledPath, LED_CTRL gled_status, /* Led */
                const char* hwidPath, /* BaseInfo */
                const char* audioEventName, const char* audioCardName,  /* audio */

                const char* fanCtrlPath, FAN_CTRL initialStatus,
                const char* interface, int scanCount,
                const std::string& rtcDevicePath)
    :   RkGenericBoard( 
                        mmcDeviceSizePath, pcieDeviceSizePath,  /* Storage */
                        tfCardDeviceSizePath,   /* Tf */
                        // keyEventPath, keyTimeOut,   /* Key */
                        rledPath, rled_status, gledPath, gled_status, /* Led */
                        hwidPath, /* BaseInfo */
                        audioEventName, audioCardName,  /* audio */

                        fanCtrlPath, initialStatus, 
                        interface, scanCount,
                        rtcDevicePath) {


    Key::keyEventNamesToPath["adc-keys"] = "null";
    Key::keyEventNamesToPath["febd0030.pwm"] = "null";     
    Key::keyMap[114] = "VOL-";   
    Key::keyMap[115] = "VOL+";
    Key::keyMap[139] = "MENU";
    Key::keyMap[158] = "ESC";
    Key::keyMap[28] = "OK";

    Camera::cameraidToInfo["cam0"] = { "cam0", "/dev/video52", "/root/cam0_test.png" };
    Camera::cameraidToInfo["cam1"] = { "cam1", "/dev/video34", "/root/cam1_test.png" };
    Camera::cameraidToInfo["cam2"] = { "cam2", "/dev/video42", "/root/cam2_test.png" };

    CM3576();
    
    log_thread_safe(LOG_LEVEL_INFO, "CM3576", "mmcDeviceSizePath: %s", mmcDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "CM3576", "tfCardDeviceSizePath: %s", tfCardDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "CM3576", "rledPath: %s, rled_status: %d", rledPath, rled_status);
    log_thread_safe(LOG_LEVEL_INFO, "CM3576", "gledPath: %s, gled_status: %d", gledPath, gled_status);
    log_thread_safe(LOG_LEVEL_INFO, "CM3576", "hwidPath: %s", hwidPath); 
    log_thread_safe(LOG_LEVEL_INFO, "CM3576", "audioEventName: %s, audioCardName:%s", audioEventName, audioCardName);
    log_thread_safe(LOG_LEVEL_INFO, "CM3576", "fanCtrlPath: %s, initialStatus: %d", fanCtrlPath, initialStatus);
    log_thread_safe(LOG_LEVEL_INFO, "CM3576", "wifi interface: %s, scanCount: %d", interface, scanCount);
    log_thread_safe(LOG_LEVEL_INFO, "CM3576", "rtcDevicePath: %s", rtcDevicePath.c_str());
}

bool CM3576::getFirmwareVersion(std::string& fwVersion) {
    fwVersion = "CM3576_RK3588_Ubuntu20.04_20241015.101112";
    return true;
}

bool CM3576::getAppVersion(std::string& appVersion) {
    appVersion = "CMBOARD_TESTAPP_CM3576_V20251203_190900";
    return true;
}