#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

const char* ssid = "SSID";
const char* password = "PASSWORD";

WebServer server(80);

const uint8_t MPU = 0x68;
int16_t ax, ay, az, gx, gy, gz;

void writeMPU(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission(true);
}

void readMPU() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom((int)MPU, 14, true);

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();

  Wire.read(); 
  Wire.read();

  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  gz = Wire.read() << 8 | Wire.read();
}

const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>ESP32 Fruit Blade</title>
<style>
html,body{
  margin:0;
  height:100%;
  overflow:hidden;
  background:#070712;
  color:white;
  font-family:Arial,sans-serif;
  touch-action:none;
  user-select:none;
}
canvas{
  position:fixed;
  inset:0;
}
#hud{
  position:fixed;
  top:15px;
  left:15px;
  z-index:5;
  font-size:22px;
  font-weight:bold;
}
#status{
  font-size:13px;
  color:#8fa0c8;
  margin-top:5px;
}
#overlay{
  position:fixed;
  inset:0;
  z-index:20;
  display:flex;
  flex-direction:column;
  justify-content:center;
  align-items:center;
  text-align:center;
  padding:25px;
  background:radial-gradient(circle at center,#1c1c3a,#070712 70%);
}
h1{
  font-size:58px;
  margin:0;
  background:linear-gradient(120deg,#22d3ee,#e879f9,#fbbf24);
  -webkit-background-clip:text;
  color:transparent;
}
p{
  color:#aab3d5;
  max-width:430px;
  line-height:1.5;
}
button{
  border:0;
  border-radius:14px;
  padding:14px 30px;
  font-size:18px;
  font-weight:bold;
  cursor:pointer;
  background:linear-gradient(120deg,#22d3ee,#e879f9);
  color:#07111a;
}
#calBox{
  width:260px;
  height:260px;
  border:2px solid rgba(255,255,255,.2);
  border-radius:25px;
  margin:20px;
  position:relative;
  display:none;
}
#dot{
  width:26px;
  height:26px;
  border-radius:50%;
  background:#22d3ee;
  position:absolute;
  left:117px;
  top:117px;
  box-shadow:0 0 25px #22d3ee;
}
#arrow{
  font-size:55px;
  margin-top:10px;
  display:none;
}
.small{
  font-size:13px;
  color:#8fa0c8;
}
</style>
</head>
<body>

<canvas id="game"></canvas>

<div id="hud">
  Score: <span id="score">0</span>
  <div id="status">Connecting to ESP32...</div>
</div>

<div id="overlay">
  <h1>FRUIT/BLADE</h1>
  <p id="msg">Open this page, connect your ESP32 blade, then calibrate your own hand movements.</p>

  <div id="arrow">➡️</div>
  <div id="calBox">
    <div id="dot"></div>
  </div>

  <button id="mainBtn">Start Calibration</button>
  <div class="small" id="stepText">Waiting for blade...</div>
</div>

<script>
const canvas = document.getElementById("game");
const ctx = canvas.getContext("2d");
const scoreEl = document.getElementById("score");
const statusEl = document.getElementById("status");
const overlay = document.getElementById("overlay");
const msg = document.getElementById("msg");
const btn = document.getElementById("mainBtn");
const stepText = document.getElementById("stepText");
const calBox = document.getElementById("calBox");
const dot = document.getElementById("dot");
const arrow = document.getElementById("arrow");

canvas.width = innerWidth;
canvas.height = innerHeight;

addEventListener("resize",()=>{
  canvas.width = innerWidth;
  canvas.height = innerHeight;
});

let sensor = {ax:0,ay:0,az:0,gx:0,gy:0,gz:0};
let connected = false;

let base = {gx:0,gy:0,gz:0,ax:0,ay:0,az:0};
let dirs = {};
let calibrationStep = 0;
let samples = [];

let running = false;
let score = 0;
let fruits = [];
let particles = [];

let blade = {
  x: innerWidth/2,
  y: innerHeight/2,
  px: innerWidth/2,
  py: innerHeight/2,
  trail:[]
};

const steps = [
  {
    name:"stable",
    title:"Hold still",
    text:"Hold the blade naturally in your hand. Keep it stable, then press Capture.",
    icon:"🤚"
  },
  {
    name:"right",
    title:"Move RIGHT",
    text:"Move/slice the blade to the RIGHT. Watch the blue dot move right, then press Capture.",
    icon:"➡️"
  },
  {
    name:"left",
    title:"Move LEFT",
    text:"Move/slice the blade to the LEFT. Watch the blue dot move left, then press Capture.",
    icon:"⬅️"
  },
  {
    name:"up",
    title:"Move UP",
    text:"Move/slice the blade UP. Watch the blue dot move up, then press Capture.",
    icon:"⬆️"
  },
  {
    name:"down",
    title:"Move DOWN",
    text:"Move/slice the blade DOWN. Watch the blue dot move down, then press Capture.",
    icon:"⬇️"
  }
];

async function pollSensor(){
  try{
    let r = await fetch("/data", {cache:"no-store"});
    sensor = await r.json();
    connected = true;
    statusEl.innerText = "Blade connected";
    stepText.innerText = "Blade connected";
  }catch(e){
    connected = false;
    statusEl.innerText = "ESP32 not responding";
  }
}
setInterval(pollSensor, 30);

function vec(){
  return {
    gx: sensor.gx - base.gx,
    gy: sensor.gy - base.gy,
    gz: sensor.gz - base.gz,
    ax: sensor.ax - base.ax,
    ay: sensor.ay - base.ay,
    az: sensor.az - base.az
  };
}

function mag(v){
  return Math.sqrt(
    v.gx*v.gx + v.gy*v.gy + v.gz*v.gz +
    v.ax*v.ax*0.08 + v.ay*v.ay*0.08 + v.az*v.az*0.08
  );
}

function sim(a,b){
  let dot =
    a.gx*b.gx + a.gy*b.gy + a.gz*b.gz +
    a.ax*b.ax*0.08 + a.ay*b.ay*0.08 + a.az*b.az*0.08;

  let ma = mag(a);
  let mb = mag(b);

  if(ma < 1 || mb < 1) return 0;
  return dot/(ma*mb);
}

function avgSamples(){
  let o = {gx:0,gy:0,gz:0,ax:0,ay:0,az:0};
  for(let s of samples){
    o.gx += s.gx;
    o.gy += s.gy;
    o.gz += s.gz;
    o.ax += s.ax;
    o.ay += s.ay;
    o.az += s.az;
  }
  let n = samples.length || 1;
  o.gx /= n;
  o.gy /= n;
  o.gz /= n;
  o.ax /= n;
  o.ay /= n;
  o.az /= n;
  return o;
}

function showStep(){
  let s = steps[calibrationStep];

  msg.innerHTML = "<b>" + s.title + "</b><br>" + s.text;
  arrow.innerText = s.icon;
  arrow.style.display = "block";
  calBox.style.display = "block";
  btn.innerText = calibrationStep === 0 ? "Capture Still Position" : "Capture " + s.title;
  samples = [];
}

function startCalibration(){
  calibrationStep = 0;
  showStep();
}

function captureStep(){
  let avg = avgSamples();

  if(calibrationStep === 0){
    base = avg;
  }else{
    let name = steps[calibrationStep].name;
    dirs[name] = {
      gx: avg.gx - base.gx,
      gy: avg.gy - base.gy,
      gz: avg.gz - base.gz,
      ax: avg.ax - base.ax,
      ay: avg.ay - base.ay,
      az: avg.az - base.az
    };
  }

  calibrationStep++;

  if(calibrationStep >= steps.length){
    msg.innerHTML = "<b>Calibration complete ✅</b><br>Your blade directions are saved.";
    arrow.style.display = "none";
    calBox.style.display = "none";
    btn.innerText = "Start Game";
    btn.onclick = startGame;
  }else{
    showStep();
  }
}

btn.onclick = () => {
  if(!connected){
    msg.innerHTML = "Blade not connected yet.<br>Check ESP32 Serial Monitor URL and WiFi.";
    return;
  }
  btn.onclick = captureStep;
  startCalibration();
};

function updateCalibrationVisual(){
  if(calibrationStep <= 0 || calibrationStep >= steps.length) return;

  let v = vec();

  let x = 117;
  let y = 117;

  x += Math.max(-100, Math.min(100, v.gz/90));
  y += Math.max(-100, Math.min(100, v.gx/90));

  dot.style.left = x + "px";
  dot.style.top = y + "px";
}

setInterval(()=>{
  if(calibrationStep < steps.length){
    samples.push({...sensor});
    if(samples.length > 35) samples.shift();
    updateCalibrationVisual();
  }
},35);

class Fruit{
  constructor(){
    this.x = Math.random()*canvas.width;
    this.y = canvas.height + 50;
    this.r = 28 + Math.random()*20;
    this.vx = (Math.random()-0.5)*5;
    this.vy = -(10 + Math.random()*8);
    this.color = ["#ff4d6d","#fbbf24","#22c55e","#a855f7","#f97316"][Math.floor(Math.random()*5)];
    this.bomb = Math.random() < 0.12;
  }

  update(){
    this.x += this.vx;
    this.y += this.vy;
    this.vy += 0.25;
  }

  draw(){
    ctx.beginPath();
    ctx.fillStyle = this.bomb ? "#111" : this.color;
    ctx.arc(this.x,this.y,this.r,0,Math.PI*2);
    ctx.fill();

    if(this.bomb){
      ctx.strokeStyle = "#ff4d6d";
      ctx.lineWidth = 4;
      ctx.stroke();
      ctx.fillStyle = "#ff4d6d";
      ctx.fillRect(this.x-3,this.y-this.r-12,6,14);
    }else{
      ctx.fillStyle = "rgba(255,255,255,.35)";
      ctx.beginPath();
      ctx.arc(this.x-this.r/3,this.y-this.r/3,this.r/4,0,Math.PI*2);
      ctx.fill();
    }
  }
}

function spawnFruit(){
  if(running) fruits.push(new Fruit());
}
setInterval(spawnFruit,800);

function addParticles(x,y,color){
  for(let i=0;i<14;i++){
    particles.push({
      x:x,
      y:y,
      vx:(Math.random()-0.5)*8,
      vy:(Math.random()-0.5)*8,
      life:35,
      color:color
    });
  }
}

function applyBladeMovement(){
  let v = vec();
  let power = mag(v);

  let right = sim(v, dirs.right || v);
  let left = sim(v, dirs.left || v);
  let up = sim(v, dirs.up || v);
  let down = sim(v, dirs.down || v);

  let dx = (right - left) * 34;
  let dy = (down - up) * 34;

  if(power < 2600){
    dx *= 0.2;
    dy *= 0.2;
  }

  blade.px = blade.x;
  blade.py = blade.y;

  blade.x += dx;
  blade.y += dy;

  blade.x += (canvas.width/2 - blade.x) * 0.015;
  blade.y += (canvas.height/2 - blade.y) * 0.015;

  blade.x = Math.max(0, Math.min(canvas.width, blade.x));
  blade.y = Math.max(0, Math.min(canvas.height, blade.y));

  blade.trail.push({x:blade.x,y:blade.y});
  if(blade.trail.length > 12) blade.trail.shift();

  return power > 3200;
}

function distToSegment(px,py,x1,y1,x2,y2){
  let A = px-x1;
  let B = py-y1;
  let C = x2-x1;
  let D = y2-y1;

  let dot = A*C + B*D;
  let len = C*C + D*D;
  let t = len ? dot/len : 0;

  t = Math.max(0, Math.min(1, t));

  let x = x1 + C*t;
  let y = y1 + D*t;

  return Math.hypot(px-x,y-py);
}

function drawBlade(slicing){
  if(blade.trail.length < 2) return;

  ctx.lineCap = "round";
  ctx.lineJoin = "round";

  ctx.beginPath();
  ctx.moveTo(blade.trail[0].x, blade.trail[0].y);

  for(let p of blade.trail){
    ctx.lineTo(p.x,p.y);
  }

  ctx.strokeStyle = slicing ? "#22d3ee" : "rgba(34,211,238,.35)";
  ctx.lineWidth = slicing ? 16 : 8;
  ctx.shadowColor = "#22d3ee";
  ctx.shadowBlur = slicing ? 25 : 8;
  ctx.stroke();
  ctx.shadowBlur = 0;

  ctx.fillStyle = "white";
  ctx.beginPath();
  ctx.arc(blade.x,blade.y,7,0,Math.PI*2);
  ctx.fill();
}

function startGame(){
  overlay.style.display = "none";
  running = true;
  score = 0;
  fruits = [];
  particles = [];
  scoreEl.innerText = "0";
  blade.x = canvas.width/2;
  blade.y = canvas.height/2;
}

function gameLoop(){
  ctx.clearRect(0,0,canvas.width,canvas.height);

  let bg = ctx.createRadialGradient(canvas.width/2,canvas.height/4,50,canvas.width/2,canvas.height/2,canvas.height);
  bg.addColorStop(0,"#171733");
  bg.addColorStop(1,"#05050c");
  ctx.fillStyle = bg;
  ctx.fillRect(0,0,canvas.width,canvas.height);

  let slicing = false;

  if(running){
    slicing = applyBladeMovement();

    for(let f of fruits){
      f.update();
      f.draw();

      if(slicing){
        let d = distToSegment(f.x,f.y,blade.px,blade.py,blade.x,blade.y);
        if(d < f.r + 18){
          f.dead = true;

          if(f.bomb){
            score = Math.max(0, score - 5);
            addParticles(f.x,f.y,"#ff4d6d");
          }else{
            score++;
            addParticles(f.x,f.y,f.color);
          }

          scoreEl.innerText = score;
        }
      }
    }

    fruits = fruits.filter(f => !f.dead && f.y < canvas.height + 100);

    for(let p of particles){
      p.x += p.vx;
      p.y += p.vy;
      p.vy += 0.15;
      p.life--;

      ctx.globalAlpha = Math.max(0,p.life/35);
      ctx.fillStyle = p.color;
      ctx.beginPath();
      ctx.arc(p.x,p.y,4,0,Math.PI*2);
      ctx.fill();
      ctx.globalAlpha = 1;
    }

    particles = particles.filter(p => p.life > 0);

    drawBlade(slicing);
  }

  requestAnimationFrame(gameLoop);
}

gameLoop();
</script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  Wire.setClock(400000);

  writeMPU(0x6B, 0x00);
  writeMPU(0x1B, 0x10);
  writeMPU(0x1C, 0x08);

  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Open this URL: http://");
  Serial.println(WiFi.localIP());

  server.on("/", [](){
    server.send_P(200, "text/html", html);
  });

  server.on("/data", [](){
    readMPU();

    String json = "{";
    json += "\"ax\":" + String(ax) + ",";
    json += "\"ay\":" + String(ay) + ",";
    json += "\"az\":" + String(az) + ",";
    json += "\"gx\":" + String(gx) + ",";
    json += "\"gy\":" + String(gy) + ",";
    json += "\"gz\":" + String(gz);
    json += "}";

    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();
}