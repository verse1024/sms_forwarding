#include "globals.h"
#include "wifi_config.h"
#include "config.h"
#include "web_handlers.h"
#include "modem.h"
#include "push.h"
#include "sms_process.h"

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  // 缩短初始化延时，WiFi连接会处理自己的超时
  delay(200);
  Serial1.begin(115200, SERIAL_8N1, RXD, TXD);
  Serial1.setRxBufferSize(SERIAL_BUFFER_SIZE);
  while (Serial1.available()) Serial1.read();
  modemPowerCycle();
  while (Serial1.available()) Serial1.read();
  initConcatBuffer();
  loadConfig();
  configValid = isConfigValid();

  // ---- WiFi 连接优化 ----
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);                    // 关闭 Modem Sleep，提高连接响应速度
  WiFi.setAutoReconnect(true);             // 断线后自动重连
  WiFi.setScanMethod(WIFI_FAST_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

  bool wifiConnected = false;

  if (config.networkMode == NET_MODE_WIFI || config.networkMode == NET_MODE_WIFI_MOBILE) {
    // 尝试最多2次：第一次连接WiFi1→失败→连接WiFi2→仍失败→启动AP
    const int MAX_ATTEMPTS = 2;
    
    for (int attempt = 0; attempt < MAX_ATTEMPTS && !wifiConnected; attempt++) {
      // 优先尝试 WiFi 1
      if (!wifiConnected) {
        logCaptureLn(String("第 ") + String(attempt + 1) + String(" 次尝试，连接 WiFi 1: ") + config.wifiSSID1);
        WiFi.disconnect(false);
        delay(200);
        WiFi.begin(config.wifiSSID1.c_str(), config.wifiPass1.c_str());
        
        unsigned long wifiStart = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_CONNECT_TIMEOUT) {
          blink_short(200);
        }
        if (WiFi.status() == WL_CONNECTED) {
          wifiConnected = true;
          logCaptureLn(String("WiFi 1 已连接"));
        } else {
          logCaptureLn(String("⚠️ WiFi 1 连接失败"));
        }
      }
      
      // WiFi 1 失败，尝试 WiFi 2
      if (!wifiConnected) {
        if (config.wifiSSID2.length() > 0) {
          logCaptureLn(String("尝试连接 WiFi 2: ") + config.wifiSSID2);
          WiFi.disconnect(false);
          delay(200);
          WiFi.begin(config.wifiSSID2.c_str(), config.wifiPass2.c_str());
          
          unsigned long wifiStart = millis();
          while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_CONNECT_TIMEOUT) {
            blink_short(200);
          }
          if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            logCaptureLn(String("WiFi 2 已连接"));
          } else {
            logCaptureLn(String("⚠️ WiFi 2 连接失败"));
          }
        } else {
          logCaptureLn(String("⚠️ WiFi 2 未配置"));
        }
      }
    }
    
    // 两组WiFi和两次重试都失败了 → 启动 AP 供用户配置
    if (!wifiConnected) {
      logCaptureLn(String("❌ WiFi 全部连接失败，启动 AP 热点..."));
      WiFi.mode(WIFI_AP_STA);  // 先切换为同时支持 STA 和 AP
      delay(200);
      WiFi.softAP("SMS-FWD-Setup", "admin", 1, 0, 4);  // 1=隐藏SSID, 0=无加密, 4=最大1个客户端
      delay(500);
      logCaptureLn(String("AP 已启动: SMS-FWD-Setup / admin"));
      logCaptureLn(String("请连接到该 WiFi 后访问 http://192.168.4.1 进行配置"));
    }
  } else {
    // 流量模式：不连接 WiFi
    logCaptureLn(String("流量模式，跳过 WiFi 连接"));

    // 检查 SIM 卡是否存在
    if (!simDetected) {
      logCaptureLn(String("❌ 无SIM卡，启动 AP 热点供用户配置"));
      WiFi.mode(WIFI_AP);
      delay(200);
      WiFi.softAP("SMS-FWD-Setup", "admin");
      delay(500);
      logCaptureLn(String("AP 已启动: SMS-FWD-Setup / admin"));
      logCaptureLn(String("请连接到该 WiFi 后访问 http://192.168.4.1 进行配置"));
    }
  }

  if (wifiConnected) {
    logCapture(String("IP地址: "));
    logCaptureLn(WiFi.localIP().toString());
    logCapture(String("信号强度 (RSSI): "));
    logCaptureLn(String(WiFi.RSSI()) + " dBm");
  } else {
    // WiFi 没连上没关系，设备已启动 AP，用户可以通过 AP 访问后台配置
    if (WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
      logCaptureLn(String("AP 已就绪，请访问 http://") + WiFi.softAPIP().toString() + " 配置网络");
    } else {
      logCaptureLn(String("⚠️ WiFi 连接失败"));
    }
  }

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/tools", handleToolsPage);
  server.on("/sms", handleRoot);
  server.on("/sendsms", HTTP_POST, handleSendSms);
  server.on("/ping", HTTP_POST, handlePing);
  server.on("/query", handleQuery);
  server.on("/flight", handleFlightMode);
  server.on("/at", handleATCommand);
  server.on("/log", handleLog);
  server.on("/modem", handleModem);
  server.on("/wifi", handleWifi);
  server.begin();
  logCaptureLn(String("HTTP服务器已启动"));

  // ---- NTP 时间同步 ----
  logCaptureLn(String("正在同步NTP时间..."));
  configTime(0, 0, "ntp.ntsc.ac.cn", "ntp.aliyun.com", "pool.ntp.org");
  int ntpRetry = 0;
  while (time(nullptr) < 100000 && ntpRetry < 100) {
    delay(1);
    server.handleClient();
    ntpRetry++;
  }
  if (time(nullptr) >= 100000) {
    timeSynced = true;
    logCaptureLn(String("NTP时间同步成功"));
    time_t now = time(nullptr);
    logCapture(String("当前UTC时间戳: "));
    logCaptureLn(String(now));
  } else {
    logCaptureLn(String("NTP时间同步失败，将使用设备时间"));
  }

  ssl_client.setInsecure();
  digitalWrite(LED_BUILTIN, LOW);

  // ---- 启动通知（网页已可用，发邮件不会影响用户访问） ----
  if (configValid) {
    logCaptureLn(String("配置有效，发送启动通知..."));
    String subject = "短信转发器已启动";
    String body = "设备已启动\n设备地址: " + getDeviceUrl();
    sendEmailNotification(subject.c_str(), body.c_str());
  }

  // ---- 模组初始化（较慢，但网页已可访问） ----
  modemInit();
}

void loop() {
  server.handleClient();
  if (!configValid) {
    if (millis() - lastPrintTime >= 1000) {
      lastPrintTime = millis();
      logCaptureLn(String("⚠️ 请访问 " + getDeviceUrl() + " 配置系统参数"));
    }
  }
  checkConcatTimeout();
  if (Serial.available()) Serial1.write(Serial.read());
  checkSerial1URC();
}
