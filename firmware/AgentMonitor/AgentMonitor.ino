#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config.h"
#include "font_ru.h"
#include "marquee.h"
#ifndef BRIDGE_HOST
#define BRIDGE_HOST "YOUR_MAC_HOSTNAME"
#endif
#ifndef BRIDGE_PORT
#define BRIDGE_PORT 8787
#endif

#define TFT_CS 47
#define TFT_RST 14
#define TFT_DC 21
#define TFT_MOSI 41
#define TFT_SCK 42
Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);
const uint16_t BG=0x0883, CARD=0x10E5, WHITE=0xEF7E, MUTED=0x84B5;
const uint16_t GREEN=0x4EF0, YELLOW=0xFDE4, BLUE=0x3DFF, RED=0xFB8A;
const int ROWS=7;
GFXcanvas16 strip(320,24);
String previous[ROWS], previousHeader, previousFooter1, previousFooter2;
String providers[80],projects[80],statuses[80];
String ids[80],scrollKeys[ROWS];
unsigned long scrollStarted[ROWS]={0},lastRender=0;
unsigned long elapsed[80];
int count=0;
unsigned page=0;
unsigned long lastPoll=0,lastPage=0,lastWifiAttempt=0,receivedAt=0;
bool online=false;
String clockText="--:--",vpn="UNKNOWN",internet="UNKNOWN";
int battery=-1;
bool charging=false;
bool mdnsReady=false;
IPAddress bridgeIP;
unsigned long lastResolve=0;

// Decode UTF-8 explicitly: the standard GFX print() does not support Cyrillic.
void text(int x,int y,const String &s,uint16_t color,int maxChars=46) {
  int n=0;
  for(unsigned i=0;i<s.length() && n<maxChars;n++) {
    uint32_t cp=(uint8_t)s[i++];
    if(cp>=0xC0 && cp<0xE0 && i<s.length()) cp=((cp&31)<<6)|((uint8_t)s[i++]&63);
    else if(cp>=0xE0) {cp='?'; while(i<s.length() && ((uint8_t)s[i]&0xC0)==0x80)i++;}
    int index=cp>=32 && cp<=126 ? cp-32 : cp>=0x410 && cp<=0x44F ? 95+cp-0x410 : cp==0x401 ? 159 : cp==0x451 ? 160 : '?'-32;
    for(int row=0;row<13;row++) {
      uint8_t bits=pgm_read_byte(&RU_FONT[index][row]);
      for(int col=0;col<7;col++) if(bits&(1<<(6-col)))strip.drawPixel(x+col,y+row,color);
    }
    x+=7;
  }
}
uint16_t statusColor(const String &s) {
  if(s=="WORKING")return GREEN;
  if(s=="WAITING")return YELLOW;
  if(s=="DONE")return BLUE;
  if(s=="ERROR")return RED;
  return MUTED;
}
String statusName(const String &s) {
  if(s=="WORKING")return "РАБОТА";
  if(s=="WAITING")return "ОТВЕТ";
  if(s=="DONE")return "ГОТОВО";
  if(s=="ERROR")return "ОШИБКА";
  return "ПАУЗА";
}
String movingTitle(const String &s,int row,const String &id){
  String scrollKey=id+"|"+s;
  if(scrollKeys[row]!=scrollKey){scrollKeys[row]=scrollKey;scrollStarted[row]=millis();}
  unsigned chars=0;
  for(unsigned i=0;i<s.length();i++)if(((uint8_t)s[i]&0xC0)!=0x80)chars++;
  unsigned offset=marqueeOffset(chars,22,millis()-scrollStarted[row]);
  unsigned start=0,end=s.length(),n=0;
  for(unsigned i=0;i<s.length();i++)if(((uint8_t)s[i]&0xC0)!=0x80){
    if(n==offset)start=i;
    if(n==offset+22){end=i;break;}
    n++;
  }
  return s.substring(start,end);
}
void flushStrip(int y){tft.drawRGBBitmap(0,y,strip.getBuffer(),320,24);}
void render() {
  String header="АГЕНТЫ";
  if(online) header+="                         "+clockText;
  if(header!=previousHeader) {
    strip.fillScreen(BG);text(8,5,"АГЕНТЫ",WHITE);
    if(online)text(276,5,clockText,MUTED);
    strip.drawFastHLine(8,23,304,CARD);flushStrip(0);previousHeader=header;
  }
  int active=0,done=0;
  for(int i=0;i<count;i++){if(statuses[i]!="DONE")active++;else done++;}
  // Completed sessions only fill spare slots; they never push active sessions to another page.
  int pages=max(1,(active+ROWS-1)/ROWS);
  if(page>=(unsigned)pages)page=0;
  for(int row=0;row<ROWS;row++) {
    int i=page*ROWS+row;
    bool show=online && i<count && (page==0 || i<active);
    String duration="";
    if(show && statuses[i]=="WORKING" && elapsed[i]!=0xFFFFFFFF) {
      unsigned long seconds=elapsed[i]+(millis()-receivedAt)/1000;
      duration=String(seconds/60)+":"+(seconds%60<10?"0":"")+String(seconds%60);
    }
    String title=show?movingTitle(projects[i],row,ids[i]):"";
    if(!show)scrollKeys[row]="";
    String key=show ? providers[i]+title+statuses[i]+duration : "";
    if(key!=previous[row]) {
      strip.fillScreen(BG);
      if(show){
        strip.fillRoundRect(4,1,312,22,3,CARD);
        strip.fillCircle(10,11,3,statusColor(statuses[i]));
        text(18,5,providers[i]=="CODEX"?"CX":"OC",MUTED);
        text(42,5,title,WHITE,22);
        text(204,5,statusName(statuses[i]),statusColor(statuses[i]),6);
        text(271,5,duration,MUTED,6);
      }
      flushStrip(25+row*24);previous[row]=key;
    }
  }
  String f1,f2;
  if(!online){
    f1=WiFi.status()==WL_CONNECTED?"Wi-Fi: есть":"Wi-Fi: нет";
    f2="НЕТ СВЯЗИ С MAC";
  }else{
    f1="Wi-Fi: есть   Интернет: "+String(internet=="ON"?"есть":internet=="OFF"?"нет":"?");
    String b=battery>=0?String(battery)+"%"+(charging?" +":""):"?";
    f2="VPN: "+String(vpn=="ON"?"вкл":vpn=="OFF"?"выкл":"?")+"  Mac: "+b;
    if(pages>1)f2+="  "+String(page+1)+"/"+String(pages);
  }
  if(f1!=previousFooter1){strip.fillScreen(BG);text(8,0,f1,MUTED);flushStrip(195);previousFooter1=f1;previousFooter2="";}
  if(f2!=previousFooter2){strip.fillScreen(BG);text(8,2,f2,online?MUTED:RED);flushStrip(216);previousFooter2=f2;}
}
void fetchAgents(){
  if(WiFi.status()!=WL_CONNECTED){online=false;count=0;bridgeIP=IPAddress();if(mdnsReady){MDNS.end();mdnsReady=false;}return;}
  if(!mdnsReady){String name="agent-monitor-"+String((uint32_t)ESP.getEfuseMac(),HEX);mdnsReady=MDNS.begin(name.c_str());}
  if(!mdnsReady){online=false;count=0;return;}
  if(!bridgeIP || millis()-lastResolve>30000){bridgeIP=MDNS.queryHost(BRIDGE_HOST,1000);lastResolve=millis();}
  if(!bridgeIP){online=false;count=0;return;}
  HTTPClient http;http.setTimeout(2000);http.useHTTP10(true);
  String url="http://"+bridgeIP.toString()+":"+String(BRIDGE_PORT)+"/api/agents";
  if(!http.begin(url)){online=false;count=0;return;}
  int code=http.GET();
  JsonDocument doc;
  DeserializationError err=code==200?deserializeJson(doc,http.getStream()):DeserializationError::EmptyInput;
  http.end();
  if(code!=200||err||!doc["agents"].is<JsonArray>()){online=false;count=0;bridgeIP=IPAddress();return;}
  count=0;
  for(JsonObject a:doc["agents"].as<JsonArray>()){
    if(count>=80)break;
    String status=a["status"]|"UNKNOWN";if(status=="UNKNOWN")continue;
    providers[count]=String(a["provider"]|"");providers[count].toUpperCase();
    ids[count]=String(a["id"]|"");
    projects[count]=String(a["title"] | (a["project"] | "?"));statuses[count]=status;
    elapsed[count]=a["elapsedSeconds"].is<unsigned long>()?a["elapsedSeconds"].as<unsigned long>():0xFFFFFFFF;
    count++;
  }
  clockText=String(doc["system"]["clock"]|"--:--");vpn=String(doc["system"]["vpn"]|"UNKNOWN");internet=String(doc["system"]["internet"]|"UNKNOWN");
  battery=doc["system"]["battery"].is<int>()?doc["system"]["battery"].as<int>():-1;charging=doc["system"]["charging"]|false;
  receivedAt=millis();online=true;
}
void setup(){
  SPI.begin(TFT_SCK,-1,TFT_MOSI,TFT_CS);
  tft.init(240,320);
  // This panel needs inversion disabled; Adafruit ST7789 init enables it.
  tft.invertDisplay(false);
  tft.setRotation(1);tft.fillScreen(BG);
  WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);render();
}
void loop(){
  unsigned long now=millis();
  if(WiFi.status()!=WL_CONNECTED&&now-lastWifiAttempt>10000){lastWifiAttempt=now;WiFi.reconnect();}
  if(now-lastPoll>=2000){lastPoll=now;fetchAgents();render();}
  if(now-lastPage>=6000){lastPage=now;page++;render();}
  if(now-lastRender>=100){lastRender=now;render();}
  delay(20);
}
