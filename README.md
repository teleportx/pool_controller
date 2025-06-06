# Pool Controller
Pool equipment control. Temperature maintenance, filtration, scheduled operations.

Remote control for your equipment over WiFi. Controller allows remote start filter and heater.
You can set: 
 - filter operation time
 - temperature to which the heater will heat water
 - temperature which heater will maintain

### Possible equipment scheme

<img width="648" alt="pool equipment scheme" src="https://github.com/user-attachments/assets/0c5b9a47-98ea-43e9-97e6-e782ec78dfc1" />

If necessery (e.g. three-phase connection) you can connect heater or pump over AC contactor.
You can also install temperature sensor in another location, for example in pool, but don't install sensor near the heater output, it may mess up temperature data.

### Content
  - [How it works?](#how-it-works)
    - [Controller scheme](#controller-scheme)

  - [Build and upload](#build-and-upload)
    - [Environment](#environment)
    - [Data collection](#data-collection)
  
  - [Manage](#manage)
    - [Manual](#manual)
    - [Web UI](#web-ui)
    - [API](#api)

## How it works?

You set the mode (M), operation time (OT) and pointer temperature (PT). Using a relays the pump and heater switching on. Getting temperature (T) using sensor.

**Modes:**
 - `0` Off.
 - `1` Filtering. Will switch on the pump during OT.
 - `2` Heating. Will switch on the pump and heater until temperature reaches PT.
 - `3` Maintaining. Will turn on the pump and heater when needed to maintain the PT. It will work for OT.

Maintain mode enable heater when `T + delta < PT`, and disable when `T > PT`. Now `delta = 0.5°C`.

After disable heater (regardless of the mode), the pump will run for 3 minutes to cool the heater.

Dry running check (not implemented now).

### Controller scheme

<img width="648" alt="pool equipment scheme" src="https://github.com/user-attachments/assets/e65bdb8e-31d1-4e3d-99cb-09b121ccd144" />

[Image](https://github.com/user-attachments/assets/624ebc2c-7f3a-4383-b93b-94c0c8e748e0) with best quality.
Detailed scheme in [fritizing scheme](scheme.fzz).

**Components:**
 - ESP32 
 - 4-digits display on TM1637
 - Encoder with button
 - Button
 - DS18B20 (temperature sensor)
 - 2x Relay 5V
 - Power unit 5V
 - ACS712 + 3x 1 kOm resistors (currency sensor for dry running check) (not required) 

## Build and upload

Open [platformio](https://platformio.org/) project in folder `hardware`. Create `.env` file and specify [environment](#environment) variables. Then build and upload to your esp32 board.

### Environment
 - `DEVICE_NAME` - device hostname uses for OTA.
 - `WIFI_SSID` - Wifi SSID.
 - `WIFI_PASSWORD` - Wifi password.
 - `NTP_HOST` - NTP server host.
 - `API_URL` - URL for [data collection](#data-collection) from controller. (e.g. `'"http://10.0.0.123:123/data"'`)
 - `API_KEY` - Authorization key for [data collection](#data-collection). 

### Data collection

Controller can automatically make HTTP requests to your server with data.

Controller makes `POST` request to `API_URL`. Body will contain data (see format [later](#api)) and in header `Authorization` will be `API_KEY`.

The controller will do this once every 5 seconds.

## Manage

### Manual

You can physically manage the controller using encoder (E), act button (AB) and display (D).

Coming soon...

### Web UI

In development...

### API

Access to API by HTTP using 80 port. After all control requests you will get actual data in body.

**Get data**

```http request
GET /data

Example answer:
{
    "currency": 1.393195,
    "temperature": 24,

    "heater_relay": false,
    "pump_relay": false,

    "timestamp": 1749161594,
    "uptime": 295143,

    "mode": {
        "disable_heater_time": 0,
        "duration": 25200,
        "mode": 0,
        "pointer_temperature": 35,
        "start_working_time": 251281
    }
}
```

- `currency` - Pump currency.
- `temperature` - Temperature in celsius degree.
- `pump_relay` - Pump status.
- `heater_relay` - Heater status.
- `timestamp` - UNIX timestamp when data generated. Getted from NTP server.
- `uptime` - Seconds after controller starts.
- `mode.mode` - Number of mode.
- `mode.duration` - OT.
- `mode.pointer_temperature` - PT
- `mode.start_working_time` - Time after controller starts, when mode was setted.
- `mode.disable_heater_time` - Time, when heater was disabled. (only in maintaining mode).


**Reboot controller**

```http request
POST /reboot

Authorization: <API_KEY>
```

OK answer not guaranteed. `API_KEY` is an [environment](#environment) variable.


**Set mode OFF**

```http request
POST /control

{
  "mode": 0,
  "graceful": false
}
```

- If `graceful` is false, the relays will immediately turned off (emergency stop). If true and heater enabled, the pump will run for 3 minutes to cool the heater.

**Set mode FILTERING**

```http request
POST /control

{
  "mode": 1,
  "duration": 1234
}
```

- `duration` - OT in seconds.

**Set mode HEATING**

```http request
POST /control

{
  "mode": 2,
  "pointer_temperature": 32.4
}
```

- `pointer_temperature` - PT in celsius degree.

**Set mode MAINTAINING**

```http request
POST /control

{
  "mode": 3,
  "duration": 1234,
  "pointer_temperature": 32.4
}
```

- `duration` - OT in seconds.
- `pointer_temperature` - PT in celsius degree.

