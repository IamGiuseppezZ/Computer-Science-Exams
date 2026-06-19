#ifndef CSENTINEL_HEADERS_H
#define CSENTINEL_HEADERS_H

#include <stdio.h>
#include <stdint.h>

#define SYN 0x02 //0000 0010
#define ACK 0x10 //0001 0000
#define RST 0x20 

/* ── EtherType constants ──────────────────────────────────────────────────── */
#define ETHERTYPE_IPV4   0x0800u
#define ETHERTYPE_ARP    0x0806u
#define ETHERTYPE_IPV6   0x86DDu

/* ── IP protocol numbers (IANA) ───────────────────────────────────────────── */
#define PROTO_ICMP    1u
#define PROTO_TCP     6u
#define PROTO_UDP    17u

    
/* ═══════════════════════════════════════════════════════════════════════════ */
/* L2 — Ethernet II frame header                                               */
/* ═══════════════════════════════════════════════════════════════════════════ */
/*
 *  0               1               2               3               4
 *  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                 Destination MAC Address  (6 bytes)              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Source MAC Address    (6 bytes)              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         EtherType (2 bytes)                     | 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  Total: 14 bytes (excluding 802.1Q VLAN tag)
 */

struct eth_header{
    uint8_t dst_mac[6];  /* Destination host Mac Address */
    uint8_t src_mac[6];  /* Source host Mac Address */   
    uint16_t ether_type; /* Network high-level protocol */
} __attribute__((packed));

#define ETH_HEADER_LEN 14u /* Length header L2 (no VLAN tag) */
    
/* ═══════════════════════════════════════════════════════════════════════════ */
/* IPV4 - Packet Header                                                        */
/* ═══════════════════════════════════════════════════════════════════════════ */
/* 
  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version|  IHL  |Type of Service|          Total Length         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Identification        |Flags|      Fragment Offset    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Time to Live |    Protocol   |         Header Checksum       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Source Address                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Destination Address                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                    Example Internet Datagram Header
*/

struct ipv4_header {
    uint8_t  version_ihl;       /* [7:4] = Version (must be 4)
                                   [3:0] = IHL: header length in 32-bit words */
    uint8_t  dscp_ecn;          /* [7:2] = DSCP (QoS), [1:0] = ECN            */
    uint16_t total_length;      /* Total datagram length including header       */
    uint16_t identification;    /* Fragment reassembly identifier               */
    uint16_t flags_frag_offset; /* [15:13] = DF/MF flags, [12:0] = frag offset */
    uint8_t  ttl;               /* Hop limit; decremented by each router        */
    uint8_t  protocol;          /* L4 protocol: PROTO_TCP, PROTO_UDP …          */
    uint16_t checksum;          /* One's-complement checksum of the header      */
    uint32_t src_ip;            /* Source IP (network byte order)               */
    uint32_t dst_ip;            /* Destination IP (network byte order)          */
} __attribute__((packed));
#define IPV4_VERSION(h) ((h) -> (version_ihl >> 4) & 0x0F);
#define IPV4_IHL(h) ((h) -> (version_ihl) & 0x0F);
#define IPV4_HDR_LEN(h) ((uint32_t) (IPV4_IHL(h)) * 4)

/* ═══════════════════════════════════════════════════════════════════════════ */
/* L4 — TCP header  (RFC 793)                                                  */
/* ═══════════════════════════════════════════════════════════════════════════ */
/*
 *  0               1               2               3
 *  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |          Source Port          |       Destination Port         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        Sequence Number                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Acknowledgment Number                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Data  |0 0 0|N|C|E|U|A|P|R|S|F|           Window             |
 * | Offset|     |S|W|C|R|C|S|S|Y|I|                              |
 * |       |     | |R|E|G|K|H|T|N|N|                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |           Checksum            |         Urgent Pointer        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Flags byte layout (data_off_ns byte #2, then flags byte #3):
 *   bit 7: CWR  bit 6: ECE  bit 5: URG  bit 4: ACK
 *   bit 3: PSH  bit 2: RST  bit 1: SYN  bit 0: FIN
 */
    
struct tcp_header {
    uint16_t src_port;          /* Source Port         */
    uint16_t dst_port;          /* Destination Port    */
    uint32_t seq_num;           /* Sequence Number     */
    uint32_t ack_num;           /* Acknowldegment Number (valid when ACK set) */
    uint8_t data_off_ns;         /* [7:4] = data offeset (int 32 bit words)
                                   [3:1] = reserved (must be 0)
                                   [0]   = NS (Nonce Sum, RFC 3540)           */
    uint8_t flags;              /* Control Bit (see TCP flags below           */
    uint16_t window_size;        /* Receive Window (flow control)              */
    uint16_t checksum;           /* Checksum segment's tcp                     */
    uint16_t urgent_ptr;         /* Urgent data pointer  (valid when URG set)  */        
} __attribute__((packed));

/* TCP control bit masks — apply against tcp->flags with bitwise AND */
#define TCP_FLAG_FIN  0x01u   /* Finish: no more data from sender             */
#define TCP_FLAG_SYN  0x02u   /* Synchronise sequence numbers                 */
#define TCP_FLAG_RST  0x04u   /* Reset the connection                         */
#define TCP_FLAG_PSH  0x08u   /* Push: deliver buffered data immediately       */
#define TCP_FLAG_ACK  0x10u   /* Acknowledgment field is significant          */
#define TCP_FLAG_URG  0x20u   /* Urgent pointer field is significant          */
#define TCP_FLAG_ECE  0x40u   /* ECN-Echo                                     */
#define TCP_FLAG_CWR  0x80u   /* Congestion Window Reduced                    */

/* XMAS scan mask: FIN(0x01) | PSH (0x08) | URG (0x20) = 0x29 */
#define TCP_XMAS_MASK   (TCP_FLAG_FIN | TCP_FLAG_PSH | TCP_FLAG_URG)

/* Data offset in bytes */
#define TCP_DATA_OFFSET(h)  ((uint32_t) (((h) -> data_off_ns >> 4) & 0x0F) * 4u)

/* ═══════════════════════════════════════════════════════════════════════════ */
/* L4 — UDP header  (RFC 768)                                                  */
/* ═══════════════════════════════════════════════════════════════════════════ */
/*
 *  0               1               2               3
 *  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |          Source Port          |       Destination Port         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |             Length            |           Checksum             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  Fixed 8 bytes — no options.
 */
struct udp_header {
    uint16_t src_port;   /* Source port (may be zero if unused)    */
    uint16_t dst_port;   /* Destination port                       */
    uint16_t length;     /* Header + payload length (>= 8)         */
    uint16_t checksum;   /* Optional in IPv4, mandatory in IPv6    */
} __attribute__((packed));

#define UDP_HDR_LEN  8u

#endif
