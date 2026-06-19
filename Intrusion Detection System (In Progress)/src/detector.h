/*
 * detector.h — C-Sentinel Micro-IDS public interface
 *
 * Declares the three detection engines (Xmas scan, Null scan, SYN flood),
 * the statistics bookkeeping types, and the alert emission API.
 *
 * All state is encapsulated inside detector.c; callers interact only
 * through the functions declared here.
*/

#ifndef CSENTINEL_DETECTOR_H
#define CSENTINEL_DETECTOR_H

#include <stdint.h> /* uint8_t, uint16_t, uint32_t, uint64_t */
#include <time.h>   /* time_t */
#include "headers.h"

/* ── IDS Tuning constants ─────────────────────────────────────────────────── */

/*
 * SYN_FLOOD_THRESHOLD: number of SYN-only (no ACK) packets from a single 
 * source IP within SYN_FLOOD_WINDOWS_SEC seconds that triggers an alert.
 *
 * Real-world tuning guidance:
 *  A legitimate TCP client rarely sends > 10 SYN/sec to the same server.
 *  Flood tools typically send hundreds per seconds.
 *  Start conservative (100/10 s) and adjust per your baseline traffic.
*/

#define SYN_FLOOD_THRESHOLD         100u        /* SYN packets within the time window */
#define SYN_FLOOW_WINDOW_SEC        10u         /* Time window in seconds             */    
    
/*
 * TRACKER_TABLE_SIZE: number of slots in the open-addressed SYN Hash Table.
 * Must be a power of 2 so (hash & (SIZE -1)) gives you a valid index.
 * 1024 entries = 1 KB overhead
*/
#define TRACKER_TABLE_SIZE          1024u


/* ── Alert severity ───────────────────────────────────────────────────────── */

typedef enum {
    SEVERITY_LOW          = 1,      /* Informational - unusual but not necessary bad */ 
    SEVERITY_MEDIUM      = 2,      /* Notable anomaly - warrants investigation      */
    SEVERITY_HIGH        = 3       /* Active attack pattern detected                */
} alert_severity_t;

/* ── SYN tracking table entry ─────────────────────────────────────────────── */
/*
 * One entry per unique source IP observed sending SYN-only packets.
 * Stored in a flat array; collisions resolved with linear probing.
*/

typedef struct {
    uint32_t src_ip;         /* Key: source IP in network byte order         */
    uint32_t syn_count;     /* Number of SYN-only packets in current window */
    time_t   window_start;  /* Epoch timestamp when the current window beagan */
    int      in_use;        /* 1 = slot occupied, 0 = slot free             */
} syn_tracker_entry_t;

/* ── Session statistics ───────────────────────────────────────────────────── */
   
typedef struct {
    uint64_t total_packets;     /* Every frame captured by pcap     */
    uint64_t ipv4_packets;      /* Frames with EtherType 0x0800     */
    uint64_t tcp_packets;       /* IPV4 segments with proto = 6     */
    uint64_t udp_packets;       /* IPV4 datagrams with proto = 17   */
    uint64_t xmas_scans;        /* TCP Xmas scans detected          */
    uint64_t null_scans;        /* TCP Null scans detected          */
    uint64_t syn_floods;        /* SYN flood events triggered       */
} ids_stats_t;

/* ── Lifecycle ────────────────────────────────────────────────────────────── */

/* 
 * ids_cleanup() - reset internal state (useful for unit tests).
 * No heap memory is used, so this just memsets the tables.
*/
void ids_cleanup(void);


/* ── Detection Engines ────────────────────────────────────────────────────────────── */

/*
 * detect_xmas_scan(tcp)
 *
 * Returns 1 if the TCP flags byte has FIN, PSH and URG all set simultaneously,
 * forming the "Christmas tree" patter (flag == 0x29 at minimum; other bits
 * may also be set from some scan tools.
 *
 * Theoretical basis:
 *  RFC 793 3.9 specifies that a compliant TCP implementation in CLOSED state
 *  responds with RST to any segment that doesn't carry SYN. An open port 
 *   must *not* respond at all (no RST).  Nmap exploits this asymmetry:
 *   no response → port open/filtered; RST → port closed.
 *   This lets the scanner enumerate ports without completing a three-way
 *   handshake, evading stateful firewalls that only track SYN connections.
 *
 * Returns: 1 if Xmas pattern detected, 0 otherwise.
*/

int detect_null_scan(const struct tcp_header *tcp);

/**
 * detect_syn_flood(src_ip, flags)
 *
 * Tracks SYN-only (SYN without ACK) packets per source IP in a sliding
 * time window.  Emits a SEVERITY_HIGH alert when a single source exceeds
 * SYN_FLOOD_THRESHOLD packets within SYN_FLOOD_WINDOW_SEC seconds.
 *
 * Algorithm:
 *   1. Ignore packets that are not "pure SYN" (SYN set, ACK clear).
 *   2. Look up the source IP in the hash table (open addressing, FNV-1a).
 *   3. If the current window has expired, reset the counter.
 *   4. Increment counter; if >= threshold → return 1 and reset counter
 *      so sustained floods keep generating one alert per window.
 *
 * @param src_ip  Source IP in network byte order (from ipv4_header.src_ip).
 * @param flags   TCP flags byte from tcp_header.flags.
 * Returns: 1 if a flood threshold was crossed, 0 otherwise.
 */
int detect_syn_flood(uint32_t src_ip, uint8_t flags);

/* ── Alert output ─────────────────────────────────────────────────────────── */

/**
 * emit_alert() — print a formatted, colour-coded IDS alert to stdout.
 *
 * @param severity   SEVERITY_{LOW,MEDIUM,HIGH}
 * @param rule_name  Short rule identifier  (e.g. "XMAS-SCAN")
 * @param description Human-readable explanation
 * @param src_ip     Source IP  (network byte order)
 * @param dst_ip     Destination IP (network byte order)
 * @param src_port   Source port  (network byte order)
 * @param dst_port   Destination port (network byte order)
 */
void emit_alert(alert_severity_t severity,
                const char       *rule_name,
                const char       *description,
                uint32_t          src_ip,
                uint32_t          dst_ip,
                uint16_t          src_port,
                uint16_t          dst_port);

/* ── Statistics accessors ─────────────────────────────────────────────────── */

/** Read-only view of session statistics. */
const ids_stats_t *ids_get_stats(void);

/**
 * Mutable pointer — used by main.c to increment counters without exposing
 * the storage location.  Callers must not store this pointer across
 * calls to ids_init() / ids_cleanup().
 */

ids_stats_t *ids_get_mutable_stats(void);

/** Print a formatted statistics summary to stdout. */
void ids_print_stats(void);

#endif /* CSENTINEL_DETECTOR_H */
