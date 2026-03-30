# dpdk-risk-gateway

DPDK-based pre-trade risk gateway. Sits on the wire, inspects UDP order packets at line rate, and drops anything that fails risk checks before it hits the exchange.

## what it does

- Receives raw packets via DPDK (zero-copy, kernel bypass)
- Validates Ethernet/IPv4/UDP headers
- Parses a custom `TradingOrder` protocol from the UDP payload
- Applies risk checks (currently: max quantity per order)
- Forwards passing orders, drops the rest

## building

Requires DPDK (tested with 23.11) and meson/ninja.

```
export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:$PKG_CONFIG_PATH
meson setup builddir
ninja -C builddir
```

## running

With a real NIC bound to DPDK:

```
sudo ./builddir/riskgw
```

For development/testing without a physical NIC, use a veth pair with `af_packet`:

```
sudo ip link add veth0 type veth peer name veth1
sudo ip link set veth0 up
sudo ip link set veth1 up

sudo LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH \
  ./builddir/riskgw \
  --no-huge --no-pci -l 0 \
  --vdev 'net_af_packet0,iface=veth0'
```

Then send packets into `veth1` — tcpreplay, scapy, whatever you prefer.

## tools

`tools/gen_pcap.cpp` — generates a test pcap with sample orders (mix of pass/drop cases):

```
g++ -std=c++17 -O3 -I include tools/gen_pcap.cpp -o builddir/gen_pcap
./builddir/gen_pcap
```

`tools/test_engine.cpp` — unit test for the risk engine logic, runs against EAL without needing a NIC.

## protocol

Orders are packed structs in the UDP payload:

```
struct TradingOrder {
    uint16_t magic;       // 0xBEEF
    uint32_t order_id;
    uint32_t symbol_id;
    uint32_t quantity;
    double   price;
};
```

## status

Early stage. The risk engine only checks quantity limits right now. Next up: per-symbol position tracking, magic field validation, stats/telemetry, graceful shutdown.
