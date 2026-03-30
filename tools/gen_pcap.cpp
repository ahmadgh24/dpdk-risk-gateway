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

    // Order 1: qty=500 (should PASS with limit=1000)
    write_packet(f, 1, {0xBEEF, 1, 100, 500, 99.50});

    // Order 2: qty=1000 (should PASS, exactly at limit)
    write_packet(f, 2, {0xBEEF, 2, 200, 1000, 150.25});

    // Order 3: qty=1001 (should be DROPPED)
    write_packet(f, 3, {0xBEEF, 3, 300, 1001, 200.00});

    // Order 4: qty=5000 (should be DROPPED)
    write_packet(f, 4, {0xBEEF, 4, 100, 5000, 50.00});

    // Order 5: qty=1 (should PASS)
    write_packet(f, 5, {0xBEEF, 5, 400, 1, 10.00});

    fclose(f);
    printf("Generated test_orders.pcap with 5 packets\n");
    printf("  Order 1: qty=500   -> PASS\n");
    printf("  Order 2: qty=1000  -> PASS\n");
    printf("  Order 3: qty=1001  -> DROP\n");
    printf("  Order 4: qty=5000  -> DROP\n");
    printf("  Order 5: qty=1     -> PASS\n");
    return 0;
}
