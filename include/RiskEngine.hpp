#ifndef RISK_ENGINE_HPP
#define RISK_ENGINE_HPP

#include <rte_common.h>
#include <rte_hash.h>
#include <cstdint>
#include <atomic>

class alignas(64) RiskEngine {
public:
    RiskEngine(uint32_t limit);

    bool check_order(const uint8_t* payload);

    struct Stats {
        uint64_t total_passed;
        uint64_t total_dropped;
        uint64_t total_malformed; // Packets that don't begin with 0xBEEF
    };

    Stats get_stats() const;

private:
    static constexpr int NUM_ENTRIES = 1024;
    static constexpr int PER_SYMBOL_LIMIT = 5;
    uint32_t m_max_qty;
    std::atomic<uint64_t> m_passed;
    std::atomic<uint64_t> m_dropped;
    std::atomic<uint64_t> m_malformed;
    struct rte_hash *m_ht;
    uint32_t m_positions[NUM_ENTRIES];
};

#endif
