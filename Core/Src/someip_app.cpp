#include "someip_app.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "lwip/netif.h"
#include "lwip/ip_addr.h"

#include <string>
#include <string_view>

#include "someIp/ESomeIp.hpp"
#include "someIp/os/OS.hpp"
#include "someIp/communication/EndpointLwip.hpp" // converts ip_addr_t to someIp::IpAddr
#include "someIp/structs/Service.hpp"
#include "someIp/structs/Method.hpp"
#include "someIp/service_discovery/SdService.hpp"
#include "someIp/service_discovery/SdPool.hpp"
#include "someIp/service_discovery/Ipv4Option.hpp"
#include "someIp/service_discovery/LoadBalancingOption.hpp"

namespace {

// Must match examples/common/constants.hpp on the desktop side
constexpr uint16_t SERVICE_ID     = 0x1001;
constexpr uint16_t INSTANCE_ID    = 0x0001;
constexpr uint8_t  MAJOR_VERSION  = 1;
constexpr uint32_t MINOR_VERSION  = 0;

constexpr uint16_t RPC_METHOD_ID  = 0x0001;
constexpr uint16_t FIELD_GET_ID   = 0x0010;
constexpr uint16_t FIELD_SET_ID   = 0x0011;
constexpr uint16_t FIELD_EVENT_ID = 0x8002;
constexpr uint16_t EVENTGROUP_ID  = 0x0001;

constexpr uint16_t SERVER_PORT    = 8010;

static std::string payload_to_string(someIp::PackageRx &p) {
  return std::string(reinterpret_cast<const char *>(p.payload.data()), p.payload.size());
}

[[maybe_unused]] static someIp::sd::SdServiceInfo field_service_info() {
  return someIp::sd::SdServiceInfo{SERVICE_ID, INSTANCE_ID, MAJOR_VERSION, MINOR_VERSION};
}

void SomeIpTask(void *arg)
{
	(void)arg;

	// wait for DHCP to bind an address
	while (netif_default == nullptr ||
		 !netif_is_up(netif_default) ||
		 ip4_addr_isany_val(*netif_ip4_addr(netif_default)))
	{
	vTaskDelay(pdMS_TO_TICKS(100));
	}

	ip_addr_t local_ip = *netif_ip_addr4(netif_default);
	someIp::IpAddr local = someIp::ipaddr_from_lwip(local_ip);

	// static keeps this large object off the task stack
	static someIp::ESomeIp someip;
	someip.init(local);

#if SOMEIP_TESTCASE == 1 || SOMEIP_TESTCASE == 2
	auto echo_rr = [&someip](someIp::PackageRx &&p) {
	  std::string bytes = payload_to_string(p);
	  someip.send_response(std::move(p), bytes);
	};
#endif

#if SOMEIP_TESTCASE == 1
	// T1 request response server, no SD
#if SOMEIP_TRANSPORT == SOMEIP_TRANSPORT_TCP
	someip.listen_to_tcp_port(SERVER_PORT);
#else
	someip.listen_to_port(SERVER_PORT);
#endif
	{
	  someIp::Service *svc = someip.add_service(SERVICE_ID);
	  svc->register_method(someIp::Method(RPC_METHOD_ID, echo_rr));
	}
	printf("[RR] Test 1 server listening on port %u\n", SERVER_PORT);

#elif SOMEIP_TESTCASE == 2
	// T2 request response server offered via SD
	// UDP is always opened because SD runs over UDP
	someip.listen_to_port(SERVER_PORT);
#if SOMEIP_TRANSPORT == SOMEIP_TRANSPORT_TCP
	someip.listen_to_tcp_port(SERVER_PORT);
#endif
	{
	  someIp::Service *svc = someip.add_service(SERVICE_ID);
	  svc->register_method(someIp::Method(RPC_METHOD_ID, echo_rr));

	  someIp::sd::SdServiceInfo info{SERVICE_ID, INSTANCE_ID, MAJOR_VERSION, MINOR_VERSION};
	  info._src_port = SERVER_PORT;
	  info._weight = 10;
	  info._priority = 5;
	  auto sd_service = someIp::sd::pool_make_service<someIp::sd::SdService>(info);

	  someip.enable_sd(local);
	  someip.register_sd_service(sd_service);

	  auto ld = someIp::sd::pool_make<someIp::sd::LoadBalancingOption>(info._priority, info._weight, true);
	  auto ep = someIp::sd::pool_make<someIp::sd::Ipv4Option>(local, SERVER_PORT, someIp::sd::SdTransportProtocol::UDP, false);
	  someIp::sd::OptionVec opts;
	  opts.push_back(ld);
	  opts.push_back(ep);
	  sd_service->update_options(opts);

	  someip.offer_service(sd_service, local, info._src_port);
	}
	printf("[SD-RR] Test 2 server offering service 0x%04X via SD\n", SERVICE_ID);

#elif SOMEIP_TESTCASE == 3
	// T3 and T4 field and event group server, SD over UDP
	someip.set_transport_mode(someIp::TransportKind::UDP);
	someip.listen_to_port(SERVER_PORT);

	auto field_get = [&someip](someIp::PackageRx &&p) {
	  auto payload = someip.get_event_payload(field_service_info(), FIELD_EVENT_ID);
	  someip.send_response(std::move(p),
	      std::string_view(reinterpret_cast<const char *>(payload.data()), payload.size()));
	};
	// SET stores the value and fires the notification, responding only if asked
	auto field_set = [&someip](someIp::PackageRx &&p) {
	  bool wants_response = (p.header._message_type == MessageType::REQUEST);
	  std::string bytes = payload_to_string(p);
	  if (bytes.size() > 64) bytes.resize(64);  // MAX_EVENT_PAYLOAD
	  someip.set_event_payload(field_service_info(), FIELD_EVENT_ID, bytes);
	  if (wants_response) someip.send_response(std::move(p), bytes);
	};

	{
	  someIp::Service *svc = someip.add_service(SERVICE_ID);
	  svc->register_method(someIp::Method(FIELD_GET_ID, field_get));
	  svc->register_method(someIp::Method(FIELD_SET_ID, field_set));

	  someIp::sd::SdServiceInfo info = field_service_info();
	  info._src_port = SERVER_PORT;
	  info._weight = 10;
	  info._priority = 5;
	  auto sd_service = someIp::sd::pool_make_service<someIp::sd::SdService>(info);

	  someip.enable_sd(local);
	  someip.register_sd_service(sd_service);

	  sd_service->register_event(FIELD_EVENT_ID, EVENTGROUP_ID, true, someIp::TransportKind::UDP);

	  auto ld = someIp::sd::pool_make<someIp::sd::LoadBalancingOption>(info._priority, info._weight, true);
	  auto ep = someIp::sd::pool_make<someIp::sd::Ipv4Option>(local, SERVER_PORT, someIp::sd::SdTransportProtocol::UDP, false);
	  someIp::sd::OptionVec opts;
	  opts.push_back(ld);
	  opts.push_back(ep);
	  sd_service->update_options(opts);

	  someip.offer_service(sd_service, local, info._src_port);

	  // a field always has a value, so new subscribers get one immediately. T4 measures this.
	  someip.set_event_payload(field_service_info(), FIELD_EVENT_ID,
	                           std::string(64, 'I'));
	}
	printf("[FIELD] Test 3 field/event server offering service 0x%04X via SD\n", SERVICE_ID);

#elif SOMEIP_TESTCASE == 5
	// T5 SOME/IP-TP echo server, no SD
	someip.set_transport_mode(someIp::TransportKind::UDP_TP);
	someip.listen_to_port(SERVER_PORT);

	// the view avoids a std::string copy of the whole payload
	auto tp_echo = [&someip](someIp::PackageRx &&p) {
	  std::string_view view(reinterpret_cast<const char *>(p.payload.data()), p.payload.size());
	  someip.send_response(std::move(p), view);
	};

	{
	  someIp::Service *svc = someip.add_service(SERVICE_ID);
	  svc->register_method(someIp::Method(RPC_METHOD_ID, tp_echo));
	}
	printf("[TP] Test 5 SOME/IP-TP echo listening on port %u\n", SERVER_PORT);

#else
#error "SOMEIP_TESTCASE must be 1, 2, 3 or 5"
#endif

	// all work happens in the SOME/IP worker threads
	for (;;)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

} // namespace

extern "C" void someip_app_start(void)
{
  osThreadAttr_t attr = {};
  attr.name = "SomeIp";
  // 4 KB overflows. Cases 2 and 3 put a 768 B SD serialize buffer deep on this stack.
  attr.stack_size = 8192;
  attr.priority = osPriorityNormal;
  osThreadNew(SomeIpTask, nullptr, &attr);
}
