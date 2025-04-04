#include <WiFiUdp.h>

#include "ntp.h"
#include "config.h"

WiFiUDP ntpUDP;
NTPClient time_client(ntpUDP, ntp_host, UTC_OFFSET);
