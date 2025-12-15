#include <FS.h>
#include <LittleFS.h> 
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFiManager.h> 
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h> 
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HTTPClient.h>
#include <FastLED.h> 
#include <vector>

extern "C" {
  #include <tinyexpr.h>
} 

// ═══════════════════════════════════════════════════════════
// 🔧 HARDWARE CONFIGURATION
// ═══════════════════════════════════════════════════════════
#define LED_PIN     16      
#define NUM_LEDS    30      
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define MAX_POWER_MA 850    

CRGB leds[NUM_LEDS];

// ═══════════════════════════════════════════════════════════
// 🌍 GLOBAL OBJECTS
// ═══════════════════════════════════════════════════════════
WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiClientSecure secured_client;
UniversalTelegramBot* bot = nullptr; 

bool shouldReboot = false;
unsigned long rebootTimer = 0;
std::vector<String> systemLogs; 

// ═══════════════════════════════════════════════════════════
// 💾 DATA STRUCTURES
// ═══════════════════════════════════════════════════════════
struct Spot {
  String name;
  String lat;
  String lon;
  bool enabled;
  int forecastDays;
  String colorHex; 
  float surfPower;
  String bestDayDate;
  String humanTimeRange; 
  bool isFiring;
  String lastNotifiedID; 
  float minThreshold;  
  float epicThreshold; 
  bool useAdvanced;
  int swellMask;        
  int bestWindDir;       
};

std::vector<Spot> spots;

struct Config {
  // MASTER SWITCHES
  bool ledsGlobalEnabled; 
  int masterBrightness; 
  
  // STANDARD SURF DISPLAY
  int surfCycleTime;      
  int surfAnimType;      
  
  // EPIC SURF DISPLAY
  int epicAnimType;
  String epicColorHex;
  int epicSpeed;

  // NOTIFICATIONS
  bool tgEnabled;   
  String botToken;
  String chatId;
  int tgMode;       
  int tgThreshold;  

  // LAMP FALLBACK
  bool lampFallbackEnabled; 
  String lampColorHex;        
  int lampEffect;      
  int lampSpeed;   
  
  // 🕒 TIME & UPDATES
  int timeZoneOffset; 
  bool dndEnabled;
  int dndStart;       
  int dndEnd;
  int updateFreqMins; 

  // 🧠 PRO MATH LOGIC
  bool proFormulaEnabled;
  String proFormula;
} conf;

unsigned long lastUpdate = 0;
bool anySpotFiring = false; 
uint8_t gHue = 0; 

// ═══════════════════════════════════════════════════════════
// 🛠 LOGGING HELPER (UNIFIED)
// ═══════════════════════════════════════════════════════════
// Prints to Serial AND saves to the web Log buffer
void logSys(String msg) {
  // Print to Serial Monitor
  Serial.println(msg);
  
  // Add to Web Log (Keep last 40 lines to save RAM)
  String timeStr = timeClient.getFormattedTime();
  String stamp = "[" + timeStr.substring(0, 5) + "] ";
  if (systemLogs.size() >= 40) systemLogs.pop_back(); 
  systemLogs.insert(systemLogs.begin(), stamp + msg);
}

// ═══════════════════════════════════════════════════════════
// 🎨 LOADING ANIMATION
// ═══════════════════════════════════════════════════════════
void showLoadingAnim() {
  FastLED.clear();
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::DeepSkyBlue;
    FastLED.setBrightness(100);
    FastLED.show();
    delay(20); 
  }
  delay(200);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

// ═══════════════════════════════════════════════════════════
// 🎨 HTML INTERFACE
// ═══════════════════════════════════════════════════════════
const char* wifi_custom_css = "<style>body{background-color:#121212;color:#e0e0e0;font-family:sans-serif;}button{background-color:#6366f1;color:white;border:none;padding:10px;}</style>";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Surf Beacon</title>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&display=swap" rel="stylesheet">
<style>
:root{--bg:#020617;--card:#1e293b;--acc:#6366f1;--txt:#f8fafc;--border:#334155;--input-bg:#0f172a}
*{box-sizing:border-box}
body{font-family:'Inter',sans-serif;background:var(--bg);color:var(--txt);margin:0;padding:20px;padding-bottom:120px;font-size:14px}
.app{max-width:600px;margin:0 auto}
h1{text-align:center;background:-webkit-linear-gradient(0deg, #818cf8, #c084fc);-webkit-background-clip:text;-webkit-text-fill-color:transparent;font-weight:900;letter-spacing:-0.5px;margin-bottom:25px;font-size:24px;display:flex;justify-content:center;align-items:center;gap:12px}
.card{background:var(--card);border-radius:16px;padding:20px;margin-bottom:15px;border:1px solid var(--border);box-shadow:0 4px 6px -1px rgba(0,0,0,0.1)}
.head{display:flex;justify-content:space-between;align-items:center;margin-bottom:15px;padding-bottom:10px;border-bottom:1px solid var(--border)}
.head span{font-weight:800;letter-spacing:0.5px;color:#94a3b8;font-size:12px;text-transform:uppercase}
.sub-head{font-weight:700;color:#94a3b8;font-size:12px;text-transform:uppercase;margin-bottom:10px;display:block}
label{display:block;font-size:13px;color:#cbd5e1;margin-bottom:6px;font-weight:600}
input,select{width:100%;padding:12px 14px;background:var(--input-bg);border:1px solid var(--border);border-radius:10px;color:white;font-family:inherit;font-size:14px;outline:none;transition:0.2s;min-height:45px}
input:focus,select:focus{border-color:var(--acc)}
input[type="color"]{height:45px;padding:2px;cursor:pointer}
input[type=range] { -webkit-appearance: none; width: 100%; background: transparent; padding: 0; min-height: 30px; border: none; }
input[type=range]:focus { outline: none; border: none; }
input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; height: 20px; width: 20px; border-radius: 50%; background: var(--acc); cursor: pointer; margin-top: -8px; box-shadow: 0 2px 6px rgba(0,0,0,0.3); }
input[type=range]::-webkit-slider-runnable-track { width: 100%; height: 4px; cursor: pointer; background: #334155; border-radius: 2px; }
input[type=range]::-moz-range-thumb { height: 20px; width: 20px; border: none; border-radius: 50%; background: var(--acc); cursor: pointer; box-shadow: 0 2px 6px rgba(0,0,0,0.3); }
input[type=range]::-moz-range-track { width: 100%; height: 4px; cursor: pointer; background: #334155; border-radius: 2px; }
.grid-2{display:grid;grid-template-columns:1fr;gap:12px}
@media (min-width: 480px) { .grid-2{grid-template-columns:1fr 1fr} }
.switch{position:relative;width:44px;height:24px;background:#334155;border-radius:12px;transition:0.3s;cursor:pointer}
.switch.on{background:var(--acc)}
.switch.on::after{transform:translateX(20px)}
.switch::after{content:'';position:absolute;top:2px;left:2px;width:20px;height:20px;background:#fff;border-radius:50%;transition:0.3s}
.tabs{display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;background:#0f172a;padding:4px;border-radius:14px;margin-bottom:20px;border:1px solid var(--border)}
.tab{text-align:center;padding:10px;border-radius:10px;font-size:12px;font-weight:700;color:#64748b;cursor:pointer;transition:0.2s}
.tab.active{background:var(--card);color:white;box-shadow:0 1px 3px rgba(0,0,0,0.2)}
.epic-box{margin-bottom:20px;padding:15px;border-radius:12px;background:linear-gradient(145deg, rgba(129,140,248,0.1), rgba(192,132,252,0.02));border:1px solid rgba(129,140,248,0.4)}
.epic-head{background:-webkit-linear-gradient(0deg, #818cf8, #c084fc);-webkit-background-clip:text;-webkit-text-fill-color:transparent;margin-bottom:12px;text-transform:uppercase;font-size:12px;font-weight:900;letter-spacing:0.5px}
.spot{background:#0f172a;border:1px solid var(--border);border-radius:16px;padding:15px;margin-bottom:15px;display:flex;flex-direction:column;gap:12px}
.spot-top{display:flex;justify-content:space-between;align-items:center}
.spot-nm{background:transparent;border:none;color:white;font-weight:700;font-size:16px;width:70%;padding:0;min-height:auto}
.fab{position:fixed;bottom:25px;left:50%;transform:translateX(-50%);width:90%;max-width:580px;background:#334155;color:#94a3b8;border:1px solid #475569;padding:16px;border-radius:16px;font-weight:800;font-size:14px;box-shadow:0 10px 30px rgba(0,0,0,0.5);cursor:pointer;z-index:99;transition:0.3s;pointer-events:none}
.fab.dirty{background:var(--acc);color:white;box-shadow:0 10px 30px rgba(99,102,241,0.4);border:none;pointer-events:all}
.fab:active{transform:translateX(-50%) scale(0.98)}
.del{background:rgba(239,68,68,0.15);color:#ef4444;border:none;padding:8px 14px;border-radius:8px;font-weight:700;cursor:pointer;font-size:12px}
.compass-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin-bottom:15px}
.dir-btn{background:#1e293b;border:1px solid #475569;color:#94a3b8;padding:10px 0;font-size:11px;text-align:center;border-radius:8px;cursor:pointer;font-weight:700}
.dir-btn.active{background:var(--acc);color:white;border-color:var(--acc)}
.hidden{display:none}
.log-box{background:#0b0f19;border:1px solid #334155;border-radius:10px;padding:12px;height:140px;overflow-y:auto;font-family:'Courier New', monospace;font-size:12px;color:#cbd5e1;line-height:1.5;margin-top:10px}
.log-line{border-bottom:1px solid #1e293b;padding-bottom:4px;margin-bottom:4px}
.info-box{background:rgba(51,65,85,0.4);padding:15px;border-radius:10px;font-size:13px;color:#cbd5e1;line-height:1.6;margin-top:10px}
.info-group{margin-bottom:15px}
.info-title{color:#a5b4fc;font-weight:700;display:block;margin-bottom:4px;font-family:monospace}
.info-list{padding-left:18px;margin:4px 0 0 0;font-size:11px;color:#94a3b8}
.tooltip{position:relative;display:inline-block;cursor:pointer;margin-left:5px;color:var(--acc);}
.tooltip .tt-txt{visibility:hidden;width:260px;background-color:#1e293b;color:#e0e0e0;text-align:left;border-radius:8px;padding:12px;position:absolute;z-index:100;bottom:130%;left:50%;margin-left:-130px;opacity:0;transition:opacity 0.2s;font-size:11px;line-height:1.5;box-shadow:0 10px 25px rgba(0,0,0,0.6);border:1px solid #475569}
.tooltip:hover .tt-txt{visibility:visible;opacity:1}
.tt-h{color:#fff;font-weight:bold;margin-bottom:4px;display:block;border-bottom:1px solid #475569;padding-bottom:2px;margin-top:6px}
.tt-h:first-child{margin-top:0}
.tt-r{display:flex;justify-content:space-between;padding:1px 0;color:#94a3b8}
.tt-r span:last-child{color:var(--acc);font-weight:bold}
.calc-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin-top:12px}
.c-btn{padding:12px 0;font-size:12px;font-weight:bold;background:#334155;border:1px solid #475569;color:#e2e8f0;border-radius:8px;cursor:pointer}
.c-btn:active{transform:scale(0.96)}
.c-var{background:rgba(99,102,241,0.2);color:#818cf8;border-color:rgba(99,102,241,0.4)}
.c-act{background:#f43f5e;color:white;border-color:#f43f5e}
.db-tool{position:relative;margin-bottom:10px}
.sug-box{position:absolute;top:100%;left:0;width:100%;max-height:200px;overflow-y:auto;background:#1e293b;border:1px solid #475569;border-radius:0 0 10px 10px;z-index:50;display:none;box-shadow:0 10px 25px rgba(0,0,0,0.5)}
.sug-item{padding:12px;border-bottom:1px solid #334155;cursor:pointer;transition:0.2s}
.sug-item:hover{background:#334155}
.sug-item span{display:block;font-weight:700;color:white}
.sug-ctry{font-size:11px;color:#94a3b8;font-weight:400!important;margin-top:2px}
#pwr{cursor:pointer;transition:0.3s;color:#475569}
#pwr.on{color:#c084fc;filter:drop-shadow(0 0 8px rgba(192,132,252,0.6))}
</style></head>
<body>
<div class="app">
  <h1>SURF BEACON <span id="pwr" onclick="tgPwr(this)"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg></span></h1>
  <div class="tabs"><div class="tab active" onclick="sw(0)">WAVES</div><div class="tab" onclick="sw(1)">LAMP</div><div class="tab" onclick="sw(2)">CONFIG</div></div>
  <div id="p0">
    <div id="dbStat" style="text-align:center;font-size:11px;color:#64748b;margin-bottom:15px;font-weight:600">📡 Connecting...</div>
    <div id="list"></div>
    <div onclick="addSpot(null, true)" style="text-align:center;padding:18px;border:2px dashed #334155;border-radius:16px;color:#94a3b8;cursor:pointer;margin-top:20px;font-weight:600;transition:0.2s;background:rgba(255,255,255,0.02)">+ ADD NEW SPOT</div>
  </div>
  <div id="p1" class="hidden">
    <div class="card">
      <div class="head"><span>FALLBACK MODE</span><div id="lEn" class="switch" onclick="tg(this)"></div></div>
      <label>Animation</label>
      <select id="lEff" onchange="updUI()" class="anim-sel"></select>
      <div id="colBox" style="margin-top:12px"><label>Color</label><input type="color" id="lCol"></div>
      <div style="margin-top:12px"><label>Animation Speed (<span id="spdVal" style="color:var(--acc)">128</span>)</label><input type="range" id="lSpd" min="1" max="255" oninput="document.getElementById('spdVal').innerText=this.value"></div>
      <div style="margin-top:15px;text-align:center;font-size:11px;color:#64748b">Brightness is controlled via the 'Config' tab.</div>
    </div>
  </div>
  <div id="p2" class="hidden">
    <div class="card">
      <div class="head"><span>LED & LIGHTING</span></div>
      
      <div style="margin-bottom:20px;padding-bottom:15px;border-bottom:1px solid #334155">
         <label>Master Brightness (Surf & Lamp)</label>
         <input type="range" id="mBri" min="5" max="255" oninput="document.getElementById('mBriVal').innerText=this.value">
         <div style="text-align:right;font-size:12px;color:#94a3b8;margin-top:4px">Value: <span id="mBriVal" style="color:var(--acc);font-weight:bold">255</span></div>
      </div>

      <div style="margin-bottom:25px">
         <span class="sub-head">Standard Conditions</span>
         <div class="grid-2">
            <div><label>Cycle Time (s)</label><input type="number" id="cycT" min="2"></div>
            <div><label>Animation</label><select id="sAnim" class="anim-sel"></select></div>
         </div>
         <div style="margin-top:10px;font-size:11px;color:#64748b">ℹ️ Speed is auto-calculated based on surf score (Gentle → Strong).</div>
      </div>
      <div class="epic-box">
         <div class="epic-head">🔥 Epic Conditions</div>
         <label>Animation</label><select id="eAnim" onchange="updEpicUI()" class="anim-sel"></select>
         <div id="eColBox" style="margin-top:12px"><label>Color</label><input type="color" id="eCol"></div>
         <div style="margin-top:12px"><label>Effect Speed</label><input type="range" id="eSpd" min="1" max="255"></div>
      </div>
      <div>
         <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px">
             <span class="sub-head" style="margin:0">Night Mode (DND)</span>
             <div id="dndEn" class="switch" onclick="tg(this)"></div>
         </div>
         <div class="grid-2">
            <div><label>Quiet Start (Hour)</label><input type="number" id="dndS" min="0" max="23" placeholder="23"></div>
            <div><label>Quiet End (Hour)</label><input type="number" id="dndE" min="0" max="23" placeholder="7"></div>
         </div>
      </div>
    </div>
    <div class="card">
      <div class="head"><span>TELEGRAM NOTIFICATIONS</span><div id="tgEn" class="switch" onclick="tg(this)"></div></div>
      <div style="margin-bottom:12px">
        <label>Notify When...</label>
        <select id="tgM" onchange="updTgUI()"><option value="0">Any Good Surf (> Min)</option><option value="1">Epic Surf Only (> Epic)</option><option value="2">Custom Global Score</option></select>
      </div>
      <div id="tgCusBox" style="display:none;margin-bottom:12px;background:rgba(99,102,241,0.1);padding:12px;border-radius:10px;border:1px solid #6366f1">
        <label>Alert if Score is Above:</label><input type="number" id="tgTh" placeholder="e.g. 40">
      </div>
      <div class="grid-2"><input type="text" id="tgT" placeholder="Token"><input type="text" id="tgC" placeholder="ChatID"></div>
    </div>
    <div class="card">
      <div class="head"><span>PRO FORMULA BUILDER</span><div id="pfEn" class="switch" onclick="tg(this); updCalcUI()"></div></div>
      <div id="pfInfo" class="info-box">
         <div class="info-group">
            <span class="info-title">1. Default Logic</span>
            <div style="font-size:11px;color:#94a3b8;margin-bottom:4px">Used when "Pro Mode" is OFF for a spot.</div>
            <div style="font-family:monospace;color:#a5b4fc">Energy = (Height² × Period) × 1.5</div>
         </div>
         <div class="info-group" style="margin-bottom:0">
            <span class="info-title">2. Advanced Logic</span>
            <div style="font-size:11px;color:#94a3b8;margin-bottom:4px">Active when you enable "Pro Mode" on a spot.</div>
            <div style="font-family:monospace;color:#a5b4fc;margin-bottom:8px">Score = Energy × Wind × Angle</div>
            
            <div style="margin-top:6px;font-weight:700;color:#cbd5e1;font-size:11px">📐 Swell Angle Tolerance</div>
            <div style="font-size:11px;color:#94a3b8">Must be ±45° from selected directions.</div>

            <div style="margin-top:6px;font-weight:700;color:#cbd5e1;font-size:11px">🌬️ Wind Multipliers</div>
            <ul class="info-list" style="margin-top:2px">
               <li><b>Offshore (±45°):</b> &lt;15kph (1.3x) | &lt;30kph (1.1x) | Else (0.9x)</li>
               <li><b>Cross-shore (±100°):</b> &lt;12kph (0.9x) | Else (0.6x)</li>
               <li><b>Onshore:</b> &lt;10kph (0.8x) | &lt;20kph (0.4x) | Else (0.2x)</li>
            </ul>
         </div>
         <div style="margin-top:15px;background:rgba(99,102,241,0.1);padding:10px;border-radius:8px;font-size:11px;border:1px solid rgba(99,102,241,0.3)">
            ℹ️ <b>Global Override:</b> Toggle this switch ON to replace ALL logic above with a custom equation.
         </div>
      </div>
      <div id="pfBox" style="display:none">
        <label>Custom Equation (Overrides Default)</label>
        <input type="text" id="pForm" placeholder="e.g. h * p * off">
        <div class="calc-grid">
           <button class="c-btn c-var" onclick="ins('h')">🌊 Height</button>
           <button class="c-btn c-var" onclick="ins('p')">⏳ Period</button>
           <button class="c-btn c-var" onclick="ins('w')">🌬️ Wind</button>
           <button class="c-btn c-var" onclick="ins('off')">✅ Offshr</button>
           <button class="c-btn" onclick="ins('+')">+</button>
           <button class="c-btn" onclick="ins('-')">-</button>
           <button class="c-btn" onclick="ins('*')">*</button>
           <button class="c-btn" onclick="ins('/')">/</button>
           <button class="c-btn" onclick="ins('(')">(</button>
           <button class="c-btn" onclick="ins(')')">)</button>
           <button class="c-btn" onclick="ins('.')">.</button>
           <button class="c-btn c-act" onclick="document.getElementById('pForm').value=''">CLR</button>
        </div>
      </div>
    </div>
    <div class="card">
      <div class="head"><span>SYSTEM & UPDATES</span></div>
      <div class="grid-2">
         <div>
            <label>Update Frequency</label>
            <select id="updF"><option value="30">30 Mins</option><option value="60">1 Hour</option><option value="120">2 Hours</option><option value="180">3 Hours</option><option value="360">6 Hours</option><option value="720">12 Hours</option><option value="1440">24 Hours</option></select>
         </div>
         <div>
            <label>Time Zone</label>
            <select id="tz"></select>
         </div>
      </div>
      <label style="margin-top:15px">System Log (Live Buffer)</label>
      <div class="log-box" id="sysLog"><div class="log-line">Loading logs...</div></div>
      <div style="margin-top:10px;font-size:11px;color:#64748b;text-align:center">Device runs on GMT time internally.</div>
    </div>
  </div>
</div>
<button class="fab" onclick="save()">NO CHANGES</button>
<script>
const dirs=['N','NE','E','SE','S','SW','W','NW'];
const rndCols=['#ef4444','#f97316','#f59e0b','#84cc16','#10b981','#06b6d4','#3b82f6','#8b5cf6','#d946ef','#f43f5e'];
const anims = [
  {v:0, t:"Static Light"},
  {v:1, t:"Breathe (Pulse)"},
  {v:2, t:"Tide (Flowing)"},
  {v:3, t:"Coastal (Plasma)"},
  {v:4, t:"Swell (Gradient)"},
  {v:5, t:"Breaker (Comet)"},
  {v:6, t:"Rainbow Cycle"}
];
document.querySelectorAll('.anim-sel').forEach(s => {
  anims.forEach(a => { let opt = document.createElement('option'); opt.value = a.v; opt.text = a.t; s.appendChild(opt); });
});

const DB_URL = "https://gist.githubusercontent.com/naotokui/01c384bf58ca43261eafe6a5e2ad6e85/raw/fbd3c85bea04c7837403c0b701d37c1af25fe208/surfspots.json";
let db = []; let countries = new Set();
const tzSel = document.getElementById('tz');
for(let i=-12; i<=14; i++){ let txt = i>=0 ? `GMT+${i}` : `GMT${i}`; let opt = document.createElement('option'); opt.value = i; opt.text = txt; tzSel.appendChild(opt); }
fetch(DB_URL).then(r=>r.json()).then(data => {
  db = data.map(x => { let rawC = x.Country || x.country || "World"; let cleanC = rawC.split(',')[0].trim(); return { n: x.Spot || x.spot || x.Name || x.name || "Unknown", c: cleanC, full_c: rawC, la: x.Latitude || x.latitude || x.lat || x.Lat, lo: x.Longitude || x.longitude || x.lon || x.Lon || x.lng || x.Lng }; }).filter(x => x.la && x.lo);
  db.forEach(x => countries.add(x.c));
  document.getElementById('dbStat').innerText = `✅ ${db.length} Spots | ${countries.size} Countries`;
}).catch(e => { console.error(e); document.getElementById('dbStat').innerText = "⚠️ Database Offline"; });
function sw(t){document.querySelectorAll('.tab').forEach((e,i)=>e.classList.toggle('active',i===t));for(let i=0;i<3;i++)document.getElementById('p'+i).classList.toggle('hidden',i!==t);}
function tg(el){el.classList.toggle('on'); markDirty();}
function tgPwr(el){el.classList.toggle('on'); save(); }
function tgAdv(el){ const header = el.parentNode; const box = header.nextElementSibling; el.classList.toggle('on'); box.style.display = el.classList.contains('on') ? 'block' : 'none'; markDirty(); }
function tgDir(btn){ btn.classList.toggle('active'); markDirty(); }
function updUI(){const eff=document.getElementById('lEff').value;document.getElementById('colBox').style.display=(eff==6)?'none':'block';}
function updEpicUI(){const eff=document.getElementById('eAnim').value;document.getElementById('eColBox').style.display=(eff==6)?'none':'block';}
function updTgUI(){const m=document.getElementById('tgM').value;document.getElementById('tgCusBox').style.display=(m==2)?'block':'none';}
function updCalcUI(){const on=document.getElementById('pfEn').classList.contains('on'); document.getElementById('pfBox').style.display = on ? 'block' : 'none'; document.getElementById('pfInfo').style.display = on ? 'none' : 'block';}
function ins(v){ const el = document.getElementById('pForm'); el.value += v; markDirty(); }
function filter(inp) { const container = inp.closest('.db-tool'); const selCtry = container.querySelector('.ctry-sel').value; const box = container.querySelector('.sug-box'); const val = inp.value.toLowerCase(); box.innerHTML = ''; const matches = db.filter(s => { const matchCtry = (selCtry === "" || s.c === selCtry); const matchName = (val.length === 0 || s.n.toLowerCase().includes(val)); return matchCtry && matchName; }).slice(0, 100); if(matches.length > 0 && (val.length >= 1 || selCtry !== "")) { box.style.display='block'; matches.forEach(m => { const d = document.createElement('div'); d.className='sug-item'; d.innerHTML = `<span>${m.n}</span><span class="sug-ctry">${m.full_c}</span>`; d.onclick = () => { const spotEl = inp.closest('.spot'); spotEl.querySelector('.spot-nm').value = m.n + " (" + m.c + ")"; spotEl.querySelector('.la').value = m.la; spotEl.querySelector('.lo').value = m.lo; container.style.display='none'; markDirty(); }; box.appendChild(d); }); } else { box.style.display='none'; } }
function mkSpot(s, showDbTool = false){ 
    let btns=''; for(let i=0;i<8;i++){ let m = s.mask || 0; let active = (m >> i) & 1 ? 'active' : ''; btns += `<div class="dir-btn ${active}" onclick="tgDir(this)" data-v="${1<<i}">${dirs[i]}</div>`; } 
    let wOpts=''; dirs.forEach((d,i)=>{ let wd = s.wDir || 0; wOpts+=`<option value="${i*45}" ${wd==i*45?'selected':''}>${d}</option>`; }); 
    let cOpts = '<option value="">All Countries</option>'; Array.from(countries).sort().forEach(c => cOpts += `<option value="${c}">${c}</option>`); 
    const div=document.createElement('div');div.className='spot'; 
    const dbStyle = showDbTool ? 'display:block' : 'display:none'; 
    div.innerHTML=`<div class="spot-top"><input class="spot-nm" value="${s.name}" placeholder="Spot Name"><div class="switch ${s.enabled?'on':''}" onclick="tg(this)"></div></div>
    <div class="db-tool" style="${dbStyle}"><div style="font-size:10px;text-transform:uppercase;color:#6366f1;font-weight:900;margin-bottom:5px">Find in Library</div><div class="grid-2" style="margin-bottom:5px"><select class="ctry-sel" onchange="filter(this.parentElement.nextElementSibling)">${cOpts}</select><input placeholder="Search name..." oninput="filter(this)"></div><div class="sug-box"></div></div>
    <div class="grid-2"><input class="la" value="${s.lat}" placeholder="Lat"><input class="lo" value="${s.lon}" placeholder="Lon"></div>
    <div style="margin-top:5px;margin-bottom:10px"><input type="color" class="clr" value="${s.col}"></div>
    <div style="display:flex;justify-content:space-between;align-items:center;background:rgba(255,255,255,0.05);padding:10px;border-radius:10px;margin-top:10px"><div><label>Forecast Days</label><input type="number" class="dy" value="${s.days}" style="width:60px;margin:0" min="1" max="7"></div><div style="text-align:right"><label>Max Score</label><span style="color:var(--acc);font-weight:900;font-size:20px">${s.pow?s.pow.toFixed(0):0}</span></div></div>
    <div style="background:rgba(255,255,255,0.05);padding:10px;border-radius:10px;margin-top:10px"><div style="margin-bottom:8px;font-size:11px;font-weight:800;color:#cbd5e1;display:flex;align-items:center">NOTIFY ME WHEN SCORE IS:<div class="tooltip">ℹ️<div class="tt-txt"><span class="tt-h">🌊 OCEAN (HI, Indo)</span><div class="tt-r"><span>Fair:</span> <span>> 10</span></div><div class="tt-r"><span>Good:</span> <span>> 30</span></div><div class="tt-r"><span>Epic:</span> <span>> 80</span></div><span class="tt-h">🌎 OPEN (Euro, US)</span><div class="tt-r"><span>Fair:</span> <span>> 5</span></div><div class="tt-r"><span>Good:</span> <span>> 15</span></div><div class="tt-r"><span>Epic:</span> <span>> 50</span></div><span class="tt-h">💧 SEA (Med, Baltic)</span><div class="tt-r"><span>Fair:</span> <span>> 2</span></div><div class="tt-r"><span>Good:</span> <span>> 6</span></div><div class="tt-r"><span>Epic:</span> <span>> 25</span></div></div></div></div><div class="grid-2"><div><label>Min Score</label><input type="number" class="t-min" value="${s.tMin}"></div><div><label>Epic Score</label><input type="number" class="t-epic" value="${s.tEpic}"></div></div></div>
    <div style="background:rgba(255,255,255,0.05);padding:10px;border-radius:10px;margin-top:10px"><div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:0px"><span style="font-size:11px;font-weight:800;color:#cbd5e1">PRO MODE</span><div class="switch ${s.adv?'on':''}" onclick="tgAdv(this)"></div></div><div class="pro-box" style="display:${s.adv?'block':'none'}"><label style="margin-top:0">Best Swell Directions</label><div class="compass-grid">${btns}</div><label>Ideal Offshore Wind From</label><select class="wind-sel">${wOpts}</select></div></div>
    <div style="text-align:right;margin-top:10px"><button class="del" onclick="this.closest('.spot').remove(); markDirty();">DELETE SPOT</button></div>`; 
    return div; 
}
function addSpot(s=null, openTool=false){ if(!s) { const rndColor = rndCols[Math.floor(Math.random()*rndCols.length)]; s={name:"",lat:"",lon:"",enabled:true,days:3,pow:0,col:rndColor,adv:false,mask:0,wDir:0,tMin:15,tEpic:50}; } const el = mkSpot(s, openTool); document.getElementById('list').appendChild(el); if(!s && !openTool) markDirty(); }
function markDirty() { const b = document.querySelector('.fab'); b.classList.add('dirty'); b.innerText = "SAVE CHANGES"; }
document.addEventListener('input', (e) => { if(e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') markDirty(); });
function loadData() {
  fetch('/api/data').then(r=>r.json()).then(d=>{ 
    document.getElementById('cycT').value=d.cycT; document.getElementById('sAnim').value=d.sAnim; document.getElementById('eAnim').value=d.eAnim; document.getElementById('eCol').value=d.eCol; document.getElementById('eSpd').value=d.eSpd; 
    if(d.gLed)document.getElementById('pwr').classList.add('on'); else document.getElementById('pwr').classList.remove('on');
    if(d.tgEn)document.getElementById('tgEn').classList.add('on'); else document.getElementById('tgEn').classList.remove('on');
    document.getElementById('tgM').value=d.tgM || 0; document.getElementById('tgTh').value=d.tgTh || 30; document.getElementById('tgT').value=d.tgT;document.getElementById('tgC').value=d.tgC; 
    if(d.lEn)document.getElementById('lEn').classList.add('on'); else document.getElementById('lEn').classList.remove('on');
    document.getElementById('lEff').value=d.lEff; document.getElementById('lCol').value=d.lCol; document.getElementById('lSpd').value=d.lSpd; document.getElementById('spdVal').innerText=d.lSpd; 
    if(d.dndEn)document.getElementById('dndEn').classList.add('on'); else document.getElementById('dndEn').classList.remove('on');
    document.getElementById('tz').value = d.tz || 1; document.getElementById('dndS').value = d.dndS; document.getElementById('dndE').value = d.dndE; 
    if(d.pfEn)document.getElementById('pfEn').classList.add('on'); else document.getElementById('pfEn').classList.remove('on');
    document.getElementById('pForm').value = d.pForm || ""; document.getElementById('updF').value = d.uFreq || 60; document.getElementById('mBri').value = d.mBri || 255; document.getElementById('mBriVal').innerText = d.mBri || 255; 
    let logH=""; d.logs.forEach(l => logH += `<div class="log-line">${l}</div>`); if(logH) document.getElementById('sysLog').innerHTML=logH; else document.getElementById('sysLog').innerHTML="<div class='log-line'>No logs yet.</div>"; 
    document.getElementById('list').innerHTML = '';
    d.spots.forEach(s => addSpot(s, false)); 
    if(d.spots.length === 0) { addSpot(null, true); } 
    updUI();updEpicUI(); updTgUI(); updCalcUI();
    document.getElementById('dbStat').innerText = "✅ System Online";
    const b = document.querySelector('.fab'); b.classList.remove('dirty'); b.innerText = "NO CHANGES";
  }).catch(e => { console.error("Waiting for device..."); setTimeout(loadData, 2000); });
}
function save(){ 
  const b = document.querySelector('.fab'); b.innerText="SAVING..."; 
  const spots=[]; document.querySelectorAll('.spot').forEach(el=>{ let m=0; el.querySelectorAll('.dir-btn.active').forEach(b=>{ m += parseInt(b.dataset.v); }); const switches = el.querySelectorAll('.switch'); const advSwitch = switches[switches.length-1]; spots.push({ name:el.querySelector('.spot-nm').value, lat:el.querySelector('.la').value,lon:el.querySelector('.lo').value, enabled:switches[0].classList.contains('on'), days:parseInt(el.querySelector('.dy').value), col:el.querySelector('.clr').value, adv:advSwitch.classList.contains('on'), mask: m, wDir: parseInt(el.querySelector('.wind-sel').value), tMin: parseInt(el.querySelector('.t-min').value), tEpic: parseInt(el.querySelector('.t-epic').value) }); }); 
  fetch('/api/save',{method:'POST',body:JSON.stringify({ gLed:document.getElementById('pwr').classList.contains('on'), cycT:document.getElementById('cycT').value, sAnim:document.getElementById('sAnim').value, eAnim:document.getElementById('eAnim').value, eCol:document.getElementById('eCol').value, eSpd:document.getElementById('eSpd').value, tgEn:document.getElementById('tgEn').classList.contains('on'), tgM:document.getElementById('tgM').value, tgTh:document.getElementById('tgTh').value, tgT:document.getElementById('tgT').value,tgC:document.getElementById('tgC').value, lEn:document.getElementById('lEn').classList.contains('on'), lCol:document.getElementById('lCol').value, lEff:document.getElementById('lEff').value, lSpd:document.getElementById('lSpd').value, dndEn:document.getElementById('dndEn').classList.contains('on'), tz:document.getElementById('tz').value, dndS:document.getElementById('dndS').value, dndE:document.getElementById('dndE').value, pfEn:document.getElementById('pfEn').classList.contains('on'), pForm:document.getElementById('pForm').value, uFreq:document.getElementById('updF').value, mBri: document.getElementById('mBri').value, spots:spots })})
  .then(r=>{ if(r.ok){ b.innerText="REBOOTING..."; setTimeout(loadData, 4000); } else { b.innerText="ERROR"; } }).catch(e=>b.innerText="RETRYING..."); 
}
loadData();
</script></body></html>
)rawliteral";

// ═══════════════════════════════════════════════════════════
// 🧠 OCEANOGRAPHER LOGIC
// ═══════════════════════════════════════════════════════════

CRGB hexToCRGB(String hex) {
  if (hex.startsWith("#")) hex.remove(0, 1);
  long number = strtol(hex.c_str(), NULL, 16);
  return CRGB(number >> 16, (number >> 8) & 0xFF, number & 0xFF);
}

int angleDiff(int a, int b) {
  int d = abs(a - b);
  if (d > 180) d = 360 - d;
  return d;
}

bool isSwellDirectionGood(int actualDir, int mask) {
  if (mask == 0) return true; 
  for (int i = 0; i < 8; i++) {
    if ((mask >> i) & 1) { 
      int targetAngle = i * 45; 
      if (angleDiff(actualDir, targetAngle) <= 45) return true;
    }
  }
  return false;
}

float calculateSurfQuality(float swH, float swP, float swDir, float windWaveH, float wSpd, float wDir, Spot &s) {
  float offshoreFactor = 1.0;
  if (s.useAdvanced) {
      int diffWind = angleDiff((int)wDir, s.bestWindDir);
      if (diffWind <= 45) offshoreFactor = 1.0;          
      else if (diffWind <= 90) offshoreFactor = 0.5;     
      else offshoreFactor = 0.0;                         
      if (offshoreFactor < 1.0 && wSpd > 20) offshoreFactor = 0.0;
  }

  if (conf.proFormulaEnabled && conf.proFormula.length() > 2) {
      te_variable vars[] = {
          {"h", &swH}, 
          {"p", &swP}, 
          {"w", &wSpd}, 
          {"off", &offshoreFactor}
      };
      
      int err;
      te_expr *n = te_compile(conf.proFormula.c_str(), vars, 4, &err);
      
      if (n) {
          double res = te_eval(n);
          te_free(n);
          if (res >= 0 && res < 10000) return (float)res; 
      }
  }

  float energy = (swH * swH) * swP * 1.5;

  if (!s.useAdvanced) {
    if (wSpd > 35.0) return energy * 0.4; 
    if (wSpd > 20.0) return energy * 0.8; 
    return energy;
  }

  if (!isSwellDirectionGood((int)swDir, s.swellMask)) return 0;

  float windMultiplier = 1.0;
  int diffWind = angleDiff((int)wDir, s.bestWindDir);
  if (diffWind <= 45) { 
    if (wSpd < 15.0) windMultiplier = 1.3; else if (wSpd < 30.0) windMultiplier = 1.1; else windMultiplier = 0.9; 
  } else if (diffWind <= 100) { 
    if (wSpd < 12.0) windMultiplier = 0.9; else windMultiplier = 0.6; 
  } else { 
    if (wSpd < 10.0) windMultiplier = 0.8; else if (wSpd < 20.0) windMultiplier = 0.4; else windMultiplier = 0.2; 
  }
  
  float chopFactor = 1.0;
  if (swH > 0.5) { 
     float ratio = windWaveH / swH;
     if (ratio > 0.8) chopFactor = 0.6; else if (ratio > 0.5) chopFactor = 0.8; 
  }

  return energy * windMultiplier * chopFactor;
}

// ═══════════════════════════════════════════════════════════
// 🆕 HISTORY / EVENT PERSISTENCE
// ═══════════════════════════════════════════════════════════

void loadHistory() {
  if (LittleFS.exists("/history.json")) {
    File f = LittleFS.open("/history.json", "r");
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, f);
    f.close();
    
    if (!error) {
      logSys("📂 HISTORY LOADED");
      for (auto& s : spots) {
        if (doc.containsKey(s.name)) {
          s.lastNotifiedID = doc[s.name].as<String>();
        }
      }
    }
  }
}

void saveHistory() {
  DynamicJsonDocument doc(4096);
  for (const auto& s : spots) {
    if (s.lastNotifiedID.length() > 0) {
      doc[s.name] = s.lastNotifiedID;
    }
  }
  File f = LittleFS.open("/history.json", "w");
  serializeJson(doc, f);
  f.close();
}

// ═══════════════════════════════════════════════════════════
// 🌐 WEATHER API
// ═══════════════════════════════════════════════════════════

struct DaySwell {
  String date;
  float h, p, d, ww; 
};

String getHumanTimeRange(const std::vector<int>& hours) {
  if (hours.empty()) return "No specific time";
  bool em = false, lm = false, af = false, ev = false;
  for (int h : hours) {
    if (h >= 6 && h < 9) em = true;
    else if (h >= 9 && h < 12) lm = true;
    else if (h >= 12 && h < 17) af = true;
    else if (h >= 17 && h <= 21) ev = true;
  }
  String res = "";
  if (em) res += "Early Morning";
  if (lm) res += (res.length() > 0 ? ", " : "") + String("Late Morning");
  if (af) res += (res.length() > 0 ? ", " : "") + String("Afternoon");
  if (ev) res += (res.length() > 0 ? ", " : "") + String("Evening");
  int commas = 0;
  for(int i=0; i<res.length(); i++) if(res.charAt(i) == ',') commas++;
  if (commas > 0) {
    int lastComma = res.lastIndexOf(',');
    res = res.substring(0, lastComma) + " &" + res.substring(lastComma + 1);
  }
  return res;
}

void updateForecast() {
  logSys("Starting Forecast Update...");
  
  if (WiFi.status() != WL_CONNECTED) { 
    logSys("Error: WiFi Disconnected");
    return; 
  }

  HTTPClient http;
  http.setTimeout(12000); 
  
  bool foundWaves = false;
  bool anyError = false;
  
  for(int i=0; i<spots.size(); i++) {
    if(!spots[i].enabled || spots[i].lat.length() < 2) continue;
    yield();
    logSys("🔎 ANALYZING: " + spots[i].name);
    
    std::vector<DaySwell> swellData;
    String marineUrl = "http://marine-api.open-meteo.com/v1/marine?latitude=" + spots[i].lat + 
                 "&longitude=" + spots[i].lon + 
                 "&daily=swell_wave_height_max,swell_wave_period_max,swell_wave_direction_dominant,wind_wave_height_max&timezone=auto";
    
    bool marineSuccess = false;
    http.begin(marineUrl);
    int cM = http.GET();
    
    if(cM == 200) {
      DynamicJsonDocument* dM = new DynamicJsonDocument(24000); 
      if (dM) {
        DeserializationError error = deserializeJson(*dM, http.getString());
        if(!error) {
           JsonArray t = (*dM)["daily"]["time"];
           JsonArray sh = (*dM)["daily"]["swell_wave_height_max"];
           JsonArray sp = (*dM)["daily"]["swell_wave_period_max"];
           JsonArray sd = (*dM)["daily"]["swell_wave_direction_dominant"];
           JsonArray ww = (*dM)["daily"]["wind_wave_height_max"];
           for(int k=0; k<t.size() && k<7; k++) swellData.push_back({t[k].as<String>(), sh[k], sp[k], sd[k], ww[k]});
           marineSuccess = true;
        } 
        delete dM; 
      }
    }
    http.end();

    if(!marineSuccess || swellData.empty()) { 
      logSys("❌ Marine Failed: " + spots[i].name);
      anyError = true;
      continue; 
    }

    String weatherUrl = "http://api.open-meteo.com/v1/forecast?latitude=" + spots[i].lat + 
                 "&longitude=" + spots[i].lon + 
                 "&hourly=wind_speed_10m,wind_direction_10m&timezone=auto";

    http.begin(weatherUrl);
    int cW = http.GET();
    
    float globalMaxScore = -1.0;
    String bestTotalDate = "";
    std::vector<int> bestDayHours; 

    if(cW == 200) {
      DynamicJsonDocument* dW = new DynamicJsonDocument(48000); 
      if (dW) {
        DeserializationError error = deserializeJson(*dW, http.getString());
        if(!error) {
           JsonArray wS = (*dW)["hourly"]["wind_speed_10m"];
           JsonArray wD = (*dW)["hourly"]["wind_direction_10m"];
           int daysToCheck = constrain(spots[i].forecastDays, 1, 7);
           for(int dayIdx = 0; dayIdx < daysToCheck; dayIdx++) {
              if(dayIdx >= swellData.size()) break;
              float dayMaxScore = -1.0;
              std::vector<int> currentDayHours;
              for(int h = 6; h <= 20; h++) { 
                 int globalHourIdx = (dayIdx * 24) + h;
                 if(globalHourIdx >= wS.size()) break;
                 float score = calculateSurfQuality(swellData[dayIdx].h, swellData[dayIdx].p, swellData[dayIdx].d, swellData[dayIdx].ww, wS[globalHourIdx], wD[globalHourIdx], spots[i]);
                 if(score > dayMaxScore) dayMaxScore = score;
              }
              for(int h = 6; h <= 20; h++) { 
                 int globalHourIdx = (dayIdx * 24) + h;
                 if(globalHourIdx >= wS.size()) break;
                 float score = calculateSurfQuality(swellData[dayIdx].h, swellData[dayIdx].p, swellData[dayIdx].d, swellData[dayIdx].ww, wS[globalHourIdx], wD[globalHourIdx], spots[i]);
                 if(score >= (dayMaxScore * 0.85) && score >= spots[i].minThreshold) {
                    currentDayHours.push_back(h);
                 }
              }
              if(dayMaxScore > globalMaxScore) {
                  globalMaxScore = dayMaxScore;
                  bestTotalDate = swellData[dayIdx].date;
                  bestDayHours = currentDayHours;
              }
           }
        }
        delete dW;
      }
    }
    http.end();
    
    spots[i].surfPower = globalMaxScore;
    spots[i].bestDayDate = bestTotalDate;
    spots[i].humanTimeRange = getHumanTimeRange(bestDayHours);
    spots[i].isFiring = (globalMaxScore >= spots[i].minThreshold);

    String logInfo = "👉 Score: " + String(globalMaxScore, 1);
    if(spots[i].isFiring) logInfo += " (FIRING!)";
    logSys(logInfo);

    if (spots[i].isFiring && conf.tgEnabled) { 
        String currentEventID = spots[i].bestDayDate + "_" + String(globalMaxScore,0); 
        bool shouldNotify = false;
        if (conf.tgMode == 0) shouldNotify = (globalMaxScore >= spots[i].minThreshold);
        else if (conf.tgMode == 1) shouldNotify = (globalMaxScore >= spots[i].epicThreshold);
        else if (conf.tgMode == 2) shouldNotify = (globalMaxScore >= conf.tgThreshold);

        if (shouldNotify && spots[i].lastNotifiedID != currentEventID) {
             if(bot != nullptr && conf.botToken.length() > 5) {
                logSys("🔔 Notifying Telegram...");
                String msg = "🏄‍♂️ *SURF CALLING!* \n\n";
                msg += "📍 " + spots[i].name + "\n";
                msg += "📅 " + spots[i].bestDayDate + "\n";
                msg += "🕒 " + spots[i].humanTimeRange + "\n"; 
                msg += "🌊 Score: " + String(globalMaxScore, 0) + "\n";
                if (globalMaxScore > spots[i].epicThreshold) msg += "🔥 EPIC CONDITIONS!";
                else msg += "😎 Looking good.";
                bot->sendMessage(conf.chatId, msg, "");
                spots[i].lastNotifiedID = currentEventID;
                saveHistory(); 
            }
        }
        foundWaves = true;
    }
    delay(100); 
  }
  if (!anyError) logSys("Update Finished Successfully");
  anySpotFiring = foundWaves;
}

// ═══════════════════════════════════════════════════════════
// 🎨 UNIFIED LED ENGINE (REFINED LOOPING)
// ═══════════════════════════════════════════════════════════

bool isNightMode() {
  if (!conf.dndEnabled) return false;
  int currentHour = timeClient.getHours();
  if (conf.dndStart > conf.dndEnd) return (currentHour >= conf.dndStart || currentHour < conf.dndEnd);
  else return (currentHour >= conf.dndStart && currentHour < conf.dndEnd);
}

// Helper for "Coastal" effect
void coastal_effect(CRGB baseColor, uint8_t speed) {
  uint32_t t = millis() * (speed / 4); 
  
  for(int i = 0; i < NUM_LEDS; i++) {
    uint8_t noise = inoise8(i * 30, t / 10);
    CRGB highlight = CRGB::White;
    CRGB med = baseColor; med.nscale8(180);
    CRGBPalette16 p = CRGBPalette16(baseColor, med, highlight, baseColor);
    leds[i] = ColorFromPalette(p, noise, 255, LINEARBLEND);
  }
}

void renderEffect(uint8_t effectType, CRGB baseColor, uint8_t speed, uint8_t brightness) {
  
  FastLED.setBrightness(brightness);

  // Normalization: speed 1-255 -> BPM 5-60
  uint8_t bpm = map(speed, 1, 255, 5, 60);

  switch(effectType) {
    case 0: // STATIC LIGHT
      fill_solid(leds, NUM_LEDS, baseColor);
      break;

    case 1: // BREATHE (Pulse)
      {
        fill_solid(leds, NUM_LEDS, baseColor);
        uint8_t dim = beatsin8(bpm, 50, 255); 
        for(int i=0; i<NUM_LEDS; i++) leds[i].nscale8(dim);
      }
      break;

    case 2: // TIDE (Flowing In/Out)
      {
        uint8_t phase = beat8(bpm);
        for(int i=0; i<NUM_LEDS; i++) {
           uint8_t wave = sin8(phase + (i * 10));
           CRGB c1 = baseColor;
           CRGB c2 = baseColor; c2 += CRGB(80,80,80); 
           leds[i] = blend(c1, c2, wave);
        }
      }
      break;

    case 3: // COASTAL (Plasma/Water)
      coastal_effect(baseColor, speed);
      break;

    case 4: // SWELL (Gradient)
      {
        uint8_t pos = beat8(bpm);
        for(int i=0; i<NUM_LEDS; i++) {
          uint8_t dist = sin8(pos + (i * 16)); 
          leds[i] = baseColor;
          leds[i].nscale8(map(dist, 0, 255, 40, 255));
        }
      }
      break;

    case 5: // BREAKER (Comet/Shooting Star)
      {
        fadeToBlackBy(leds, NUM_LEDS, 20);
        uint16_t pos = beat16(bpm); 
        int pixelIndex = scale16(pos, NUM_LEDS); 
        
        if(pixelIndex < NUM_LEDS) {
          leds[pixelIndex] = CRGB::White;
        }
        if(pixelIndex > 0) leds[pixelIndex - 1] += baseColor;
      }
      break;
      
    case 6: // RAINBOW CYCLE
      fill_rainbow(leds, NUM_LEDS, gHue, 7);
      break;
  }
}

void runAnimation() {
  if (!conf.ledsGlobalEnabled || isNightMode()) { 
    FastLED.clear(); 
    FastLED.show(); 
    return; 
  }
  
  EVERY_N_MILLISECONDS(20) { gHue++; }

  // 1. SURF MODE
  if (anySpotFiring) {
    int activeCount = 0; int indices[20]; 
    for(int i=0; i<spots.size(); i++) if(spots[i].enabled && spots[i].isFiring) indices[activeCount++] = i;
    
    if (activeCount > 0) {
      int cycleTime = (conf.surfCycleTime < 2) ? 2 : conf.surfCycleTime;
      int currentIndex = (millis() / (cycleTime * 1000)) % activeCount;
      Spot &s = spots[indices[currentIndex]];

      bool isEpic = (s.surfPower >= s.epicThreshold); 
      
      // Determine Configs
      int anim = isEpic ? conf.epicAnimType : conf.surfAnimType;
      CRGB col = isEpic ? hexToCRGB(conf.epicColorHex) : hexToCRGB(s.colorHex);
      
      // ⬇️ DYNAMIC GENTLE SPEED CALCULATION
      uint8_t spd;
      
      if (isEpic) {
        spd = conf.epicSpeed; 
      } else {
        // Range: 30 (Very Slow/Calm) to 110 (Active but Gentle)
        const int MIN_ANIM_SPEED = 30;  
        const int MAX_ANIM_SPEED = 110; 
        
        float range = s.epicThreshold - s.minThreshold;
        if (range <= 0) range = 1.0; 
        
        float ratio = (s.surfPower - s.minThreshold) / range;
        if (ratio < 0.0) ratio = 0.0;
        if (ratio > 1.0) ratio = 1.0;
        
        spd = MIN_ANIM_SPEED + (int)(ratio * (MAX_ANIM_SPEED - MIN_ANIM_SPEED));
      }
      
      renderEffect(anim, col, spd, conf.masterBrightness);
      FastLED.show();
      return; 
    }
  }

  // 2. LAMP FALLBACK MODE
  if (conf.lampFallbackEnabled) {
      CRGB col = hexToCRGB(conf.lampColorHex);
      renderEffect(conf.lampEffect, col, conf.lampSpeed, conf.masterBrightness);
      FastLED.show();
  } else { 
      // OFF
      fill_solid(leds, NUM_LEDS, CRGB::Black); 
      FastLED.show(); 
  }
}

// ═══════════════════════════════════════════════════════════
// 💾 CONFIG & SERVER
// ═══════════════════════════════════════════════════════════
void loadConfig() {
  if (LittleFS.exists("/conf_v16.json")) {
    File f = LittleFS.open("/conf_v16.json", "r");
    DynamicJsonDocument* doc = new DynamicJsonDocument(8192);
    deserializeJson(*doc, f);
    f.close();

    conf.ledsGlobalEnabled = (*doc)["gLed"] | true;
    conf.tgEnabled = (*doc)["tgEn"] | true;
    conf.masterBrightness = (*doc)["mBri"] | 255; 

    conf.surfCycleTime = (*doc)["cycT"] | 20; 
    conf.surfAnimType = (*doc)["sAnim"] | 1;  
    conf.epicAnimType = (*doc)["eAnim"] | 2; 
    conf.epicColorHex = (*doc)["eCol"].as<String>();
    if(conf.epicColorHex.length() != 7) conf.epicColorHex = "#ff0000";
    conf.epicSpeed = (*doc)["eSpd"] | 180;
    
    conf.botToken = (*doc)["tgT"].as<String>();
    conf.chatId = (*doc)["tgC"].as<String>();
    conf.tgMode = (*doc)["tgM"] | 0;      
    conf.tgThreshold = (*doc)["tgTh"] | 30; 

    conf.lampFallbackEnabled = (*doc)["lEn"] | false;
    conf.lampEffect = (*doc)["lEff"] | 1; 
    conf.lampSpeed = (*doc)["lSpd"] | 25; 
    conf.lampColorHex = (*doc)["lCol"] | "#FFB74D"; 
    
    conf.dndEnabled = (*doc)["dndEn"] | false;
    conf.timeZoneOffset = (*doc)["tz"] | 1; 
    conf.dndStart = (*doc)["dndS"] | 23;
    conf.dndEnd = (*doc)["dndE"] | 7;
    conf.updateFreqMins = (*doc)["uFreq"] | 60; 
    
    conf.proFormulaEnabled = (*doc)["pfEn"] | false; 
    conf.proFormula = (*doc)["pForm"].as<String>();

    spots.clear();
    JsonArray sArr = (*doc)["spots"];
    for (JsonObject s : sArr) {
      Spot ns;
      ns.name = s["name"].as<String>();
      ns.lat = s["lat"].as<String>();
      ns.lon = s["lon"].as<String>();
      ns.enabled = s["enabled"];
      ns.forecastDays = s["days"];
      ns.colorHex = s["col"].as<String>();
      if(ns.colorHex.length() != 7) ns.colorHex = "#00e5ff"; 
      ns.surfPower = 0;
      ns.useAdvanced = s["adv"] | false;
      ns.swellMask = s["mask"] | 0;
      ns.bestWindDir = s["wDir"] | 0;
      ns.minThreshold = s["tMin"] | 15.0;
      ns.epicThreshold = s["tEpic"] | 50.0;
      ns.bestDayDate = "";
      ns.humanTimeRange = ""; 
      ns.lastNotifiedID = "";
      spots.push_back(ns);
    }
    delete doc; 
  } else {
    // DEFAULTS
    conf.ledsGlobalEnabled = true; conf.tgEnabled = true;
    conf.surfCycleTime=20; 
    conf.surfAnimType=1; // Breathe (Pulse) as requested
    conf.lampColorHex="#FFB74D"; 
    conf.lampEffect=3; // Coastal
    conf.lampSpeed=40; 
    conf.timeZoneOffset=1; conf.dndStart=23; conf.dndEnd=7;
    conf.epicAnimType=6; // Rainbow Cycle as requested
    conf.epicColorHex="#ff0000"; conf.epicSpeed=200;
    conf.tgMode=0; conf.tgThreshold=30;
    conf.proFormulaEnabled = false;
    conf.proFormula = "";
    conf.updateFreqMins = 60; 
    conf.masterBrightness = 255;
  }
}

void saveConfigToDisk() {
  showLoadingAnim(); 
  DynamicJsonDocument* doc = new DynamicJsonDocument(8192);
  (*doc)["gLed"] = conf.ledsGlobalEnabled;
  (*doc)["tgEn"] = conf.tgEnabled;
  (*doc)["mBri"] = conf.masterBrightness;

  (*doc)["cycT"] = conf.surfCycleTime;
  (*doc)["sAnim"] = conf.surfAnimType;
  (*doc)["eAnim"] = conf.epicAnimType;
  (*doc)["eCol"] = conf.epicColorHex;
  (*doc)["eSpd"] = conf.epicSpeed;
  (*doc)["tgT"] = conf.botToken;
  (*doc)["tgC"] = conf.chatId;
  (*doc)["tgM"] = conf.tgMode;         
  (*doc)["tgTh"] = conf.tgThreshold;   
  (*doc)["lEn"] = conf.lampFallbackEnabled;
  (*doc)["lEff"] = conf.lampEffect;
  (*doc)["lSpd"] = conf.lampSpeed;
  (*doc)["lCol"] = conf.lampColorHex;
  (*doc)["dndEn"] = conf.dndEnabled;
  (*doc)["tz"] = conf.timeZoneOffset;
  (*doc)["dndS"] = conf.dndStart;
  (*doc)["dndE"] = conf.dndEnd;
  (*doc)["uFreq"] = conf.updateFreqMins; 
  
  (*doc)["pfEn"] = conf.proFormulaEnabled;
  (*doc)["pForm"] = conf.proFormula;

  JsonArray sArr = doc->createNestedArray("spots");
  for (const auto &s : spots) {
    JsonObject obj = sArr.createNestedObject();
    obj["name"] = s.name;
    obj["lat"] = s.lat;
    obj["lon"] = s.lon;
    obj["enabled"] = s.enabled;
    obj["days"] = s.forecastDays;
    obj["col"] = s.colorHex;
    obj["adv"] = s.useAdvanced;
    obj["mask"] = s.swellMask;
    obj["wDir"] = s.bestWindDir;
    obj["tMin"] = s.minThreshold;
    obj["tEpic"] = s.epicThreshold;
  }
  File f = LittleFS.open("/conf_v16.json", "w"); 
  serializeJson(*doc, f);
  f.close();
  delete doc;
}

void handleGetData() {
  DynamicJsonDocument* doc = new DynamicJsonDocument(8192);
  (*doc)["gLed"] = conf.ledsGlobalEnabled;
  (*doc)["tgEn"] = conf.tgEnabled;
  (*doc)["mBri"] = conf.masterBrightness;

  (*doc)["cycT"] = conf.surfCycleTime;
  (*doc)["sAnim"] = conf.surfAnimType;
  (*doc)["eAnim"] = conf.epicAnimType;
  (*doc)["eCol"] = conf.epicColorHex;
  (*doc)["eSpd"] = conf.epicSpeed;
  (*doc)["tgT"] = conf.botToken;
  (*doc)["tgC"] = conf.chatId;
  (*doc)["tgM"] = conf.tgMode;       
  (*doc)["tgTh"] = conf.tgThreshold; 
  (*doc)["lEn"] = conf.lampFallbackEnabled;
  (*doc)["lEff"] = conf.lampEffect;
  (*doc)["lSpd"] = conf.lampSpeed;
  (*doc)["lCol"] = conf.lampColorHex;
  (*doc)["dndEn"] = conf.dndEnabled;
  (*doc)["tz"] = conf.timeZoneOffset;
  (*doc)["dndS"] = conf.dndStart;
  (*doc)["dndE"] = conf.dndEnd;
  (*doc)["uFreq"] = conf.updateFreqMins; 
  
  (*doc)["pfEn"] = conf.proFormulaEnabled;
  (*doc)["pForm"] = conf.proFormula;
  
  JsonArray lArr = doc->createNestedArray("logs");
  for(const String &l : systemLogs) lArr.add(l);

  JsonArray sArr = doc->createNestedArray("spots");
  for (const auto &s : spots) {
    JsonObject obj = sArr.createNestedObject();
    obj["name"] = s.name;
    obj["lat"] = s.lat;
    obj["lon"] = s.lon;
    obj["enabled"] = s.enabled;
    obj["days"] = s.forecastDays;
    obj["col"] = s.colorHex;
    obj["pow"] = s.surfPower; 
    obj["adv"] = s.useAdvanced;
    obj["mask"] = s.swellMask;
    obj["wDir"] = s.bestWindDir;
    obj["tMin"] = s.minThreshold;
    obj["tEpic"] = s.epicThreshold;
  }
  String res; serializeJson(*doc, res);
  server.send(200, "application/json", res);
  delete doc;
}

void handleSaveData() {
  if (!server.hasArg("plain")) return;
  DynamicJsonDocument* doc = new DynamicJsonDocument(8192);
  DeserializationError error = deserializeJson(*doc, server.arg("plain"));
  if (error) { server.send(500, "text/plain", "JSON Error"); delete doc; return; }
  
  conf.ledsGlobalEnabled = (*doc)["gLed"];
  conf.tgEnabled = (*doc)["tgEn"];
  conf.masterBrightness = (*doc)["mBri"];

  conf.surfCycleTime = (*doc)["cycT"];
  conf.surfAnimType = (*doc)["sAnim"];
  conf.epicAnimType = (*doc)["eAnim"];
  conf.epicColorHex = (*doc)["eCol"].as<String>();
  conf.epicSpeed = (*doc)["eSpd"];
  conf.botToken = (*doc)["tgT"].as<String>();
  conf.chatId = (*doc)["tgC"].as<String>();
  conf.tgMode = (*doc)["tgM"];          
  conf.tgThreshold = (*doc)["tgTh"];    
  conf.lampFallbackEnabled = (*doc)["lEn"];
  conf.lampEffect = (*doc)["lEff"];
  conf.lampSpeed = (*doc)["lSpd"];
  conf.lampColorHex = (*doc)["lCol"].as<String>();
  conf.dndEnabled = (*doc)["dndEn"];
  conf.timeZoneOffset = (*doc)["tz"];
  conf.dndStart = (*doc)["dndS"];
  conf.dndEnd = (*doc)["dndE"];
  conf.updateFreqMins = (*doc)["uFreq"]; 
  
  conf.proFormulaEnabled = (*doc)["pfEn"]; 
  conf.proFormula = (*doc)["pForm"].as<String>();

  timeClient.setTimeOffset(conf.timeZoneOffset * 3600);
  spots.clear();
  JsonArray sArr = (*doc)["spots"];
  for (JsonObject s : sArr) {
    Spot newSpot;
    newSpot.name = s["name"].as<String>();
    newSpot.lat = s["lat"].as<String>();
    newSpot.lon = s["lon"].as<String>();
    newSpot.enabled = s["enabled"];
    newSpot.forecastDays = s["days"];
    newSpot.colorHex = s["col"].as<String>();
    newSpot.surfPower = 0; 
    newSpot.useAdvanced = s["adv"];
    newSpot.swellMask = s["mask"];
    newSpot.bestWindDir = s["wDir"];
    newSpot.bestDayDate = "";
    newSpot.humanTimeRange = ""; 
    newSpot.minThreshold = s["tMin"] | 15.0; 
    newSpot.epicThreshold = s["tEpic"] | 50.0;
    newSpot.lastNotifiedID = "";
    spots.push_back(newSpot);
  }
  saveConfigToDisk(); 
  server.send(200, "text/plain", "OK");
  delete doc;
  shouldReboot = true;
  rebootTimer = millis();
}

void setup() {
  delay(1000); 

  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // ⚡ SAFETY: USB-C power limit
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_POWER_MA); 
  FastLED.setBrightness(255);
  
  showLoadingAnim();

  if(!LittleFS.begin(true)) Serial.println("LittleFS Error");
  loadConfig();
  loadHistory(); 
  
  secured_client.setInsecure(); 
  bot = new UniversalTelegramBot(conf.botToken, secured_client);

  WiFiManager wm; 
  wm.setCustomHeadElement(wifi_custom_css); 
  wm.setConnectTimeout(180);
  if(!wm.autoConnect("SurfBeacon_Setup")) ESP.restart();

  if(MDNS.begin("surfbeacon")) Serial.println("MDNS started");

  server.on("/", [](){ server.send(200, "text/html", index_html); });
  server.on("/api/data", handleGetData);
  server.on("/api/save", HTTP_POST, handleSaveData);
  server.begin();

  timeClient.begin();
  timeClient.setTimeOffset(conf.timeZoneOffset * 3600);
  
  logSys("⏳ Waiting for NTP time...");
  int retry = 0;
  while(!timeClient.update() && retry < 10) {
    timeClient.forceUpdate();
    delay(500);
    retry++;
  }
  logSys("Time Sync: " + timeClient.getFormattedTime());

  // Initial update
  updateForecast();
  lastUpdate = millis(); 
}

void loop() {
  server.handleClient();
  timeClient.update();
  
  if (shouldReboot && millis() - rebootTimer > 1500) ESP.restart();
  
  // 1. UPDATE FORECAST
  unsigned long intervalMs = (unsigned long)conf.updateFreqMins * 60000;
  if(millis() - lastUpdate > intervalMs) { 
    updateForecast(); 
    lastUpdate = millis(); 
  }

  // 2. FLUID ANIMATION
  EVERY_N_MILLISECONDS(20) {
    runAnimation();
  }
}
