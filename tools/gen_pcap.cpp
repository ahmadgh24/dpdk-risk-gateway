#include <cstdio>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include "../include/Protocol.hpp"

static constexpr uint16_t htons_ce(uint16_t v) { return __builtin_bswap16(v); }
static constexpr uint32_t htonl_ce(uint32_t v) { return __builtin_bswap32(v); }

#pragma pack(push, 1)
struct PcapFileHeader {
    uint32_t magic = 0xa1b2c3d4;
    uint16_t version_major = 2;
    uint16_t version_minor = 4;
    int32_t thiszone = 0;
    uint32_t sigfigs = 0;
    uint32_t snaplen = 65535;
    uint32_t network = 1; // LINKTYPE_ETHERNET
};

struct PcapRecordHeader {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};

struct EthernetHeader {
    uint8_t dst[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
    uint8_t src[6] = {0x00,0x11,0x22,0x33,0x44,0x55};
    uint16_t ethertype = htons_ce(0x0800);
};

struct IpHeader {
    uint8_t  ver_ihl = 0x45;
    uint8_t  tos = 0;
    uint16_t tot_len;
    uint16_t id = 0;
    uint16_t frag_off = 0;
    uint8_t  ttl = 64;
    uint8_t  protocol = 17; // UDP
    uint16_t check = 0;
    uint32_t saddr = htonl_ce(0x0a000001); // 10.0.0.1
    uint32_t daddr = htonl_ce(0x0a000002); // 10.0.0.2
};

struct UdpHeader {
    uint16_t sport = htons_ce(12345);
    uint16_t dport = htons_ce(54321);
    uint16_t len;
    uint16_t check = 0;
};
#pragma pack(pop)

static void write_packet(FILE *f, uint32_t ts, const TradingOrder &order) {
    EthernetHeader eth;
    IpHeader ip;
    UdpHeader udp;

    udp.len = htons(sizeof(UdpHeader) + sizeof(TradingOrder));
    ip.tot_len = htons(sizeof(IpHeader) + sizeof(UdpHeader) + sizeof(TradingOrder));

    uint32_t pkt_len = sizeof(eth) + sizeof(ip) + sizeof(udp) + sizeof(order);

    PcapRecordHeader rec = {ts, 0, pkt_len, pkt_len};
    fwrite(&rec, sizeof(rec), 1, f);
    fwrite(&eth, sizeof(eth), 1, f);
    fwrite(&ip, sizeof(ip), 1, f);
    fwrite(&udp, sizeof(udp), 1, f);
    fwrite(&order, sizeof(order), 1, f);
}

int main() {
    FILE *f = fopen("test_orders.pcap", "wb");
    if (!f) { perror("fopen"); return 1; }

    PcapFileHeader hdr;
    fwrite(&hdr, sizeof(hdr), 1, f);

    const uint32_t NUM_ORDERS = 10000;
    uint16_t symbols[] = {100, 200, 300, 400};

    for (uint32_t i = 0; i < NUM_ORDERS; i++) {
        uint32_t qty = (i % 3 == 0) ? 1500 : 500;  // ~1/3 dropped
        uint16_t sym = symbols[i % 4];
        double price = 50.0 + (i % 100);
        write_packet(f, i, {0xBEEF, i + 1, sym, qty, price});
    }

    fclose(f);
    printf("Generated test_orders.pcap with %u packets\n", NUM_ORDERS);
    return 0;
}
