#include "RiskEngine.hpp"
#include "Protocol.hpp"
#include <cstdio>

RiskEngine::RiskEngine(uint32_t limit)
    : m_max_qty(limit), m_passed(0), m_dropped(0) {}

bool RiskEngine::check_order(const uint8_t *payload) {
    auto *order = reinterpret_cast<const TradingOrder *>(payload);
    if (order->quantity <= m_max_qty) {
        m_passed++;
        printf("PASS order_id=%u symbol=%u qty=%u price=%.2f\n",
               order->order_id, order->symbol_id, order->quantity, order->price);
        return true;
    }
    m_dropped++;
    printf("DROP order_id=%u symbol=%u qty=%u price=%.2f\n",
           order->order_id, order->symbol_id, order->quantity, order->price);
    return false;
}

RiskEngine::Stats RiskEngine::get_stats() const {
    return {m_passed, m_dropped};
}
