#include "push.h"
#include "web_handlers.h"
#include "config.h"
#include "modem.h"
#include <HTTPClient.h>
#include <mbedtls/md.h>
#include <base64.h>
#include <sys/time.h>

// 最大推送消息长度（字节），超过此长度将截断
#define MAX_PUSH_LENGTH 2000

// URL编码辅助函数
String urlEncode(const String& str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encoded += '+';
    } else if (isalnum(c)) {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}

// ---- JSON转义函数 ----
String jsonEscape(const String& str) {
  String result = "";
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (c == '"') result += "\\\"";
    else if (c == '\\') result += "\\\\";
    else if (c == '\n') result += "\\n";
    else if (c == '\r') result += "\\r";
    else if (c == '\t') result += "\\t";
    else result += c;
  }
  return result;
}

// ---- PDU时间戳转成yyyy-MM-dd HH:mm:ss格式 ----
// PDU时间格式: YYMMDDHHmmSSn (如 26072308511632 = 2026-07-23 08:51:16)
String formatPduTimestamp(const char* timestamp) {
  if (timestamp == NULL || strlen(timestamp) == 0) {
    // 如果没有时间戳，使用当前时间
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t epoch = tv.tv_sec;
    struct tm* t = gmtime(&epoch);
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    return String(buf);
  }

  String ts = String(timestamp);
  if (ts.length() >= 12) {
    int year = ts.substring(0, 2).toInt() + 2000;
    int month = ts.substring(2, 4).toInt();
    int day = ts.substring(4, 6).toInt();
    int hour = ts.substring(6, 8).toInt();
    int minute = ts.substring(8, 10).toInt();
    int second = ts.substring(10, 12).toInt();
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    return String(buf);
  }
  // 如果格式不对，返回原始值
  return ts;
}

// 截断过长的消息
String truncateMessage(const String& msg, int maxLen) {
  if (msg.length() <= maxLen) return msg;
  return msg.substring(0, maxLen) + " ... (已截断，超长省略)";
}

// ---- 钉钉加签 ----
String dingtalkSign(const String& secret, int64_t timestamp) {
  String stringToSign = String(timestamp) + "\n" + secret;
  
  uint8_t hmacResult[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret.c_str(), secret.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)stringToSign.c_str(), stringToSign.length());
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);
  
  String base64Encoded = base64::encode(hmacResult, 32);
  return urlEncode(base64Encoded);
}

// 获取当前UTC毫秒级时间戳（用于钉钉签名）
int64_t getUtcMillis() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) == 0) {
    return (int64_t)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
  }
  return (int64_t)time(nullptr) * 1000LL;
}

// 发送邮件通知函数
void sendEmailNotification(const char* subject, const char* body) {
  if (config.smtpServer.length() == 0 || config.smtpUser.length() == 0 || 
      config.smtpPass.length() == 0 || config.smtpSendTo.length() == 0) {
    logCaptureLn(String("邮件配置不完整，跳过发送"));
    return;
  }
  
  auto statusCallback = [](SMTPStatus status) {
    logCaptureLn(String(status.text));
  };
  smtp.connect(config.smtpServer.c_str(), config.smtpPort, statusCallback);
  if (smtp.isConnected()) {
    smtp.authenticate(config.smtpUser.c_str(), config.smtpPass.c_str(), readymail_auth_password);

    SMTPMessage msg;
    String from = "sms notify <"; from += config.smtpUser; from += ">";
    msg.headers.add(rfc822_from, from.c_str());
    String to = "your_email <"; to += config.smtpSendTo; to += ">";
    msg.headers.add(rfc822_to, to.c_str());
    msg.headers.add(rfc822_subject, subject);
    msg.text.body(body);
    msg.timestamp = time(nullptr);
    smtp.send(msg);
    logCaptureLn(String("邮件发送完成"));
  } else {
    logCaptureLn(String("邮件服务器连接失败"));
  }
}

// 发送单个推送通道（WiFi网络）
void sendToChannel(const PushChannel& channel, const char* sender, const char* message, const char* timestamp) {
  if (!channel.enabled) return;

  // 检查URL
  bool needUrl = (channel.type == PUSH_TYPE_POST_JSON || channel.type == PUSH_TYPE_BARK || 
                  channel.type == PUSH_TYPE_GET || channel.type == PUSH_TYPE_DINGTALK || 
                  channel.type == PUSH_TYPE_CUSTOM);
  if (needUrl && channel.url.length() == 0) return;
  
  HTTPClient http;
  String channelName = channel.name.length() > 0 ? channel.name : ("通道" + String(channel.type));
  logCaptureLn(String("发送到推送通道: " + channelName));
  
  int httpCode = 0;
  String senderEscaped = jsonEscape(String(sender));
  String messageEscaped = jsonEscape(String(message));
  
  // ---- 关键修复：格式化时间戳 ----
  String formattedTs = formatPduTimestamp(timestamp);
  String timestampEscaped = jsonEscape(formattedTs);

  // 截断过长消息
  String displayMsg = truncateMessage(message, MAX_PUSH_LENGTH - 100);

  switch (channel.type) {
    case PUSH_TYPE_POST_JSON: {
      http.begin(channel.url);
      http.addHeader("Content-Type", "application/json");
      String jsonData = "{";
      jsonData += "\"sender\":\"" + senderEscaped + "\",";
      jsonData += "\"message\":\"" + jsonEscape(displayMsg) + "\",";
      jsonData += "\"timestamp\":\"" + timestampEscaped + "\"";
      jsonData += "}";
      logCaptureLn(String("POST JSON: " + jsonData));
      httpCode = http.POST(jsonData);
      break;
    }
    
    case PUSH_TYPE_BARK: {
      http.begin(channel.url);
      http.addHeader("Content-Type", "application/json");
      String jsonData = "{";
      jsonData += "\"title\":\"" + senderEscaped + "\",";
      jsonData += "\"body\":\"" + jsonEscape(displayMsg) + "\"";
      jsonData += "}";
      logCaptureLn(String("BARK JSON: " + jsonData));
      httpCode = http.POST(jsonData);
      break;
    }
    
    case PUSH_TYPE_GET: {
      String getUrl = channel.url;
      if (getUrl.indexOf('?') == -1) {
        getUrl += "?";
      } else {
        getUrl += "&";
      }
      getUrl += "sender=" + urlEncode(String(sender));
      getUrl += "&message=" + urlEncode(displayMsg);
      getUrl += "&timestamp=" + urlEncode(formattedTs);
      logCaptureLn(String("GET URL: " + getUrl));
      http.begin(getUrl);
      httpCode = http.GET();
      break;
    }
    
    case PUSH_TYPE_DINGTALK: {
      String webhookUrl = channel.url;
      if (channel.key1.length() > 0) {
        int64_t ts = getUtcMillis();
        String sign = dingtalkSign(channel.key1, ts);
        if (webhookUrl.indexOf('?') == -1) {
          webhookUrl += "?";
        } else {
          webhookUrl += "&";
        }
        char tsBuf[21];
        snprintf(tsBuf, sizeof(tsBuf), "%lld", ts);
        webhookUrl += "timestamp=" + String(tsBuf) + "&sign=" + sign;
      }
      
      http.begin(webhookUrl);
      http.addHeader("Content-Type", "application/json");
      String jsonData = "{\"msgtype\":\"text\",\"text\":{\"content\":\"";
      jsonData += "📱短信通知\\n发送者: " + senderEscaped + "\\n内容: " + jsonEscape(displayMsg) + "\\n时间: " + timestampEscaped;
      jsonData += "\"}}";
      logCaptureLn(String("钉钉: " + jsonData));
      httpCode = http.POST(jsonData);
      break;
    }

    case PUSH_TYPE_PUSHPLUS: {
      String pushUrl = channel.url.length() > 0 ? channel.url : "http://www.pushplus.plus/send";
      http.begin(pushUrl);
      http.addHeader("Content-Type", "application/json");
      String channelValue = "wechat";
      if (channel.key2.length() > 0) {
          if (channel.key2 == "wechat" || channel.key2 == "extension" || channel.key2 == "app") {
              channelValue = channel.key2;
          } else {
              logCaptureLn(String("Invalid PushPlus channel '" + channel.key2 + "'. Using default 'wechat'."));
          }
      }
      String jsonData = "{";
      jsonData += "\"token\":\"" + channel.key1 + "\",";
      jsonData += "\"title\":\"短信来自: " + senderEscaped + "\",";
      jsonData += "\"content\":\"<b>发送者:</b> " + senderEscaped + "<br><b>时间:</b> " + timestampEscaped + "<br><b>内容:</b><br>" + jsonEscape(displayMsg) + "\",";
      jsonData += "\"channel\":\"" + channelValue + "\"";
      jsonData += "}";
      logCaptureLn(String("PushPlus: " + jsonData));
      httpCode = http.POST(jsonData);
      break;
    }

    case PUSH_TYPE_SERVERCHAN: {
      String scUrl = channel.url.length() > 0 ? channel.url : ("https://sctapi.ftqq.com/" + channel.key1 + ".send");
      http.begin(scUrl);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String postData = "title=" + urlEncode("短信来自: " + String(sender));
      postData += "&desp=" + urlEncode("**发送者:** " + String(sender) + "\n\n**时间:** " + formattedTs + "\n\n**内容:**\n\n" + displayMsg);
      logCaptureLn(String("Server酱: " + postData));
      httpCode = http.POST(postData);
      break;
    }
    
    case PUSH_TYPE_CUSTOM: {
      if (channel.customBody.length() == 0) {
        logCaptureLn(String("自定义模板为空，跳过"));
        return;
      }
      http.begin(channel.url);
      http.addHeader("Content-Type", "application/json");
      String body = channel.customBody;
      body.replace("{sender}", senderEscaped);
      body.replace("{message}", jsonEscape(displayMsg));
      body.replace("{timestamp}", timestampEscaped);
      logCaptureLn(String("自定义: " + body));
      httpCode = http.POST(body);
      break;
    }
    
    case PUSH_TYPE_FEISHU: {
      String webhookUrl = channel.url;
      String jsonData = "{";
      
      if (channel.key1.length() > 0) {
        int64_t ts = time(nullptr);
        String stringToSign = String(ts) + "\n";
        uint8_t hmacResult[32];
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
        mbedtls_md_hmac_starts(&ctx, (const unsigned char*)channel.key1.c_str(), channel.key1.length());
        mbedtls_md_hmac_update(&ctx, (const unsigned char*)stringToSign.c_str(), stringToSign.length());
        mbedtls_md_hmac_finish(&ctx, hmacResult);
        mbedtls_md_free(&ctx);
        String sign = base64::encode(hmacResult, 32);
        
        jsonData += "\"timestamp\":\"" + String(ts) + "\",";
        jsonData += "\"sign\":\"" + sign + "\",";
      }
      
      jsonData += "\"msg_type\":\"text\",";
      jsonData += "\"content\":{\"text\":\"";
      jsonData += "📱短信通知\\n发送者: " + senderEscaped + "\\n内容: " + jsonEscape(displayMsg) + "\\n时间: " + timestampEscaped;
      jsonData += "\"}}";
      
      http.begin(webhookUrl);
      http.addHeader("Content-Type", "application/json");
      logCaptureLn(String("飞书: " + jsonData));
      httpCode = http.POST(jsonData);
      break;
    }
    
    case PUSH_TYPE_GOTIFY: {
      String gotifyUrl = channel.url;
      if (!gotifyUrl.endsWith("/")) gotifyUrl += "/";
      gotifyUrl += "message?token=" + channel.key1;
      
      http.begin(gotifyUrl);
      http.addHeader("Content-Type", "application/json");
      String jsonData = "{";
      jsonData += "\"title\":\"短信来自: " + senderEscaped + "\",";
      jsonData += "\"message\":\"" + jsonEscape(displayMsg) + "\\n\\n时间: " + timestampEscaped + "\",";
      jsonData += "\"priority\":5";
      jsonData += "}";
      logCaptureLn(String("Gotify: " + jsonData));
      httpCode = http.POST(jsonData);
      break;
    }
    
    case PUSH_TYPE_TELEGRAM: {
      String tgBaseUrl = channel.url.length() > 0 ? channel.url : "https://api.telegram.org";
      if (tgBaseUrl.endsWith("/")) tgBaseUrl.remove(tgBaseUrl.length() - 1);
      
      String tgUrl = tgBaseUrl + "/bot" + channel.key2 + "/sendMessage";
      http.begin(tgUrl);
      http.addHeader("Content-Type", "application/json");
      
      String jsonData = "{";
      jsonData += "\"chat_id\":\"" + channel.key1 + "\",";
      String text = "📱短信通知\n发送者: " + senderEscaped + "\n内容: " + jsonEscape(displayMsg) + "\n时间: " + formattedTs;
      // Telegram有4096字符限制，需要再次截断
      if (text.length() > 4000) text = text.substring(0, 4000) + "...";
      jsonData += "\"text\":\"" + jsonEscape(text) + "\"";
      jsonData += "}";
      
      logCaptureLn(String("Telegram: " + jsonData));
      httpCode = http.POST(jsonData);
      break;
    }
    
    default:
      logCaptureLn(String("未知推送类型"));
      return;
  }
  
  if (httpCode > 0) {
    logCaptureF("[%s] 响应码: %d\n", channelName.c_str(), httpCode);
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      String response = http.getString();
      logCaptureLn(String("响应: " + response));
    }
  } else {
    logCaptureF("[%s] HTTP请求失败: %s\n", channelName.c_str(), http.errorToString(httpCode).c_str());
  }
  http.end();
}

// 发送短信到所有启用的推送通道（支持三种网络模式）
void sendSMSToServer(const char* sender, const char* message, const char* timestamp) {
  bool hasEnabledChannel = false;
  for (int i = 0; i < MAX_PUSH_CHANNELS; i++) {
    if (isPushChannelValid(config.pushChannels[i])) {
      hasEnabledChannel = true;
      break;
    }
  }
  
  if (!hasEnabledChannel) {
    logCaptureLn(String("没有启用的推送通道"));
    return;
  }
  
  logCaptureLn(String("\n=== 开始多通道推送 ==="));
  
  switch (config.networkMode) {
    case NET_MODE_WIFI: {
      // 仅WiFi模式：严格检查，必须连接到正确的WiFi
      if (WiFi.status() != WL_CONNECTED) {
        logCaptureLn(String("❌ WiFi未连接，跳过推送"));
        return;
      }
      // 额外验证：确保连接的是配置的WiFi之一
      String currentSSID = WiFi.SSID();
      if (currentSSID != config.wifiSSID1 && currentSSID != config.wifiSSID2) {
        logCaptureLn(String("❌ 当前连接的WiFi (") + currentSSID + ")不是配置的WiFi，跳过推送");
        return;
      }
      for (int i = 0; i < MAX_PUSH_CHANNELS; i++) {
        if (isPushChannelValid(config.pushChannels[i])) {
          sendToChannel(config.pushChannels[i], sender, message, timestamp);
          delay(100);
        }
      }
      break;
    }
    case NET_MODE_MOBILE: {
      // 仅移动数据模式
      if (!modemReady) {
        logCaptureLn(String("模组未就绪，跳过推送"));
        return;
      }
      if (!activateMobileData()) {
        logCaptureLn(String("移动数据激活失败，跳过推送"));
        return;
      }
      for (int i = 0; i < MAX_PUSH_CHANNELS; i++) {
        if (isPushChannelValid(config.pushChannels[i])) {
          sendToChannelMobile(config.pushChannels[i], sender, message, timestamp);
          delay(100);
        }
      }
      deactivateMobileData();
      break;
    }
    case NET_MODE_WIFI_MOBILE: {
      // WiFi+移动数据模式：按优先级依次尝试
      bool pushedViaWifi = false;
      
      // 第1步：尝试WiFi 1
      if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == config.wifiSSID1) {
        logCaptureLn(String("使用WiFi 1推送: ") + WiFi.SSID());
        for (int i = 0; i < MAX_PUSH_CHANNELS; i++) {
          if (isPushChannelValid(config.pushChannels[i])) {
            sendToChannel(config.pushChannels[i], sender, message, timestamp);
            delay(100);
          }
        }
        pushedViaWifi = true;
      }
      
      // 第2步：如果WiFi 1没连上，尝试WiFi 2
      if (!pushedViaWifi && WiFi.status() == WL_CONNECTED && WiFi.SSID() == config.wifiSSID2 && config.wifiSSID2.length() > 0) {
        logCaptureLn(String("使用WiFi 2推送: ") + WiFi.SSID());
        for (int i = 0; i < MAX_PUSH_CHANNELS; i++) {
          if (isPushChannelValid(config.pushChannels[i])) {
            sendToChannel(config.pushChannels[i], sender, message, timestamp);
            delay(100);
          }
        }
        pushedViaWifi = true;
      }
      
      // 第3步：如果两组WiFi都没连上，走流量
      if (!pushedViaWifi) {
        logCaptureLn(String("WiFi均未连接，回退到移动数据推送"));
        if (!modemReady) {
          logCaptureLn(String("模组未就绪，跳过推送"));
          return;
        }
        if (!activateMobileData()) {
          logCaptureLn(String("移动数据激活失败，跳过推送"));
          return;
        }
        for (int i = 0; i < MAX_PUSH_CHANNELS; i++) {
          if (isPushChannelValid(config.pushChannels[i])) {
            sendToChannelMobile(config.pushChannels[i], sender, message, timestamp);
            delay(100);
          }
        }
        deactivateMobileData();
      }
      break;
    }
  }
  
  logCaptureLn(String("=== 多通道推送完成 ===\n"));
}

// 通过模组AT HTTP命令发送推送（移动数据模式）
void sendToChannelMobile(const PushChannel& channel, const char* sender, const char* message, const char* timestamp) {
  if (!channel.enabled) return;
  
  bool needUrl = (channel.type == PUSH_TYPE_POST_JSON || channel.type == PUSH_TYPE_BARK || 
                  channel.type == PUSH_TYPE_GET || channel.type == PUSH_TYPE_DINGTALK || 
                  channel.type == PUSH_TYPE_CUSTOM);
  if (needUrl && channel.url.length() == 0) return;
  
  String channelName = channel.name.length() > 0 ? channel.name : ("通道" + String(channel.type));
  logCaptureLn(String("[移动数据] 发送到推送通道: " + channelName));
  
  String senderEscaped = jsonEscape(String(sender));
  String messageEscaped = jsonEscape(message);
  
  // ---- 格式化时间戳 ----
  String formattedTs = formatPduTimestamp(timestamp);
  String timestampEscaped = jsonEscape(formattedTs);

  // 截断过长消息
  String displayMsg = truncateMessage(message, MAX_PUSH_LENGTH - 100);

  String reqUrl = "";
  String reqBody = "";
  String contentType = "";
  bool isGet = false;
  
  switch (channel.type) {
    case PUSH_TYPE_POST_JSON: {
      reqUrl = channel.url;
      contentType = "application/json";
      reqBody = "{\"sender\":\"" + senderEscaped + "\",\"message\":\"" + jsonEscape(displayMsg) + "\",\"timestamp\":\"" + timestampEscaped + "\"}";
      break;
    }
    case PUSH_TYPE_BARK: {
      reqUrl = channel.url;
      contentType = "application/json";
      reqBody = "{\"title\":\"" + senderEscaped + "\",\"body\":\"" + jsonEscape(displayMsg) + "\"}";
      break;
    }
    case PUSH_TYPE_GET: {
      reqUrl = channel.url;
      if (reqUrl.indexOf('?') == -1) reqUrl += "?";
      else reqUrl += "&";
      reqUrl += "sender=" + urlEncode(String(sender));
      reqUrl += "&message=" + urlEncode(displayMsg);
      reqUrl += "&timestamp=" + urlEncode(formattedTs);
      isGet = true;
      break;
    }
    case PUSH_TYPE_DINGTALK: {
      reqUrl = channel.url;
      if (channel.key1.length() > 0) {
        int64_t ts = getUtcMillis();
        String sign = dingtalkSign(channel.key1, ts);
        if (reqUrl.indexOf('?') == -1) reqUrl += "?";
        else reqUrl += "&";
        char tsBuf[21];
        snprintf(tsBuf, sizeof(tsBuf), "%lld", ts);
        reqUrl += "timestamp=" + String(tsBuf) + "&sign=" + sign;
      }
      contentType = "application/json";
      reqBody = "{\"msgtype\":\"text\",\"text\":{\"content\":\"📱短信通知\\n发送者: " + senderEscaped + "\\n内容: " + jsonEscape(displayMsg) + "\\n时间: " + timestampEscaped + "\"}}";
      break;
    }
    case PUSH_TYPE_PUSHPLUS: {
      reqUrl = channel.url.length() > 0 ? channel.url : "http://www.pushplus.plus/send";
      String channelValue = "wechat";
      if (channel.key2.length() > 0) {
        if (channel.key2 == "wechat" || channel.key2 == "extension" || channel.key2 == "app") {
          channelValue = channel.key2;
        }
      }
      contentType = "application/json";
      reqBody = "{\"token\":\"" + channel.key1 + "\",\"title\":\"短信来自: " + senderEscaped + "\",\"content\":\"<b>发送者:</b> " + senderEscaped + "<br><b>时间:</b> " + timestampEscaped + "<br><b>内容:</b><br>" + jsonEscape(displayMsg) + "\",\"channel\":\"" + channelValue + "\"}";
      break;
    }
    case PUSH_TYPE_SERVERCHAN: {
      reqUrl = channel.url.length() > 0 ? channel.url : ("https://sctapi.ftqq.com/" + channel.key1 + ".send");
      contentType = "application/x-www-form-urlencoded";
      reqBody = "title=" + urlEncode("短信来自: " + String(sender));
      reqBody += "&desp=" + urlEncode("**发送者:** " + String(sender) + "\n\n**时间:** " + formattedTs + "\n\n**内容:**\n\n" + displayMsg);
      break;
    }
    case PUSH_TYPE_CUSTOM: {
      if (channel.customBody.length() == 0) {
        logCaptureLn(String("[移动数据] 自定义模板为空，跳过"));
        return;
      }
      reqUrl = channel.url;
      contentType = "application/json";
      reqBody = channel.customBody;
      reqBody.replace("{sender}", senderEscaped);
      reqBody.replace("{message}", jsonEscape(displayMsg));
      reqBody.replace("{timestamp}", timestampEscaped);
      break;
    }
    case PUSH_TYPE_FEISHU: {
      reqUrl = channel.url;
      contentType = "application/json";
      String jsonData = "{";
      if (channel.key1.length() > 0) {
        int64_t ts = time(nullptr);
        String stringToSign = String(ts) + "\n";
        uint8_t hmacResult[32];
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
        mbedtls_md_hmac_starts(&ctx, (const unsigned char*)channel.key1.c_str(), channel.key1.length());
        mbedtls_md_hmac_update(&ctx, (const unsigned char*)stringToSign.c_str(), stringToSign.length());
        mbedtls_md_hmac_finish(&ctx, hmacResult);
        mbedtls_md_free(&ctx);
        String sign = base64::encode(hmacResult, 32);
        jsonData += "\"timestamp\":\"" + String(ts) + "\",\"sign\":\"" + sign + "\",";
      }
      jsonData += "\"msg_type\":\"text\",\"content\":{\"text\":\"📱短信通知\\n发送者: " + senderEscaped + "\\n内容: " + jsonEscape(displayMsg) + "\\n时间: " + timestampEscaped + "\"}}";
      reqBody = jsonData;
      break;
    }
    case PUSH_TYPE_GOTIFY: {
      reqUrl = channel.url;
      if (!reqUrl.endsWith("/")) reqUrl += "/";
      reqUrl += "message?token=" + channel.key1;
      contentType = "application/json";
      reqBody = "{\"title\":\"短信来自: " + senderEscaped + ",\"message\":\"" + jsonEscape(displayMsg) + "\\n\\n时间: " + timestampEscaped + ",\"priority\":5}";
      break;
    }
    case PUSH_TYPE_TELEGRAM: {
      String tgBaseUrl = channel.url.length() > 0 ? channel.url : "https://api.telegram.org";
      if (tgBaseUrl.endsWith("/")) tgBaseUrl.remove(tgBaseUrl.length() - 1);
      reqUrl = tgBaseUrl + "/bot" + channel.key2 + "/sendMessage";
      contentType = "application/json";
      String text = "\u{1F4F1}\u77ED\u4FE1\u901A\u77E5\n\u53D1\u9001\u8005: " + senderEscaped + "\n\u5185\u5BB9: " + jsonEscape(displayMsg) + "\n\u65F6\u95F4: " + formattedTs;
      if (text.length() > 4000) text = text.substring(0, 4000) + "...";
      reqBody = "{\"chat_id\":\"" + channel.key1 + "\",\"text\":\"" + jsonEscape(text) + "\"}";
      break;
    }
    default:
      logCaptureLn(String("[移动数据] 未知推送类型"));
      return;
  }
  
  // 设置URL（注意：AT指令中URL值不需要末尾分号）
  String setUrlCmd = "AT+HTTPPARA=\"URL\",\"" + reqUrl + "\"";
  String resp = sendATCommand(setUrlCmd.c_str(), 5000);
  if (resp.indexOf("OK") < 0) {
    logCaptureLn(String("[移动数据] 设置URL失败: " + resp));
    return;
  }
  
  // 设置Content-Type
  String setCtCmd = "AT+HTTPPARA=\"CONTENT\",\"" + contentType + "\"";
  resp = sendATCommand(setCtCmd.c_str(), 3000);
  if (resp.indexOf("OK") < 0) {
    logCaptureLn(String("[移动数据] 设置Content-Type失败: " + resp));
    return;
  }
  
  bool success = false;
  
  if (isGet) {
    // GET请求
    resp = sendATCommand("AT+HTTPACTION=0", 30000);
    if (resp.indexOf("+HTTPACTION:") >= 0) {
      int codeIdx = resp.indexOf(",");
      if (codeIdx >= 0) {
        int nextComma = resp.indexOf(",", codeIdx + 1);
        if (nextComma >= 0) {
          int httpCode = resp.substring(codeIdx + 1, nextComma).toInt();
          logCaptureF("[移动数据][%s] 响应码: %d\n", channelName.c_str(), httpCode);
          success = (httpCode >= 200 && httpCode < 300);
        }
      }
    } else {
      logCaptureLn(String("[移动数据] HTTP GET请求失败: " + resp));
    }
  } else {
    // POST请求：发送数据
    String dataCmd = "AT+HTTPDATA=" + String(reqBody.length()) + ",10000";
    resp = sendATCommand(dataCmd.c_str(), 3000);
    if (resp.indexOf(">") < 0) {
      if (resp.indexOf("DOWNLOAD") < 0) {
        logCaptureLn(String("[移动数据] HTTPDATA准备失败: " + resp));
        return;
      }
    }
    // 发送数据体
    delay(100);
    while (Serial1.available()) Serial1.read();
    Serial1.print(reqBody);
    
    // 等待发送完成
    unsigned long start = millis();
    bool dataSent = false;
    while (millis() - start < 15000) {
      if (Serial1.available()) {
        String r = Serial1.readStringUntil('\n');
        if (r.indexOf("OK") >= 0) { dataSent = true; break; }
      }
      server.handleClient();
    }
    if (!dataSent) {
      logCaptureLn(String("[移动数据] 数据发送超时"));
      return;
    }
    
    // 执行POST
    resp = sendATCommand("AT+HTTPACTION=1", 30000);
    if (resp.indexOf("+HTTPACTION:") >= 0) {
      int codeIdx = resp.indexOf(",");
      if (codeIdx >= 0) {
        int nextComma = resp.indexOf(",", codeIdx + 1);
        if (nextComma >= 0) {
          int httpCode = resp.substring(codeIdx + 1, nextComma).toInt();
          logCaptureF("[移动数据][%s] 响应码: %d\n", channelName.c_str(), httpCode);
          success = (httpCode >= 200 && httpCode < 300);
        }
      }
    } else {
      logCaptureLn(String("[移动数据] HTTP POST请求失败: " + resp));
    }
  }
  
  if (success) {
    logCaptureLn(String("[移动数据][" + channelName + "] 推送成功"));
  } else {
    logCaptureLn(String("[移动数据][" + channelName + "] 推送失败"));
  }
}
