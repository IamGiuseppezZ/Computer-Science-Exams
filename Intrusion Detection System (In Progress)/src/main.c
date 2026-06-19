/*
 * main.c — C-Sentinel entry point
 *
 * Responsibilities:
 *   1. Open a live pcap capture handle on a network interface.
 *   2. Install a BPF (Berkeley Packet Filter) to capture only IPv4
 *      TCP/UDP traffic, reducing CPU load.
 *   3. For each captured frame: walk the Ethernet → IPv4 → TCP/UDP
 *      header chain by casting the raw buffer pointer to our packed
 *      struct types (defined in headers.h).
 *   4. Feed each TCP segment to the IDS engine in detector.c.
 *   5. Print statistics on exit (Ctrl-C / SIGTERM).
 *
 * Build:  make
 * Run:    sudo ./csentinel [interface]
 *
 * Note: _DEFAULT_SOURCE is defined via -D in the Makefile DEFS variable,
 * which exposes sigaction(2) and related POSIX extensions on Linux/glibc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <net/if.h>      /* IF_NAMESIZE  */

#include <arpa/inet.h>   /* ntohs, ntohl, inet_ntop */
#include <pcap/pcap.h>   /* pcap_*                  */


/* ═══════════════════════════════════════════════════════════════════════════ */
/* Module-level globals                                                         */
/* ═══════════════════════════════════════════════════════════════════════════ */

/*
 * The pcap handle must be visible to the signal handler so that
 * pcap_breakloop() can be called from an async context.
 * Declared volatile to prevent the compiler from caching it in a register.
 */

