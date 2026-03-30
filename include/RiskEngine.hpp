#ifndef RISK_ENGINE_HPP
#define RISK_ENGINE_HPP

#include <rte_common.h>
#include <cstdint>

class alignas(64) RiskEngine {
public:
    RiskEngine(uint32_t limit);

    bool check_order(const uint8_t* payload);

    struct Stats {
        uint64_t total_passed;
        uint64_t total_dropped;
    };

    Stats get_stats() const;

private:
    uint32_t m_max_qty;
    uint64_t m_passed;
    uint64_t m_dropped;
};

#endif
