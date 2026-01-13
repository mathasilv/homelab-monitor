#include <Arduino.h>
#include <TFT_eSPI.h>

#define LED_PIN PC13

// ============================================
// CRT AMBER PHOSPHOR COLOR PALETTE (RGB565)
// ============================================
#define CRT_BLACK      0x1080  // Preto com tom âmbar
#define CRT_AMBER_DIM  0x6260  // Âmbar escuro (linhas)
#define CRT_AMBER_MED  0xCC80  // Âmbar médio (labels)
#define CRT_AMBER      0xFDE0  // Âmbar brilhante (valores)
#define CRT_AMBER_HI   0xFEE0  // Âmbar highlight
#define CRT_AMBER_WARN 0xFCC0  // Âmbar aviso
#define CRT_AMBER_CRIT 0xF800  // Vermelho para crítico

TFT_eSPI tft = TFT_eSPI();

String line;
bool needsFullRedraw = true;

// Server variables
String ip = "---.---.---.---", ip_old;
String up = "--", up_old;
String load1 = "--", load1_old;
String load5 = "--", load5_old;
String load15 = "--", load15_old;
String cpuPct = "0", cpuPct_old;
String cpuTemp = "--", cpuTemp_old;
String ramUsed = "--", ramUsed_old;
String ramTotal = "--", ramTotal_old;
String ramPct = "0", ramPct_old;
String swapUsed = "--", swapUsed_old;
String swapTotal = "--", swapTotal_old;
String swapPct = "0", swapPct_old;
String diskUsed = "--", diskUsed_old;
String diskTotal = "--", diskTotal_old;
String diskPct = "0", diskPct_old;
String netRx = "--", netRx_old;
String netTx = "--", netTx_old;
String diskRead = "--", diskRead_old;
String diskWrite = "--", diskWrite_old;
String procCount = "--", procCount_old;
String smbClients = "--", smbClients_old;
String connIn = "--", connIn_old;

// Layout - 320x240, 11 linhas de conteúdo
const int SCREEN_W = 320;
const int SCREEN_H = 240;
const int LINE_H = 21;
const int START_Y = 5;

const int Y_TITLE  = START_Y;
const int Y_IP     = START_Y + LINE_H;
const int Y_UP     = START_Y + LINE_H * 2;
const int Y_LOAD   = START_Y + LINE_H * 3;
const int Y_CPU    = START_Y + LINE_H * 4;
const int Y_RAM    = START_Y + LINE_H * 5;
const int Y_SWAP   = START_Y + LINE_H * 6;
const int Y_DISK   = START_Y + LINE_H * 7;
const int Y_NET    = START_Y + LINE_H * 8;
const int Y_IO     = START_Y + LINE_H * 9;
const int Y_STAT   = START_Y + LINE_H * 10;

// ============================================
// CRT BORDER
// ============================================
void drawCRTBorder() {
  tft.drawRect(0, 0, SCREEN_W, SCREEN_H, CRT_AMBER_DIM);
  tft.drawRect(2, 2, SCREEN_W - 4, SCREEN_H - 4, CRT_AMBER_DIM);
}

// ============================================
// CRT STYLE PROGRESS BAR
// ============================================
void drawCRTBar(int x, int y, int barW, int h, int percent) {
  uint16_t fillColor = CRT_AMBER;
  if (percent > 90) fillColor = CRT_AMBER_CRIT;
  else if (percent > 75) fillColor = CRT_AMBER_WARN;
  
  tft.drawRect(x, y, barW, h, CRT_AMBER_DIM);
  tft.fillRect(x + 1, y + 1, barW - 2, h - 2, CRT_BLACK);
  
  int fillW = map(percent, 0, 100, 0, barW - 4);
  int segW = 3;
  int gap = 1;
  
  for (int sx = 0; sx < fillW; sx += (segW + gap)) {
    int sw = min(segW, fillW - sx);
    if (sw > 0) {
      tft.fillRect(x + 2 + sx, y + 2, sw, h - 4, fillColor);
    }
  }
}

// ============================================
// TEXT DRAWING
// ============================================
void drawGlowText(int x, int y, const char* text, uint16_t color, int size) {
  tft.setTextSize(size);
  tft.setTextColor(CRT_AMBER_DIM, CRT_BLACK);
  tft.setCursor(x + 1, y + 1);
  tft.print(text);
  tft.setTextColor(color, CRT_BLACK);
  tft.setCursor(x, y);
  tft.print(text);
}

void drawCentered(const char* text, int y, uint16_t color, int size) {
  tft.setTextSize(size);
  int w = strlen(text) * 6 * size;
  int x = (SCREEN_W - w) / 2;
  drawGlowText(x, y, text, color, size);
}

void drawLabel(int x, int y, const char* label) {
  tft.setTextSize(1);
  tft.setTextColor(CRT_AMBER_MED, CRT_BLACK);
  tft.setCursor(x, y);
  tft.print(label);
}

void clearArea(int x, int y, int w, int h) {
  tft.fillRect(x, y, w, h, CRT_BLACK);
}

// ============================================
// DISPLAY FUNCTIONS
// ============================================
void drawStaticElements() {
  tft.fillScreen(CRT_BLACK);
  drawCRTBorder();
  drawCentered("[ SERVER MONITOR ]", Y_TITLE, CRT_AMBER_HI, 2);
  
  drawLabel(8, Y_UP, "UP:");
  drawLabel(8, Y_LOAD, "LOAD:");
  drawLabel(8, Y_CPU, "CPU:");
  drawLabel(8, Y_RAM, "RAM:");
  drawLabel(8, Y_SWAP, "SWP:");
  drawLabel(8, Y_DISK, "DSK:");
  drawLabel(8, Y_NET, "NET:");
  drawLabel(8, Y_IO, "I/O:");
}

void updateIP() {
  if (ip != ip_old || needsFullRedraw) {
    tft.setTextSize(1);
    int w = ip_old.length() * 6;
    int x = (SCREEN_W - w) / 2;
    tft.setTextColor(CRT_BLACK, CRT_BLACK);
    tft.setCursor(x, Y_IP);
    tft.print(ip_old);
    
    w = ip.length() * 6;
    x = (SCREEN_W - w) / 2;
    tft.setTextColor(CRT_AMBER, CRT_BLACK);
    tft.setCursor(x, Y_IP);
    tft.print(ip);
    ip_old = ip;
  }
}

void updateDisplay() {
  updateIP();
  
  // Uptime
  if (up != up_old || needsFullRedraw) {
    clearArea(32, Y_UP, 150, 10);
    tft.setTextSize(1);
    tft.setTextColor(CRT_AMBER, CRT_BLACK);
    tft.setCursor(32, Y_UP);
    tft.print(up);
    up_old = up;
  }
  
  // Load Average
  if (load1 != load1_old || load5 != load5_old || load15 != load15_old || needsFullRedraw) {
    clearArea(44, Y_LOAD, 180, 10);
    tft.setTextSize(1);
    tft.setTextColor(CRT_AMBER, CRT_BLACK);
    tft.setCursor(44, Y_LOAD);
    tft.print(load1);
    tft.print(" / ");
    tft.print(load5);
    tft.print(" / ");
    tft.print(load15);
    load1_old = load1; load5_old = load5; load15_old = load15;
  }
  
  // CPU bar + temp
  if (cpuPct != cpuPct_old || cpuTemp != cpuTemp_old || needsFullRedraw) {
    drawCRTBar(44, Y_CPU, 130, 11, cpuPct.toInt());
    clearArea(180, Y_CPU, 130, 11);
    tft.setTextSize(1);
    tft.setTextColor(CRT_AMBER_HI, CRT_BLACK);
    tft.setCursor(180, Y_CPU + 2);
    tft.print(cpuPct);
    tft.print("%  ");
    tft.setTextColor(CRT_AMBER_MED, CRT_BLACK);
    tft.print(cpuTemp);
    tft.print("C");
    cpuPct_old = cpuPct; cpuTemp_old = cpuTemp;
  }
  
  // RAM bar + info
  if (ramPct != ramPct_old || ramUsed != ramUsed_old || needsFullRedraw) {
    drawCRTBar(44, Y_RAM, 130, 11, ramPct.toInt());
    clearArea(180, Y_RAM, 130, 11);
    tft.setTextSize(1);
    tft.setTextColor(CRT_AMBER_HI, CRT_BLACK);
    tft.setCursor(180, Y_RAM + 2);
    tft.print(ramPct);
    tft.print("%  ");
    tft.setTextColor(CRT_AMBER_MED, CRT_BLACK);
    tft.print(ramUsed);
    tft.print("/");
    tft.print(ramTotal);
    tft.print("M");
    ramPct_old = ramPct; ramUsed_old = ramUsed; ramTotal_old = ramTotal;
  }
  
  // Swap bar + info
  if (swapPct != swapPct_old || swapUsed != swapUsed_old || needsFullRedraw) {
    drawCRTBar(44, Y_SWAP, 130, 11, swapPct.toInt());
    clearArea(180, Y_SWAP, 130, 11);
    tft.setTextSize(1);
    tft.setTextColor(CRT_AMBER_HI, CRT_BLACK);
    tft.setCursor(180, Y_SWAP + 2);
    tft.print(swapPct);
    tft.print("%  ");
    tft.setTextColor(CRT_AMBER_MED, CRT_BLACK);
    tft.print(swapUsed);
    tft.print("/");
    tft.print(swapTotal);
    tft.print("M");
    swapPct_old = swapPct; swapUsed_old = swapUsed; swapTotal_old = swapTotal;
  }
  
  // Disk bar + info
  if (diskPct != diskPct_old || diskUsed != diskUsed_old || needsFullRedraw) {
    drawCRTBar(44, Y_DISK, 130, 11, diskPct.toInt());
    clearArea(180, Y_DISK, 130, 11);
    tft.setTextSize(1);
    tft.setTextColor(CRT_AMBER_HI, CRT_BLACK);
    tft.setCursor(180, Y_DISK + 2);
    tft.print(diskPct);
    tft.print("%  ");
    tft.setTextColor(CRT_AMBER_MED, CRT_BLACK);
    tft.print(diskUsed);
    tft.print("/");
    tft.print(diskTotal);
    tft.print("G");
    diskPct_old = diskPct; diskUsed_old = diskUsed; diskTotal_old = diskTotal;
  }
  
  // Network
  if (netRx != netRx_old || netTx != netTx_old || needsFullRedraw) {
    clearArea(44, Y_NET, 270, 10);
    tft.setTextSize(1);
    tft.setTextColor(CRT_AMBER, CRT_BLACK);
    tft.setCursor(44, Y_NET);
    tft.print("Rx:");
    tft.print(netRx);
    tft.print("  Tx:");
    tft.print(netTx);
    tft.setTextColor(CRT_AMBER_DIM, CRT_BLACK);
    tft.print(" KB/s");
    netRx_old = netRx; netTx_old = netTx;
  }
  
  // Disk I/O
  if (diskRead != diskRead_old || diskWrite != diskWrite_old || needsFullRedraw) {
    clearArea(44, Y_IO, 270, 10);
    tft.setTextSize(1);
    tft.setTextColor(CRT_AMBER, CRT_BLACK);
    tft.setCursor(44, Y_IO);
    tft.print("R:");
    tft.print(diskRead);
    tft.print("  W:");
    tft.print(diskWrite);
    tft.setTextColor(CRT_AMBER_DIM, CRT_BLACK);
    tft.print(" MB/s");
    diskRead_old = diskRead; diskWrite_old = diskWrite;
  }
  
  // Stats: PROC | SMB | CONN
  if (procCount != procCount_old || smbClients != smbClients_old || connIn != connIn_old || needsFullRedraw) {
    clearArea(8, Y_STAT, 304, 12);
    tft.setTextSize(1);
    
    // PROC
    tft.setTextColor(CRT_AMBER_MED, CRT_BLACK);
    tft.setCursor(8, Y_STAT);
    tft.print("PROC:");
    tft.setTextColor(CRT_AMBER, CRT_BLACK);
    tft.print(procCount);
    
    // SMB
    tft.setTextColor(CRT_AMBER_MED, CRT_BLACK);
    tft.setCursor(100, Y_STAT);
    tft.print("SMB:");
    tft.setTextColor(CRT_AMBER, CRT_BLACK);
    tft.print(smbClients);
    
    // CONN
    tft.setTextColor(CRT_AMBER_MED, CRT_BLACK);
    tft.setCursor(180, Y_STAT);
    tft.print("CONN:");
    tft.setTextColor(CRT_AMBER, CRT_BLACK);
    tft.print(connIn);
    
    procCount_old = procCount; smbClients_old = smbClients; connIn_old = connIn;
  }
  
  needsFullRedraw = false;
}

void applyKV(const String& k, const String& v) {
  if (k == "ip") ip = v;
  else if (k == "up") up = v;
  else if (k == "load_1") load1 = v;
  else if (k == "load_5") load5 = v;
  else if (k == "load_15") load15 = v;
  else if (k == "cpu_pct") cpuPct = v;
  else if (k == "cpu_temp") cpuTemp = v;
  else if (k == "ram_used") ramUsed = v;
  else if (k == "ram_total") ramTotal = v;
  else if (k == "ram_pct") ramPct = v;
  else if (k == "swap_used") swapUsed = v;
  else if (k == "swap_total") swapTotal = v;
  else if (k == "swap_pct") swapPct = v;
  else if (k == "disk_used") diskUsed = v;
  else if (k == "disk_total") diskTotal = v;
  else if (k == "disk_pct") diskPct = v;
  else if (k == "net_rx") netRx = v;
  else if (k == "net_tx") netTx = v;
  else if (k == "disk_read") diskRead = v;
  else if (k == "disk_write") diskWrite = v;
  else if (k == "proc_count") procCount = v;
  else if (k == "smb_clients") smbClients = v;
  else if (k == "conn_in") connIn = v;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  tft.init();
  tft.setRotation(3);
  tft.invertDisplay(false);
  
  drawStaticElements();
  updateDisplay();
  
  Serial.begin(115200);
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == 0x0D) continue;
    if (c == 0x0A) {
      String s = line;
      line = "";
      s.trim();
      if (s.length() == 0) continue;
      if (s == "END") {
        updateDisplay();
        continue;
      }
      int eq = s.indexOf(0x3D);
      if (eq > 0) {
        String k = s.substring(0, eq);
        String v = s.substring(eq + 1);
        k.trim(); v.trim();
        applyKV(k, v);
      }
    } else {
      if (line.length() < 200) line += c;
    }
  }
}
