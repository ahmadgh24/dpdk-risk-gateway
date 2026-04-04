#ifndef LATENCY_TRACKER_HPP
#define LATENCY_TRACKER_HPP

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <rte_cycles.h>

class alignas(64) LatencyTracker {
public:
    LatencyTracker() : m_count(0), m_max(0) {}

    void record(uint64_t cycles) {
        m_samples[m_count.load(std::memory_order_relaxed) % NUM_SAMPLES] = cycles;
        m_count.fetch_add(1, std::memory_order_relaxed);
        uint64_t cur_max = m_max.load(std::memory_order_relaxed);
        while (cycles > cur_max &&
               !m_max.compare_exchange_weak(cur_max, cycles, std::memory_order_relaxed));
    }

    void print_stats() const {
        uint64_t count = m_count.load(std::memory_order_relaxed);
        if (count == 0) {
            printf("[latency] no samples\n");
            return;
        }

        uint64_t n = count < NUM_SAMPLES ? count : NUM_SAMPLES;
        uint64_t buf[NUM_SAMPLES];
        for (uint64_t i = 0; i < n; i++)
            buf[i] = m_samples[i];

        std::sort(buf, buf + n);

        double hz = static_cast<double>(rte_get_tsc_hz());
        auto to_ns = [hz](uint64_t cycles) { return cycles * 1e9 / hz; };

        printf("[latency] samples=%lu  min=%.0fns  P50=%.0fns  P99=%.0fns  max=%.0fns\n",
               n,
               to_ns(buf[0]),
               to_ns(buf[n * 50 / 100]),
               to_ns(buf[n * 99 / 100]),
               to_ns(m_max.load(std::memory_order_relaxed)));
    }

private:
    static constexpr uint64_t NUM_SAMPLES = 10000;
    uint64_t m_samples[NUM_SAMPLES];
    std::atomic<uint64_t> m_count;
    std::atomic<uint64_t> m_max;
};

#endif
