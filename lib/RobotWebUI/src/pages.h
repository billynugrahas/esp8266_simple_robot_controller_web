// PROGMEM Budget: INDEX_HTML
// Simplified: motor control + connection status + system info
//
// Last updated:  2026-04-17
#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Robot Control</title>
<style>
/* === BASE === */
*{box-sizing:border-box;margin:0;padding:0}
:root{--bg:#0f172a;--card:#1e293b;--border:#334155;--text:#e2e8f0;--text-bright:#f8fafc;--text-muted:#64748b;--accent:#2563eb;--success:#16a34a;--success-light:#4ade80;--danger:#dc2626;--success-bg:#14532d;--danger-bg:#450a0a;--radius-card:16px;--radius-badge:999px;--radius-alert:10px;--radius-btn:10px;--shadow-card:0 25px 50px rgba(0,0,0,0.5);--font-stack:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;--transition-fast:0.1s;--transition-normal:0.2s;--btn-bg:#475569}
body{font-family:var(--font-stack);background:var(--bg);color:var(--text);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:1rem}
.card{background:var(--card);border-radius:var(--radius-card);padding:2rem;width:100%;max-width:420px;box-shadow:var(--shadow-card);border:1px solid var(--border)}

/* === CONNECTION === */
.card-header{text-align:center;margin-bottom:1.5rem}
.card-header h1{font-size:1.5rem;font-weight:700;color:var(--text-bright)}
.card-header p{font-size:0.85rem;color:var(--text-muted);margin-top:0.25rem}
.status-badge{display:inline-flex;align-items:center;gap:6px;padding:4px 12px;border-radius:var(--radius-badge);font-size:0.8rem;font-weight:700}
.connected{background:var(--success-bg);color:#86efac}
.disconnected{background:var(--danger-bg);color:#fca5a5}
.status-dot{width:8px;height:8px;border-radius:50%;transition:background var(--transition-normal)}
.connected .status-dot{background:var(--success-light)}
.disconnected .status-dot{background:var(--danger)}

/* === ALERT === */
.alert{padding:0.75rem 1rem;border-radius:var(--radius-alert);font-size:0.85rem;margin-bottom:1rem;display:none}
.alert-success{background:var(--success-bg);color:#86efac;border-left:3px solid var(--success)}
.alert-danger{background:var(--danger-bg);color:#fca5a5;border-left:3px solid var(--danger)}
.alert-warning{background:#451a03;color:#fbbf24;border-left:3px solid #d97706}
.alert.show{display:block}

/* === CONTROLS === */
.section{margin-bottom:1.5rem}
.section-title{font-size:0.75rem;font-weight:400;text-transform:uppercase;letter-spacing:0.08em;color:var(--text-muted);margin-bottom:0.75rem}
.divider{border:none;border-top:1px solid var(--border);margin:1.5rem 0}
.estop-btn{width:100%;height:48px;background:var(--danger);color:#fff;border:none;border-radius:var(--radius-btn);font-size:0.85rem;font-weight:700;cursor:pointer;transition:transform var(--transition-fast);margin-bottom:1rem}
.estop-btn:active{transform:scale(0.96);background:#b91c1c}
.dpad{display:grid;grid-template-areas:". up ." "left center right" ". down .";grid-template-columns:repeat(3,48px);grid-template-rows:repeat(3,48px);gap:4px;justify-content:center;user-select:none;-webkit-user-select:none;-webkit-tap-highlight-color:transparent;touch-action:manipulation;margin:0 auto}
.dpad-up{grid-area:up}.dpad-left{grid-area:left}.dpad-center{grid-area:center}.dpad-right{grid-area:right}.dpad-down{grid-area:down}
.dpad button{background:var(--btn-bg);color:var(--text);border:none;border-radius:var(--radius-btn);font-size:1.2rem;cursor:pointer;transition:transform var(--transition-fast),background var(--transition-normal);display:flex;align-items:center;justify-content:center}
.dpad button:active{transform:scale(0.96);background:var(--accent)}
.slider-row{display:flex;align-items:center;gap:8px;margin-top:0.75rem}
.slider-row label{font-size:0.85rem;color:var(--text-muted);min-width:80px}
.slider-row span{font-size:0.85rem;font-weight:500;color:var(--text);min-width:32px;text-align:right}
.slider-row input{flex:1;height:44px;-webkit-appearance:none;appearance:none;background:var(--border);border-radius:8px;outline:none;cursor:pointer}
.slider-row input::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;border-radius:50%;background:var(--accent);cursor:pointer}
.slider-row input::-moz-range-thumb{width:24px;height:24px;border-radius:50%;background:var(--accent);border:none;cursor:pointer}

/* === SYSTEM INFO === */
.info-row{display:flex;justify-content:space-between;align-items:center;padding:0.6rem 0;border-bottom:1px solid var(--card)}
.info-row:last-child{border-bottom:none}
.info-label{font-size:0.85rem;color:var(--text-muted)}
.info-value{font-size:0.85rem;font-weight:500;color:var(--text)}
</style>
</head>
<body>
<div class="card">

<!-- SECTION: HEADER + CONNECTION -->
<div class="card-header">
<h1>Robot Control</h1>
<div class="status-badge disconnected" id="ws-badge" style="margin-top:0.5rem">
<span class="status-dot" id="ws-dot"></span>
<span id="ws-status">Disconnected</span>
</div>
</div>
<div class="alert" id="alert"></div>

<hr class="divider">

<!-- SECTION: MOTOR CONTROLS -->
<div class="section">
<p class="section-title">CONTROLS</p>
<button class="estop-btn" id="estop-btn">EMERGENCY STOP</button>
<div class="dpad">
<button class="dpad-up" data-dir="forward">&#9650;</button>
<button class="dpad-left" data-dir="left">&#9664;</button>
<button class="dpad-center" data-dir="stop">&#9632;</button>
<button class="dpad-right" data-dir="right">&#9654;</button>
<button class="dpad-down" data-dir="back">&#9660;</button>
</div>
<div class="slider-row">
<label>Speed</label>
<input type="range" id="speed-slider" min="0" max="100" value="75">
<span id="speed-val">75%</span>
</div>
</div>

<hr class="divider">

<!-- SECTION: CALIBRATION -->
<div class="section">
<p class="section-title">CALIBRATION</p>
<div class="slider-row">
<label>Left Motor</label>
<input type="range" id="coef-left" min="0" max="100" value="100">
<span id="coef-left-val">100%</span>
</div>
<div class="slider-row">
<label>Right Motor</label>
<input type="range" id="coef-right" min="0" max="100" value="100">
<span id="coef-right-val">100%</span>
</div>
</div>

<hr class="divider">

<!-- SECTION: SYSTEM INFO -->
<div class="section">
<p class="section-title">SYSTEM</p>
<div class="info-row"><span class="info-label">Address</span><span class="info-value" id="val-ip">--</span></div>
<div class="info-row"><span class="info-label">mDNS</span><span class="info-value">robot.local</span></div>
<div class="info-row"><span class="info-label">Uptime</span><span class="info-value" id="val-up">--</span></div>
</div>

</div>

<!-- SECTION: JAVASCRIPT -->
<script>
var ws,rDelay=1000,curSpeed=75;

/* --- WebSocket --- */
function connectWS(){
ws=new WebSocket('ws://'+location.host+'/ws');
ws.onopen=function(){
  rDelay=1000;
  updateConnection(true);
  sendCoef();
};
ws.onclose=function(){
  updateConnection(false);
  showAlert('Connection lost -- reconnecting...','danger');
  setTimeout(connectWS,rDelay);
  rDelay=Math.min(rDelay*2,10000);
};
ws.onmessage=function(e){
  var msg=JSON.parse(e.data);
  if(msg.t==='sys')updateSystemInfo(msg.d);
  if(msg.t==='ack'&&msg.d){
    if(msg.d.msg==='motor_timeout')showAlert('Motor timeout -- auto-stopped','warning');
    if(msg.d.alert)showAlert(msg.d.msg,msg.d.alert);
  }
};
}

/* --- Connection status --- */
function updateConnection(on){
var badge=document.getElementById('ws-badge');
var status=document.getElementById('ws-status');
if(on){badge.className='status-badge connected';status.textContent='Connected';}
else{badge.className='status-badge disconnected';status.textContent='Disconnected';}
}

/* --- System info --- */
function updateSystemInfo(d){
if(d.ip!==undefined)document.getElementById('val-ip').textContent=d.ip;
if(d.up!==undefined)document.getElementById('val-up').textContent=formatUptime(d.up);
}
function formatUptime(s){
var h=Math.floor(s/3600);var m=Math.floor((s%3600)/60);s=s%60;
return(h?h+'h ':'')+(m?m+'m ':'')+s+'s';
}

/* --- Alerts --- */
function showAlert(msg,type){
var el=document.getElementById('alert');
el.textContent=msg;
el.className='alert alert-'+type+' show';
setTimeout(function(){el.classList.remove('show');},2500);
}

/* --- Motor commands --- */
function sendMotor(dir){
if(ws&&ws.readyState===1)ws.send(JSON.stringify({t:"motor",d:{dir:dir,spd:curSpeed}}));
}

/* --- Speed slider --- */
var slider=document.getElementById('speed-slider');
var speedVal=document.getElementById('speed-val');
slider.oninput=function(){curSpeed=parseInt(this.value);speedVal.textContent=curSpeed+'%'};

/* --- Coefficient sliders --- */
var coefLeftSlider=document.getElementById('coef-left');
var coefLeftVal=document.getElementById('coef-left-val');
var coefRightSlider=document.getElementById('coef-right');
var coefRightVal=document.getElementById('coef-right-val');
if(localStorage.getItem('coef-left')){var cl=localStorage.getItem('coef-left');coefLeftSlider.value=cl;coefLeftVal.textContent=cl+'%';}
if(localStorage.getItem('coef-right')){var cr=localStorage.getItem('coef-right');coefRightSlider.value=cr;coefRightVal.textContent=cr+'%';}
function sendCoef(){
localStorage.setItem('coef-left',coefLeftSlider.value);
localStorage.setItem('coef-right',coefRightSlider.value);
if(ws&&ws.readyState===1)ws.send(JSON.stringify({t:"coef",d:{left:parseInt(coefLeftSlider.value)/100,right:parseInt(coefRightSlider.value)/100}}));
}
coefLeftSlider.oninput=function(){coefLeftVal.textContent=this.value+'%';sendCoef()};
coefRightSlider.oninput=function(){coefRightVal.textContent=this.value+'%';sendCoef()};

/* --- D-pad hold-to-drive --- */
var holdTimer=null,holdDir=null;
function startHold(dir){stopHold();holdDir=dir;sendMotor(dir);holdTimer=setInterval(function(){sendMotor(holdDir)},200)}
function stopHold(){if(holdTimer){clearInterval(holdTimer);holdTimer=null;if(holdDir&&holdDir!=='stop')sendMotor('stop');holdDir=null}}
var dpadBtns=document.querySelectorAll('.dpad button');
for(var i=0;i<dpadBtns.length;i++){
dpadBtns[i].addEventListener('pointerdown',function(e){e.preventDefault();startHold(this.getAttribute('data-dir'))});
dpadBtns[i].addEventListener('pointerup',stopHold);
dpadBtns[i].addEventListener('pointerleave',stopHold);
dpadBtns[i].addEventListener('pointercancel',stopHold);
}

/* --- E-stop --- */
document.getElementById('estop-btn').addEventListener('pointerdown',function(e){
e.preventDefault();sendMotor('stop');
});

connectWS();
</script>
</body>
</html>
)rawliteral";
