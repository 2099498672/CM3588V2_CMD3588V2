#include <thread>
#include <iostream>
#include <signal.h>
#include <atomic>
#include <csignal>
#include <vector>

#include "common/Constants.h"
#include "Uart.h"
#include "task/TaskQueue.h"
#include "task/TaskHandler.h"
#include "util/Timer.h"
#include "util/ZenityDialog.h"
#include "util/theradpoolv1/thread_pool.h"
#include "protocol/ProtocolParser.h"
#include "hardware/RkGenericBoard.h"
#include "project/BoardFactory.h"

const char *APP_TAG = "TestApp";    

BoardFactory factory;

std::vector<std::shared_ptr<interrupt_flag>> interrupt_flags_vector;           
thread_pool work_thread(8);                                             

std::queue<std::shared_ptr<interrupt_flag>> interrupt_flags_queue_task;      
thread_pool work_thread_task(8);                                            

void signal_handler(int signum);

int main() {
    // 自动获取板卡名称逻辑在这里
    std::string detected_board_name = "CM3588V2_CMD3588V2";
    std::unique_ptr<RkGenericBoard> Board = factory.create_board_v1(detected_board_name);     
    if (!Board) {
        log_thread_safe(LOG_LEVEL_ERROR, APP_TAG, "无法创建板卡实例，程序退出");
        return 0;
    }
    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "创建板卡实例: %s", detected_board_name.c_str());

    ProtocolParser protocol(factory.uart);
    TaskQueue taskQueue;
    TaskHandler taskHandler(protocol);

    std::shared_ptr<interrupt_flag> recv_flag = std::make_shared<interrupt_flag>();
    interrupt_flags_vector.push_back(recv_flag);

    std::signal(SIGINT, signal_handler);
    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "已注册 SIGINT 信号处理函数，等待退出信号...");

    std::shared_ptr<interrupt_flag> task_handler_flag = 
        work_thread.submit_interruptible([&taskQueue, &taskHandler, &Board](interrupt_flag& flag) {
            log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "任务处理线程启动");
            Task task;
            while (flag.is_stop_requested() == false) {
                if (taskQueue.tryPopFor(task, 10)) {
                    taskHandler.processTask(task, Board);
                }
            }
            log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "任务处理线程退出");
        });
    interrupt_flags_vector.push_back(task_handler_flag);

    std::shared_ptr<interrupt_flag> sleep_for_main = std::make_shared<interrupt_flag>();
    interrupt_flags_vector.push_back(sleep_for_main);

    uint8_t buffer[1024];
    while(!recv_flag->is_stop_requested()) {
        ssize_t len = factory.uart->receiveData(buffer, sizeof(buffer));
        if (len > 0) {
            protocol.writeBuffer(buffer, static_cast<uint16_t>(len));
        } 

        int parseResult = protocol.parseFrame();
        if (parseResult == 1) {
            Task task = protocol.getTask();
            taskQueue.push(task);
        }  
    }

    while (!sleep_for_main->is_stop_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "接收数据，解析线程退出");
    log_thread_safe(LOG_LEVEL_INFO, APP_TAG, "退出主循环，程序结束");
    return 0;
}

void signal_handler(int signal) {
    if (signal == SIGINT) {
        log_thread_safe(LOG_LEVEL_INFO, APP_TAG, " ctrl + c 按下，准备退出...");
        for(std::shared_ptr<interrupt_flag> flag : interrupt_flags_vector) {
            flag->request_stop();
        }

        while(!interrupt_flags_queue_task.empty()) {
            std::shared_ptr<interrupt_flag> flag = interrupt_flags_queue_task.front();
            flag->request_stop();
            interrupt_flags_queue_task.pop();
        }
    }
}
