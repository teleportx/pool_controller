#ifndef NTP_H
#define NTP_H

#include <NTPClient.h>

constexpr int UTC_OFFSET = 3600 * 3;

extern NTPClient time_client;

#endif NTP_H
