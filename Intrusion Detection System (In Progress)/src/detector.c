/*
 * detector.c — c-sentinel micro-ids detection engine
 *
 * all detector state lives here as file-scope statics.  external code
 * interacts exclusively through the api declared in detector.h.
*/

#include <stdint.h>
#include <stdio.h>     /* printf, snprintf           */
#include <string.h>    /* memset                     */
#include <time.h>      /* time, difftime             */
#include <arpa/inet.h> /* ntohs, ntohl               */

#include "detector.h"
#include "headers.h"

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Module-level state (invisible outside this translation unit)                */
/* ═══════════════════════════════════════════════════════════════════════════ */

static syn_tracker_entry_t g_syn_table[TRACKER_TABLE_SIZE];
static ids_stats_t         g_stats;

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Internal helpers                                                             */
/* ═══════════════════════════════════════════════════════════════════════════ */

/*
 * hash_ip() - FNV-1a 32-bit hash for a 4 byte IPv4 Address.
 *
 * FNV-1a (Fowler-Noll-Vo) was chosen because: 
 *  - Simple: easy to implement
 *  - Low collision rate for Ip Addressess
 *  - No external dependencies.
 *
 * The result is masked to [0, TRACKER_TABLE_SIZE) using bitwise AND, which
 * works correctly only when the table size is a power of two.
 *
 *  FNV offset basis:   0x811C9DC5
 *  FNV prime:          0x01000193
*/

static uint32_t hash_ip(uint32_t ip_net){
    const uint32_t FNV_PRIME = 0x01000193u;
    const uint32_t FNV_OFFSET = 0x811C9DC5u;

    const uint8_t *b = (const uint8_t *) &ip_net;
    uint32_t hash = FNV_OFFSET;

    for(int i = 0; i < 4; ++i){
        hash ^= (uint32_t)b[i];
        hash *= FNV_PRIME;
    }
    return hash & (TRACKER_TABLE_SIZE - 1u);
}

/*
 * tracker_lookup_or_insert() - find or create a SYN tracking entry
 *
 * Strategy: open addressing with linear probing.
 *  - Start at hash_ip(src_ip).
 *  - Walk forward until we find either:
 *      1. (an empty slot -> initialise and return it, 
 *      2. a slot whose src_ip matches -> return it (we're using
 *      linear probing, so it's possible we don't find the entry in
 *      hash_ip() but a few entries later maybe).
 *  - If the entire table is full, return NULL
 *  Complexity: O(1) average, O(n) worst case with pathological IP distribution
*/

static syn_tracker_entry_t *tracker_lookup_or_insert(uint32_t src_ip){
    uint32_t idx    = hash_ip(src_ip) & (TRACKER_TABLE_SIZE - 1u);
    uint8_t start   = idx;

    do {
        syn_tracker_entry_t *e = &g_syn_table[idx];
        if (!(e -> in_use)) {
            /* Empty slot: we are not using it, insert a new entry */
            e -> src_ip         = src_ip;
            e -> syn_count      = 0u;
            e -> window_start   = time(NULL);
            e -> in_use         = 1;
            return e;
        }
        if (e -> src_ip == src_ip){
            return e; /* Existing entry for this ip */
        }
        idx = (idx + 1u) & (TRACKER_TABLE_SIZE - 1u);
    } while (idx != start);
    return NULL; /* Table is full, we return NULL (no entry avaible) */
}

/*
 * ip_net_to_str() — convert a network-byte-order 32-bit IP to dotted-decimal.
 * @buf must be at least INET_ADDRSTRLEN (16) bytes.
*/

static void ip_net_to_str(uint32_t ip_net, char* buf, size_t len){
    uint32_t h = ntohl(ip_net);
    snprintf(buf, len, "%u.%u.%u.%u", 
            (h >> 24) & 0xFFu,
            (h >> 16) & 0xFFu,
            (h >> 8)  & 0xFFu,
             h        & 0xFFu);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Lifecycle                                                                    */
/* ═══════════════════════════════════════════════════════════════════════════ */

void ids_init(void){
    memset(g_syn_table, 0, sizeof(g_syn_table));
    memset(&g_stats, 0, sizeof(g_stats));
}

void ids_cleanup(void){
    memset(g_syn_table, 0, sizeof(g_syn_table));
    /* Clean-up hash table */
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Detection engines                                                            */
/* ═══════════════════════════════════════════════════════════════════════════ */

/*
 * detect_xmas_scan()
 *
 * Bitwise operation breakdown:
 *
 *   flags byte layout:
 *     bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0
 *      CWR  ECE  URG  ACK  PSH  RST  SYN  FIN
 *
 *   TCP_XMAS_MASK = TCP_FLAG_FIN | TCP_FLAG_PSH | TCP_FLAG_URG
 *                 = 0x01         | 0x08         | 0x20
 *                 = 0b00101001   = 0x29
 *
 *   (tcp->flags & 0x29) == 0x29
 *     → all three bits are set, regardless of the other five bits.
 *     → This correctly handles scanners that also set ECE/CWR.
 */

int detect_xmas_scan(const struct tcp_header* tcp){
    return (tcp -> flags & TCP_XMAS_MASK) == TCP_XMAS_MASK;
    /* It returns 1 if Xmas Bits are active, 0 otherwhise */
}

/*
 * detect_null_scan()
 *
 * The simplest possible check: tcp->flags must be exactly zero.
 * No masking needed — the entire byte must be 0x00.
*/

int detect_null_scan(const struct tcp_header *tcp){
    return tcp -> flags == 0x00u;
}

/*
 * detect_syn_flood()
 *
 * Sliding-window SYN rate limiter per source IP.
 *
 * Why (flags & (SYN|ACK)) == SYN:
 *   A legitimate SYN/ACK (second handshake step) has both bits set.
 *   We only want "initial SYN" — the first step — so we exclude ACK.
 *   Using a bitmask instead of == SYN ensures we aren't fooled by
 *   scanners that set extra flags alongside SYN.
 *
 *   Bit math:
 *     TCP_FLAG_SYN | TCP_FLAG_ACK = 0x02 | 0x10 = 0x12
 *     (flags & 0x12) == 0x02  →  SYN is set, ACK is clear.
 */

int detect_syn_flood(uint32_t src_ip, uint8_t flags){
    /* Gate: only pure SYN packets are interesting */
    if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) != TCP_FLAG_SYN){
        return 0;
    }

    syn_tracker_entry_t *e = tracker_lookup_or_insert(src_ip);
    if (!e) return 0;       /* Table is full */  

    time_t now = time(NULL);

    /* If the time window has expired, reset the counter and restart */
    if (difftime(now, e -> window_start) > (double) SYN_FLOOW_WINDOW_SEC){
        e -> syn_count      = 0u;
        e -> window_start   = now;
    }
    e -> syn_count++;

    if (e -> syn_count >= SYN_FLOOD_THRESHOLD) {
        e -> syn_count      = 0u;
        e -> window_start   = now;
        return 1;
    }
    return 0;
}

/*
 * ANSI escape codes for terminal colouring.
 * Colour-coded by severity: blue (low), yellow (medium), bold-red (high).
 * The reset code ensures subsequent output is not accidentally coloured.
 */
static const char *severity_colour(alert_severity_t sev)
{
    switch (sev) {
        case SEVERITY_HIGH:   return "\033[1;31m";   /* Bold red    */
        case SEVERITY_MEDIUM: return "\033[1;33m";   /* Bold yellow */
        case SEVERITY_LOW:    return "\033[1;34m";   /* Bold blue   */
        default:              return "";
    }
}

static const char *severity_label(alert_severity_t sev)
{
    switch (sev) {
        case SEVERITY_HIGH:   return "[HIGH]";
        case SEVERITY_MEDIUM: return "[MED] ";
        case SEVERITY_LOW:    return "[LOW] ";
        default:              return "[???] ";
    }
}


void emit_alert(alert_severity_t severity,
                const char       *rule_name,
                const char       *description,
                uint32_t          src_ip,
                uint32_t          dst_ip,
                uint16_t          src_port,
                uint16_t          dst_port)
{
    char src_buf[16], dst_buf[16];
    ip_net_to_str(src_ip, src_buf, sizeof(src_buf));
    ip_net_to_str(dst_ip, dst_buf, sizeof(dst_buf));

    const char *colour = severity_colour(severity);
    const char *reset  = "\033[0m";
    const char *label  = severity_label(severity);

    printf("%s"
           "╔═══════════════════════════════════════════════════════╗\n"
           "║  ⚠  ALERT  %-6s  %-30s      ║\n"
           "╠═══════════════════════════════════════════════════════╣\n"
           "║  %s:%u → %s:%u\n"
           "║  %s\n"
           "╚═══════════════════════════════════════════════════════╝\n"
           "%s\n",
           colour,
           label, rule_name,
           src_buf, ntohs(src_port),
           dst_buf, ntohs(dst_port),
           description,
           reset);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Statistics                                                                   */
/* ═══════════════════════════════════════════════════════════════════════════ */

const ids_stats_t *ids_get_stats(void)
{
    return &g_stats;
}

ids_stats_t *ids_get_mutable_stats(void)
{
    return &g_stats;
}

void ids_print_stats(void)
{
    const ids_stats_t *s = &g_stats;

    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║          C-Sentinel  —  Session Statistics       ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  Total frames captured  : %-22llu ║\n", (unsigned long long)s->total_packets);
    printf("║  IPv4 packets           : %-22llu ║\n", (unsigned long long)s->ipv4_packets);
    printf("║  TCP  segments          : %-22llu ║\n", (unsigned long long)s->tcp_packets);
    printf("║  UDP  datagrams         : %-22llu ║\n", (unsigned long long)s->udp_packets);
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  Xmas scans detected    : %-22llu ║\n", (unsigned long long)s->xmas_scans);
    printf("║  Null scans detected    : %-22llu ║\n", (unsigned long long)s->null_scans);
    printf("║  SYN flood events       : %-22llu ║\n", (unsigned long long)s->syn_floods);
    printf("╚══════════════════════════════════════════════════╝\n");
}
