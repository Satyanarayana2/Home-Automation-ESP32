# ESP32 Telegram-Based Smart Room Controller
A smart home automation system using an ESP32 microcontroller and Telegram bot to control and monitor appliances like lights, fan, AC, TV, and geyser. Supports auto-off timers, usage tracking (in hours), admin-only access, and full control via Telegram commands.

---

## 🔧 Features

* **Manual Control** via Telegram commands
* **Auto-Off Timer**: Schedule a device to turn off after N minutes
* **Usage Tracking**: Records total “ON” time (in hours) over the last 24hrs
* **Daily Reset**: Automatically resets usage counters every 24hrs
* **Admin-Only Access**: Restricts control to a single Telegram user ID
* **Status Reports**: View ON/OFF state and usage for all or individual devices

---

## 📦 Hardware & GPIO Mapping

| Device | GPIO Pin |
| :----: | :------: |
|  Light |    12    |
|   Fan  |    14    |
|   AC   |    27    |
|   TV   |    26    |
| Geyser |    25    |

> You can modify pins by editing the `devices[]` array in `main.cpp`.

---

## 💠 Setup & Installation

1. **Clone this repository**

   ```bash
   git clone https://github.com/YourUsername/esp32-room-controller.git
   cd esp32-room-controller
   ```

2. **Install Arduino Libraries**

   * `UniversalTelegramBot`
   * `WiFiClientSecure`
   * (Built-in) `WiFi` & `time.h`

   Or with PlatformIO, add to `platformio.ini`:

   ```ini
   lib_deps =
     ayushsharma82/UniversalTelegramBot
     bblanchon/ArduinoJson
   ```

3. **Configure Credentials** in `src/main.cpp`

   ```cpp
   #define WIFI_SSID       "<Your_WiFi_SSID>"
   #define WIFI_PASSWORD   "<Your_WiFi_Password>"
   #define BOT_TOKEN       "<Your_Telegram_Bot_Token>"
   #define ADMIN_CHAT_ID   "<Your_Telegram_User_ID>"
   ```

4. **Select Board & Upload**

   * **Arduino IDE**: Select “DOIT ESP32 DEVKIT V1” and upload
   * **PlatformIO**: `env: esp32dev` → Build & Upload

5. **Open Serial Monitor** at **115200baud** to view logs.

---

## 🤖 Telegram Commands

| Command                       | Description                                      |
| ----------------------------- | ------------------------------------------------ |
| `/start`                      | Show help & list of available commands           |
| `/list_all_devices`           | List all configured device names                 |
| `/<device>on`                 | Turn a specific device ON (e.g. `/lighton`)      |
| `/<device>off`                | Turn a specific device OFF (e.g. `/fanoff`)      |
| `/autooff <device> <minutes>` | Turn ON + auto-off after given minutes           |
| `/usage <device>`             | Show last-24hr usage (in hours) for one device   |
| `/status`                     | Show ON/OFF state & usage for **all** devices    |
| `/status <device>`            | Show state & usage for a **specific** device     |
| `/reset`                      | **Admin only**: Reset all usage counters to zero |

> All commands must be sent by the admin user defined in `ADMIN_CHAT_ID`.

---

## 📁 Project Structure

```
esp32-room-controller/
├── src/
│   └── main.cpp        # Main firmware code
├── platformio.ini      # (if using PlatformIO)
└── README.md           # This documentation
```

---

## 🔒 Security & Best Practices

* **Keep credentials private**: Don’t commit your SSID, password, or Bot token to public repos.
* **Admin ID**: Only the Telegram user ID in `ADMIN_CHAT_ID` can control devices.
* **Certificate**: The code uses `setCACert()` with `TELEGRAM_CERTIFICATE_ROOT` for secure TLS.

---

## 📬 Contributing

Bug reports, feature requests and pull requests are welcome!
Please open an issue for discussion before submitting major changes.

