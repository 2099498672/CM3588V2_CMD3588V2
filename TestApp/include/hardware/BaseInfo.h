#ifndef __BASEINFO_H__
#define __BASEINFO_H__

#include <stdio.h>

#include "util/Log.h"

class BaseInfo {
private:
    const char* BASEINFO_TAG = "BASEINFO";
    const char* HWID_PATH;

public:
    BaseInfo(const char* hwidPath = "/proc/device-tree/hw-board/hw-version");
    virtual bool getHwId(std::string& hwid);
    virtual bool getFirmwareVersion(std::string& fwVersion);
    virtual bool getAppVersion(std::string& appVersion);

};


#endif // __BASEINFO_H__