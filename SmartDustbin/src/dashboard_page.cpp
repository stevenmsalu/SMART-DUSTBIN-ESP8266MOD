#include "dashboard_page.h"

// Monitoring-only dashboard — no external CDN; served from flash (WP-11.4).
const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Dustbin</title>
<style>
:root{
--bg-page:#f4f7f5;--bg-card:#ffffff;--bg-card-subtle:#eef6f0;
--text-primary:#1a2e22;--text-secondary:#4a5f52;--text-muted:#6b7f73;
--border:#d4e4da;--accent:#2d8a4e;--accent-strong:#1f6b3a;--accent-soft:#e8f5ec;
--status-ok:#2d8a4e;--status-active:#0d7a6a;--status-wait:#b8860b;--status-bad:#c0392b;
--shadow:0 1px 3px rgba(26,46,34,0.08);
}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;background:var(--bg-page);color:var(--text-primary);line-height:1.5;padding:16px;max-width:960px;margin:0 auto}
h1{font-size:1.35rem;font-weight:600;margin-bottom:4px;color:var(--text-primary)}
.sub{color:var(--text-secondary);font-size:0.9rem;margin-bottom:16px}
.banner{padding:12px 14px;border-radius:10px;margin-bottom:16px;background:var(--bg-card);border:1px solid var(--border);border-left-width:4px;font-size:0.9rem;color:var(--text-secondary)}
.banner.ok{border-left-color:var(--status-ok)}
.banner.bad{border-left-color:var(--status-bad);color:var(--text-primary)}
.grid{display:grid;gap:12px;grid-template-columns:1fr}
@media(min-width:720px){.grid{grid-template-columns:1fr 1fr}}
.card{background:var(--bg-card);border:1px solid var(--border);border-radius:10px;padding:16px;box-shadow:var(--shadow)}
.card h2{font-size:0.95rem;font-weight:600;margin-bottom:12px;color:var(--accent-strong)}
.card.hero{background:var(--bg-card-subtle);grid-column:1/-1}
.row{display:flex;justify-content:space-between;align-items:flex-start;gap:12px;margin:8px 0;font-size:0.9rem;min-height:44px}
.lab{color:var(--text-muted);flex:1 1 auto}
.val{font-weight:600;text-align:right;color:var(--text-primary);flex:0 1 auto}
.val.addr{word-break:break-all;max-width:58%}
.chip{display:inline-block;padding:4px 10px;border-radius:999px;font-size:0.85rem;font-weight:600;border:1px solid var(--border)}
.chip.ok{background:var(--accent-soft);color:var(--status-ok)}
.chip.active{background:#e0f2ef;color:var(--status-active);border-color:#b8ddd6}
.chip.wait{background:#faf3e0;color:var(--status-wait);border-color:#e8d9a8}
.chip.muted{background:#f0f4f1;color:var(--text-muted)}
.foot{margin-top:16px;font-size:0.8rem;color:var(--text-muted);line-height:1.45}
details.card summary{cursor:pointer;list-style:none;font-weight:600;color:var(--accent-strong)}
details.card summary::-webkit-details-marker{display:none}
details.card summary::after{content:' ▾';color:var(--text-muted);font-weight:400}
details[open].card summary::after{content:' ▴'}
details .inner{padding-top:8px}
</style>
</head>
<body>
<h1>Smart Dustbin</h1>
<p class="sub">Live status from your dustbin</p>
<div id="conn" class="banner bad">Connecting…</div>
<div class="grid">
<section class="card hero">
<h2>How the bin is behaving</h2>
<div class="row"><span class="lab">Something nearby</span><span class="val" id="nearby">—</span></div>
<div class="row"><span class="lab">Lid position</span><span class="val" id="lidPos">—</span></div>
<div class="row"><span class="lab">Closing soon</span><span class="val" id="closingSoon">—</span></div>
<div class="row"><span class="lab">Indicator light</span><span class="val" id="indLight">—</span></div>
<p class="foot">Indicator turns on while the lid is open.</p>
</section>
<section class="card">
<h2>Network</h2>
<div class="row"><span class="lab">Connection</span><span class="val" id="wifiState">—</span></div>
<div class="row"><span class="lab">Network address</span><span class="val addr" id="ipState">—</span></div>
<div class="row"><span class="lab">Last refreshed</span><span class="val" id="lastUpd">—</span></div>
</section>
<details class="card">
<summary>Technical details</summary>
<div class="inner">
<div class="row"><span class="lab">Motor angle</span><span class="val" id="motorDeg">—</span></div>
<div class="row"><span class="lab">Powered on for</span><span class="val" id="uptime">—</span></div>
<p class="foot">View only — no remote control. Ultrasonic sensor measures distance; it does not identify people.</p>
</div>
</details>
</div>
<script>
const POLL_MS=2500;
let busy=false,timer=null,stale=false;
function setText(id,t){const el=document.getElementById(id);if(el)el.textContent=t;}
function setHtml(id,h){const el=document.getElementById(id);if(el)el.innerHTML=h;}
function chip(cls,label){return'<span class="chip '+cls+'">'+label+'</span>';}
function fmtUptime(ms){if(ms==null)return'—';const s=Math.floor(ms/1000);const h=Math.floor(s/3600);const m=Math.floor((s%3600)/60);const r=s%60;
return(h?h+'h ':'')+(m?m+'m ':'')+r+'s';}
function setConn(ok,msg){const b=document.getElementById('conn');b.className='banner '+(ok?'ok':'bad');b.textContent=msg;}
function lidLabel(state){if(state==='OPEN')return'Open';if(state==='CLOSED')return'Closed';return state||'—';}
async function refresh(){
if(busy)return;busy=true;
try{
const [hr,sr]=await Promise.all([fetch('/api/health'),fetch('/api/status')]);
if(!hr.ok||!sr.ok)throw new Error('http');
const health=await hr.json(),status=await sr.json();
stale=false;const now=new Date().toLocaleTimeString();
setConn(true,'Connected — view only');
setText('lastUpd',now);
const wifi=health.wifi&&health.wifi.connected;
setText('wifiState',wifi?'Connected':'Disconnected');
const ip=wifi&&health.wifi.ip?health.wifi.ip:(status.network&&status.network.ip?status.network.ip:null);
setText('ipState',ip||'—');
const up=status.device&&status.device.uptime_ms!=null?status.device.uptime_ms:(health.device&&health.device.uptime_ms);
setText('uptime',fmtUptime(up));
const detected=!!(status.approach&&status.approach.detected);
setHtml('nearby',detected?chip('active','Something detected'):chip('ok','Nothing detected'));
const lid=status.lid||{};
setText('lidPos',lidLabel(lid.state));
setHtml('closingSoon',lid.close_delay_pending?chip('wait','Yes — lid will close'):chip('muted','No'));
const whiteOn=!!(status.leds&&status.leds.white_on);
setHtml('indLight',whiteOn?chip('active','On'):chip('muted','Off'));
setText('motorDeg',lid.servo_target_angle_deg!=null?lid.servo_target_angle_deg+'°':'—');
}catch(e){
if(!stale){stale=true;setConn(false,'Connection lost — data may be outdated');}
}finally{busy=false;}
}
function start(){refresh();timer=setInterval(refresh,POLL_MS);}
start();
</script>
</body>
</html>
)HTML";