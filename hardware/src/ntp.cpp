#include <WiFiUdp.h>

#include "ntp.h"
#include "config.h"

WiFiUDP ntpUDP;
NTPClient time_client(ntpUDP, ntp_host, UTC_OFFSET);

unsigned long long get_hardware_timestamp() {
    return esp_timer_get_time() / 1000000;
}
