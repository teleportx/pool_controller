#ifndef NTP_H
#define NTP_H

#include <NTPClient.h>

constexpr int UTC_OFFSET = 3600 * 3;

extern NTPClient time_client;

unsigned long long get_hardware_timestamp();

#endif NTP_H
