#ifndef UDP_BASELINE_APP_H
#define UDP_BASELINE_APP_H

// Plain UDP echo server, no SOME/IP. Latency floor for the T1 and T3 numbers.
// Driven by the B1_udp_rr and B3_udp_notify clients.
// Build with UDP_BASELINE=1 to start this instead of the SOME/IP app.

#ifdef __cplusplus
extern "C" {
#endif

// Call once after the scheduler is running. Waits for DHCP internally.
void udp_baseline_start(void);

#ifdef __cplusplus
}
#endif

#endif /* UDP_BASELINE_APP_H */
