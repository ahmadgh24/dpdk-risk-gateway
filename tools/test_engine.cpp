#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <cstdio>
#include <cstring>
#include "RiskEngine.hpp"
#include "Protocol.hpp"

#define NUM_MBUFS 1024
#define MBUF_CACHE_SIZE 128

static rte_mbuf *build_packet(rte_mempool *pool, const TradingOrder &order) {
    rte_mbuf *m = rte_pktmbuf_alloc(pool);
    if (!m) return nullptr;

    size_t pkt_len = sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr) + sizeof(TradingOrder);
    char *data = rte_pktmbuf_append(m, pkt_len);
    if (!data) { rte_pktmbuf_free(m); return nullptr; }

    memset(data, 0, sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr));
    memcpy(data + sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr),
           &order, sizeof(order));
    return m;
}

int main(int argc, char *argv[]) {
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) { fprintf(stderr, "EAL init failed\n"); return 1; }

    rte_mempool *pool = rte_pktmbuf_pool_create("TEST_POOL", NUM_MBUFS,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (!pool) { fprintf(stderr, "Cannot create mempool\n"); return 1; }

    RiskEngine engine(1000);

    TradingOrder orders[] = {
        {0xBEEF, 1, 100, 500,  99.50},   // PASS
        {0xBEEF, 2, 200, 1000, 150.25},  // PASS (at limit)
        {0xBEEF, 3, 300, 1001, 200.00},  // DROP
        {0xBEEF, 4, 100, 5000, 50.00},   // DROP
        {0xBEEF, 5, 400, 1,    10.00},   // PASS
    };

    printf("\n=== Running Risk Engine Test ===\n\n");

    for (const auto &order : orders) {
        rte_mbuf *m = build_packet(pool, order);
        if (!m) { fprintf(stderr, "Failed to build packet\n"); return 1; }

        size_t hdr_offset = sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr);
        const uint8_t *payload = rte_pktmbuf_mtod_offset(m, const uint8_t *, hdr_offset);

        engine.check_order(payload);
        rte_pktmbuf_free(m);
    }

    auto stats = engine.get_stats();
    printf("\n=== Results ===\n");
    printf("Passed:  %lu\n", stats.total_passed);
    printf("Dropped: %lu\n", stats.total_dropped);

    rte_eal_cleanup();
    return 0;
}
