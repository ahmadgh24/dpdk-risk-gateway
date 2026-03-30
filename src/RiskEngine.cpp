#include "RiskEngine.hpp"
#include "Protocol.hpp"
#include <cstdio>

RiskEngine::RiskEngine(uint32_t limit)
    : m_max_qty(limit), m_passed(0), m_dropped(0) {}

bool RiskEngine::check_order(const uint8_t *payload) {
    auto *order = reinterpret_cast<const TradingOrder *>(payload);
    if (order->quantity <= m_max_qty) {
        m_passed++;
        return true;
    }
    m_dropped++;
    return false;
}

RiskEngine::Stats RiskEngine::get_stats() const {
    return {m_passed, m_dropped};
}
