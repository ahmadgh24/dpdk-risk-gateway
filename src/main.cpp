#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_cycles.h>
#include <cstdio>
#include "RiskEngine.hpp"
#include "Protocol.hpp"
#include "LatencyTracker.hpp"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static inline int port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    rte_eth_dev_info dev_info;
    rte_eth_dev_info_get(port, &dev_info);

    rte_eth_conf port_conf = {};
    port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_NONE;
    port_conf.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;
    if (rte_eth_dev_configure(port, 1, 1, &port_conf) != 0)
        return -1;

    uint16_t nb_rx_desc = RX_RING_SIZE;
    uint16_t nb_tx_desc = TX_RING_SIZE;
    rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rx_desc, &nb_tx_desc);

    if (rte_eth_rx_queue_setup(port, 0, nb_rx_desc, rte_eth_dev_socket_id(port),
                               &dev_info.default_rxconf, mbuf_pool) != 0)
        return -1;

    if (rte_eth_tx_queue_setup(port, 0, nb_tx_desc, rte_eth_dev_socket_id(port),
                               &dev_info.default_txconf) != 0)
        return -1;

    if (rte_eth_dev_start(port) != 0)
        return -1;

    rte_eth_promiscuous_enable(port);
    return 0;
}

static bool is_valid_order_packet(struct rte_mbuf *m) {
    const size_t min_len = sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr)
                         + sizeof(rte_udp_hdr) + sizeof(TradingOrder);
    if (rte_pktmbuf_pkt_len(m) < min_len)
        return false;

    auto *eth = rte_pktmbuf_mtod(m, const rte_ether_hdr *);
    if (rte_be_to_cpu_16(eth->ether_type) != RTE_ETHER_TYPE_IPV4)
        return false;

    auto *ip = (const rte_ipv4_hdr *)(eth + 1);
    if (ip->next_proto_id != IPPROTO_UDP)
        return false;

    return true;
}

static __rte_noreturn void lcore_main(uint16_t port, RiskEngine &engine, LatencyTracker &tracker) {
    printf("Core %u processing port %u\n", rte_lcore_id(), port);

    for (;;) {
        struct rte_mbuf *bufs[BURST_SIZE];
        uint16_t nb_tx = 0;

        const uint16_t nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);
        if (nb_rx == 0)
            continue;

        for (uint16_t i = 0; i < nb_rx; i++) {
            if (!is_valid_order_packet(bufs[i])) {
                rte_pktmbuf_free(bufs[i]);
                continue;
            }

            const size_t hdr_offset = sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr);
            const uint8_t *payload = rte_pktmbuf_mtod_offset(bufs[i], const uint8_t *, hdr_offset);

            uint64_t t0 = rte_rdtsc();
            bool pass = engine.check_order(payload);
            tracker.record(rte_rdtsc() - t0);

            if (pass)
                bufs[nb_tx++] = bufs[i];
            else
                rte_pktmbuf_free(bufs[i]);
        }

        const uint16_t nb_sent = rte_eth_tx_burst(port, 0, bufs, nb_tx);
        for (uint16_t i = nb_sent; i < nb_tx; i++)
            rte_pktmbuf_free(bufs[i]);
    }
}

struct StatsArgs {
    const RiskEngine *engine;
    const LatencyTracker *tracker;
};

static int stats_loop(void *arg) {
    auto *a = static_cast<StatsArgs *>(arg);
    for (;;) {
        rte_delay_ms(1000);
        auto s = a->engine->get_stats();
        printf("[stats] passed: %lu  dropped: %lu\n", s.total_passed, s.total_dropped);
        a->tracker->print_stats();
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        fprintf(stderr, "Invalid EAL arguments\n");
        return -1;
    }
    argc -= ret;
    argv += ret;

    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == nullptr) {
        fprintf(stderr, "Cannot create mbuf pool\n");
        return -1;
    }

    uint16_t port_id;
    RTE_ETH_FOREACH_DEV(port_id) {
        if (port_init(port_id, mbuf_pool) != 0) {
            fprintf(stderr, "Cannot init port %u\n", port_id);
            return -1;
        }
    }

    uint16_t nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0) {
        fprintf(stderr, "No available ports\n");
        return -1;
    }

    RiskEngine engine(1000);
    LatencyTracker tracker;

    uint16_t first_port = 0;
    RTE_ETH_FOREACH_DEV(first_port) { break; }

    unsigned stats_lcore = rte_get_next_lcore(rte_lcore_id(), 1, 0);
    if (stats_lcore == RTE_MAX_LCORE) {
        fprintf(stderr, "Need at least 2 lcores (-l 0,1)\n");
        return -1;
    }
    StatsArgs stats_args = {&engine, &tracker};
    rte_eal_remote_launch(stats_loop, &stats_args, stats_lcore);

    lcore_main(first_port, engine, tracker);
}
