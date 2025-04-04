#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#define XSTR(x) #x
#define ENV(x) XSTR(x)

namespace PIN {
    constexpr unsigned int CURRENCY_SENSOR = 39;
    constexpr unsigned int TEMPERATURE_SENSOR = 34;
    constexpr unsigned int ACTION_BUTTON = 35;
    constexpr unsigned int DISPLAY_CLK = 32;
    constexpr unsigned int DISPLAY_DIO = 33;
    constexpr unsigned int ENCODER_SW = 25; // Encoder button
    constexpr unsigned int ENCODER_DT = 26;
    constexpr unsigned int ENCODER_CLK = 27;
    constexpr unsigned int PUMP_RELAY = 12;
    constexpr unsigned int HEATER_RELAY = 13;
}

const auto device_name = ENV(DEVICE_NAME);
const auto wifi_ssid = ENV(WIFI_SSID);
const auto wifi_password = ENV(WIFI_PASSWORD);
const auto ntp_host = ENV(NTP_HOST);
const auto api_key = ENV(API_KEY);
const auto api_url = ENV(API_URL);

#endif //HARDWARE_CONFIG_H
