#ifndef __KEY_H__
#define __KEY_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <map>

#include "util/Log.h"

#define DEV_INPUT "/dev/input"
#define EVENT_DEV "event"

class Key {
private:
    const char* KEY_TAG = "KEY";    

public:
    int timeOut;
    int fd;
    std::map<int, std::string> keyMap;
    const char* KEY_EVENT_PATH;
    Key();
    Key(const char* eventPath, int timeOut);
    ~Key();

    // Wait for key press or IR press v1.0
    int waitForKeyAndIrPress(const char* keyPath = "/dev/input/by-path/platform-adc-keys-event",            /*adc-keys*/
                             const char* irKeyPath = "/dev/input/by-path/platform-febd0030.pwm-event",    /*febd0030.pwm*/
                             int timeOut = 60);

    // Wait for key press v2.0
    std::map<std::string, std::string> keyEventNamesToPath;
    virtual int waitForKeyPress(int timeOut = 60);
};

#endif