#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <time.h>

// WiFi and Telegram Bot credentials
#define WIFI_SSID       "<Wifi SSID>"  // ← replace with your WiFi SSID
#define WIFI_PASSWORD   "<Wifi Password>"  // ← replace with your WiFi Password
#define BOT_TOKEN       "<Bot Token>"  // ← replace with your Bot Token

// Telegram polling interval
const unsigned long BOT_MTBS = 1000;
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;

// Telegram Admin User ID (only this user can control the bot)
#define ADMIN_CHAT_ID   "123456789"  // ← replace with your Admin Telegram numeric user ID

#define DEVICE_COUNT    5  // Total number of controlled devices

// Structure to hold device information
struct Device {
  String  name;          // Device name (used in commands)
  uint8_t pin;           // GPIO pin
  bool    state;         // ON/OFF state
  time_t  onTime;        // Timestamp when turned ON
  float   usageHours;    // Accumulated usage in the last 24 hrs
  time_t  autoOffTarget; // Scheduled auto-off timestamp
};

// List of devices and their GPIO mappings
Device devices[DEVICE_COUNT] = {
  { "light",   12, false, 0, 0.0, 0 },
  { "fan",     14, false, 0, 0.0, 0 },
  { "ac",      27, false, 0, 0.0, 0 },
  { "tv",      26, false, 0, 0.0, 0 },
  { "geyser",  25, false, 0, 0.0, 0 }
};

// Connect ESP32 to Wi‑Fi network
void connectWiFi() {
  Serial.print("Connecting to WiFi SSID ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP:");
  Serial.println(WiFi.localIP());
}

// Initialize NTP time (IST = UTC+5:30)
void initTime() {
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for time...");
    delay(1000);
  }
  Serial.println("Time initialized.");
}

// Find a Device by its name (case‑insensitive)
Device* getDeviceByName(String name) {
  name.toLowerCase();
  for (int i = 0; i < DEVICE_COUNT; i++) {
    if (devices[i].name == name) return &devices[i];
  }
  return nullptr;
}

// Turn a device ON (and record the ON time)
void turnDeviceOn(Device &d) {
  if (!d.state) {
    d.state = true;
    d.onTime = now();
    digitalWrite(d.pin, HIGH);
  }
}

// Turn a device OFF (and accumulate usage)
void turnDeviceOff(Device &d) {
  if (d.state) {
    d.state = false;
    time_t offTime = now();
    d.usageHours += float(offTime - d.onTime) / 3600.0;
    d.onTime = 0;
    d.autoOffTarget = 0;
    digitalWrite(d.pin, LOW);
  }
}

// Reset all usage counters to zero
void resetUsage() {
  for (int i = 0; i < DEVICE_COUNT; i++) {
    devices[i].usageHours = 0;
  }
}

// Handle incoming Telegram messages
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    // Enforce admin-only control
    if (chat_id != ADMIN_CHAT_ID) {
      bot.sendMessage(chat_id, "Access denied: Not authorized.", "");
      continue;
    }

    String text = bot.messages[i].text;
    text.toLowerCase();

    if (!text.startsWith("/")) continue;
    text.remove(0, 1);  // strip leading '/'

    // /start
    if (text == "start") {
      String reply = "Available commands:\n";
      reply += "/<device>on\n/<device>off\n/autooff <device> <mins>\n";
      reply += "/usage <device>\n/status\n/status <device>\n";
      reply += "/list_all_devices\n/reset\n";
      bot.sendMessage(chat_id, reply, "");
    }
    // /list_all_devices
    else if (text == "list_all_devices") {
      String list = "Devices:\n";
      for (int j = 0; j < DEVICE_COUNT; j++) {
        list += "- " + devices[j].name + "\n";
      }
      bot.sendMessage(chat_id, list, "");
    }
    // /status (all)
    else if (text == "status") {
      String status = "Device Status & Usage:\n";
      for (int j = 0; j < DEVICE_COUNT; j++) {
        Device &d = devices[j];
        float usage = d.usageHours;
        if (d.state) usage += float(now() - d.onTime) / 3600.0;
        status += d.name + ": " + (d.state ? "ON" : "OFF")
               + ", " + String(usage, 2) + " hrs\n";
      }
      bot.sendMessage(chat_id, status, "");
    }
    // /status <device>
    else if (text.startsWith("status ")) {
      String devName = text.substring(7);
      Device* d = getDeviceByName(devName);
      if (d) {
        float usage = d->usageHours;
        if (d->state) usage += float(now() - d->onTime) / 3600.0;
        String msg = d->name + ": " + (d->state ? "ON" : "OFF")
                   + ", " + String(usage, 2) + " hrs";
        bot.sendMessage(chat_id, msg, "");
      } else {
        bot.sendMessage(chat_id, "Device not found.", "");
      }
    }
    // /reset
    else if (text == "reset") {
      resetUsage();
      bot.sendMessage(chat_id, "Usage stats reset to 0.", "");
    }
    // /autooff <device> <minutes>
    else if (text.startsWith("autooff ")) {
      int sp1 = text.indexOf(' ');
      int sp2 = text.indexOf(' ', sp1 + 1);
      String name = text.substring(sp1 + 1, sp2);
      int mins = text.substring(sp2 + 1).toInt();
      Device* d = getDeviceByName(name);
      if (d && mins > 0) {
        d->autoOffTarget = now() + mins * 60;
        turnDeviceOn(*d);
        bot.sendMessage(chat_id, d->name + " will auto-off in " + mins + " mins.", "");
      }
    }
    // /usage <device>
    else if (text.startsWith("usage ")) {
      String name = text.substring(6);
      Device* d = getDeviceByName(name);
      if (d) {
        float usage = d->usageHours;
        if (d->state) usage += float(now() - d->onTime) / 3600.0;
        bot.sendMessage(chat_id, d->name + " used for " + String(usage,2) + " hrs in last 24 hrs", "");
      }
    }
    // Generic <device>on / <device>off
    else {
      bool isOn = false;
      String devName = text;
      if (devName.endsWith("on")) {
        isOn = true;
        devName.remove(devName.length() - 2);
      } else if (devName.endsWith("off")) {
        devName.remove(devName.length() - 3);
      } else {
        continue;
      }
      Device* d = getDeviceByName(devName);
      if (d) {
        if (isOn) {
          turnDeviceOn(*d);
          bot.sendMessage(chat_id, d->name + " turned ON", "");
        } else {
          turnDeviceOff(*d);
          bot.sendMessage(chat_id, d->name + " turned OFF", "");
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Initialize all device pins
  for (int i = 0; i < DEVICE_COUNT; i++) {
    pinMode(devices[i].pin, OUTPUT);
    digitalWrite(devices[i].pin, LOW);
  }
  connectWiFi();
  initTime();
  bot_lasttime = millis();
}

void loop() {
  // Poll Telegram
  if (millis() - bot_lasttime > BOT_MTBS) {
    int n = bot.getUpdates(bot.last_message_received + 1);
    while (n) {
      handleNewMessages(n);
      n = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }

  // Auto‑off and daily reset
  time_t nowT = now();
  static time_t lastReset = nowT;

  // Check auto-off timers
  for (int i = 0; i < DEVICE_COUNT; i++) {
    if (devices[i].autoOffTarget > 0 && nowT >= devices[i].autoOffTarget) {
      turnDeviceOff(devices[i]);
    }
  }

  // Reset usage every 24 hrs
  if (nowT - lastReset >= 86400) {
    resetUsage();
    lastReset = nowT;
  }
}
