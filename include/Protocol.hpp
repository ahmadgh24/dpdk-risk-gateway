#pragma once
#include <cstdint>

struct __attribute__((__packed__)) TradingOrder {
    uint16_t magic;
    uint32_t order_id;
    uint32_t symbol_id;
    uint32_t quantity;
    double price;
};
