#include <DNSServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include "GoogleRootCA.h"
#include "TinyLCD9488.h"

// Confirmed TinyLCD -> XIAO ESP32-C6 wiring.
static constexpr int PIN_LCD_MOSI = D10;
static constexpr int PIN_LCD_SCLK = D8;
static constexpr int PIN_LCD_CS = D9;
static constexpr int PIN_LCD_DC = D0;
static constexpr int PIN_LCD_RST = D3;
static constexpr int PIN_TOUCH_IRQ = D1;
static constexpr int PIN_TOUCH_CS = D2;
static constexpr int PIN_TOUCH_DOUT = D4;
static constexpr int PIN_BOOT_BUTTON = 9;

static constexpr char SETUP_AP_NAME[] = "ESP-YT-Setup";
static constexpr char SETUP_AP_PASSWORD[] = "setup-subs";
static constexpr char EASTERN_TIMEZONE[] =
    "EST5EDT,M3.2.0/2,M11.1.0/2";
static constexpr uint32_t YOUTUBE_REFRESH_MS = 30UL * 60UL * 1000UL;
static constexpr uint32_t ERROR_RETRY_MS = 30UL * 1000UL;
static constexpr uint32_t PAGE_ROTATION_MS = 30UL * 1000UL;
static constexpr uint32_t TOUCH_DEBOUNCE_MS = 12;
static constexpr uint32_t TAP_MIN_MS = 10;
static constexpr uint32_t TAP_MAX_MS = 1000;
static constexpr uint32_t DOUBLE_TAP_MIN_GAP_MS = 40;
static constexpr uint32_t DOUBLE_TAP_WINDOW_MS = 600;
static constexpr uint32_t TOUCH_STUCK_CHECK_MS = 1500;
static constexpr uint16_t TOUCH_RELEASE_PRESSURE = 80;

TinyLCD9488 display(PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_MOSI,
                    PIN_LCD_SCLK, PIN_TOUCH_DOUT);
Preferences preferences;
DNSServer dnsServer;
WebServer webServer(80);

bool setupPortalActive = false;
bool restartRequested = false;
uint32_t restartRequestedAt = 0;
uint32_t lastFetchAttempt = 0;
uint32_t nextFetchDelay = 0;
uint32_t lastServerRetry = 0;
uint32_t pageChangedAt = 0;
int lastClockMinute = -1;
bool clockReady = false;
bool timeSyncStarted = false;
bool screenRotated180 = false;
bool touchRawPressed = false;
bool touchStablePressed = false;
bool pendingSingleTap = false;
bool suppressTapRelease = false;
uint32_t touchRawChangedAt = 0;
uint32_t touchPressedAt = 0;
uint32_t firstTapReleasedAt = 0;
String configuredSsid;
String configuredPassword;
String youtubeApiKey;
String youtubeHandle;

enum class DisplayPage { SubscriberCounter, Clock };
DisplayPage activePage = DisplayPage::SubscriberCounter;

struct ChannelStats {
  bool valid = false;
  String title = "YouTube Channel";
  uint64_t subscribers = 0;
  uint64_t views = 0;
  uint64_t videos = 0;
};

ChannelStats currentStats;

static constexpr uint16_t COLOR_BACKGROUND = 0x0862;
static constexpr uint16_t COLOR_PANEL = 0x10C4;
static constexpr uint16_t COLOR_PANEL_LIGHT = 0x2147;
static constexpr uint16_t COLOR_YOUTUBE = 0xF800;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;
static constexpr uint16_t COLOR_MUTED = 0xA534;
static constexpr uint16_t COLOR_ACCENT = 0x2EDF;
static constexpr uint16_t COLOR_GOOD = 0x4E8B;
static constexpr uint16_t COLOR_WARNING = 0xFD20;

String dashboardStatus = "Starting...";
uint16_t dashboardStatusColor = COLOR_WARNING;

void centeredText(const String &text, int16_t y, uint16_t color,
                  uint8_t size) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextWrap(false);
  display.setTextSize(size);
  display.setTextColor(color);
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((display.width() - static_cast<int16_t>(w)) / 2 - x1, y);
  display.print(text);
}

String formatNumber(uint64_t value) {
  char raw[32];
  snprintf(raw, sizeof(raw), "%llu", static_cast<unsigned long long>(value));
  String source(raw);
  String result;
  result.reserve(source.length() + source.length() / 3);
  for (size_t i = 0; i < source.length(); ++i) {
    if (i > 0 && (source.length() - i) % 3 == 0) result += ',';
    result += source[i];
  }
  return result;
}

String compactNumber(uint64_t value) {
  if (value < 1000) return formatNumber(value);
  const char *suffix = "K";
  double scaled = value / 1000.0;
  if (value >= 1000000000ULL) {
    suffix = "B";
    scaled = value / 1000000000.0;
  } else if (value >= 1000000ULL) {
    suffix = "M";
    scaled = value / 1000000.0;
  }
  char result[20];
  snprintf(result, sizeof(result), scaled >= 100.0 ? "%.0f%s" : "%.1f%s",
           scaled, suffix);
  return String(result);
}

String formattedUpdateTimestamp() {
  time_t now = 0;
  time(&now);
  if (now <= 1735689600) return "Updated from YouTube";

  struct tm localTime;
  if (localtime_r(&now, &localTime) == nullptr) return "Updated from YouTube";

  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char timezone[8] = "ET";
  strftime(timezone, sizeof(timezone), "%Z", &localTime);

  char result[52];
  snprintf(result, sizeof(result), "Updated %s %d, %d %02d:%02d %s",
           months[localTime.tm_mon], localTime.tm_mday,
           localTime.tm_year + 1900, localTime.tm_hour, localTime.tm_min,
           timezone);
  return String(result);
}

void drawHeader() {
  display.fillRoundRect(12, 10, 456, 58, 12, COLOR_YOUTUBE);
  display.fillRoundRect(27, 25, 43, 29, 7, COLOR_WHITE);
  display.fillTriangle(44, 31, 44, 48, 58, 39, COLOR_YOUTUBE);
  String displayedHandle = youtubeHandle.isEmpty() ? String("YOUTUBE")
                                                    : String("@") + youtubeHandle;
  displayedHandle.toUpperCase();
  uint8_t handleSize = 3;
  if (displayedHandle.length() > 18) handleSize = 2;
  if (displayedHandle.length() > 27) handleSize = 1;
  display.setTextSize(handleSize);
  display.setTextColor(COLOR_WHITE);
  display.setCursor(87, handleSize == 3 ? 28 : (handleSize == 2 ? 32 : 36));
  display.print(displayedHandle);
}

void drawStatusBar(const String &message, uint16_t color) {
  display.fillRect(0, 298, 480, 22, COLOR_PANEL);
  centeredText(message, 305, color, 1);
}

void drawSplash() {
  display.fillScreen(COLOR_BACKGROUND);
  drawHeader();
  centeredText("SUBSCRIBER", 108, COLOR_WHITE, 5);
  centeredText("COUNTER", 158, COLOR_ACCENT, 5);
  centeredText("Starting...", 246, COLOR_MUTED, 2);
}

void drawConnectingScreen(const String &ssid) {
  display.fillScreen(COLOR_BACKGROUND);
  drawHeader();
  centeredText("CONNECTING", 103, COLOR_WHITE, 4);
  centeredText(ssid, 158, COLOR_ACCENT, ssid.length() > 20 ? 2 : 3);
  centeredText("Please wait", 225, COLOR_MUTED, 2);
  drawStatusBar("Wi-Fi connection in progress", COLOR_WARNING);
}

void drawSetupScreen(const String &reason) {
  display.fillScreen(COLOR_BACKGROUND);
  drawHeader();
  centeredText("SETUP WI-FI", 84, COLOR_WHITE, 4);
  centeredText("Join this hotspot:", 130, COLOR_MUTED, 2);
  centeredText(SETUP_AP_NAME, 158, COLOR_ACCENT, 3);
  centeredText("Password: setup-subs", 198, COLOR_WHITE, 2);
  centeredText("The setup page should open automatically", 234, COLOR_MUTED,
               1);
  centeredText("or browse to 192.168.4.1", 251, COLOR_MUTED, 1);
  drawStatusBar(reason, COLOR_WARNING);
}

void drawMetricCard(int16_t x, const String &label, const String &value) {
  display.fillRoundRect(x, 221, 214, 64, 9, COLOR_PANEL);
  display.setTextColor(COLOR_MUTED);
  display.setTextSize(1);
  display.setCursor(x + 12, 231);
  display.print(label);
  display.setTextColor(COLOR_WHITE);
  display.setTextSize(3);
  display.setCursor(x + 12, 250);
  display.print(value);
}

void drawDashboard(const ChannelStats &stats, const String &status,
                   uint16_t statusColor) {
  display.fillScreen(COLOR_BACKGROUND);
  drawHeader();
  centeredText("SUBSCRIBERS", 83, COLOR_MUTED, 2);

  const String subscriberText = stats.valid ? formatNumber(stats.subscribers)
                                            : String("--");
  uint8_t numberSize = 8;
  if (subscriberText.length() > 8) numberSize = 7;
  if (subscriberText.length() > 10) numberSize = 6;
  centeredText(subscriberText, 119, COLOR_WHITE, numberSize);

  display.fillRoundRect(156, 193, 168, 20, 10, COLOR_PANEL_LIGHT);
  centeredText("YOUTUBE PUBLIC TOTAL", 199, COLOR_ACCENT, 1);
  drawMetricCard(18, "TOTAL VIEWS",
                 stats.valid ? compactNumber(stats.views) : String("--"));
  drawMetricCard(248, "PUBLIC VIDEOS",
                 stats.valid ? formatNumber(stats.videos) : String("--"));
  drawStatusBar(status, statusColor);
}

bool readLocalTime(struct tm &localTime) {
  time_t now = 0;
  time(&now);
  return now > 1735689600 && localtime_r(&now, &localTime) != nullptr;
}

void drawClockScreen(bool fullRedraw) {
  struct tm localTime;
  if (!readLocalTime(localTime)) return;

  if (fullRedraw) {
    display.fillScreen(COLOR_BACKGROUND);
    drawHeader();
    centeredText("EASTERN TIME", 82, COLOR_MUTED, 2);
    display.fillRoundRect(36, 111, 408, 88, 14, COLOR_PANEL);

    char dateText[40];
    strftime(dateText, sizeof(dateText), "%A, %b %d, %Y", &localTime);
    centeredText(dateText, 224, COLOR_WHITE, 3);

    char timezone[8] = "ET";
    strftime(timezone, sizeof(timezone), "%Z", &localTime);
    centeredText(timezone, 267, COLOR_ACCENT, 2);
    drawStatusBar("Alternates with subscriber count every 30 seconds",
                  COLOR_GOOD);
  } else {
    display.fillRoundRect(36, 111, 408, 88, 14, COLOR_PANEL);
  }

  char timeText[12];
  strftime(timeText, sizeof(timeText), "%H:%M", &localTime);
  centeredText(timeText, 119, COLOR_WHITE, 10);
  lastClockMinute = localTime.tm_min;
}

void drawServerErrorScreen(const String &title, const String &detail) {
  activePage = DisplayPage::SubscriberCounter;
  display.fillScreen(COLOR_BACKGROUND);
  drawHeader();
  centeredText("SERVER OFFLINE", 93, COLOR_WARNING, 4);
  centeredText(title, 151, COLOR_WHITE, title.length() > 22 ? 2 : 3);
  centeredText(detail, 205, COLOR_MUTED, detail.length() > 38 ? 1 : 2);
  centeredText("Subscriber counter will keep trying", 249, COLOR_MUTED, 1);
  drawStatusBar("Retrying automatically in 30 seconds", COLOR_WARNING);
}

void setDashboardStatus(const String &message, uint16_t color) {
  dashboardStatus = message;
  dashboardStatusColor = color;
  if (activePage == DisplayPage::SubscriberCounter) {
    drawStatusBar(dashboardStatus, dashboardStatusColor);
  }
}

void showPage(DisplayPage page) {
  activePage = page;
  pageChangedAt = millis();
  if (activePage == DisplayPage::Clock) {
    drawClockScreen(true);
  } else {
    drawDashboard(currentStats, dashboardStatus, dashboardStatusColor);
  }
}

uint16_t readTouch12(uint8_t command) {
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_LCD_CS, HIGH);
  digitalWrite(PIN_TOUCH_CS, LOW);
  SPI.transfer(command);
  const uint16_t value =
      (static_cast<uint16_t>(SPI.transfer(0x00)) << 8) | SPI.transfer(0x00);
  digitalWrite(PIN_TOUCH_CS, HIGH);
  SPI.endTransaction();
  return (value >> 3) & 0x0FFF;
}

void initializeTouch() {
  pinMode(PIN_TOUCH_CS, OUTPUT);
  digitalWrite(PIN_TOUCH_CS, HIGH);
  pinMode(PIN_TOUCH_IRQ, INPUT_PULLUP);

  // Finish with a power-down conversion so PENIRQ remains enabled while idle.
  readTouch12(0x90);
  touchRawPressed = digitalRead(PIN_TOUCH_IRQ) == LOW;
  touchStablePressed = touchRawPressed;
  touchRawChangedAt = millis();
  Serial.printf("Touch initialized: IRQ=D1, CS=D2, DOUT=D4, idle=%s\n",
                touchRawPressed ? "pressed/low" : "released/high");
}

void saveDisplayRotation() {
  preferences.begin("espyt", false);
  preferences.putBool("rotate180", screenRotated180);
  preferences.end();
}

void handleSingleTap() {
  if (!clockReady) return;
  const DisplayPage nextPage =
      activePage == DisplayPage::SubscriberCounter
          ? DisplayPage::Clock
          : DisplayPage::SubscriberCounter;
  Serial.println(nextPage == DisplayPage::Clock
                     ? "Touch: single tap -> clock"
                     : "Touch: single tap -> subscriber counter");
  showPage(nextPage);
}

void handleDoubleTap() {
  screenRotated180 = !screenRotated180;
  display.setRotation180(screenRotated180);
  saveDisplayRotation();
  Serial.printf("Touch: double tap -> display rotation %s\n",
                screenRotated180 ? "180 degrees" : "normal");
  if (clockReady) {
    showPage(activePage);
  } else {
    drawSplash();
  }
}

void updateTouch() {
  const uint32_t now = millis();

  // A noisy jumper or interrupted XPT2046 conversion can occasionally leave
  // PENIRQ asserted after the finger is gone. Check pressure during an
  // abnormally long press and explicitly return the controller to PENIRQ idle
  // instead of leaving gesture input wedged until the next reboot.
  if (touchStablePressed &&
      now - touchPressedAt >= TOUCH_STUCK_CHECK_MS) {
    const uint16_t rawZ1 = readTouch12(0xB0);
    readTouch12(0x90);
    if (rawZ1 < TOUCH_RELEASE_PRESSURE &&
        digitalRead(PIN_TOUCH_IRQ) == HIGH) {
      touchRawPressed = false;
      touchStablePressed = false;
      suppressTapRelease = false;
      touchRawChangedAt = now;
      Serial.println("Touch: recovered controller from stuck press");
    } else {
      // It is a real long press. It is not a tap, and its eventual release
      // must not become one merely because this timer was restarted.
      suppressTapRelease = true;
      touchPressedAt = now;
    }
  }

  const bool rawPressed = digitalRead(PIN_TOUCH_IRQ) == LOW;

  if (rawPressed != touchRawPressed) {
    touchRawPressed = rawPressed;
    touchRawChangedAt = now;
  }

  if (rawPressed != touchStablePressed &&
      now - touchRawChangedAt >= TOUCH_DEBOUNCE_MS) {
    touchStablePressed = rawPressed;
    if (touchStablePressed) {
      touchPressedAt = now;
      const uint16_t rawX = readTouch12(0xD0);
      const uint16_t rawY = readTouch12(0x90);
      const uint16_t rawZ1 = readTouch12(0xB0);
      readTouch12(0x90);  // Return the controller to PENIRQ-enabled idle mode.
      Serial.printf("Touch down: x=%u y=%u z1=%u\n", rawX, rawY, rawZ1);

      // Recognize a double tap on its second press. This is both faster and
      // more tolerant of the uneven release timing of a resistive panel.
      if (pendingSingleTap) {
        const uint32_t gap = now - firstTapReleasedAt;
        if (gap >= DOUBLE_TAP_MIN_GAP_MS &&
            gap <= DOUBLE_TAP_WINDOW_MS) {
          pendingSingleTap = false;
          suppressTapRelease = true;
          handleDoubleTap();
        } else if (gap < DOUBLE_TAP_MIN_GAP_MS) {
          // Ignore a brief contact bounce rather than treating it as tap two.
          suppressTapRelease = true;
        }
      }
    } else {
      if (suppressTapRelease) {
        suppressTapRelease = false;
        return;
      }
      const uint32_t pressDuration = now - touchPressedAt;
      if (pressDuration >= TAP_MIN_MS && pressDuration <= TAP_MAX_MS) {
        pendingSingleTap = true;
        firstTapReleasedAt = now;
      }
    }
  }

  if (pendingSingleTap &&
      now - firstTapReleasedAt > DOUBLE_TAP_WINDOW_MS) {
    pendingSingleTap = false;
    handleSingleTap();
  }
}

String htmlEscape(const String &input) {
  String output;
  output.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); ++i) {
    switch (input[i]) {
      case '&': output += F("&amp;"); break;
      case '<': output += F("&lt;"); break;
      case '>': output += F("&gt;"); break;
      case '\"': output += F("&quot;"); break;
      case '\'': output += F("&#39;"); break;
      default: output += input[i];
    }
  }
  return output;
}

String portalPage(const String &message = "") {
  const int networkCount = WiFi.scanNetworks(false, true);
  String html;
  html.reserve(11000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>ESP-YT Setup</title><style>");
  html += F("body{margin:0;background:#07131d;color:#eef7ff;font-family:-apple-system,BlinkMacSystemFont,Segoe UI,sans-serif}");
  html += F("main{max-width:560px;margin:auto;padding:22px}header{background:#e62117;border-radius:18px;padding:20px;margin-bottom:18px}");
  html += F("h1{margin:0;font-size:27px}h2{margin:5px 0 0;font-size:16px;font-weight:500;opacity:.9}");
  html += F("section{background:#102433;border-radius:18px;padding:20px;margin-top:14px}label{display:block;font-weight:700;margin:14px 0 7px}");
  html += F("select,input{box-sizing:border-box;width:100%;font-size:17px;padding:13px;border:1px solid #426277;border-radius:11px;background:#07131d;color:white}");
  html += F("button{width:100%;border:0;border-radius:12px;background:#24b9ff;color:#00121f;font-size:18px;font-weight:800;padding:15px;margin-top:20px}");
  html += F("small{display:block;color:#a8bdca;line-height:1.45;margin-top:8px}.msg{background:#3d2e08;color:#ffd96b;padding:11px;border-radius:10px}.signal{color:#8ee3ad}");
  html += F("a{color:#64ceff}</style></head><body><main><header><h1>ESP-YT Subscriber Counter</h1><h2>Configure any public YouTube channel</h2></header>");
  if (message.length()) {
    html += F("<div class='msg'>");
    html += htmlEscape(message);
    html += F("</div>");
  }
  html += F("<form method='post' action='/save'><section><h2>Choose your Wi-Fi</h2><label for='ssid'>Network</label><select id='ssid' name='ssid' required>");
  if (networkCount <= 0) {
    html += F("<option value=''>No networks found - reload to scan again</option>");
  } else {
    for (int i = 0; i < networkCount; ++i) {
      const String ssid = WiFi.SSID(i);
      html += F("<option value=\"");
      html += htmlEscape(ssid);
      html += F("\">");
      html += htmlEscape(ssid);
      html += F(" &nbsp; (");
      html += String(WiFi.RSSI(i));
      html += F(" dBm)");
      if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) html += F(" - open");
      html += F("</option>");
    }
  }
  html += F("</select><label for='password'>Wi-Fi password</label><input id='password' name='password' type='password' autocomplete='current-password'>");
  html += F("<small>The password stays in this ESP32's local flash.</small></section>");
  html += F("<section><h2>YouTube connection</h2><label for='handle'>YouTube @handle</label><input id='handle' name='handle' type='text' inputmode='text' autocomplete='off' autocapitalize='none' placeholder='@YourChannel' required>");
  html += F("<small>Enter the channel handle shown after youtube.com/@. The leading @ is optional.</small>");
  html += F("<label for='apikey'>YouTube Data API key</label><input id='apikey' name='apikey' type='password' required>");
  html += F("<small>Enable YouTube Data API v3 for the key. Restrict the key to that API when possible. The device uses it only to read public channel statistics.</small>");
  html += F("<button type='submit'>Save and start counter</button></section></form>");
  html += F("<small style='text-align:center;margin:18px'>Setup hotspot: ESP-YT-Setup &bull; Device address: 192.168.4.1</small></main></body></html>");
  WiFi.scanDelete();
  return html;
}

void redirectToPortal() {
  webServer.sendHeader("Location", "http://192.168.4.1/", true);
  webServer.send(302, "text/plain", "");
}

void handlePortalRoot() {
  webServer.send(200, "text/html; charset=utf-8", portalPage());
}

void handleSaveConfiguration() {
  String ssid = webServer.arg("ssid");
  String password = webServer.arg("password");
  String apiKey = webServer.arg("apikey");
  String handle = webServer.arg("handle");
  ssid.trim();
  apiKey.trim();
  handle.trim();
  while (handle.startsWith("@")) handle.remove(0, 1);

  bool validHandle = !handle.isEmpty() && handle.length() <= 100;
  for (size_t i = 0; i < handle.length() && validHandle; ++i) {
    const char c = handle[i];
    if (c <= 0x20 || c == '/' || c == '?' || c == '#' || c == '&' || c == '=') {
      validHandle = false;
    }
  }

  if (ssid.isEmpty() || apiKey.isEmpty() || !validHandle) {
    webServer.send(400, "text/html; charset=utf-8",
                   portalPage("Select Wi-Fi and enter a valid YouTube @handle and API key."));
    return;
  }

  preferences.begin("espyt", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", password);
  preferences.putString("apikey", apiKey);
  preferences.putString("handle", handle);
  preferences.end();

  String response = F("<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'><style>body{background:#07131d;color:white;font-family:sans-serif;text-align:center;padding:50px}div{max-width:440px;margin:auto;background:#102433;padding:30px;border-radius:20px}h1{color:#64ceff}</style><div><h1>Saved</h1><p>The counter is restarting and will connect to <b>");
  response += htmlEscape(ssid);
  response += F("</b> and load <b>@");
  response += htmlEscape(handle);
  response += F("</b>.</p><p>You can close this page.</p></div>");
  webServer.send(200, "text/html; charset=utf-8", response);
  restartRequested = true;
  restartRequestedAt = millis();
  drawConnectingScreen(ssid);
}

void startSetupPortal(const String &reason) {
  setupPortalActive = true;
  WiFi.disconnect(true, false);
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(SETUP_AP_NAME, SETUP_AP_PASSWORD);
  delay(200);

  dnsServer.start(53, "*", WiFi.softAPIP());
  webServer.on("/", HTTP_GET, handlePortalRoot);
  webServer.on("/save", HTTP_POST, handleSaveConfiguration);
  webServer.on("/generate_204", HTTP_ANY, redirectToPortal);
  webServer.on("/hotspot-detect.html", HTTP_ANY, redirectToPortal);
  webServer.on("/ncsi.txt", HTTP_ANY, redirectToPortal);
  webServer.on("/connecttest.txt", HTTP_ANY, redirectToPortal);
  webServer.onNotFound(redirectToPortal);
  webServer.begin();

  drawSetupScreen(reason);
  Serial.printf("Setup portal active: Wi-Fi '%s', password '%s', http://%s/\n",
                SETUP_AP_NAME, SETUP_AP_PASSWORD,
                WiFi.softAPIP().toString().c_str());
}

bool loadConfiguration() {
  preferences.begin("espyt", true);
  configuredSsid = preferences.getString("ssid", "");
  configuredPassword = preferences.getString("pass", "");
  youtubeApiKey = preferences.getString("apikey", "");
  youtubeHandle = preferences.getString("handle", "");
  preferences.end();
  youtubeHandle.trim();
  while (youtubeHandle.startsWith("@")) youtubeHandle.remove(0, 1);
  return !configuredSsid.isEmpty() && !youtubeApiKey.isEmpty() &&
         !youtubeHandle.isEmpty();
}

void clearConfiguration() {
  preferences.begin("espyt", false);
  preferences.clear();
  preferences.end();
  configuredSsid = "";
  configuredPassword = "";
  youtubeApiKey = "";
  youtubeHandle = "";
  screenRotated180 = false;
  display.setRotation180(false);
}

bool connectToConfiguredWifi() {
  drawConnectingScreen(configuredSsid);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(configuredSsid.c_str(), configuredPassword.c_str());

  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 20000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) return false;

  Serial.printf("Wi-Fi connected to %s, IP %s\n", configuredSsid.c_str(),
                WiFi.localIP().toString().c_str());
  return true;
}

bool synchronizeClock() {
  struct tm localTime;
  if (readLocalTime(localTime)) {
    clockReady = true;
    return true;
  }

  lastServerRetry = millis();
  setDashboardStatus("Checking DNS and time servers...", COLOR_WARNING);

  IPAddress resolvedAddress;
  Serial.println("Checking DNS for time.google.com");
  if (WiFi.hostByName("time.google.com", resolvedAddress) != 1) {
    Serial.println("DNS server unavailable; will retry without rebooting");
    lastServerRetry = millis();
    drawServerErrorScreen("DNS UNAVAILABLE",
                          "Cannot resolve time.google.com");
    return false;
  }
  Serial.printf("DNS resolved time.google.com to %s\n",
                resolvedAddress.toString().c_str());

  setDashboardStatus("Synchronizing secure clock...", COLOR_WARNING);
  // America/New_York with automatic US daylight-saving transitions. Unlike
  // configTime(0, 0, ...), configTzTime applies and retains the timezone.
  if (!timeSyncStarted) {
    configTzTime(EASTERN_TIMEZONE, "time.google.com", "pool.ntp.org");
    timeSyncStarted = true;
  }
  const uint32_t started = millis();
  time_t now = 0;
  while (millis() - started < 15000) {
    time(&now);
    if (now > 1735689600) {
      Serial.printf("Clock synchronized: %lld\n", static_cast<long long>(now));
      clockReady = true;
      return true;
    }
    delay(250);
  }
  Serial.println("Time server unavailable; will retry without rebooting");
  lastServerRetry = millis();
  drawServerErrorScreen("TIME SERVER UNREACHABLE",
                        "DNS works, but NTP did not answer");
  return false;
}

String extractJsonString(const String &json, const String &key) {
  const String token = String('"') + key + '"';
  int position = json.indexOf(token);
  if (position < 0) return "";
  position = json.indexOf(':', position + token.length());
  if (position < 0) return "";
  const int firstQuote = json.indexOf('"', position + 1);
  if (firstQuote < 0) return "";
  const int secondQuote = json.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) return "";
  return json.substring(firstQuote + 1, secondQuote);
}

bool fetchYouTubeStats(ChannelStats &stats, String &errorMessage) {
  if (WiFi.status() != WL_CONNECTED) {
    errorMessage = "Wi-Fi disconnected";
    return false;
  }

  String encodedHandle;
  const String handleWithAt = "@" + youtubeHandle;
  encodedHandle.reserve(handleWithAt.length() * 3);
  static constexpr char HEX_DIGITS[] = "0123456789ABCDEF";
  for (size_t i = 0; i < handleWithAt.length(); ++i) {
    const uint8_t c = static_cast<uint8_t>(handleWithAt[i]);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
        c == '~') {
      encodedHandle += static_cast<char>(c);
    } else {
      encodedHandle += '%';
      encodedHandle += HEX_DIGITS[c >> 4];
      encodedHandle += HEX_DIGITS[c & 0x0F];
    }
  }

  String url = F("https://www.googleapis.com/youtube/v3/channels?part=snippet%2Cstatistics&forHandle=");
  url += encodedHandle;
  url += F("&key=");
  url += youtubeApiKey;

  WiFiClientSecure secureClient;
  secureClient.setCACert(GOOGLE_GTS_ROOTS);
  HTTPClient http;
  http.setConnectTimeout(12000);
  http.setTimeout(15000);
  if (!http.begin(secureClient, url)) {
    errorMessage = "Could not start YouTube request";
    return false;
  }

  const int statusCode = http.GET();
  const String response = http.getString();
  if (statusCode <= 0) {
    char tlsMessage[192] = {};
    const int tlsError = secureClient.lastError(tlsMessage, sizeof(tlsMessage));
    Serial.printf("YouTube transport error %d (%s), TLS %d: %s\n",
                  statusCode, HTTPClient::errorToString(statusCode).c_str(),
                  tlsError, tlsMessage);
  }
  http.end();

  if (statusCode != HTTP_CODE_OK) {
    errorMessage = statusCode <= 0
                       ? String("YouTube connection failed (") + String(statusCode) + ")"
                       : String("YouTube API error ") + String(statusCode);
    Serial.printf("YouTube HTTP %d: %s\n", statusCode, response.c_str());
    return false;
  }

  const String subscriberText = extractJsonString(response, "subscriberCount");
  const String viewText = extractJsonString(response, "viewCount");
  const String videoText = extractJsonString(response, "videoCount");
  const String title = extractJsonString(response, "title");
  if (subscriberText.isEmpty() || viewText.isEmpty() || videoText.isEmpty()) {
    errorMessage = response.indexOf(F("\"items\": []")) >= 0
                       ? String("Channel @") + youtubeHandle + " not found"
                       : String("Channel statistics missing");
    Serial.println(response);
    return false;
  }

  stats.subscribers = strtoull(subscriberText.c_str(), nullptr, 10);
  stats.views = strtoull(viewText.c_str(), nullptr, 10);
  stats.videos = strtoull(videoText.c_str(), nullptr, 10);
  if (!title.isEmpty()) stats.title = title;
  stats.valid = true;
  return true;
}

void refreshChannelStats() {
  lastFetchAttempt = millis();
  setDashboardStatus("Updating from YouTube...", COLOR_WARNING);
  String error;
  ChannelStats fresh = currentStats;
  if (fetchYouTubeStats(fresh, error)) {
    currentStats = fresh;
    nextFetchDelay = YOUTUBE_REFRESH_MS;
    dashboardStatus = formattedUpdateTimestamp();
    dashboardStatusColor = COLOR_GOOD;
    if (activePage == DisplayPage::SubscriberCounter) {
      drawDashboard(currentStats, dashboardStatus, dashboardStatusColor);
    }
    Serial.printf("@%s: %llu subscribers, %llu views, %llu videos\n",
                  youtubeHandle.c_str(),
                  static_cast<unsigned long long>(currentStats.subscribers),
                  static_cast<unsigned long long>(currentStats.views),
                  static_cast<unsigned long long>(currentStats.videos));
  } else {
    nextFetchDelay = ERROR_RETRY_MS;
    dashboardStatus = error + " - retrying";
    dashboardStatusColor = COLOR_WARNING;
    if (activePage == DisplayPage::SubscriberCounter) {
      if (currentStats.valid) {
        drawDashboard(currentStats, dashboardStatus, dashboardStatusColor);
      } else {
        drawServerErrorScreen("YOUTUBE UNREACHABLE", error);
      }
    }
    Serial.println(error);
  }
}

void updateDisplayRotation() {
  if (!clockReady) return;

  const uint32_t now = millis();
  if (now - pageChangedAt >= PAGE_ROTATION_MS) {
    showPage(activePage == DisplayPage::SubscriberCounter
                 ? DisplayPage::Clock
                 : DisplayPage::SubscriberCounter);
    return;
  }

  if (activePage == DisplayPage::Clock) {
    struct tm localTime;
    if (readLocalTime(localTime) && localTime.tm_min != lastClockMinute) {
      drawClockScreen(false);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // The RTC may retain a valid UTC epoch across a reset, but the process
  // timezone does not. Apply Eastern time before checking the retained clock.
  setenv("TZ", EASTERN_TIMEZONE, 1);
  tzset();
  delay(200);
  preferences.begin("espyt", true);
  screenRotated180 = preferences.getBool("rotate180", false);
  preferences.end();
  display.begin();
  display.setRotation180(screenRotated180);
  initializeTouch();
  drawSplash();

  pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);
  delay(100);
  if (digitalRead(PIN_BOOT_BUTTON) == LOW) {
    clearConfiguration();
    startSetupPortal("Saved setup cleared with BOOT button");
    return;
  }

  if (!loadConfiguration()) {
    startSetupPortal("Waiting for first-time setup");
    return;
  }

  if (!connectToConfiguredWifi()) {
    startSetupPortal("Saved Wi-Fi could not connect");
    return;
  }

  drawDashboard(currentStats, "Connected - loading channel", COLOR_WARNING);
  if (!synchronizeClock()) {
    return;
  }

  dashboardStatus = "Connected - loading channel";
  dashboardStatusColor = COLOR_WARNING;
  showPage(DisplayPage::SubscriberCounter);
  refreshChannelStats();
}

void loop() {
  // Keep touch diagnostics and rotation available even while the setup portal
  // is active. Page switching starts once network time is available.
  updateTouch();

  if (setupPortalActive) {
    dnsServer.processNextRequest();
    webServer.handleClient();
    if (restartRequested && millis() - restartRequestedAt > 1800) ESP.restart();
    delay(2);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    setDashboardStatus("Wi-Fi lost - reconnecting", COLOR_WARNING);
    WiFi.reconnect();
    delay(1000);
    if (WiFi.status() != WL_CONNECTED) {
      updateDisplayRotation();
      return;
    }
    setDashboardStatus("Wi-Fi restored", COLOR_GOOD);
  }

  if (!clockReady) {
    if (millis() - lastServerRetry >= ERROR_RETRY_MS && synchronizeClock()) {
      dashboardStatus = "Servers restored - loading channel";
      dashboardStatusColor = COLOR_GOOD;
      showPage(DisplayPage::SubscriberCounter);
      refreshChannelStats();
    }
    delay(50);
    return;
  }

  if (millis() - lastFetchAttempt >= nextFetchDelay) refreshChannelStats();
  updateDisplayRotation();
  delay(50);
}
