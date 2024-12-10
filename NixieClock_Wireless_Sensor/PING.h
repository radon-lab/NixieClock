#include <ping.h>
ping_option ping_options;

enum {
  PING_START,
  PING_WAIT,
  PING_END
};
uint8_t ping_state = PING_START;

void pingUpdate(void *opt, void *resp) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#if DEBUG_MODE
  ping_resp* ping_resp = reinterpret_cast<struct ping_resp*>(resp);
  if (ping_resp->ping_err != -1) {
    Serial.print("Ping end success, time: ");
    Serial.print(ping_resp->resp_time);
    Serial.println("ms");
  }
  else {
    Serial.println("Ping end error!");
  }
#endif
  ping_state = PING_END;
}

boolean pingCheck(void) {
  switch (ping_state) {
    case PING_START:
      memset(&ping_options, 0, sizeof(struct ping_option));

      ping_options.count = 1;
      ping_options.coarse_time = 1;
      ping_options.ip = WiFi.gatewayIP();

      ping_options.recv_function = reinterpret_cast<ping_recv_function>(&pingUpdate);
      ping_options.sent_function = NULL;

      if (ping_start(&ping_options)) {
#if DEBUG_MODE
        Serial.print("Ping start to IP address: ");
        Serial.println(WiFi.gatewayIP());
#endif
        ping_state = PING_WAIT;
      }
      else {
#if DEBUG_MODE
        Serial.print("Ping start error!");
#endif
        ping_state = PING_END;
      }
      break;
    case PING_END: return true;
  }

  return false;
}
