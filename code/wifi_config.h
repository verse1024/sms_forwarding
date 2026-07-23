// WiFi配置 - 支持2组WiFi，优先使用第一组，第一组连不上使用第二组
// 如果两组都连不上，ESP32会启动 AP 热点 SMS-FWD-Setup (密码: admin) 供用户配置

// WiFi 1 (主用)
#define WIFI_SSID_1 "MiFi-1701"
#define WIFI_PASS_1 "1234567890"

// WiFi 2 (备用)
#define WIFI_SSID_2 "SSID2"
#define WIFI_PASS_2 "Password"

// WiFi连接超时时间(毫秒)
#define WIFI_CONNECT_TIMEOUT 15000

// WiFi重连等待时间(毫秒)
#define WIFI_RECONNECT_DELAY 500