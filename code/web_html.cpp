#include "config_types.h"

// ===== 配置页面 =====
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>短信转发配置</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; min-height: 100vh; background: #f5f7fa; color: #333; }
    .header { text-align: center; padding: 24px 16px; background: linear-gradient(135deg, #4CAF50, #388E3C); color: white; }
    .header h1 { font-size: 26px; font-weight: 600; }
    .header p { font-size: 13px; opacity: 0.9; margin-top: 4px; }
    .container { max-width: 640px; margin: 0 auto; padding: 12px; }
    /* Nav tabs */
    .nav { display: flex; gap: 0; margin-bottom: 16px; background: white; border-radius: 8px; overflow: hidden; box-shadow: 0 1px 4px rgba(0,0,0,0.08); }
    .nav a { flex: 1; text-align: center; padding: 12px; text-decoration: none; color: #666; font-size: 15px; font-weight: 500; transition: all 0.2s; }
    .nav a.active { background: #4CAF50; color: white; }
    .nav a:not(.active):hover { background: #f5f5f5; }
    /* Section cards */
    .card { background: white; border-radius: 8px; padding: 16px; margin-bottom: 12px; box-shadow: 0 1px 3px rgba(0,0,0,0.06); }
    .card-title { font-size: 15px; font-weight: 600; color: #333; margin-bottom: 12px; padding-left: 24px; position: relative; }
    .card-title::before { position: absolute; left: 0; font-size: 16px; }
    .card-title.icon-ip::before { content: "🌐"; }
    .card-title.icon-mode::before { content: "📶"; }
    .card-title.icon-wifi::before { content: "📡"; }
    .card-title.icon-account::before { content: "🔐"; }
    .card-title.icon-email::before { content: "📧"; }
    .card-title.icon-push::before { content: "🔗"; }
    .card-title.icon-admin::before { content: "👤"; }
    .card-title.icon-blacklist::before { content: "🚫"; }
    /* Status bar */
    .status-bar { background: #e3f2fd; border-left: 3px solid #2196F3; padding: 10px 12px; border-radius: 4px; margin-bottom: 12px; font-size: 14px; color: #1565c0; }
    /* Radio group */
    .radio-group { margin-bottom: 12px; }
    .radio-item { display: flex; align-items: center; gap: 10px; padding: 10px 14px; border: 2px solid #e0e0e0; border-radius: 8px; margin-bottom: 8px; cursor: pointer; transition: all 0.2s; }
    .radio-item:hover { border-color: #bdbdbd; }
    .radio-item.selected { border-color: #4CAF50; background: #f1f8e9; }
    .radio-item input[type="radio"] { width: 18px; height: 18px; accent-color: #4CAF50; flex-shrink: 0; }
    .radio-item span { font-size: 15px; font-weight: 500; }
    .net-desc { padding: 10px; background: #fff8e1; border-left: 3px solid #ffc107; border-radius: 4px; font-size: 13px; color: #5d4037; line-height: 1.6; margin-bottom: 10px; }
    /* Form */
    .form-group { margin-bottom: 12px; }
    .form-group:last-child { margin-bottom: 0; }
    label { display: block; margin-bottom: 5px; font-size: 14px; font-weight: 500; color: #555; }
    input[type="text"], input[type="password"], input[type="number"], textarea, select { width: 100%; padding: 9px 11px; border: 1px solid #ddd; border-radius: 6px; font-size: 14px; transition: border-color 0.2s; }
    input:focus, textarea:focus, select:focus { outline: none; border-color: #4CAF50; box-shadow: 0 0 0 2px rgba(76,175,80,0.12); }
    textarea { resize: vertical; min-height: 50px; font-family: monospace; }
    button { padding: 10px 20px; background: #4CAF50; color: white; border: none; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: 500; transition: background 0.2s; }
    button:hover { background: #388E3C; }
    .btn-blue { background: #2196F3; }
    .btn-blue:hover { background: #1976D2; }
    .warning { padding: 10px; background: #fff3cd; border-left: 3px solid #ffc107; border-radius: 4px; font-size: 13px; color: #856404; margin-bottom: 12px; }
    .hint { font-size: 13px; color: #888; line-height: 1.5; }
    /* Push channel */
    .push-channel { border: 1px solid #e0e0e0; padding: 14px; margin-bottom: 10px; border-radius: 6px; background: #fafafa; }
    .push-channel.enabled { background: #f1f8f1; border-color: #4CAF50; }
    .push-channel-header { display: flex; align-items: center; margin-bottom: 10px; }
    .push-channel-header input[type="checkbox"] { width: 18px; height: 18px; accent-color: #ffffffff; }
    .push-channel-header label { margin: 0; margin-left: 8px; font-size: 14px; font-weight: 500; cursor: pointer; }
    .push-channel-body { display: none; }
    .push-channel.enabled .push-channel-body { display: block; }
    .push-type-hint { font-size: 12px; color: #666; margin-top: 4px; padding: 6px 8px; background: #f5f5f5; border-radius: 4px; }
    .label-inline { display: inline; font-weight: normal; }
    /* Submit */
    .submit-btn { width: 100%; padding: 12px; font-size: 16px; margin-top: 8px; }
    @media(max-width:480px) {
      .container { padding: 8px; }
      .card { padding: 12px; }
    }
  </style>
</head>
<body>
  <div class="header">
    <h1>📱 短信转发器</h1>
  </div>
  <div class="container">
    <div class="nav">
      <a href="/" class="active">⚙️ 系统配置</a>
      <a href="/tools">🧰 工具箱</a>
    </div>

    <form action="/save" method="POST">

      <!-- 设备状态 -->
      <div class="status-bar">网络模式: %NET_MODE_TEXT% | SIM卡: %SIM_STATUS%</div>

      <!-- 网络模式 -->
      <div class="card">
        <div class="card-title icon-mode">网络模式</div>
        <div class="radio-group" id="netModeGroup">
          <label class="radio-item %NET_MODE_WIFI_SEL%">
            <input type="radio" name="networkMode" value="0" onchange="updateNetDesc()" %NET_MODE_WIFI_CHECK%>
            <span>📶 WiFi 模式</span>
          </label>
          <label class="radio-item %NET_MODE_MOBILE_SEL%">
            <input type="radio" name="networkMode" value="1" onchange="updateNetDesc()" %NET_MODE_MOBILE_CHECK%>
            <span>📱 流量模式</span>
          </label>
          <label class="radio-item %NET_MODE_WIFI_MOBILE_SEL%">
            <input type="radio" name="networkMode" value="2" onchange="updateNetDesc()" %NET_MODE_WIFI_MOBILE_CHECK%>
            <span>🔀 WiFi + 流量模式</span>
          </label>
        </div>
        <div class="net-desc" id="netModeDesc">%NET_MODE_DESC%</div>
        <button type="submit" class="btn-blue" style="width:auto;">💾 保存</button>
      </div>

      <!-- WiFi设置 -->
      <div class="card" id="wifiSection">
        <div class="card-title icon-wifi">WiFi 设置</div>
        <div class="form-group">
          <label>WiFi 1 - 名称 (SSID)</label>
          <input type="text" name="wifiSSID1" value="%WIFI_SSID1%" placeholder="主用WiFi名称">
        </div>
        <div class="form-group">
          <label>WiFi 1 - 密码</label>
          <input type="password" name="wifiPass1" value="%WIFI_PASS1%" placeholder="主用WiFi密码">
        </div>
        <hr style="border:none;border-top:1px solid #eee;margin:12px 0;">
        <div class="form-group">
          <label>WiFi 2 - 名称 (SSID)</label>
          <input type="text" name="wifiSSID2" value="%WIFI_SSID2%" placeholder="备用WiFi名称（留空则关闭）">
        </div>
        <div class="form-group">
          <label>WiFi 2 - 密码</label>
          <input type="password" name="wifiPass2" value="%WIFI_PASS2%" placeholder="备用WiFi密码">
        </div>
        <div class="hint" style="margin-top:8px;">• WiFi模式/WiFi+流量模式：优先连接WiFi 1，失败后自动切换WiFi 2<br>• 流量模式：不连接WiFi，直接使用模组流量数据。注意：切换网络模式或保存WiFi设置后设备将自动重启。</div>
      </div>

      <!-- Web管理账号 -->
      <div class="card">
        <div class="card-title icon-account">Web管理账号设置</div>
        <div class="warning">⚠️ 首次使用请修改默认密码！默认账号: admin，默认密码: admin123</div>
        <div class="form-group">
          <label>管理账号</label>
          <input type="text" name="webUser" value="%WEB_USER%" placeholder="admin">
        </div>
        <div class="form-group">
          <label>管理密码</label>
          <input type="password" name="webPass" value="%WEB_PASS%" placeholder="请设置复杂密码">
        </div>
      </div>

      <!-- 邮件通知 -->
      <div class="card">
        <div class="card-title icon-email">邮件通知设置</div>
        <div class="form-group">
          <label>SMTP服务器</label>
          <input type="text" name="smtpServer" value="%SMTP_SERVER%" placeholder="smtp.qq.com">
        </div>
        <div class="form-group">
          <label>SMTP端口</label>
          <input type="number" name="smtpPort" value="%SMTP_PORT%" placeholder="465">
        </div>
        <div class="form-group">
          <label>邮箱账号</label>
          <input type="text" name="smtpUser" value="%SMTP_USER%" placeholder="your@qq.com">
        </div>
        <div class="form-group">
          <label>邮箱密码/授权码</label>
          <input type="password" name="smtpPass" value="%SMTP_PASS%" placeholder="授权码">
        </div>
        <div class="form-group">
          <label>接收邮件地址</label>
          <input type="text" name="smtpSendTo" value="%SMTP_SEND_TO%" placeholder="receiver@example.com">
        </div>
      </div>

      <!-- HTTP推送通道 -->
      <div class="card">
        <div class="card-title icon-push">HTTP推送通道设置</div>
        <div class="hint" style="margin-bottom:12px;">可同时启用多个推送通道，每个通道独立配置。支持POST JSON、Bark、GET、钉钉、PushPlus、Server酱、飞书、Gotify、Telegram等。</div>
        %PUSH_CHANNELS%
      </div>

      <!-- 管理员设置 -->
      <div class="card">
        <div class="card-title icon-admin">管理员设置</div>
        <div class="form-group">
          <label>管理员手机号</label>
          <input type="text" name="adminPhone" value="%ADMIN_PHONE%" placeholder="13800138000">
        </div>
        <div class="hint">管理员可通过短信发送以下命令控制设备：<br>
          • <code>SMS:号码:内容</code> - 远程发送短信<br>
          • <code>RESET</code> - 重启设备
        </div>
      </div>

      <!-- 黑名单 -->
      <div class="card">
        <div class="card-title icon-blacklist">号码黑名单</div>
        <div class="form-group">
          <label>黑名单号码（换行分隔）</label>
          <textarea name="numberBlackList" rows="3" placeholder="每行一个号码">%NUMBER_BLACK_LIST%</textarea>
        </div>
      </div>

      <button type="submit" class="submit-btn">💾 保存全部配置</button>
    </form>
  </div>
  <script>
    var netDescs = [
      '仅通过 WiFi 连接转发短信。设备启动时必须连接 WiFi，若 WiFi 超时将自动重启重试。',
      '仅通过模组流量数据连接转发短信。设备启动时不连接 WiFi，会创建 AP 热点供配置。收到短信时自动激活数据连接进行推送。',
      'WiFi 优先，WiFi 不可用时自动回退到流量。设备启动时先尝试连接 WiFi；若 WiFi 连接超时，则启动 AP 热点并使用流量回退。'
    ];
    function updateNetDesc() {
      var sel = document.querySelector('input[name="networkMode"]:checked');
      var desc = document.getElementById('netModeDesc');
      var items = document.querySelectorAll('.radio-item');
      items.forEach(function(item) { item.classList.remove('selected'); });
      if (sel && sel.value !== undefined) {
        desc.textContent = netDescs[sel.value] || '';
        sel.closest('.radio-item').classList.add('selected');
      }
      // 显示/隐藏WiFi设置区域
      var wifiSec = document.getElementById('wifiSection');
      if (!sel || sel.value == '1') {
        wifiSec.style.display = 'none';
      } else {
        wifiSec.style.display = 'block';
      }
    }
    document.addEventListener('DOMContentLoaded', updateNetDesc);

    // ---- Push Channel JS ----
    function toggleChannel(idx) {
      var ch = document.getElementById('channel' + idx);
      var cb = document.getElementById('push' + idx + 'en');
      if (cb.checked) { ch.classList.add('enabled'); } else { ch.classList.remove('enabled'); }
    }
    function updateTypeHint(idx) {
      var sel = document.getElementById('push' + idx + 'type');
      var hint = document.getElementById('hint' + idx);
      var extraFields = document.getElementById('extra' + idx);
      var customFields = document.getElementById('custom' + idx);
      var type = parseInt(sel.value);
      extraFields.style.display = 'none'; customFields.style.display = 'none';
      document.getElementById('key1label' + idx).innerText = '参数1';
      document.getElementById('key2label' + idx).innerText = '参数2';
      document.getElementById('key1' + idx).placeholder = '';
      document.getElementById('key2' + idx).placeholder = '';
      document.getElementById('key2' + idx).closest('.form-group').style.display = 'none';
      if (type == 1) { hint.innerHTML = '<b>POST JSON格式：</b><br>{"sender":"发送者号码","message":"短信内容","timestamp":"时间戳"}'; }
      else if (type == 2) { hint.innerHTML = '<b>Bark格式：</b><br>POST {"title":"发送者号码","body":"短信内容"}'; }
      else if (type == 3) { hint.innerHTML = '<b>GET请求格式：</b><br>URL?sender=xxx&message=xxx&timestamp=xxx'; }
      else if (type == 4) { hint.innerHTML = '<b>钉钉机器人：</b><br>填写Webhook地址，如需加签请填Secret'; extraFields.style.display = 'block'; document.getElementById('key1label' + idx).innerText = 'Secret（加签密钥，可选）'; document.getElementById('key1' + idx).placeholder = 'SEC...'; }
      else if (type == 5) { hint.innerHTML = '<b>PushPlus：</b><br>填写Token，URL留空使用默认'; extraFields.style.display = 'block'; document.getElementById('key1label' + idx).innerText = 'Token'; document.getElementById('key1' + idx).placeholder = 'pushplus的token'; document.getElementById('key2' + idx).closest('.form-group').style.display = 'block'; document.getElementById('key2label' + idx).innerText = '发送渠道'; document.getElementById('key2' + idx).placeholder = 'wechat(default), extension, app'; }
      else if (type == 6) { hint.innerHTML = '<b>Server酱：</b><br>填写SendKey，URL留空使用默认'; extraFields.style.display = 'block'; document.getElementById('key1label' + idx).innerText = 'SendKey'; document.getElementById('key1' + idx).placeholder = 'SCT...'; }
      else if (type == 7) { hint.innerHTML = '<b>自定义模板：</b><br>在请求体模板中使用 {sender} {message} {timestamp} 作为占位符'; customFields.style.display = 'block'; }
      else if (type == 8) { hint.innerHTML = '<b>飞书机器人：</b><br>填写Webhook地址，如需签名验证请填Secret'; extraFields.style.display = 'block'; document.getElementById('key1label' + idx).innerText = 'Secret（签名密钥，可选）'; document.getElementById('key1' + idx).placeholder = '飞书机器人的签名密钥'; }
      else if (type == 9) { hint.innerHTML = '<b>Gotify：</b><br>填写服务器地址（如 http://gotify.example.com），Token填写应用Token'; extraFields.style.display = 'block'; document.getElementById('key1label' + idx).innerText = 'Token（应用Token）'; document.getElementById('key1' + idx).placeholder = 'A...'; }
      else if (type == 10) { hint.innerHTML = '<b>Telegram Bot：</b><br>填写Chat ID（参数1）和Bot Token（参数2），URL留空默认使用官方API'; extraFields.style.display = 'block'; document.getElementById('key1label' + idx).innerText = 'Chat ID'; document.getElementById('key1' + idx).placeholder = '123456789'; document.getElementById('key2label' + idx).innerText = 'Bot Token'; document.getElementById('key2' + idx).placeholder = '12345678:ABC...'; }
    }
    document.addEventListener('DOMContentLoaded', function() {
      for (var i = 0; i < 5; i++) { toggleChannel(i); updateTypeHint(i); }
    });
  </script>
</body>
</html>
)rawliteral";

// ===== 工具箱页面 =====
const char* toolsPage = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>工具箱 - 短信转发器</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; min-height: 100vh; background: #f5f7fa; color: #333; }
    .header { text-align: center; padding: 24px 16px; background: linear-gradient(135deg, #4CAF50, #388E3C); color: white; }
    .header h1 { font-size: 26px; font-weight: 600; }
    .container { max-width: 640px; margin: 0 auto; padding: 12px; }
    .nav { display: flex; gap: 0; margin-bottom: 16px; background: white; border-radius: 8px; overflow: hidden; box-shadow: 0 1px 4px rgba(0,0,0,0.08); }
    .nav a { flex: 1; text-align: center; padding: 12px; text-decoration: none; color: #666; font-size: 15px; font-weight: 500; }
    .nav a.active { background: #4CAF50; color: white; }
    .nav a:not(.active):hover { background: #f5f5f5; }
    .card { background: white; border-radius: 8px; padding: 16px; margin-bottom: 12px; box-shadow: 0 1px 3px rgba(0,0,0,0.06); }
    .card-title { font-size: 15px; font-weight: 600; color: #333; margin-bottom: 12px; padding-left: 24px; position: relative; }
    .card-title.icon-sms::before { content: "📤"; position: absolute; left: 0; }
    .card-title.icon-info::before { content: "📊"; position: absolute; left: 0; }
    .card-title.icon-net::before { content: "🌐"; position: absolute; left: 0; }
    .card-title.icon-modem::before { content: "🔄"; position: absolute; left: 0; }
    .card-title.icon-flight::before { content: "✈️"; position: absolute; left: 0; }
    .card-title.icon-at::before { content: "💻"; position: absolute; left: 0; }
    .card-title.icon-log::before { content: "📋"; position: absolute; left: 0; }
    .btn-group { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 12px; }
    .btn-group button { flex: 1; min-width: 100px; padding: 9px 12px; font-size: 13px; }
    .btn-blue { background: #2196F3; }
    .btn-blue:hover { background: #1976D2; }
    .btn-orange { background: #FF9800; }
    .btn-orange:hover { background: #F57C00; }
    .btn-purple { background: #9C27B0; }
    .btn-purple:hover { background: #7B1FA2; }
    .btn-red { background: #f44336; }
    .btn-red:hover { background: #d32f2f; }
    .btn-gray { background: #607D8B; }
    .btn-gray:hover { background: #455A64; }
    .hint { font-size: 13px; color: #888; margin-bottom: 12px; }
    .result-box { margin-top: 12px; padding: 10px; border-radius: 6px; display: none; font-size: 14px; }
    .result-success { background: #e8f5e9; border-left: 3px solid #4CAF50; color: #2e7d32; }
    .result-error { background: #ffebee; border-left: 3px solid #f44336; color: #c62828; }
    .result-loading { background: #fff3e0; border-left: 3px solid #FF9800; color: #e65100; }
    .result-info { background: #e3f2fd; border-left: 3px solid #2196F3; color: #1565c0; }
    form .form-group { margin-bottom: 12px; }
    label { display: block; margin-bottom: 5px; font-size: 14px; font-weight: 500; color: #555; }
    input[type="text"], textarea { width: 100%; padding: 9px 11px; border: 1px solid #ddd; border-radius: 6px; font-size: 14px; }
    textarea { resize: vertical; min-height: 70px; font-family: monospace; }
    input:focus, textarea:focus { outline: none; border-color: #4CAF50; }
    form button { width: 100%; padding: 10px; font-size: 15px; }
    .char-count { font-size: 12px; color: #888; text-align: right; margin-top: 2px; }
    #atLog { background: #1e1e1e; color: #00ff00; font-family: monospace; min-height: 100px; max-height: 200px; overflow-y: auto; padding: 10px; border-radius: 6px; margin-bottom: 8px; font-size: 13px; white-space: pre-wrap; word-break: break-all; }
    .at-input-group { display: flex; gap: 8px; }
    .at-input-group input { flex: 1; }
    .at-input-group button { width: auto; min-width: 70px; }
    #logView { background: #1e1e1e; color: #d4d4d4; padding: 10px; border-radius: 6px; font-family: monospace; font-size: 12px; line-height: 1.5; max-height: 40vh; overflow-y: auto; white-space: pre-wrap; word-break: break-all; min-height: 150px; }
    .info-table { width: 100%; border-collapse: collapse; margin-top: 8px; }
    .info-table td { padding: 6px 8px; border-bottom: 1px solid #eee; }
    .info-table td:first-child { font-weight: 500; width: 40%; color: #666; }
    @media(max-width:480px) {
      .container { padding: 8px; }
      .card { padding: 12px; }
      .btn-group button { min-width: 80px; font-size: 12px; padding: 8px; }
    }
  </style>
</head>
<body>
  <div class="header">
    <h1>📱 短信转发器</h1>
  </div>
  <div class="container">
    <div class="nav">
      <a href="/">⚙️ 系统配置</a>
      <a href="/tools" class="active">🧰 工具箱</a>
    </div>

    <form action="/sendsms" method="POST">
      <div class="card">
        <div class="card-title icon-sms">发送短信</div>
        <div class="form-group">
          <label>目标号码</label>
          <input type="text" name="phone" placeholder="13800138000" required>
        </div>
        <div class="form-group">
          <label>短信内容</label>
          <textarea name="content" placeholder="请输入短信内容..." required oninput="updateCount(this)"></textarea>
          <div class="char-count">已输入 <span id="charCount">0</span> 字符</div>
        </div>
        <button type="submit">📨 发送</button>
      </div>
    </form>

    <div class="card">
      <div class="card-title icon-info">模组信息查询</div>
      <div class="btn-group">
        <button type="button" onclick="queryInfo('ati')">固件信息</button>
        <button type="button" onclick="queryInfo('signal')">信号质量</button>
      </div>
      <div class="btn-group">
        <button type="button" onclick="queryInfo('siminfo')">SIM卡</button>
        <button type="button" onclick="queryInfo('network')">网络状态</button>
      </div>
      <div class="btn-group">
        <button type="button" onclick="queryInfo('wifi')" class="btn-blue">WiFi状态</button>
      </div>
      <div class="result-box" id="queryResult"></div>
    </div>

    <div class="card">
      <div class="card-title icon-net">网络测试</div>
      <button type="button" class="btn-orange" id="pingBtn" style="width:100%;padding:9px;" onclick="confirmPing()">Ping 测试</button>
      <div class="hint">向 8.8.8.8 进行 ping 操作</div>
      <div class="result-box" id="pingResult"></div>
    </div>

    <div class="card">
      <div class="card-title icon-modem">模组控制</div>
      <div class="btn-group">
        <button type="button" onclick="modemAction('restart')" class="btn-red">软重启</button>
        <button type="button" onclick="modemAction('hardreset')" class="btn-red">硬重启</button>
      </div>
      <div class="btn-group">
        <button type="button" onclick="modemAction('signal')">信号</button>
        <button type="button" onclick="modemAction('operator')">运营商</button>
        <button type="button" onclick="modemAction('imei')">IMEI</button>
      </div>
      <div class="result-box" id="modemResult"></div>
    </div>

    <div class="card">
      <div class="card-title icon-flight">飞行模式</div>
      <div class="btn-group">
        <button type="button" id="flightBtn" onclick="toggleFlightMode()" class="btn-red">✈️ 切换</button>
        <button type="button" onclick="queryFlightMode()" class="btn-purple">查询状态</button>
      </div>
      <div class="hint">飞行模式下模组射频关闭，无法收发短信</div>
      <div class="result-box" id="flightResult"></div>
    </div>

    <div class="card">
      <div class="card-title icon-at">AT 指令调试</div>
      <div id="atLog">等待输入指令...</div>
      <div class="at-input-group">
        <input type="text" id="atCmd" placeholder="如: AT+CSQ">
        <button type="button" onclick="sendAT()" id="atBtn">发送</button>
      </div>
      <div class="btn-group" style="margin-top:8px;">
        <button type="button" class="btn-gray" onclick="clearATLog()" style="flex:none;">清空日志</button>
      </div>
      <div class="hint">直接向模组串口发送指令并接收响应，请谨慎操作</div>
    </div>

    <div class="card">
      <div class="card-title icon-log">系统日志</div>
      <span id="logStatus" style="color:#4CAF50;font-size:12px;">● 自动刷新中</span>
      <div id="logView">加载中...</div>
      <div class="btn-group" style="margin-top:8px;">
        <button type="button" class="btn-gray" onclick="clearLogUI()" style="flex:none;">清空显示</button>
        <button type="button" class="btn-gray" onclick="refreshLog()" style="flex:none;">手动刷新</button>
      </div>
      <div style="margin-top:6px;font-size:13px;">
        <label style="cursor:pointer;"><input type="checkbox" id="logAuto" checked onchange="toggleLogAuto()"> 自动刷新 (2秒)</label>
      </div>
      <div class="hint">最多保留最近120条日志。</div>
    </div>
  </div>
  <script>
    function updateCount(el) { document.getElementById('charCount').textContent = el.value.length; }
    function queryInfo(type) {
      var result = document.getElementById('queryResult');
      result.className = 'result-box result-loading'; result.style.display = 'block';
      result.textContent = '正在查询，请稍候...';
      fetch('/query?type=' + type)
        .then(function(r) { return r.json(); })
        .then(function(data) {
          if (data.success) { result.className = 'result-box result-info'; result.innerHTML = data.message; }
          else { result.className = 'result-box result-error'; result.innerHTML = '❌ 查询失败<br>' + data.message; }
        })
        .catch(function(e) { result.className = 'result-box result-error'; result.textContent = '❌ 请求失败: ' + e; });
    }
    function confirmPing() { if (confirm('确定要执行 Ping 操作吗？')) doPing(); }
    function doPing() {
      var btn = document.getElementById('pingBtn');
      var result = document.getElementById('pingResult');
      btn.disabled = true; btn.textContent = '⏳ 正在 Ping...';
      result.className = 'result-box result-loading'; result.style.display = 'block';
      result.textContent = '正在执行 Ping 操作，请稍候（最长等待30秒）...';
      fetch('/ping', { method: 'POST' })
        .then(function(r) { return r.json(); })
        .then(function(data) {
          btn.disabled = false; btn.textContent = 'Ping 测试';
          if (data.success) { result.className = 'result-box result-success'; result.innerHTML = '✅ Ping 成功！<br>' + data.message; }
          else { result.className = 'result-box result-error'; result.innerHTML = '❌ Ping 失败<br>' + data.message; }
        })
        .catch(function(e) { btn.disabled = false; btn.textContent = 'Ping 测试'; result.className = 'result-box result-error'; result.textContent = '❌ 请求失败: ' + e; });
    }
    function modemAction(action) {
      var names = { 'restart': '软重启', 'hardreset': '硬重启', 'signal': '信号查询', 'operator': '运营商查询', 'imei': 'IMEI查询' };
      var name = names[action] || action;
      var result = document.getElementById('modemResult');
      if (action === 'hardreset') { if (!confirm('硬重启将断电重启模组，确定继续？')) return; }
      result.className = 'result-box result-loading'; result.style.display = 'block';
      result.textContent = name + '中...';
      fetch('/modem?action=' + action)
        .then(function(r) { return r.json(); })
        .then(function(data) {
          if (data.success) { result.className = 'result-box result-success'; result.innerHTML = '✅ ' + name + '成功: ' + data.message; }
          else { result.className = 'result-box result-error'; result.innerHTML = '❌ ' + name + '失败: ' + data.message; }
        })
        .catch(function(e) { result.className = 'result-box result-error'; result.textContent = '❌ 请求失败: ' + e; });
    }
    function queryFlightMode() {
      var result = document.getElementById('flightResult');
      result.className = 'result-box result-loading'; result.style.display = 'block';
      result.textContent = '正在查询飞行模式状态...';
      fetch('/flight?action=query')
        .then(function(r) { return r.json(); })
        .then(function(data) {
          if (data.success) { result.className = 'result-box result-info'; result.innerHTML = data.message; }
          else { result.className = 'result-box result-error'; result.innerHTML = '❌ 查询失败: ' + data.message; }
        })
        .catch(function(e) { result.className = 'result-box result-error'; result.textContent = '❌ 请求失败: ' + e; });
    }
    function toggleFlightMode() {
      if (!confirm('确定要切换飞行模式吗？')) return;
      var btn = document.getElementById('flightBtn');
      var result = document.getElementById('flightResult');
      btn.disabled = true;
      result.className = 'result-box result-loading'; result.style.display = 'block';
      result.textContent = '正在切换飞行模式...';
      fetch('/flight?action=toggle')
        .then(function(r) { return r.json(); })
        .then(function(data) {
          btn.disabled = false;
          if (data.success) { result.className = 'result-box result-success'; result.innerHTML = '✅ ' + data.message; }
          else { result.className = 'result-box result-error'; result.innerHTML = '❌ 切换失败: ' + data.message; }
        })
        .catch(function(e) { btn.disabled = false; result.className = 'result-box result-error'; result.textContent = '❌ 请求失败: ' + e; });
    }
    function addLog(msg, type) {
      type = type || 'resp';
      var log = document.getElementById('atLog');
      var div = document.createElement('div');
      var b = document.createElement('b');
      if (type === 'user') { b.style.color = '#fff'; b.textContent = '> '; }
      else if (type === 'error') { b.style.color = '#f44336'; b.textContent = '❌ '; }
      else { b.style.color = '#4CAF50'; b.textContent = '[RESP] '; }
      div.appendChild(b); div.appendChild(document.createTextNode(msg));
      log.appendChild(div); log.scrollTop = log.scrollHeight;
    }
    function sendAT() {
      var cmd = document.getElementById('atCmd').value.trim();
      if (!cmd) return;
      var btn = document.getElementById('atBtn');
      btn.disabled = true; btn.textContent = '...';
      addLog(cmd, 'user'); document.getElementById('atCmd').value = '';
      fetch('/at?cmd=' + encodeURIComponent(cmd))
        .then(function(r) { return r.json(); })
        .then(function(data) { addLog(data.message, data.success ? 'resp' : 'error'); })
        .catch(function(e) { addLog('网络错误: ' + e, 'error'); })
        .finally(function() { btn.disabled = false; btn.textContent = '发送'; });
    }
    function clearATLog() { document.getElementById('atLog').innerHTML = ''; }
    document.getElementById('atCmd').addEventListener('keydown', function(e) { if (e.key === 'Enter') sendAT(); });
    var logTimer = null;
    function startLogPoll() { if (logTimer) return; logTimer = setInterval(refreshLog, 2000); }
    function stopLogPoll() { if (logTimer) { clearInterval(logTimer); logTimer = null; } }
    function toggleLogAuto() { if (document.getElementById('logAuto').checked) startLogPoll(); else stopLogPoll(); }
    function clearLogUI() { document.getElementById('logView').textContent = ''; }
    function refreshLog() {
      var el = document.getElementById('logView');
      fetch('/log')
        .then(function(r) { return r.json(); })
        .then(function(lines) {
          if (!Array.isArray(lines)) return;
          el.textContent = lines.join('\n');
          el.scrollTop = el.scrollHeight;
        })
        .catch(function() { if (el.textContent === '加载中...') el.textContent = '无法获取日志'; });
    }
    document.addEventListener('DOMContentLoaded', function() { refreshLog(); startLogPoll(); });
  </script>
</body>
</html>
)rawliteral";
