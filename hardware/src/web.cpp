#include <AsyncJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "web.h"
#include "config.h"
#include "ntp.h"
#include "hardware.h"
#include "mode.h"

namespace web {
    AsyncWebServer server(80);

    String data;
    bool data_actual = false;

    void web_data(AsyncWebServerRequest *request);
    void web_reboot(AsyncWebServerRequest *request);
    void web_control(AsyncWebServerRequest *request, JsonVariant &payload);

    void collect_data();
    void send_data();

    Task collect_data_task(3000, TASK_FOREVER, &collect_data);
    Task send_data_task(5000, TASK_FOREVER, &send_data);

    void setup(Scheduler &runner) {
        server.begin();

        server.on("/data", HTTP_GET, [] (AsyncWebServerRequest *request) { web_data(request); });
        server.on("/json", HTTP_GET, [] (AsyncWebServerRequest *request) { web_data(request); });

        auto* control_handler = new AsyncCallbackJsonWebHandler(
                "/control",
                [](AsyncWebServerRequest *request, JsonVariant &json) { web_control(request, json); }
                );
        server.addHandler(control_handler);

        server.on("/reboot", HTTP_POST, [] (AsyncWebServerRequest *request) { web_reboot(request); });

        runner.addTask(collect_data_task);
        runner.addTask(send_data_task);

        collect_data_task.enable();
        send_data_task.enable();
    }

    void collect_data() {
        JsonDocument jdata;

        jdata["timestamp"] = time_client.getEpochTime();
        jdata["pump_relay"] = hardware::pump_status.get_now();
        jdata["heater_relay"] = hardware::heater_status.get_now();

        jdata["temperature"] = hardware::temperature;
        jdata["currency"] = hardware::currency;

        jdata["mode"]["mode"] = (int) mode::mode;
        jdata["mode"]["start_working_time"] = mode::start_working_time;
        jdata["mode"]["disable_heater_time"] = mode::disable_heater_time;
        jdata["mode"]["duration"] =  mode::duration;
        jdata["mode"]["pointer_temperature"] = mode::pointer_temperature;

        String data_s;
        serializeJson(jdata, data_s);

        Serial.println("Collect data");
        Serial.println(data_s);

        data = data_s;
        data_actual = true;
    }

    void send_data() {
        if (not WiFi.isConnected()) {
            Serial.println("WiFi not connected, not sending to API ");
            return;
        }

        if (not data_actual) {
            Serial.println("Data not actual, not sending to API ");
            return;
        }
        data_actual = false;

        unsigned long start = millis();
        WiFiClient client;
        HTTPClient http;

        String url = String(api_url);
        url = url.substring(1, url.length() - 1);

        http.begin(client, url);
        http.setTimeout(api_timeout);

        http.addHeader("Remote-Device-IP", WiFi.localIP().toString());
        http.addHeader("Authorization", api_key);

        int status_code = http.POST(data);

        http.end();

        if (200 <= status_code and status_code < 300) {
            Serial.print("Successfully sended data to API in ");
            Serial.print(millis() - start);
            Serial.println(" ms.");

        } else {
            Serial.print("ERROR ");
            Serial.print(status_code);
            Serial.print(" while sending data to API.\nAPI HOST: ");
            Serial.println(url);
        }
    }

    void web_data(AsyncWebServerRequest *request) {
        request->send(200, "application/json", data);
    }

    void web_reboot(AsyncWebServerRequest *request) {
        Serial.println("Remote tried to reboot.");
        if (request->hasHeader("Authorization") and request->getHeader("Authorization")->value() == api_key) {
            request->send(204);
            ESP.restart();
            return;
        }
        request->send(403);
    }

    void web_control(AsyncWebServerRequest *request, JsonVariant &payload) {
        if (not payload["mode"].is<int>()) {
            request->send(400, "application/json", R"({"detail": "mode must be int."})");
            return;
        }
        int mode_value = payload["mode"].as<int>();

        if ((mode_value == mode::Mode(mode::HEATING) or mode_value == mode::Mode(mode::MAINTAINING)) and not payload["pointer_temperature"].is<double>()) {
            request->send(400, "application/json", R"({"detail": "pointer_temperature must be double."})");
            return;
        }
        if ((mode_value == mode::Mode(mode::HEATING) or mode_value == mode::Mode(mode::MAINTAINING)) and payload["pointer_temperature"].as<double>() >= hardware::critical_temperature) {
            request->send(400, "application/json", R"({"detail": "pointer_temperature cannot be greater or equal critical temperature."})");
            return;
        }


        if ((mode_value == mode::Mode(mode::FILTERING) or mode_value == mode::Mode(mode::MAINTAINING)) and not payload["duration"].is<unsigned int>()) {
            request->send(400, "application/json", R"({"detail": "duration must be unsigned int".})");
            return;
        }

        if (mode_value == mode::Mode(mode::OFF)) {
            if (payload["duration"].is<bool>() and payload["duration"].as<bool>())
                mode::set_graceful_off();
            else
                mode::set_off();

        } else if (mode_value == mode::Mode(mode::FILTERING)) {
            mode::set_filtering(payload["duration"].as<unsigned int>());

        } else if (mode_value == mode::Mode(mode::HEATING)) {
            mode::set_heating(payload["pointer_temperature"].as<double>());

        } else if (mode_value == mode::Mode(mode::MAINTAINING)) {
            mode::set_maintaining(payload["pointer_temperature"].as<double>(), payload["duration"].as<unsigned int>());

        } else {
            request->send(400, "application/json", R"({"detail": "Unknown mode."})");
            return;
        }

        Serial.println("Accepted control request.");
        collect_data();
        request->send(200, data);
    }
}
