#include "RiskEngine.hpp"
#include "Protocol.hpp"
#include <cstdio>

RiskEngine::RiskEngine(uint32_t limit)
    : m_max_qty{limit}, m_passed{0}, m_dropped{0}, m_malformed{0}, m_positions{} {
        rte_hash_parameters params{};
        params.name = "symbol_positions";
        params.entries = NUM_ENTRIES;
        params.key_len = sizeof(uint32_t);
        m_ht = rte_hash_create(&params);
    }

bool RiskEngine::check_order(const uint8_t *payload) {
    auto *order = reinterpret_cast<const TradingOrder *>(payload);
    if (order->magic != 0xBEEF) {
        m_malformed++;
        return false;
    }

    if (order->quantity > m_max_qty) {
        m_dropped++;
        return false;
    }
    
    int32_t idx = rte_hash_add_key(m_ht, &order->symbol_id);
    if (idx < 0 || ++m_positions[idx] > PER_SYMBOL_LIMIT) {
        m_dropped++;
        return false;
    }

    m_passed++;
    return true;
}

RiskEngine::Stats RiskEngine::get_stats() const {
    return {m_passed, m_dropped, m_malformed};
}
