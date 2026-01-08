#include "project/CM3588V2_CMD3588V2/CM3588V2_CMD3588V2.h"

CM3588V2_CMD3588V2::CM3588V2_CMD3588V2() {
}

CM3588V2_CMD3588V2::CM3588V2_CMD3588V2( 
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
    std::string board_name = "RkGenericBoard";

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

    Adc::adc_map["adc4"] = "/sys/bus/iio/devices/iio:device0/in_voltage4_raw";
    Adc::adc_map["adc5"] = "/sys/bus/iio/devices/iio:device0/in_voltage5_raw";
    Adc::adc_map["adc6"] = "/sys/bus/iio/devices/iio:device0/in_voltage6_raw";
    Adc::adc_map["adc7"] = "/sys/bus/iio/devices/iio:device0/in_voltage7_raw";

    CM3588V2_CMD3588V2();
        
    // LogInfo("CM3588V2_CMD3588V2", "CM3588V2_CMD3588V2 with Fan initialized");
    // LogInfo("CM3588V2_CMD3588V2", "mmcDeviceSizePath: %s", mmcDeviceSizePath);
    // LogInfo("CM3588V2_CMD3588V2", "pcieDeviceSizePath: %s", pcieDeviceSizePath);
    // LogInfo("CM3588V2_CMD3588V2", "tfCardDeviceSizePath: %s", tfCardDeviceSizePath);
    // LogInfo("CM3588V2_CMD3588V2", "rledPath: %s, rled_status: %d", rledPath, rled_status);
    // LogInfo("CM3588V2_CMD3588V2", "gledPath: %s, gled_status: %d", gledPath, gled_status);
    // LogInfo("CM3588V2_CMD3588V2", "hwidPath: %s", hwidPath);
    // LogInfo("CM3588V2_CMD3588V2", "audioEventName: %s", audioEventName);
    // LogInfo("CM3588V2_CMD3588V2", "fanCtrlPath: %s", fanCtrlPath);
    // LogInfo("CM3588V2_CMD3588V2", "interface: %s, scanCount: %d", interface, scanCount);
    // LogInfo("CM3588V2_CMD3588V2", "rtcDevicePath: %s", rtcDevicePath.c_str());

    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "CM3588V2_CMD3588V2 with Fan initialized");
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "mmcDeviceSizePath: %s", mmcDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "pcieDeviceSizePath: %s", pcieDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "tfCardDeviceSizePath: %s", tfCardDeviceSizePath);
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "rledPath: %s, rled_status: %d", rledPath, rled_status);
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "gledPath: %s, gled_status: %d", gledPath, gled_status);
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "hwidPath: %s", hwidPath);
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "audioEventName: %s", audioEventName);
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "fanCtrlPath: %s", fanCtrlPath);
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "interface: %s, scanCount: %d", interface, scanCount);
    log_thread_safe(LOG_LEVEL_INFO, "CM3588V2_CMD3588V2", "rtcDevicePath: %s", rtcDevicePath.c_str());
}

bool CM3588V2_CMD3588V2::getFirmwareVersion(std::string& fwVersion) {
    fwVersion = "CM3588V2_CMD3588V2_RK3588_Ubuntu20.04_20241015.101112";
    return true;
}

bool CM3588V2_CMD3588V2::getAppVersion(std::string& appVersion) {
    appVersion = "CMBOARD_TESTAPP_CM3588V2_CMD3588V2_V20251216_103800";
    return true;
}

std::string CM3588V2_CMD3588V2::get_board_name() {
    return board_name;
}