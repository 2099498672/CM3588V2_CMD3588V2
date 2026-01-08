#include "hardware/Key.h"
#include <cstring>
#include <dirent.h>
#include <sys/ioctl.h>
#include <vector>

Key::Key() :timeOut(60), fd(-1) {

}

Key::Key(const char* eventPath, int timeOut) : KEY_EVENT_PATH(eventPath), timeOut(timeOut), fd(-1) {

}

Key::~Key() {
    if(fd >= 0) {
        close(fd);
    }
}

// int Key::waitForKeyPress(int timeOut) {
//     fd = open(KEY_EVENT_PATH, O_RDONLY);
//     if(fd < 0) {
//         LogError(KEY_TAG, "Failed to open key event path: %s", KEY_EVENT_PATH);
//         fd = -1;
//     }


//     struct input_event ev;
//     fd_set readfds;
//     struct timeval timeout;

//     // 初始化文件描述符集
//     FD_ZERO(&readfds);
//     FD_SET(fd, &readfds);

//     // 设置超时时间
//     timeout.tv_sec = timeOut;
//     timeout.tv_usec = 0;

//     printf("开始监听 %s，超时时间: %d秒...\n", KEY_EVENT_PATH, timeOut);

//     // 使用select等待事件或超时
//     int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
//     if (ret == -1) {
//         perror("select错误");
//         close(fd);
//         fd = -1;
//         return -1;
//     } 
//     else if (ret == 0) {
//         // 超时
//         printf("超时，%d秒内没有按键按下\n", timeOut);
//         return -1;
//     }

//     // 有事件发生，读取事件
//     ssize_t n = read(fd, &ev, sizeof(ev));
//     if (n == -1) {
//         perror("读取事件失败");
//         return -1;
//     }

//     // 判断是否为按键事件
//     if (ev.type == EV_KEY) {
//         // 按键按下事件
//         if (ev.value == 1) {
//             printf("检测到按键按下 - 按键码: %d\n", ev.code);
//             // 打印按键名称
//             // const char* keyName = getKeyName(ev.code);
//             // 根据用户要求，有按键按下时返回"没有按键按下"
//             // printf("没有按键按下\n");
//         }
//         // 按键释放事件
//         // else if (ev.value == 0) {
//         //     printf("检测到按键释放 - 按键码: %d\n", ev.code);
//         // }
//     }
//     return ev.code;
// }

int Key::waitForKeyPress(int timeOut) {
    if (keyEventNamesToPath.empty()) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "Key event names to path map is empty.");
        return -1;
    }

    // 遍历打印map键
    for (const auto& pair : keyEventNamesToPath) {
        const std::string& eventName = pair.first;
        log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "Listening for key event: %s", eventName.c_str());
    }

    DIR *dir;
    struct dirent *entry;
    int temp_fd = -1;
    char device_name[256] = {0};
    char device_path[256] = {0};
    dir = opendir(DEV_INPUT);
    if (dir == NULL) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "无法打开输入设备目录: %s", DEV_INPUT);
        return -1;
    }

    int keyEventCount = 0;
    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "开始扫描输入设备目录: %s", DEV_INPUT);
    while ((entry = readdir(dir)) != NULL) {
        // 检查是否为 event 设备
        if (strncmp(entry->d_name, EVENT_DEV, strlen(EVENT_DEV)) == 0) {
            memset(device_name, 0, sizeof(device_name));
            memset(device_path, 0, sizeof(device_path));
            snprintf(device_path, sizeof(device_path), "%s/%s", DEV_INPUT, entry->d_name);

            temp_fd = open(device_path, O_RDONLY);
            if (temp_fd == -1) {
                log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "无法打开设备文件: %s", device_path);
                continue;
            }

            if (ioctl(temp_fd, EVIOCGNAME(sizeof(device_name)), device_name) < 0) {
                log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "ioctl EVIOCGNAME 失败: %s", strerror(errno));
                close(temp_fd);
                continue;
            }

            log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "检查设备: %s -> %s", device_path, device_name);
            for (auto& pair : keyEventNamesToPath) {
                const std::string& eventName = pair.first;
                if (strstr(device_name, eventName.c_str()) != NULL) {
                    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "匹配按键事件设备: %s -> %s", device_path, device_name);
                    pair.second = device_path; // 设置map中的路径值
                    keyEventCount++;
                }
            }
        }
    }
    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "需要的按键事件设备数量: %zu", keyEventNamesToPath.size());
    if (keyEventCount != keyEventNamesToPath.size()) {
        log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "已找到的按键事件设备数量: %d", keyEventCount);
        closedir(dir);
        return -1;
    }
    closedir(dir);

    int device_fds[16];
    int device_count = 0;
    int maxFd = -1;
    bool allOpened = true;
    for (const auto& pair : keyEventNamesToPath) {
        if (device_count >= 16) {
            log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "设备数量超过最大限制16");
            break;
        }
        const std::string& eventPath = pair.second;
        device_fds[device_count] = open(eventPath.c_str(), O_RDONLY);
        maxFd = (device_fds[device_count] > maxFd) ? device_fds[device_count] : maxFd;
        if (device_fds[device_count] < 0) {
            log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "无法打开目标设备文件: %s", eventPath.c_str());
            allOpened = false;
        }
        device_count++;
    }

    if (!allOpened) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "未能打开所有目标设备文件");
        for (int i = 0; i < device_count; i++) {
            if (device_fds[i] >= 0) {
                close(device_fds[i]);
            }
        }
        return -1;
    }

    struct input_event ev;
    fd_set readfds;
    struct timeval timeout;
    // 初始化文件描述符集
    FD_ZERO(&readfds);
    for(int i = 0; i < device_count; i++) {
        FD_SET(device_fds[i], &readfds);
    }

    // 设置超时时间
    timeout.tv_sec = timeOut;
    timeout.tv_usec = 0;
    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "开始监听按键事件，超时时间: %d秒...", timeOut);
    // 使用select等待事件或超时
    int ret = select(maxFd + 1, &readfds, NULL, NULL, &timeout);
    if (ret == -1) {
        log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "select错误: %s", strerror(errno));
        for (int i = 0; i < device_count; i++) {
            close(device_fds[i]);
        }
        return -1;
    } else if (ret == 0) {
        // 超时
        log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "超时，%d秒内没有按键按下", timeOut);
        for (int i = 0; i < device_count; i++) {
            close(device_fds[i]);
        }
        return 0;
    }

    int pressedKeyCode = -1;
    for (int i = 0; i < device_count; i++) {
        if (FD_ISSET(device_fds[i], &readfds)) {
            // 有事件发生，读取事件
            ssize_t n = read(device_fds[i], &ev, sizeof(ev));
            if (n == -1) {
                log_thread_safe(LOG_LEVEL_ERROR, KEY_TAG, "读取事件失败: %s", strerror(errno));
                continue;
            }

            // 判断是否为按键事件
            if (ev.type == EV_KEY) {
                // 按键按下事件
                if (ev.value == 1) {
                    log_thread_safe(LOG_LEVEL_INFO, KEY_TAG, "检测到按键按下 - 按键码: %d", ev.code);
                    pressedKeyCode = ev.code;
                    break; // 只处理第一个检测到的按键
                }
            }
        }
    }
    for (int i = 0; i < device_count; i++) {
        close(device_fds[i]);
    }
    return pressedKeyCode;
}

int Key::waitForKeyAndIrPress(const char* keyPath, const char* irKeyPath, int timeOut) {
    int keyFd = open(keyPath, O_RDONLY);
    if (keyFd < 0) {
        LogError(KEY_TAG, "Failed to open key event path: %s", keyPath);
        return -1;
    }

    int irFd = open(irKeyPath, O_RDONLY);
    if (irFd < 0) {
        LogError(KEY_TAG, "Failed to open IR key event path: %s", irKeyPath);
        close(keyFd);
        return -1;
    }

    struct input_event ev;
    fd_set readfds;
    struct timeval timeout;

    // 初始化文件描述符集
    FD_ZERO(&readfds);
    FD_SET(keyFd, &readfds);
    FD_SET(irFd, &readfds);
    int maxFd = (keyFd > irFd) ? keyFd : irFd;

    // 设置超时时间
    timeout.tv_sec = timeOut;
    timeout.tv_usec = 0;

    printf("开始监听按键和红外按键，超时时间: %d秒...\n", timeOut);

    // 使用select等待事件或超时
    int ret = select(maxFd + 1, &readfds, NULL, NULL, &timeout);
    if (ret == -1) {
        perror("select错误");
        close(keyFd);
        close(irFd);
        return -1;
    } 
    else if (ret == 0) {
        // 超时
        printf("超时，%d秒内没有按键或红外按键按下\n", timeOut);
        close(keyFd);
        close(irFd);
        return -1;
    }

    int activeFd = -1;
    if (FD_ISSET(keyFd, &readfds)) {
        activeFd = keyFd;
    } else if (FD_ISSET(irFd, &readfds)) {
        activeFd = irFd;
    }

    // 有事件发生，读取事件
    ssize_t n = read(activeFd, &ev, sizeof(ev));
    if (n == -1) {
        perror("读取事件失败");
        close(keyFd);
        close(irFd);
        return -1;
    }

    // 判断是否为按键事件
    if (ev.type == EV_KEY) {
        // 按键按下事件
        if (ev.value == 1) {
            if (activeFd == keyFd) {
                printf("检测到按键按下 - 按键码: %d\n", ev.code);
            } else {
                printf("检测到红外按键按下 - 按键码: %d\n", ev.code);
            }
        }
    }
    close(keyFd);
    close(irFd);
    return ev.code;
}

// int Key::waitForKeyAndIrPress(const char* keyPath, const char* irKeyPath, int timeOut) {
//     int keyFd = open(keyPath, O_RDONLY);
//     if (keyFd < 0) {
//         LogError(KEY_TAG, "Failed to open key event path: %s", keyPath);
//         return -1;
//     }

//     int irFd = open(irKeyPath, O_RDONLY);
//     if (irFd < 0) {
//         LogError(KEY_TAG, "Failed to open IR key event path: %s", irKeyPath);
//         close(keyFd);
//         return -1;
//     }

//     struct input_event ev;
//     fd_set readfds;
//     struct timeval timeout;

//     // 初始化文件描述符集
//     FD_ZERO(&readfds);
//     FD_SET(keyFd, &readfds);
//     FD_SET(irFd, &readfds);
//     int maxFd = (keyFd > irFd) ? keyFd : irFd;

//     // 设置超时时间
//     timeout.tv_sec = timeOut;
//     timeout.tv_usec = 0;

//     printf("开始监听按键和红外按键，超时时间: %d秒...\n", timeOut);

//     // 使用select等待事件或超时
//     int ret = select(maxFd + 1, &readfds, NULL, NULL, &timeout);
//     if (ret == -1) {
//         perror("select错误");
//         close(keyFd);
//         close(irFd);
//         return -1;
//     } 
//     else if (ret == 0) {
//         // 超时
//         printf("超时，%d秒内没有按键或红外按键按下\n", timeOut);
//         close(keyFd);
//         close(irFd);
//         return -1;
//     }

//     int activeFd = -1;
//     if (FD_ISSET(keyFd, &readfds)) {
//         activeFd = keyFd;
//     } else if (FD_ISSET(irFd, &readfds)) {
//         activeFd = irFd;
//     }

//     // 有事件发生，读取事件
//     ssize_t n = read(activeFd, &ev, sizeof(ev));
//     if (n == -1) {
//         perror("读取事件失败");
//         close(keyFd);
//         close(irFd);
//         return -1;
//     }

//     // 判断是否为按键事件
//     if (ev.type == EV_KEY) {
//         // 按键按下事件
//         if (ev.value == 1) {
//             if (activeFd == keyFd) {
//                 printf("检测到按键按下 - 按键码: %d\n", ev.code);
//             } else {
//                 printf("检测到红外按键按下 - 按键码: %d\n", ev.code);
//             }
//         }
//     }
//     close(keyFd);
//     close(irFd);
//     return ev.code;
// }