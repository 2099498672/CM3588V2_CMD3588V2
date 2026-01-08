#ifndef __ADC_H__
#define __ADC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <map>

#include "util/Log.h"

class Adc {
public:
    const char* ADC_TAG = "ADC";
    virtual int adcTest(std::string adcPath);

    std::map<std::string, std::string> adc_map;
};

#endif