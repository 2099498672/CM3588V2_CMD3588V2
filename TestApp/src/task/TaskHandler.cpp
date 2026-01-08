#include "task/TaskHandler.h"
#include "hardware/TestInterface.h"
#include <iostream>
#include "util/theradpoolv1/thread_pool.h"
#include "util/ZenityDialog.h"
    
extern std::queue<std::shared_ptr<interrupt_flag>> interrupt_flags_queue_task; 
extern thread_pool work_thread_task;

ZenityDialog dialog;               // 适用于ubuntu gnome桌面的图形化对话框, 没有也没关系

TaskHandler::TaskHandler(ProtocolParser& protocol) : protocol_(protocol), keyThreadStopFlag_(false) {}

// TODO: 建议将每个测试项单独成一个类，方便维护和扩展
// TODO: 读取某个json键值，请先判断是否存在
// TODO: 使用 try catch 捕获异常，防止程序崩溃。  若发送过来的json格式不对，又没有捕获异常，程序会崩溃退出
void TaskHandler::processTask(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    // std::cout << "处理任务: 类型=" << task.data["type"].asString() << ", 子命令=" << task.subCommand << std::endl;
    switch (task.subCommand) {
        case CMD_HANDSHAKE:                         
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_HANDSHAKE : 0x01");
            handleHandshake(task);
            break;
        case CMD_BEGIN_TEST:                        
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_BEGIN_TEST : 0x02");
            while(!interrupt_flags_queue_task.empty()) {
                std::shared_ptr<interrupt_flag> flag = interrupt_flags_queue_task.front();
                flag->request_stop();
                interrupt_flags_queue_task.pop();
            }
            handleBeginTest(task);
            break;

        case CMD_OVER_TEST:                         
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_OVER_TEST : 0x03");
            while(!interrupt_flags_queue_task.empty()) {
                std::shared_ptr<interrupt_flag> flag = interrupt_flags_queue_task.front();
                flag->request_stop();
                interrupt_flags_queue_task.pop();
            }
            handleOverTest(task);
            break;

        case CMD_SIGNAL_TOBEMEASURED:              
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_SIGNAL_TOBEMEASURED : 0x05");
            handleSignalToBeMeasured(task);
            executeTestAndRespond(task, Board);
            break;

        case CMD_SET_SYS_TIME:
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_SET_SYS_TIME : 0x0A");
            handleSetSysTime(task);
            break;

        case CMD_SIGNAL_EXEC:                   
            std::cout << "CMD_SIGNAL_EXEC" << std::endl;
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_SIGNAL_EXEC : 0x0D");
            handleSignalExec(task);
            executeTestAndRespond(task, Board);
            break;

        case CMD_SIGNAL_TOBEMEASURED_COMBINE:     
            log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "CMD_SIGNAL_TOBEMEASURED_COMBINE : 0x0F");
            handleSignalToBeMeasuredCombine(task);
            executeTestAndRespond(task, Board);
            break;

        default:
            log_thread_safe(LOG_LEVEL_WARN, TaskHandlerTag, "未知子命令: 0x%02X", task.subCommand);
            break;
    }
}

void TaskHandler::executeTestAndRespond(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    switch (task.subCommand) {
        case CMD_SIGNAL_TOBEMEASURED:                 // 单个待测测试命令：0x05（5）
            switch(stringToTestItem(task.data["type"].asString())) {
                case STORAGE:                         // storage
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行存储测试");
                    storage_test(task, Board);
                break;

                case SWITCHS:                         // storage
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行按键测试");
                    switchs_test(task, Board);
                break;

                case SERIAL:                          // serial
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行串口测试");
                    serial_test(task, Board);
                break;

                case RTC:                             // rtc
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行RTC测试");
                    rtc_test(task, Board);
                break;

                case BLUETOOTH:                      // bluetooth
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行蓝牙测试");
                    bluetooth_test(task, Board);
                break;

                case WIFI:                           // wifi
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行WiFi测试");
                    wifi_test(task, Board);
                break;

                case CAMERA:                         // camera
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行相机测试");
                    camera_test(task, Board);
                break;

                case BASE:                          // base_info
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行基础信息测试");
                    baseinfo_test(task, Board);
                break;

                case MICROPHONE:                    // microphone
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行麦克风测试");
                    microphone_test(task, Board);
                break;

                case COMMON:                            // common
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行通用测试");
                    common_test(task, Board);
                break;
            }
        break;

        case CMD_SIGNAL_EXEC:                         // 单个执行命令：0x0D（13）
            switch(task.data["order"].asInt()) {
                case SIGNAL_EXEC_ORDER_LN:            // ln : 0x02(2)
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行LN测试");
                    ln_test(task, Board);
                break;

                case SIGNAL_EXEC_ORDER_GPIO:          // gpio : 0x04(4)
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行GPIO测试");
                    gpio_test(task, Board);
                break;

                case SIGNAL_EXEC_ORDER_MANUAL:           // manual : 0x05(5)
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行手动测试");
                    manual_test(task, Board);
                break;
            }
        break;

        case CMD_SIGNAL_TOBEMEASURED_COMBINE:         // 单个待测组合命令：0x0F（15）
            switch(stringToTestItem(task.data["type"].asString())) {
                case NET:                              // net
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行net测试");          
                    net_test(task, Board);
                break;

                case TYPEC:                            // typec
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "执行typec测试");
                    typec_test(task, Board);
                break;
            }
        break;
    }
}

void TaskHandler::switchs_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    responseData["testCase"]["testResult"] = "OK";
    Json::Value& itemList = responseData["testCase"]["itemList"];
    response["result"] = "true";        

    std::map<int, std::string> keyMap;
    for (Json::Value& item : itemList) {                 // boot_11   power_12 固定格式分割  boot 键值名称，11 键值
        if (item["enable"].asBool() == false) {         // 未启用的按键，跳过检测
            item["testResult"] = "SKIP";
            continue;
        }
        size_t underscorePos = item["type"].asString().find('_');   // 找到下划线位置
        std::string keyName = item["type"].asString().substr(0, underscorePos);  // 截取按键名称
        int keyCode = std::stoi(item["type"].asString().substr(underscorePos + 1));  // 截取按键代码并转换为整数
        keyMap[keyCode] = keyName;   // 存入待测按键映射表
    }

    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "--> switch test app 自身配置的按键有:");
    for (const auto& pair : Board->Key::keyMap) {
        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "--> switch 按键代码: %d, 按键名称: %s", pair.first, pair.second.c_str());
    }

    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "--> switch 从配置文件中识别到要检测的按键有:");
    for (const auto& pair : keyMap) {
        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "--> switch 按键代码: %d, 按键名称: %s", pair.first, pair.second.c_str());
        // if (Board->Key::keyMap.count(pair.first) == 0 ) {
        //     log_thread_safe(LOG_LEVEL_WARN, TaskHandlerTag, "--> switch 警告: 按键代码 %d 在板卡按键映射表中未找到对应名称", pair.first);
        // }
    }

    int detectCount = keyMap.size();
    // std::cout << "需要检测的按键数量: " << detectCount << std::endl;
    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "需要检测的按键数量: %d", detectCount);

    // TODO: 如果上一次还在运行，想办法停止上一次线程。 或者多开几个taskhander线程，每个任务都有阻塞处理.其它任务也有类似问题。需要优化
    // std::thread keyThread ([this, Board = Board.get(), detectCount, keyMap, responseData]() mutable {
    //     int remainingCount = detectCount;
    //     Json::Value localResponseData = responseData;
    //     Json::Value localResponse;
    //     localResponse["result"] = "true";
        
    //     while(remainingCount > 0) {
    //         if (Board == nullptr) {
    //             std::cout << "Board指针无效" << std::endl;
    //             break;
    //         }
            
    //         int ret = Board->Key::waitForKeyAndIrPress();
    //         if (ret == -1) {
    //             // continue; // 超时或错误，继续等待
    //             std::cout << "按键检测错误或超时，线程退出" << std::endl;
    //             break;
    //         }

    //         if (Board->Key::keyMap.count(ret) && keyMap.count(ret) && Board->Key::keyMap[ret] == keyMap[ret]) {
    //             std::cout << "按键: " << Board->Key::keyMap[ret] << "按下，测试通过" << std::endl;
                
    //             Json::Value& itemList = localResponseData["testCase"]["itemList"];
    //             for (Json::Value& item : itemList) {
    //                 size_t underscorePos = item["type"].asString().find('_');
    //                 std::string keyName = item["type"].asString().substr(0, underscorePos);
    //                 int keyCode = std::stoi(item["type"].asString().substr(underscorePos + 1));
    //                 if (keyCode == ret) {
    //                     item["testResult"] = "OK";
    //                     break;
    //                 }
    //             }

    //             if (keyMap.count(ret)) {
    //                 // 从待测按键映射表中移除已测试按键
    //                 keyMap.erase(ret);
    //                 remainingCount--;
    //                 std::cout << "按键移除 " << Board->Key::keyMap[ret] << std::endl;
    //                 std::cout << "剩余待测按键数量: " << remainingCount << std::endl;
    //             }

    //             if (remainingCount == 0) {
    //                 std::cout << "所有按键测试完成，发送响应" << std::endl;
    //                 localResponse["cmdType"] = 1;
    //                 localResponse["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //                 localResponse["data"] = localResponseData;
    //                 this->protocol_.sendResponse(localResponse);
    //             }
    //         }
    //     }
        
    // });
    // keyThread.detach();

    std::shared_ptr<interrupt_flag> keyThreadStopFlag = 
        work_thread_task.submit_interruptible([this, Board = Board.get(), detectCount, keyMap, responseData](interrupt_flag& flag) mutable {

        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "-> 可中断任务线程");

        int remainingCount = detectCount;
        Json::Value localResponseData = responseData;
        Json::Value localResponse;
        localResponse["result"] = "true";

        while(remainingCount > 0 && !flag.is_stop_requested()) {
            if (Board == nullptr) {
                log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "Board指针无效");
                break;
            }
            
            // int ret = Board->Key::waitForKeyAndIrPress();
            int ret = Board->Key::waitForKeyPress(10);
            if (ret == -1) {
                log_thread_safe(LOG_LEVEL_WARN, TaskHandlerTag, "按键检测配置过程中有错误发生，退出检测");
                break;
            } else if (ret == 0) {
                log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "按键检测等待超时，继续等待按键按下...");
                continue; // 超时，继续等待
            }

            if (Board->Key::keyMap.count(ret) && keyMap.count(ret) && Board->Key::keyMap[ret] == keyMap[ret]) {
                log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "按键: %s 按下，测试通过", Board->Key::keyMap[ret].c_str());
                
                Json::Value& itemList = localResponseData["testCase"]["itemList"];
                for (Json::Value& item : itemList) {
                    size_t underscorePos = item["type"].asString().find('_');
                    std::string keyName = item["type"].asString().substr(0, underscorePos);
                    int keyCode = std::stoi(item["type"].asString().substr(underscorePos + 1));
                    if (keyCode == ret) {
                        item["testResult"] = "OK";
                        break;
                    }
                }

                if (keyMap.count(ret)) {
                    // 从待测按键映射表中移除已测试按键
                    keyMap.erase(ret);
                    remainingCount--;
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "按键移除 %s", Board->Key::keyMap[ret].c_str());
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "剩余待测按键数量: %d", remainingCount);
                }

                if (remainingCount == 0) {
                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "所有按键测试完成，发送响应");
                    localResponse["cmdType"] = 1;
                    localResponse["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
                    localResponse["data"] = localResponseData;
                    this->protocol_.sendResponse(localResponse);
                }
            }
        }
        log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "按键测试线程退出");
    });
    interrupt_flags_queue_task.push(keyThreadStopFlag);
}

void TaskHandler::storage_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    Json::Value& store = responseData["testCase"]["store"];
    response["result"] = "true";

    int i = 0, j = 0, k = 0;
    Board->scan_usb_with_libusb();

    for (const auto& size : Board->usbDiskSizeList) {
        std::cout << "U盘容量: " << size << " GB" << std::endl;
    }

    for (const auto& info : Board->facilityUsbInfoList3_0) {
        std::cout << "设备 VID: 0x" << std::hex << info.vid 
                  << ", PID: 0x" << std::hex << info.pid 
                  << ", 类型: " << info.usbType << std::dec << std::endl;
    }

    for (const auto& info : Board->facilityUsbInfoList2_0) {
        std::cout << "设备 VID: 0x" << std::hex << info.vid 
                  << ", PID: 0x" << std::hex << info.pid 
                  << ", 类型: " << info.usbType << std::dec << std::endl;
    }


    Board->lsusbGetVidPidInfo();


    for (Json::Value& item : store) {
        switch (stringToTestItem(item["name"].asString())) {
            case DDR: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                float size = Board->getDdrSize();
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f", size);
                std::string strSize(buffer);
                item["testValue"] = strSize;
                // TODO: 保留数据两位小数
                if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                    item["testResult"] = "OK";
                } else {
                    item["testResult"] = "NG";
                    response["result"] = "false";
                }
            } break;

            case EMMC: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                double size = Board->getEmmcSize();
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f", size);
                std::string strSize(buffer);
                item["testValue"] = strSize;
                // TODO: 保留数据两位小数
                if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                    item["testResult"] = "OK";
                } else {
                    item["testResult"] = "NG";
                    response["result"] = "false";
                }
            } break;

            case TF: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                double size = Board->getTfCardSize();
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f", size);
                std::string strSize(buffer);
                item["testValue"] = strSize;
                // TODO: 保留数据两位小数
                if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                    item["testResult"] = "OK";
                } else {

                    item["testResult"] = "NG";
                    response["result"] = "false";
                }
            } break;

            case USB: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "OK";
                    item["testValue"] = "SKIP";
                    break;
                }
        
                if (i <= Board->usbDiskCount) {
                    i++;
                    double size = Board->usbDiskSizeList[i];
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "%.2f", size);
                    std::string strSize(buffer);
                    if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                        item["testValue"] = strSize;
                        item["testResult"] = "OK";
                    } else {
                        item["testValue"] = strSize;
                        item["testResult"] = "NG";
                        response["result"] = "false";
                    }
                } else {
                    item["testValue"] = 0;
                    item["testResult"] = "NG";
                    response["result"] = "false";
                    break;
                }
            } break;

            case USB_PCIE: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                double size = Board->getPcieSize();
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.2f", size);
                std::string strSize(buffer);
                if (size >= item["min"].asDouble() && size <= item["max"].asDouble()) {
                    item["testValue"] = strSize;
                    item["testResult"] = "OK";
                } else {
                    item["testValue"] = strSize;
                    item["testResult"] = "NG";
                    response["result"] = "false";
                }
            } break;

            case FACILITYUSB3_0: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                // printf("facilityUsbCount3_0=%d, j=%d\n", Board->facilityUsbCount3_0, j);
                // for (auto & info : Board->facilityUsbInfoList3_0) {
                //     if (info.vid == std::stoi(item["isVid"].asString()) &&
                //         info.pid == std::stoi(item["isPid"].asString())) {
                //             item["testResult"] = "OK";
                //             break;
                //     }
                // }

                for (auto & lsusbInfo : Board->lsusbFacilityUsbInfoList) {
                    if (lsusbInfo.vid == std::stoi(item["isVid"].asString()) &&
                        lsusbInfo.pid == std::stoi(item["isPid"].asString())) {
                            item["testResult"] = "OK";
                            break;
                    }
                }

                // if (j < Board->facilityUsbCount3_0) {
                //     j++;
                //     // item["testValue"]["vid"] = Board->facilityUsbInfoList3_0[j].vid;
                //     // item["testValue"]["pid"] = Board->facilityUsbInfoList3_0[j].pid;
                //     // item["testValue"]["usbType"] = Board->facilityUsbInfoList3_0[j].usbType;
                //     item["testResult"] = "OK";
                //     break;
                // } else {
                //     item["testValue"]["vid"] = 0;
                //     item["testValue"]["pid"] = 0;
                //     item["testValue"]["usbType"] = "0.0";
                //     item["testResult"] = "NG";
                //     response["result"] = "false";
                //     break;
                // }
            } break;

            case FACILITYUSB2_0: {
                if (item["enable"].asBool() == false) {
                    item["testResult"] = "SKIP";
                    break;
                }

                for (auto & lsusbInfo : Board->lsusbFacilityUsbInfoList) {
                    if (lsusbInfo.vid == std::stoi(item["isVid"].asString()) &&
                        lsusbInfo.pid == std::stoi(item["isPid"].asString())) {
                            item["testResult"] = "OK";
                            break;
                    }
                }

                // printf("facilityUsbCount2_0=%d, j=%d\n", Board->facilityUsbCount2_0, k);
                // for (auto & info : Board->facilityUsbInfoList2_0) {
                //     if (info.vid == std::stoi(item["isVid"].asString()) &&
                //         info.pid == std::stoi(item["isPid"].asString())) {
                //             item["testResult"] = "OK";
                //             break;
                //     }
                // }

                // if (k < Board->facilityUsbCount2_0) {
                //     k++;
                //     // item["testValue"]["vid"] = Board->facilityUsbInfoList2_0[k].vid;
                //     // item["testValue"]["pid"] = Board->facilityUsbInfoList2_0[k].pid;
                //     // item["testValue"]["usbType"] = Board->facilityUsbInfoList2_0[k].usbType;
                //     item["testResult"] = "OK";
                //     break;
                // } else {
                //     item["testValue"]["vid"] = 0;
                //     item["testValue"]["pid"] = 0;
                //     item["testValue"]["usbType"] = "0.0";
                //     item["testResult"] = "NG";
                //     response["result"] = "false";
                //     break;
                // }
            } break;

            default:
                break;
        }
    }
    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

// void TaskHandler::serial_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
//     Json::Value response;
//     Json::Value responseData;
//     responseData = task.data;

//     std::thread serialThread ([this, Board = Board.get(), responseData]() mutable {
//         Json::Value response;
//         if (responseData["testCase"]["enable"].asBool() == false) {
//             return;
//         }

//         int serial485Count = 0;
//         std::vector<std::string> deviceList;
//         std::vector<bool> sendResult;
//         std::vector<bool> recvResult;

//         int serialCanCount = 0;
//         std::vector<std::string> canDeviceList;
//         std::vector<bool> canSendResult;
//         std::vector<bool> canRecvResult;

//         std::cout << "开始扫描串口设备..." << std::endl;
//         Json::Value& groupList = responseData["testCase"]["groupList"];

//         // 第一遍遍历：收集485设备并测试232设备
//         for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
//             Json::Value& group = groupList[i];
            
//             // 获取 itemList 数组
//             Json::Value& itemList = group["itemList"];
            
//             // 遍历每个 item
//             for (Json::ArrayIndex j = 0; j < itemList.size(); ++j) {
//                 Json::Value& item = itemList[j];
//                 if (item["enable"].asBool() == false) {
//                     item["testResult"] = "SKIP";
//                     continue;
//                 }
//                 // 提取 serialPath
//                 if (item.isMember("serialPath") && item["serialPath"].isString()) {
//                     std::string serialPath = item["serialPath"].asString();
//                     std::string serialName = item["serialName"].asString();
//                     std::cout << "发现串口设备: " << serialPath << ", 名称: " << serialName << std::endl;
//                     std::cout << "进行分类或测试: " << serialPath << ", 名称: " << serialName << std::endl;

//                     if (strstr(serialName.c_str(), "232") != NULL) {
//                         std::cout << "测试串口: " << serialPath << std::endl;
//                         bool ret = Board->serialTest(serialPath.c_str());
//                         if (ret) {
//                             item["testResult"] = "OK";
//                         } else {
//                             item["testResult"] = "NG";
//                             response["result"] = "false";
//                         }
//                     } else if (strstr(serialName.c_str(), "485") != NULL) {
//                         deviceList.push_back(serialPath);
//                         serial485Count++;
//                     } else if (strstr(serialName.c_str(), "CAN") != NULL || strstr(serialName.c_str(), "can") != NULL) {
//                         canDeviceList.push_back(serialPath);
//                         serialCanCount++;
//                     }
//                 }
//             }
//         }

//         // 先打印出来看看
//         std::cout << "找到 " << serial485Count << " 个 485 串口设备 : " <<  deviceList.size() << std::endl;
//         for (size_t i = 0; i < deviceList.size(); ++i) {
//             std::cout << "485 设备: " << deviceList[i] << std::endl;
//         }

//         std::cout << "找到 " << serialCanCount << " 个 CAN 设备 : " <<  canDeviceList.size() << std::endl;
//         for (size_t i = 0; i < canDeviceList.size(); ++i) {
//             std::cout << "CAN 设备: " << canDeviceList[i] << std::endl;
//         }

//         // 测试485设备
//         if (!deviceList.empty() || !canDeviceList.empty()) {
//             Board->serial485TestRetry(deviceList, sendResult, recvResult);
//             // 打印结果
//             for (size_t i = 0; i < deviceList.size(); ++i) {
//                 std::cout << "485 设备: " << deviceList[i]
//                         << ", 发送: " << (sendResult[i] ? "成功" : "失败")
//                         << ", 接收: " << (recvResult[i] ? "成功" : "失败")
//                         << std::endl;
//             }

//             Board->serialCanTestRetry(canDeviceList, canSendResult, canRecvResult);
//             // 打印结果
//             for (size_t i = 0; i < canDeviceList.size(); ++i) {
//                 std::cout << "CAN 设备: " << canDeviceList[i]
//                         << ", 发送: " << (canSendResult[i] ? "成功" : "失败")
//                         << ", 接收: " << (canRecvResult[i] ? "成功" : "失败")
//                         << std::endl;
//             }

//             // 第二遍遍历：设置485设备的测试结果
//             int index = 0;
//             int canIndex = 0;
//             for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
//                 Json::Value& group = groupList[i];
//                 Json::Value& itemList = group["itemList"];
                
//                 for (Json::ArrayIndex j = 0; j < itemList.size(); ++j) {
//                     Json::Value& item = itemList[j];
//                     if (item["enable"].asBool() == false) {
//                         continue;
//                     }
//                     if (item.isMember("serialPath") && item["serialPath"].isString()) {
//                         std::string serialName = item["serialName"].asString();
                        
//                         if (strstr(serialName.c_str(), "485") != NULL) {
//                             if (index < (int)deviceList.size()) {
//                                 if (sendResult[index] && recvResult[index]) {
//                                     item["testResult"] = "OK";
//                                 } else {
//                                     item["testResult"] = "NG";
//                                     response["result"] = "false";
//                                 }
//                                 index++;
//                             } else {
//                                 item["testResult"] = "NG";
//                                 response["result"] = "false";
//                             }
//                         } else if (strstr(serialName.c_str(), "CAN") != NULL || strstr(serialName.c_str(), "can") != NULL) {
//                             if (canIndex < (int)canDeviceList.size()) {
//                                 if (canSendResult[canIndex] && canRecvResult[canIndex]) {
//                                     item["testResult"] = "OK";
//                                 } else {
//                                     item["testResult"] = "NG";
//                                     response["result"] = "false";
//                                 }
//                                 canIndex++;
//                             } else {
//                                 item["testResult"] = "NG";
//                                 response["result"] = "false";
//                             }
//                         }
//                     }
//                 }
//             }
//         }

//         response["cmdType"] = 1;
//         response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
//         response["data"] = responseData;
//         protocol_.sendResponse(response);
//     });
//     serialThread.detach();
// }

void TaskHandler::serial_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    std::shared_ptr<interrupt_flag> serialThreadStopFlag =
        work_thread_task.submit_interruptible([this, Board = Board.get(), responseData](interrupt_flag& flag) mutable {
            Json::Value response;
            if (responseData["testCase"]["enable"].asBool() == false) {
                return;
            }

            Json::Value& groupList = responseData["testCase"]["groupList"];

            for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
                Json::Value& group = groupList[i];
                
                // 获取 itemList 数组
                Json::Value& itemList = group["itemList"];
                
                // 遍历每个 item
                for (Json::ArrayIndex j = 0; j < itemList.size(); ++j) {
                    Json::Value& item = itemList[j];
                    if (item["enable"].asBool() == false) {
                        item["testResult"] = "SKIP";
                        continue;
                    }

                    if (item.isMember("serialPath") && item["serialPath"].isString()) {
                        std::string serialPath = item["serialPath"].asString();
                        std::string serialName = item["serialName"].asString();

                        if (strstr(serialName.c_str(), "232") != NULL) {
                            bool ret = Board->serialTest(serialPath.c_str());
                            if (ret) {
                                item["testResult"] = "OK";
                            } else {
                                item["testResult"] = "NG";
                                response["result"] = "false";
                            }
                        } else if (strstr(serialName.c_str(), "485") != NULL) {
                            int fd = Board->open_serial_device(serialPath.c_str());
                            if (fd < 0) {
                                log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "open 485 device %s failed", serialPath.c_str());
                                item["testResult"] = "NG";
                                response["result"] = "false";
                            } else {
                                bool ret = Board->serial485_test(fd, 10);
                                if (ret) {
                                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "485 device %s test success", serialPath.c_str());
                                    item["testResult"] = "OK";
                                } else {
                                    log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "485 device %s test failed", serialPath.c_str());
                                    item["testResult"] = "NG";
                                    response["result"] = "false";
                                }
                                close(fd);
                            }
                        } else if (strstr(serialName.c_str(), "CAN") != NULL || strstr(serialName.c_str(), "can") != NULL) {
                            int can_fd = Board->open_can_device(serialPath.c_str());
                            if (can_fd < 0) {
                                log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "open can device %s failed", serialPath.c_str());
                                item["testResult"] = "NG";
                                response["result"] = "false";
                            } else {
                                bool ret = Board->can_test(can_fd, 10);
                                if (ret) {
                                    log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "can device %s test success", serialPath.c_str());
                                    item["testResult"] = "OK";
                                } else {
                                    log_thread_safe(LOG_LEVEL_ERROR, TaskHandlerTag, "can device %s test failed", serialPath.c_str());
                                    item["testResult"] = "NG";
                                    response["result"] = "false";
                                }
                                close(can_fd);
                            }
                        }
                    }
                }
            }

            response["cmdType"] = 1;
            response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
            response["data"] = responseData;
            protocol_.sendResponse(response);
        });
    interrupt_flags_queue_task.push(serialThreadStopFlag);

}

void TaskHandler::rtc_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    std::string rtc_time = responseData["testCase"]["timeValue"].asString();
    int year, month, day, hour, minute, second;
    struct rtc_time set_tm;
    struct rtc_time ret_time;
    char timeBuf[128];
    memset(timeBuf, 0, sizeof(timeBuf));
    sscanf(rtc_time.c_str(), "%d/%d/%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    set_tm.tm_year = year - 1900;
    set_tm.tm_mon = month - 1;
    set_tm.tm_mday = day;
    set_tm.tm_hour = hour;
    set_tm.tm_min = minute;
    set_tm.tm_sec = second;

    std::thread rtcThread([this, Board = Board.get(), set_tm, responseData]() mutable {
        Json::Value response;  // 在lambda内部定义response

        if (Board->setAndWait(set_tm, 2)) {
            char timeBuf[128];
            memset(timeBuf, 0, sizeof(timeBuf));
            sprintf(timeBuf, "%d/%d/%d %d:%d:%d", set_tm.tm_year + 1900, set_tm.tm_mon + 1, set_tm.tm_mday, set_tm.tm_hour, set_tm.tm_min, set_tm.tm_sec);
            std::string timeStr(timeBuf);
            responseData["testCase"]["testResult"] = "OK";
            responseData["testResult"] = "OK";
            responseData["testCase"]["testValue"] = timeStr;
            response["result"] = "true";
        } else {
            responseData["testCase"]["testResult"] = "NG";
            responseData["testResult"] = "NG";
            responseData["testCase"]["testValue"] = "NULL";
            response["result"] = "false";
        }
        
        response["cmdType"] = 1;
        response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
        response["data"] = responseData;
        protocol_.sendResponse(response);
    });

    rtcThread.detach();
}

void TaskHandler::ln_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;
    response["result"] = "true";

    std::string lnName = responseData["testOrder"]["ln"].asString();
    std::cout << "LN name to write and read: " << lnName << std::endl;

    //   "mac" : "0C63FC412AEE", 
    std::string mac = responseData["testOrder"]["mac"].asString();
    uint8_t lan_mac[6];
    sscanf(mac.c_str(), "%02x%02x%02x%02x%02x%02x", &lan_mac[0], &lan_mac[1], &lan_mac[2], &lan_mac[3], &lan_mac[4], &lan_mac[5]);

    if (Board->vendor_storage_write_mac(VENDOR_LAN_MAC_ID, lan_mac, 6) == 0) {
        std::cout << "LAN MAC write successful: " << mac << std::endl;
        uint8_t buffer[512];
        uint16_t len;
        int ret;
        len = sizeof(buffer);
        if (Board->vendor_storage_read_test(VENDOR_LAN_MAC_ID, buffer, &len) == 0) {
            std::cout << "LAN MAC read successful, data length: " << len << std::endl;
            char readMac[18];
            memset(readMac, 0, sizeof(readMac));
            sprintf(readMac, "%02X%02X%02X%02X%02X%02X", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
            std::string readMacStr(readMac);
            std::cout << "LAN MAC Data: " << readMacStr << std::endl;
            if (readMacStr == mac) {
                // responseData["testOrder"]["result"] = "true";
                // response["result"] = "true";
            } else {
                responseData["testOrder"]["result"] = "false";
                response["result"] = "false";
            }
        } else {
            responseData["testOrder"]["result"] = "false";
            response["result"] = "false";
        }


    } else {
        responseData["testOrder"]["result"] = "false";
        response["result"] = "false";
    }



    if (Board->vendor_storage_write_ln(lnName.c_str()) == 0) {
        uint8_t buffer[512];
        uint16_t len;
        int ret;
        len = sizeof(buffer);
        if (Board->vendor_storage_read_test(VENDOR_CUSTOM_ID_1, buffer, &len) == 0) {
            std::cout << "LN read successful, data length: " << len << std::endl;
            std::string readLn((char*)buffer, len);
            std::cout << "LN Data: " << readLn << std::endl;
            if (readLn == lnName) {
                // responseData["testOrder"]["result"] = "true";
                // response["result"] = "true";
            } else {
                responseData["testOrder"]["result"] = "false";
                response["result"] = "false";
            }
        } else {
            responseData["testOrder"]["result"] = "false";
            response["result"] = "false";
        }
    } else {
        responseData["testOrder"]["result"] = "false";
        response["result"] = "false";
    }

    


    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_EXEC_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

void TaskHandler::gpio_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {     // 执行速度较快且不
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;
    response["result"] = "true";
    responseData["testResult"] = "OK";

    Json::Value &beanList = responseData["testOrder"]["beanList"];

    int direction;
    if (responseData["testOrder"].isMember("gpioType")) {
        direction = responseData["testOrder"]["gpioType"].asInt();
      
    } else {
        direction = 0;                                  // 默认设置为输出
    }

    LogInfo("TaskHandlerGPIO",  "direction: %d", direction);
    for (Json::ArrayIndex i = 0; i < beanList.size(); ++i) {
        int num = beanList[i]["num"].asInt();
        int group = beanList[i]["group"].asInt();
        int level = beanList[i]["level"].asInt();

        bool gpioResult;
        if (direction == 1) {
            gpioResult = Board->gpioGetValue(group * 32 + num, level);
            beanList[i]["result"] = gpioResult ? "true" : "false";
            beanList[i]["testValue"] = gpioResult ? "true" : "false";
            beanList[i]["testResult"] = gpioResult ? "OK" : "NG";
            if (!gpioResult) {
                response["result"] = "false";
                responseData["testResult"] = "NG";
            }
        } else {
            gpioResult = Board->gpioSetValue(group * 32 + num, level);
            beanList[i]["testValue"] = gpioResult ? "true" : "false";
            beanList[i]["testResult"] = gpioResult ? "OK" : "NG";
            if (!gpioResult) {
                response["result"] = "false";
                responseData["testResult"] = "NG";
            }
        }
    }

    if (direction == 1) {
        responseData["type"] = "gpio";
        responseData["testOrder"]["type"] = "1";
        response["cmdType"] = 1;
        response["subCommand"] = CMD_SIGNAL_EXEC_RES;
        response["data"] = responseData;
        protocol_.sendResponse(response);
    } else {
        response["cmdType"] = 1;
        response["subCommand"] = CMD_SIGNAL_EXEC_RES;
        response["data"] = responseData;
        protocol_.sendResponse(response);
    }
}

void TaskHandler::manual_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;
    response["result"] = "true";        

    std::string ledName = responseData["testOrder"]["artificialType"].asString();
    bool status = responseData["testOrder"]["state"].asBool();

    if (strstr(ledName.c_str(), "red") != NULL || strstr(ledName.c_str(), "Red") != NULL) {
        bool ret = Board->setRledStatus(status ? LED_ON : LED_OFF);
        if (ret) {
            response["result"] = "true";        
        } else {
            response["result"] = "false";
        }
    } else if (strstr(ledName.c_str(), "green") != NULL || strstr(ledName.c_str(), "Green") != NULL) {
        bool ret = Board->setGledStatus(status ? LED_ON : LED_OFF);
        if (ret) {
            response["result"] = "true";        
        } else {
            response["result"] = "false";
        }
    } else if (strstr(ledName.c_str(), "fan") != NULL || strstr(ledName.c_str(), "Fan") != NULL) {
        bool ret = Board->fanCtrl(status ? FAN_ON : FAN_OFF);
        if (ret) {
            response["result"] = "true";        
        } else {
            response["result"] = "false";
        }
    }
    // else if (strstr(ledName.c_str(), "TypeC-DP") != NULL) {
    //     std::thread dpLanesThread([this, Board = Board.get(), responseData]() mutable {
    //         Json::Value response;  // 在lambda内部定义response
    //         int timeOut = 30; // 30秒超时
    //         while (timeOut--) {
    //             int ret = Board->typeCTest("/sys/class/typec/port0/orientation");
    //             if (ret == 1 || ret == 0) {
    //                 int ret = Board->readDpLanes("/sys/devices/platform/fed80000.phy/dp_lanes");
    //                 std::cout << "readDpLanes ret = " << ret << std::endl;
    //                 if (ret == 4) {
    //                     response["result"] = "true";
    //                     // responseData["testCase"]["testResult"] = "OK";
    //                     responseData["testOrder"]["testValue"] = ret;
    //                     responseData["testResult"] = "OK";

    //                     response["cmdType"] = 1;
    //                     response["subCommand"] = CMD_SIGNAL_EXEC_RES;
    //                     response["data"] = responseData;
    //                     protocol_.sendResponse(response);
    //                     break; 
    //                 } else {
    //                     response["result"] = "false";
    //                     responseData["testOrder"]["testValue"] = ret;
    //                     responseData["testResult"] = "NG";

    //                     response["cmdType"] = 1;
    //                     response["subCommand"] = CMD_SIGNAL_EXEC_RES;
    //                     response["data"] = responseData;
    //                     protocol_.sendResponse(response);
    //                 }
    //                 sleep(1);
    //             }
    //             sleep(1);
    //         }
    //     });
    //     dpLanesThread.detach();
    // }

    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_EXEC_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

void TaskHandler::net_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;
    response["result"] = "true";

    bool nic = false;
    bool nic2 = false;
    bool nic3 = false;
    

    char res[512];
    memset(res, 0, sizeof(res));
    if (responseData["testCase"]["groupData"]["testCase"].isMember("nic") && responseData["testCase"]["groupData"]["testCase"].isMember("ip")) {
        std::string nic = responseData["testCase"]["groupData"]["testCase"]["nic"].asString();
        std::string ip = responseData["testCase"]["groupData"]["testCase"]["ip"].asString();
        LogInfo("TaskHandler", "nic=%s, ip=%s", nic.c_str(), ip.c_str());

        int firstDot = ip.find('.');
        int secondDot = ip.find('.', firstDot + 1);
        std::string resultIp = ip.substr(0, secondDot);
        LogInfo("TaskHandler", "resultIp=%s", resultIp.c_str());
        char ipBuf[INET_ADDRSTRLEN];
        bool ret = Board->getInterfaceIp(nic.c_str(), ipBuf, sizeof(ipBuf));
        if (ret && strstr(ipBuf, resultIp.c_str()) != NULL) {
            char nicRes[128];
            memset(nicRes, 0, sizeof(nicRes));
            sprintf(nicRes, "%s:%s|", "DHCP1", ipBuf);
            strcat(res, nicRes);
            responseData["testCase"]["groupData"]["testCase"]["nicTestValue"] = std::string(ipBuf);
            responseData["testCase"]["groupData"]["testCase"]["nictestResult"] = "OK";
        } else {
            responseData["testCase"]["groupData"]["testCase"]["nicTestValue"] = "NULL";
            responseData["testCase"]["groupData"]["testCase"]["nictestResult"] = "NG";
            response["result"] = "false";
        }
    } 
    if(responseData["testCase"]["groupData"]["testCase"].isMember("nic2") && responseData["testCase"]["groupData"]["testCase"].isMember("ip2")) {
        std::string nic = responseData["testCase"]["groupData"]["testCase"]["nic2"].asString();
        std::string ip = responseData["testCase"]["groupData"]["testCase"]["ip2"].asString();
        LogInfo("TaskHandler", "nic=%s, ip=%s", nic.c_str(), ip.c_str());

        int firstDot = ip.find('.');
        int secondDot = ip.find('.', firstDot + 1);
        std::string resultIp = ip.substr(0, secondDot);
        LogInfo("TaskHandler", "resultIp=%s", resultIp.c_str());
        char ipBuf[INET_ADDRSTRLEN];
        bool ret = Board->getInterfaceIp(nic.c_str(), ipBuf, sizeof(ipBuf));
        if (ret && strstr(ipBuf, resultIp.c_str()) != NULL) {
            char nicRes[128];
            memset(nicRes, 0, sizeof(nicRes));
            sprintf(nicRes, "%s:%s|", "DHCP2", ipBuf);
            strcat(res, nicRes);
            responseData["testCase"]["groupData"]["testCase"]["nic2TestValue"] = std::string(ipBuf);
            responseData["testCase"]["groupData"]["testCase"]["nic2testResult"] = "OK";
        } else {
            responseData["testCase"]["groupData"]["testCase"]["nic2TestValue"] = "NULL";
            responseData["testCase"]["groupData"]["testCase"]["nic2testResult"] = "NG";
            response["result"] = "false";
        }
    } 
    if(responseData["testCase"]["groupData"]["testCase"].isMember("nic3") && responseData["testCase"]["groupData"]["testCase"].isMember("ip3")){
        std::string nic = responseData["testCase"]["groupData"]["testCase"]["nic3"].asString();
        std::string ip = responseData["testCase"]["groupData"]["testCase"]["ip3"].asString();
        char ipBuf[INET_ADDRSTRLEN];
        bool ret = Board->getInterfaceIp(nic.c_str(), ipBuf, sizeof(ipBuf));
        if (ret) {
            char nicRes[128];
            memset(nicRes, 0, sizeof(nicRes));
            sprintf(nicRes, "|%s:%s", "DHCP3", ipBuf);              
            strcat(res, nicRes);
            responseData["testCase"]["groupData"]["testCase"]["nic3TestValue"] = std::string(ipBuf);
            responseData["testCase"]["groupData"]["testCase"]["nic3testResult"] = "OK";
        } else {
            responseData["testCase"]["groupData"]["testCase"]["nic3TestValue"] = "NULL";
            responseData["testCase"]["groupData"]["testCase"]["nic3testResult"] = "NG";
            response["result"] = "false";
        }
    } 

    responseData["testCase"] = responseData["testCase"]["groupData"]["testCase"];
    responseData["testCase"]["testValue"] = std::string(res);
    responseData["testCase"]["testResult"] = response["result"].asString() == "true" ? "OK" : "NG";


    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

void TaskHandler::bluetooth_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    if (responseData["testCase"].isMember("enable") && responseData["testCase"]["enable"].asBool() == false) {
        return;
    }

    std::thread bluetoothThread([this, Board = Board.get(), responseData]() mutable {
        Json::Value response;  // 在lambda内部定义response
        response["result"] = "true";
        if (Board->scanBluetoothDevices()) {
            if (Board->bluetoothScanResult.count > 0) {
            Board->printScanResult();

            char rssiBuf[256];
            memset(rssiBuf, 0, sizeof(rssiBuf));
            sprintf(rssiBuf, "bluetoothMAC = %s;bluetoothSize = %d", Board->bluetoothScanResult.devices[0].macAddress, Board->bluetoothScanResult.count);
            responseData["testCase"]["rssi"]["testResult"] = "OK";
            responseData["testCase"]["rssi"]["testValue"] = std::string(rssiBuf);
            responseData["testResult"] = "OK";
            responseData["testCase"]["testValue"] = std::string(rssiBuf);
            // responseData["testCase"]["rssi"]["testDevicesCount"] = Board->bluetoothScanResult.count;
            // responseData["testCase"]["rssi"]["testDevicesMac"] = Board->bluetoothScanResult.devices[0].macAddress;
            responseData["testCase"]["testValue"] = std::string(rssiBuf);
            responseData["testCase"]["testResult"] = "OK";
            response["result"] = "true";
            }
        } else {
            responseData["testCase"]["rssi"]["testResult"] = "NG";
            responseData["testCase"]["rssi"]["testValue"] = -1;
            responseData["testCase"]["testResult"] = "NG";
            response["result"] = "false";
        }
        
        response["cmdType"] = 1;
        response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
        response["data"] = responseData;
        protocol_.sendResponse(response);
    });
    bluetoothThread.detach();
}

void TaskHandler::wifi_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    std::string ssid = responseData["testCase"]["wifiSsidList"][0]["ssid"].asString();
    int signalStrength = Board->wpaScanWifi(ssid.c_str());
    if (signalStrength != -1) {
        responseData["testCase"]["wifiSsidList"][0]["testValue"] = signalStrength;
        responseData["testCase"]["wifiSsidList"][0]["testResult"] = "OK";
        responseData["testCase"]["testResult"] = "OK";
        responseData["testCase"]["testValue"] = signalStrength;
        response["result"] = "true";
    } else {
        responseData["testCase"]["wifiSsidList"][0]["testValue"] = "NULL";
        responseData["testCase"]["wifiSsidList"][0]["testResult"] = "NG";
        response["result"] = "false";
    }
    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

void TaskHandler::typec_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    dialog.show("TYPE_C", "请插入TYPE-C设备");
    std::thread typecThread([this, Board = Board.get(), responseData]() mutable {
        Json::Value response;  // 在lambda内部定义response
        response["result"] = "true";
        responseData["testCase"]["testResult"] = "OK";

        int count = 2;
        int first = -1;
        int timeOut = 60; // 60秒超时

        // Json::Value positive2_0;
        // Json::Value positive3_0;
        // Json::Value reverse2_0;
        // Json::Value reverse3_0;
        Board->scan_usb_with_libusb();
        for (const auto& info : Board->facilityUsbInfoList3_0) {
        std::cout << "设备 VID: 0x" << std::hex << info.vid 
                  << ", PID: 0x" << std::hex << info.pid 
                  << ", 类型: " << info.usbType << std::dec << std::endl;
        }

        for (const auto& info : Board->facilityUsbInfoList2_0) {
            std::cout << "设备 VID: 0x" << std::hex << info.vid 
                    << ", PID: 0x" << std::hex << info.pid 
                    << ", 类型: " << info.usbType << std::dec << std::endl;
        }

        bool testValue = false;  // 是否找到匹配的测试设备
        bool allTestValue = true; // 是否所有测试项都已测试完毕
        while(count) {
            int ret = Board->typeCTest("/sys/class/typec/port0/orientation");
            if ((ret == 1 || ret == 0) && count == 2) {
                sleep(1);
                Board->scan_usb_with_libusb();
                Json::Value& groupList = responseData["testCase"]["groupData"]["testCase"]["groupList"];
                for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
                    if (groupList[i]["type"].asString() == "positive") {
                        for (Json::ArrayIndex j = 0; j < groupList[i]["itemList"].size(); ++j) {
                            
                            if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "3.0") != NULL) {
                                for (const auto& info : Board->facilityUsbInfoList3_0) {
                                    if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
                                        info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
                                        char buf[256] = {0};
                                        sprintf(buf, "%d-%d", info.pid, info.vid);
                                        groupList[i]["itemList"][j]["testResult"] = "OK";
                                        groupList[i]["itemList"][j]["testValue"] = std::string(buf);
                                        std::cout << "找到匹配的3.0设备: VID=0x" << std::hex << info.vid 
                                                  << ", PID=0x" << std::hex << info.pid 
                                                  << ", 类型=" << info.usbType << std::dec << std::endl;
                                        testValue = true;
                                    } 
                                }
                                if (testValue == false) {
                                    allTestValue = false;
                                    groupList[i]["itemList"][j]["testResult"] = "NG";
                                    groupList[i]["itemList"][j]["testValue"] = "NG";
                                    responseData["testCase"]["testResult"] = "NG";
                                    std::cout << "未找到匹配的3.0设备: VID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["vid"].asString()) 
                                              << ", PID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["pid"].asString()) 
                                              << std::dec << std::endl;
                                } else {
                                    testValue = false; // 重置，继续下一个设备的匹配
                                }
                                
                            } else if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "2.0") != NULL) {
                                for (const auto& info : Board->facilityUsbInfoList2_0) {
                                    if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
                                        info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
                                        char buf[256] = {0};
                                        sprintf(buf, "%d-%d", info.pid, info.vid);
                                        groupList[i]["itemList"][j]["testResult"] = "OK";
                                        groupList[i]["itemList"][j]["testValue"] = std::string(buf);
                                        std::cout << "找到匹配的2.0设备: VID=0x" << std::hex << info.vid 
                                                  << ", PID=0x" << std::hex << info.pid 
                                                  << ", 类型=" << info.usbType << std::dec << std::endl;
                                        testValue = true;
                                    } 
                                }

                                if (testValue == false) {
                                    allTestValue = false;
                                    groupList[i]["itemList"][j]["testResult"] = "NG";
                                    groupList[i]["itemList"][j]["testValue"] = "NG";
                                    responseData["testCase"]["testResult"] = "NG";
                                    std::cout << "未找到匹配的2.0设备: VID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["vid"].asString()) 
                                              << ", PID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["pid"].asString()) 
                                              << std::dec << std::endl;
                                } else {
                                    testValue = false; // 重置，继续下一个设备的匹配
                                }
                            }
                        }
                    }
                }


                first = ret;
                count--;
                std::cout << "Type-C 设备 第一次插入" << (ret == 1 ? "正插" : "反插") << std::endl;
                dialog.update("TYPE_C", "请翻转TYPE-C设备");
            }

            if (ret != -1 && ret != first && count == 1) {
                sleep(1);
                Board->scan_usb_with_libusb();
                for (const auto& info : Board->facilityUsbInfoList3_0) {
                std::cout << "设备 VID: 0x" << std::hex << info.vid 
                        << ", PID: 0x" << std::hex << info.pid 
                        << ", 类型: " << info.usbType << std::dec << std::endl;
                }

                for (const auto& info : Board->facilityUsbInfoList2_0) {
                    std::cout << "设备 VID: 0x" << std::hex << info.vid 
                            << ", PID: 0x" << std::hex << info.pid 
                            << ", 类型: " << info.usbType << std::dec << std::endl;
                }

                Json::Value& groupList = responseData["testCase"]["groupData"]["testCase"]["groupList"];
                for (Json::ArrayIndex i = 0; i < groupList.size(); ++i) {
                    if (groupList[i]["type"].asString() == "negative") {
                        for (Json::ArrayIndex j = 0; j < groupList[i]["itemList"].size(); ++j) {
                            
                            if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "3.0") != NULL) {
                                for (const auto& info : Board->facilityUsbInfoList3_0) {
                                    if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
                                        info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
                                        char buf[256] = {0};
                                        sprintf(buf, "%d-%d", info.pid, info.vid);
                                        groupList[i]["itemList"][j]["testResult"] = "OK";
                                        groupList[i]["itemList"][j]["testValue"] = std::string(buf);
                                        std::cout << "找到匹配的3.0设备: VID=0x" << std::hex << info.vid 
                                                  << ", PID=0x" << std::hex << info.pid 
                                                  << ", 类型=" << info.usbType << std::dec << std::endl;

                                        testValue = true;
                                    } 
                                }

                                if (testValue == false) {
                                    allTestValue = false;
                                    groupList[i]["itemList"][j]["testResult"] = "NG";
                                    groupList[i]["itemList"][j]["testValue"] = "NG";
                                    responseData["testCase"]["testResult"] = "NG";
                                    std::cout << "未找到匹配的3.0设备: VID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["vid"].asString()) 
                                              << ", PID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["pid"].asString()) 
                                              << std::dec << std::endl;
                                } else {
                                    testValue = false; // 重置，继续下一个设备的匹配
                                }

                            } else if (strstr(groupList[i]["itemList"][j]["name"].asString().c_str(), "2.0") != NULL) {
                                for (const auto& info : Board->facilityUsbInfoList2_0) {
                                    if (info.vid == std::stoi(groupList[i]["itemList"][j]["vid"].asString()) && 
                                        info.pid == std::stoi(groupList[i]["itemList"][j]["pid"].asString())) {
                                        // groupList[i]["itemList"][j]["testResult"] = "OK";
                                        // groupList[i]["itemList"][j]["testValue"]["vid"] = info.vid;
                                        // groupList[i]["itemList"][j]["testValue"]["pid"] = info.pid;
                                        // groupList[i]["itemList"][j]["testValue"]["usbType"] = info.usbType;
                                        char buf[256] = {0};
                                        sprintf(buf, "%d-%d", info.pid, info.vid);
                                        groupList[i]["itemList"][j]["testResult"] = "OK";
                                        groupList[i]["itemList"][j]["testValue"] = std::string(buf);
                                        std::cout << "找到匹配的2.0设备: VID=0x" << std::hex << info.vid 
                                                  << ", PID=0x" << std::hex << info.pid 
                                                  << ", 类型=" << info.usbType << std::dec << std::endl;
                                        testValue = true;
                                    } 
                                }
                                if (testValue == false) {
                                    allTestValue = false;
                                    groupList[i]["itemList"][j]["testResult"] = "NG";
                                    groupList[i]["itemList"][j]["testValue"] = "NG";
                                    responseData["testCase"]["testResult"] = "NG";
                                    std::cout << "未找到匹配的2.0设备: VID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["vid"].asString()) 
                                              << ", PID=0x" << std::hex << std::stoi(groupList[i]["itemList"][j]["pid"].asString()) 
                                              << std::dec << std::endl;
                                } else {
                                    testValue = false; // 重置，继续下一个设备的匹配
                                }
                            }
                        }
                    }
                }

                count--;
                std::cout << "Type-C 设备 第二次插入" << (ret == 1 ? "正插" : "反插") << std::endl;
                dialog.close();
           
            }
    
            if (timeOut-- == 0) {
                dialog.close();
                responseData["testCase"] = responseData["testCase"]["groupData"]["testCase"];

                std::cout << "Type-C 测试超时" << std::endl;
                responseData["testCase"]["testResult"] = "NG";
                responseData["testCase"]["testValue"] = "NULL";
                responseData["testResult"] = "NG";
                response["result"] = "false";
                break;
            }
            sleep(1);
        }
        responseData["testCase"] = responseData["testCase"]["groupData"]["testCase"];
        responseData["testCase"]["testResult"] = allTestValue ? "OK" : "NG";
        response["cmdType"] = 1;
        response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
        response["data"] = responseData;
        protocol_.sendResponse(response);
    });

    typecThread.detach();
}

void TaskHandler::camera_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    if (responseData["testCase"].isMember("enable") && responseData["testCase"]["enable"].asBool() == false) {
        return;
    }

    std::thread camThread([this, Board = Board.get(), responseData]() mutable {
        Json::Value response;  // 在lambda内部定义response
        response["result"] = "true";
        if (responseData["testCase"].isMember("cameraId")) {
            std::string cameraId = responseData["testCase"]["cameraId"].asString();
            std::string cameraName = "cam" + cameraId;
            if (Board->cameraidToInfo.find(cameraName) == Board->cameraidToInfo.end()) {
                responseData["testCase"]["testResult"] = "NG";
                responseData["testResult"] = "NG";
                response["result"] = "false";
            } else {
                bool ret = Board->cameraCapturePng(Board->cameraidToInfo.at(cameraName).devicePath.c_str(), 
                                                    Board->cameraidToInfo.at(cameraName).outputFilePath.c_str());
                
                if (ret) {
                    responseData["testCase"]["testResult"] = "OK";
                    responseData["testResult"] = "OK";
                    responseData["testCase"]["testValue"] = "0";
                    response["result"] = "true";
                } else {
                    responseData["testCase"]["testResult"] = "NG";
                    responseData["testResult"] = "NG";
                    responseData["testCase"]["testValue"] = "1";
                    response["result"] = "false";
                }
            }
            response["cmdType"] = 1;
            response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
            response["data"] = responseData;
            protocol_.sendResponse(response);
        }
    });
    camThread.detach();

    // if (responseData["testCase"].isMember("cameraId") && responseData["testCase"]["cameraId"].asInt() == 0) {
    //     std::string picturePixel;
    //     int ret = Board->cameraTestCam0(picturePixel);
    //     if (ret) {
    //         responseData["testCase"]["testResult"] = "OK";
    //         responseData["testCase"]["testValue"] = "0";
    //         responseData["testResult"] = "OK";
    //         responseData["testValue"] = picturePixel;
    //         response["result"] = "true";
    //     } else {
    //         responseData["testCase"]["testResult"] = "NG";
    //         responseData["testCase"]["testValue"] = "1";
    //         responseData["testResult"] = "NG";
    //         response["result"] = "false";
    //     }
    //     response["cmdType"] = 1;
    //     response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //     response["data"] = responseData;
    //     protocol_.sendResponse(response);
    // } else if (responseData["testCase"].isMember("cameraId") && responseData["testCase"]["cameraId"].asInt() == 1) {
    //     std::string picturePixel;
    //     int ret = Board->cameraTestCam1(picturePixel);
    //     if (ret) {
    //         responseData["testCase"]["testResult"] = "OK";
    //         responseData["testCase"]["testValue"] = "0";
    //         responseData["testResult"] = "OK";
    //         responseData["testValue"] = picturePixel;
    //         response["result"] = "true";
    //     } else {
    //         responseData["testCase"]["testResult"] = "NG";
    //         responseData["testCase"]["testValue"] = "1";
    //         responseData["testResult"] = "NG";
    //         response["result"] = "false";
    //     }
    //     response["cmdType"] = 1;
    //     response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //     response["data"] = responseData;
    //     protocol_.sendResponse(response);
    // } else if (responseData["testCase"].isMember("cameraId") && responseData["testCase"]["cameraId"].asInt() == 2) {
    //     // std::string picturePixel;
    //     // int ret = Board->cameraTestCam1(picturePixel);
    //     // if (ret) {
    //     //     responseData["testCase"]["testResult"] = "OK";
    //     //     responseData["testCase"]["testValue"] = "0";
    //     //     responseData["testResult"] = "OK";
    //     //     responseData["testValue"] = picturePixel;
    //     //     response["result"] = "true";
    //     // } else {
    //     //     responseData["testCase"]["testResult"] = "NG";
    //     //     responseData["testCase"]["testValue"] = "1";
    //     //     responseData["testResult"] = "NG";
    //     //     response["result"] = "false";
    //     // }
    //     // response["cmdType"] = 1;
    //     // response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    //     // response["data"] = responseData;
    //     // protocol_.sendResponse(response);
    // }
}

void TaskHandler::baseinfo_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    if (responseData["testCase"]["firmwareVersion"]["enable"].asBool() == true) {
        std::string firmwareVersion;
        bool ret = Board->getFirmwareVersion(firmwareVersion);
        responseData["testCase"]["firmwareVersion"]["testValue"] = ret ? firmwareVersion : "NULL"; 
    }

    if (responseData["testCase"]["hwid"]["enable"].asBool() == true) {
        std::string hwid;
        int ret = Board->getHwId(hwid);
        responseData["testCase"]["hwid"]["testValue"] = hwid;
        // int ret = Board->adcTest("/sys/bus/iio/devices/iio:device0/in_voltage2_raw");
        // if (ret > 4090 && ret <= 4096) {
        //     responseData["testCase"]["hwid"]["testValue"] = "4096";
        // } else {
        //     responseData["testCase"]["hwid"]["testValue"] = ret;
        // }
    }

    if (responseData["testCase"]["ln"]["enable"].asBool() == true) {
        char buffer[512];
        uint16_t len = sizeof(buffer);
        if (Board->vendor_storage_read_test(VENDOR_CUSTOM_ID_1, (uint8_t*)buffer, &len) == 0) {
            std::string readLn((char*)buffer, len);
            std::cout << "LN Data: " << readLn << std::endl;
            responseData["testCase"]["ln"]["testValue"] = readLn;
        } else {
            responseData["testCase"]["ln"]["testValue"] = "NULL";
        }
    }

    if (responseData["testCase"]["mac"]["enable"].asBool() == true) { 
        uint8_t buffer[512];
        uint16_t len = sizeof(buffer);
        if (Board->vendor_storage_read_test(VENDOR_LAN_MAC_ID, buffer, &len) == 0) {
            char readMac[18];
            memset(readMac, 0, sizeof(readMac));
            sprintf(readMac, "%02X:%02X:%02X:%02X:%02X:%02X", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
            std::string readMacStr(readMac);
            std::cout << "LAN MAC Data: " << readMacStr << std::endl;
            responseData["testCase"]["mac"]["testValue"] = readMacStr;
        } else {
            responseData["testCase"]["mac"]["testValue"] = "NULL";
        }
    }

    if (responseData["testCase"]["testAppVersion"]["enable"].asBool() == true) {
        std::string appVersion;
        int ret = Board->getAppVersion(appVersion);
        responseData["testCase"]["testAppVersion"]["testValue"] = ret ? appVersion : "NULL";
    }


    response["cmdType"] = 1;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
    response["data"] = responseData;
    protocol_.sendResponse(response);
}

void dpLanes(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;


}

void TaskHandler::microphone_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

    if (responseData["testCase"].isMember("enable") && responseData["testCase"]["enable"].asBool() == false) {
        return;
    }

    if (responseData["testCase"].isMember("isAutoTest") && responseData["testCase"]["isAutoTest"].asBool() == true) {
        std::thread micThread([this, Board = Board.get(), responseData]() mutable {
            Json::Value response;  // 在lambda内部定义response
            response["result"] = "true";
            int ret = Board->recordAndCompareV2(3);   // 请看recordAndCompareV2函数说明
            if (ret == 0x03) {
                responseData["testCase"]["testResult"] = "OK";
                responseData["testResult"] = "OK";
                response["result"] = "true";
            } else if (ret == 0x02) {
                responseData["testCase"]["testResult"] = "NG";
                responseData["testResult"] = "NG";
                responseData["testCase"]["desc"] = "right filed";
                response["result"] = "false";
            } else if (ret == 0x01) {
                responseData["testCase"]["testResult"] = "NG";
                responseData["testResult"] = "NG";
                responseData["testCase"]["desc"] = "left filed";
                response["result"] = "false";
            } else {
                responseData["testCase"]["testResult"] = "NG";
                responseData["testResult"] = "NG";
                responseData["testCase"]["desc"] = "left and right filed";
                response["result"] = "false";
            }
            response["cmdType"] = 1;
            response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
            response["data"] = responseData;
            protocol_.sendResponse(response);
        });
        micThread.detach();
    }
}

void TaskHandler::common_test(const Task& task, std::unique_ptr<RkGenericBoard>& Board) {
    Json::Value response;
    Json::Value responseData;
    responseData = task.data;

/*
{
        "cmdType" : 1,
        "data" :
        {
                "testCase" :
                {
                        "enable" : true,
                        "isAutoTest" : true,
                        "priority" : "1",
                        "rangeCaseList" :
                        [
                                {
                                        "enable" : true,
                                        "max" : 10.0,
                                        "min" : 5.0,
                                        "name" : "temp1"
                                },
                                {
                                        "enable" : true,
                                        "max" : 100.0,
                                        "min" : 50.0,
                                        "name" : "temp2"
                                }
                        ],
                        "testMode" : "1",
                        "type" : "common"
                },
                "testFuncCode" : 10,
                "type" : "common"
        },
        "result" : false,
        "subCommand" : 5
}

*/

    if (responseData["testCase"].isMember("enable") && responseData["testCase"]["enable"].asBool() == false) {
        return;
    }

    // std::shared_ptr<interrupt_flag> keyThreadStopFlag = 
    //     work_thread_task.submit_interruptible([this, Board = Board.get(), detectCount, keyMap, responseData](interrupt_flag& flag) mutable {
    //     int remainingCount = detectCount;
    //     Json::Value localResponseData = responseData;
    //     Json::Value localResponse;
    //     localResponse["result"] = "true";
        
    std::shared_ptr<interrupt_flag> common_thread_stop_flag = 
        work_thread_task.submit_interruptible([this, Board = Board.get(), responseData](interrupt_flag& flag) mutable {
        Json::Value localResponseData = responseData;
        Json::Value localResponse;
        localResponse["result"] = "true";

        for (Json::ArrayIndex i = 0; i < localResponseData["testCase"]["rangeCaseList"].size(); ++i) {
            if (flag.is_stop_requested()) {
                log_thread_safe(LOG_LEVEL_INFO, TaskHandlerTag, "通用测试线程退出.");
                break;
            }

            if (localResponseData["testCase"]["rangeCaseList"][i]["enable"].asBool() == false) {
                continue;
            }

            std::string common_name = localResponseData["testCase"]["rangeCaseList"][i]["name"].asString();
            int max = localResponseData["testCase"]["rangeCaseList"][i]["max"].asInt();
            int min = localResponseData["testCase"]["rangeCaseList"][i]["min"].asInt();

            if (strstr(common_name.c_str(), "adc") != NULL) {
                if (Board->Adc::adc_map.find(common_name) != Board->Adc::adc_map.end()) {
                    int adc_value = Board->adcTest(Board->Adc::adc_map[common_name].c_str());
                    localResponseData["testCase"]["rangeCaseList"][i]["testValue"] = adc_value;
                    if (adc_value >= min && adc_value <= max) {
                        localResponseData["testCase"]["rangeCaseList"][i]["testResult"] = "OK";
                    } else {
                        localResponseData["testCase"]["rangeCaseList"][i]["testResult"] = "NG";
                        localResponseData["testCase"]["testResult"] = "NG";
                        localResponse["result"] = "false";
                    }
                } else {
                    localResponseData["testCase"]["rangeCaseList"][i]["testValue"] = "NULL";
                    localResponseData["testCase"]["rangeCaseList"][i]["testResult"] = "NG";
                    localResponseData["testCase"]["testResult"] = "NG";
                    localResponse["result"] = "false";
                }
            }
        }

        localResponse["cmdType"] = 1;
        localResponse["subCommand"] = CMD_SIGNAL_TOBEMEASURED_RES;
        localResponse["data"] = localResponseData;
        protocol_.sendResponse(localResponse);
    });
    interrupt_flags_queue_task.push(common_thread_stop_flag);
}











////////////////////////////////////////////////////////// 这里可以整改为一个函数 //////////////////////////////////////////////////////////
void TaskHandler::handleHandshake(const Task& task) {  // 0x01
    Json::Value response;
    response["cmdType"] = 2;
    response["desc"] = "okay";
    response["result"] = true;
    response["subCommand"] = CMD_HANDSHAKE;
    Json::Value data;
    data["model"] = "CMD3588_TESTAPP";
    data["version"] = "CMD3588_TESTAPP_V20250923_1";
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleBeginTest(const Task& task) {  // 0x02
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_BEGIN_TEST;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleOverTest(const Task& task) {  // 0x03
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_OVER_TEST;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleSignalToBeMeasured(const Task& task) {  // 0x05
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleSetSysTime(const Task& task) {
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_SET_SYS_TIME;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);    
}

void TaskHandler::handleSignalExec(const Task& task) {  // 0x0D
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_SIGNAL_EXEC;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

void TaskHandler::handleSignalToBeMeasuredCombine(const Task& task) {
    Json::Value response;
    response["cmdType"] = 2;
    response["result"] = true;
    response["subCommand"] = CMD_SIGNAL_TOBEMEASURED_COMBINE;
    response["desc"] = "xxx";
    Json::Value data;
    response["data"] = data;
    protocol_.sendResponse(response, task.cmdIndex);
}

Json::Value TaskHandler::buildResponse(const Task& task, const TestResult& result, const std::string& testName) {
    Json::Value response;
    response["subCommand"] = result.responseCommand;
    response["type"] = testName;
    response["cmdIndex"] = task.cmdIndex;
    response["success"] = result.success;
    response["errorCode"] = result.errorCode;
    response["errorMsg"] = result.errorMsg;
    response["data"] = result.data;

    return response;
}


TestItem TaskHandler::stringToTestItem(const std::string& str) {
    if (str == "microphone") return MICROPHONE;
    if (str == "serial") return SERIAL;
    if (str == "gpio") return GPIO;
    if (str == "net") return NET;
    if (str == "rtc") return RTC;
    if (str == "storage") return STORAGE;
    if (str == "ddr") return DDR;
    if (str == "emmc") return EMMC;
    if (str == "wifi") return WIFI;
    if (str == "bluetooth") return BLUETOOTH;
    if (str == "base") return BASE;
    if (str == "switchs") return SWITCHS;
    if (str == "camera") return CAMERA;
    if (str == "typec") return TYPEC;
    if (str == "common") return COMMON;

    // TODO: 注意使用模糊匹配时的顺序，防止误判，防止均包含关键字符串
    if (strstr(str.c_str(), "facility") != NULL && strstr(str.c_str(), "3.0")) return FACILITYUSB3_0;  // 包含facilityUsb的字符串都认为是工装USB测试
    if (strstr(str.c_str(), "facility") != NULL && strstr(str.c_str(), "2.0")) return FACILITYUSB2_0;  // 包含facilityUsb的字符串都认为是工装USB测试
    if (strstr(str.c_str(), "usb_tf") != NULL) return TF;                                              // 包含tf的字符串都认为是TF卡测试
    if (strstr(str.c_str(), "usb_pcie") != NULL) return USB_PCIE;                                      // 包含usb_pcie的字符串都认为是USB_PCIE测试
    if (strstr(str.c_str(), "usb") != NULL) return USB;                                                // 包含usb的字符串都认为是U盘测试，放在usb_tf和usb_pcie后面 !!!!!!!!!!!!!
    if (strstr(str.c_str(), "232") != NULL) return SERIAL_232;                                         // 包含232的字符串都认为是232测试
    if (strstr(str.c_str(), "485") != NULL) return SERIAL_485;                                         // 包含485的字符串都认为是485测试
    if (strstr(str.c_str(), "can") != NULL) return CAN;                                                // 包含can的字符串都认为是can测试



    return NONE;
}


// 停止当前运行的按键线程
void TaskHandler::stopKeyThread() {
    std::lock_guard<std::mutex> lock(keyThreadMutex_);
    
    if (keyThread_ && keyThread_->joinable()) {
        keyThreadStopFlag_ = true;
        keyThread_->join();
        keyThread_.reset();
        keyThreadStopFlag_ = false;
        std::cout << "已停止前一个按键检测线程" << std::endl;
    }
}

 // 这份协议对于本测试程序来说过于复杂。也没有必要使用串口协议，可以使用lvgl
