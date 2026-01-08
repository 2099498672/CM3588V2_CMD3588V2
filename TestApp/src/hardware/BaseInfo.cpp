#include "hardware/BaseInfo.h"

BaseInfo::BaseInfo(const char* hwidPath) : HWID_PATH(hwidPath) {

}

bool BaseInfo::getHwId(std::string& hwid) {
    FILE* fp = fopen(HWID_PATH, "r");
    if (!fp) {
        LogError(BASEINFO_TAG, "Cannot open hwid path: %s", HWID_PATH);
        return false;
    }

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        hwid = std::string(buffer);
        // 去除末尾的换行符
        hwid.erase(hwid.find_last_not_of("\n\r") + 1);
        fclose(fp);
        LogInfo(BASEINFO_TAG, "HWID: %s", hwid.c_str());
        return true;
    } else {
        LogError(BASEINFO_TAG, "Failed to read hwid from: %s", HWID_PATH);
        fclose(fp);
        return false;
    }
}

bool BaseInfo::getFirmwareVersion(std::string& fwVersion) {
    return false;
}

bool BaseInfo::getAppVersion(std::string& appVersion) {
    return false;
}