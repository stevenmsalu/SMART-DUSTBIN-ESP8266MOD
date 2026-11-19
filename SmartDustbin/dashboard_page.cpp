#include "dashboard_page.h"

// Monitoring-only dashboard — no external CDN; served from flash.
const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Dustbin</title>
<style>
:root{--bg:#0f1419;--card:#1a2332;--text:#e8eef4;--muted:#93a4b5;--ok:#3dd68c;--warn:#f5c542;--bad:#f07178;--line:#2d3a4d;}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;background:var(--bg);color:var(--text);line-height:1.45;padding:12px}
h1{font-size:1.25rem;margin-bottom:4px}
.sub{color:var(--muted);font-size:.85rem;margin-bottom:12px}
.banner{padding:10px 12px;border-radius:8px;margin-bottom:12px;background:var(--card);border:1px solid var(--line)}
.banner.ok{border-color:var(--ok)}
.banner.warn{border-color:var(--warn)}
.banner.bad{border-color:var(--bad)}
.grid{display:grid;gap:10px;grid-template-columns:1fr}
@media(min-width:720px){.grid{grid-template-columns:1fr 1fr}}
.card{background:var(--card);border:1px solid var(--line);border-radius:8px;padding:12px}
.card h2{font-size:.95rem;margin-bottom:8px;color:#c5d3e0}
.row{display:flex;justify-content:space-between;gap:8px;margin:6px 0;font-size:.9rem}
.lab{color:var(--muted)}
.val{font-weight:600;text-align:right}
.pill{display:inline-block;padding:2px 8px;border-radius:999px;font-size:.8rem;border:1px solid var(--line)}
.pill.ok{background:#163d2f;color:var(--ok)}
.pill.warn{background:#3d3316;color:var(--warn)}
.pill.bad{background:#3d1c1f;color:var(--bad)}
.pill.muted{background:#1e2633;color:var(--muted)}
</style>
</head>
<body>
<h1>Smart Dustbin Monitor</h1>
<p class="sub">Local read-only dashboard · approach + lid</p>
<div id="conn" class="banner bad">Connecting to device API…</div>
<div class="grid">
<section class="card"><h2>Device</h2>
<div class="row"><span class="lab">API</span><span class="val" id="apiState">—</span></div>
<div class="row"><span class="lab">Wi‑Fi</span><span class="val" id="wifiState">—</span></div>
<div class="row"><span class="lab">IP</span><span class="val" id="ipState">—</span></div>
<div class="row"><span class="lab">Uptime</span><span class="val" id="uptime">—</span></div>
<div class="row"><span class="lab">Last update</span><span class="val" id="lastUpd">—</span></div>
</section>
<section class="card"><h2>Approach</h2>
<div class="row"><span class="lab">Detection</span><span class="val" id="approach">—</span></div>
</section>
<section class="card"><h2>Lid</h2>
<div class="row"><span class="lab">State</span><span class="val" id="lidState">—</span></div>
<div class="row"><span class="lab">Servo target</span><span class="val" id="servoDeg">—</span></div>
<div class="row"><span class="lab">Close delay</span><span class="val" id="closeDly">—</span></div>
</section>
<section class="card"><h2>White LED</h2>
<div class="row"><span class="lab">Status (lid linked)</span><span class="val" id="ledW">—</span></div>
</section>
</div>
<script>
const POLL_MS=2500;
let busy=false,timer=null,stale=false;
function setText(id,t){const el=document.getElementById(id);if(el)el.textContent=t;}
function pill(cls,label){return'<span class="pill '+cls+'">'+(label||cls).toUpperCase()+'</span>';}
function fmtUptime(ms){if(ms==null)return'—';const s=Math.floor(ms/1000);const h=Math.floor(s/3600);const m=Math.floor((s%3600)/60);const r=s%60;
return(h?h+'h ':'')+(m?m+'m ':'')+r+'s';}
function setConn(cls,msg){const b=document.getElementById('conn');b.className='banner '+cls;b.textContent=msg;}
async function refresh(){
if(busy)return;busy=true;
try{
const [hr,sr]=await Promise.all([fetch('/api/health'),fetch('/api/status')]);
if(!hr.ok||!sr.ok)throw new Error('http');
const health=await hr.json(),status=await sr.json();
stale=false;const now=new Date().toLocaleTimeString();
setConn('ok','Device online · monitoring only');
setText('apiState','Available');
setText('lastUpd',now);
const wifi=health.wifi&&health.wifi.connected;
setText('wifiState',wifi?'Connected':'Disconnected');
setText('ipState',wifi&&health.wifi.ip?health.wifi.ip:'—');
const up=status.device&&status.device.uptime_ms!=null?status.device.uptime_ms:(health.device&&health.device.uptime_ms);
setText('uptime',fmtUptime(up));
const appr=status.approach&&status.approach.detected;
document.getElementById('approach').innerHTML=appr?pill('warn','detected'):pill('ok','clear');
const lid=status.lid||{};
setText('lidState',lid.state||'—');
setText('servoDeg',lid.servo_target_angle_deg!=null?lid.servo_target_angle_deg+'°':'—');
setText('closeDly',lid.close_delay_pending?'Pending':'Not pending');
setText('ledW',status.leds&&status.leds.white_on?'ON':'OFF');
}catch(e){
if(!stale){stale=true;setConn('bad','API unavailable — showing last data if any');setText('apiState','Unavailable');}
}finally{busy=false;}
}
function start(){refresh();timer=setInterval(refresh,POLL_MS);}
start();
</script>
</body>
</html>
)HTML";