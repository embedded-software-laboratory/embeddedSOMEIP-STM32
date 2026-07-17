#ifndef SOMEIP_APP_H
#define SOMEIP_APP_H

// C wrapper, the C++ middleware cannot be included from main.c

// One scenario per flashed image, selected by SOMEIP_TESTCASE
//   1 = request response, no SD -> T1_rr_latency
//   2 = request response with SD -> T2_sd_ttfm
//   3 = field and event group, SD, UDP -> T3_notify_rr and T4_sd_ttfn
//   5 = SOME/IP-TP echo, no SD -> T5_tp_rtt
// SOMEIP_TRANSPORT applies to cases 1 and 2 only, must match the client sweep
#define SOMEIP_TRANSPORT_UDP  1
#define SOMEIP_TRANSPORT_TCP  2

#ifndef SOMEIP_TESTCASE
#define SOMEIP_TESTCASE 2
#endif

#ifndef SOMEIP_TRANSPORT
#define SOMEIP_TRANSPORT SOMEIP_TRANSPORT_UDP
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Call once after the scheduler is running. Waits for DHCP internally.
void someip_app_start(void);

#ifdef __cplusplus
}
#endif

#endif /* SOMEIP_APP_H */
