#include "hardware/Adc.h"

int Adc::adcTest(std::string adcPath) {
    int fd = open(adcPath.c_str(), O_RDONLY);
    if (fd < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, ADC_TAG, "无法打开ADC设备: %s", adcPath.c_str());
        return -1;
    }

    char buf[16];
    ssize_t len = read(fd, buf, sizeof(buf) - 1);
    if (len < 0) {
        log_thread_safe(LOG_LEVEL_ERROR, ADC_TAG, "读取ADC设备失败: %s", adcPath.c_str());
        close(fd);
        return -1;
    }

    buf[len] = '\0'; // 确保字符串以null结尾
    std::string adcValue(buf);
    log_thread_safe(LOG_LEVEL_INFO, ADC_TAG, "ADC设备 %s 读取值: %s", adcPath.c_str(), adcValue.c_str());
    // 转化为整数值
    int value = std::stoi(adcValue);
    
    close(fd);

    return value;
}