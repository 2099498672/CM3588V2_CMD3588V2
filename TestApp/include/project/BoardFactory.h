#ifndef __BOARD_FACTORY_H__
#define __BOARD_FACTORY_H__

#include <memory>
#include "project/CM3588S2/CM3588S2.h"
#include "project/CM3588V2_CMD3588V2/CM3588V2_CMD3588V2.h"
#include "hardware/RkGenericBoard.h"
#include "common/Constants.h"
#include "Uart.h"

#include "util/theradpoolv1/thread_pool.h"

// 工厂类
class BoardFactory {
public:
    // 根据板卡名称创建对应的板卡实例
    std::unique_ptr<RkGenericBoard> create_board(const std::string& board_name);
    std::unique_ptr<RkGenericBoard> create_board_v1(const std::string& board_name);
    BOARD_NAME string_to_enum(const std::string& board_name_str);
    Uart* uart = nullptr;
    ~BoardFactory();
};

#endif