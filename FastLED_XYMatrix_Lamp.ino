//
// By Serge Skorodinsky, 11/15/2019
//


/*
To add effects:
  1) constant color, depends on Style. No animation. Usage of Speed slider?
  2) constant color, randomly changing.
  3) 2D gradient with four corners colors randomly changing.
*/

/*
WiFiManager:
 1)fix date/time on the top of root page when in captive portal
 2) add diag-info - captive portal ipaddress:port
 3)correct diag-info:
   1) SSID
   2) four IPs: own, subnet mask, gateway, dns
   3) hostname
   
2/19/2021:
Change filesystem from SPIFFS (obsoleted) to LittleFS      
*/
 

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
WiFiManager wifiManager;

#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

#include "SSVXYMatrix.h"
#include "SSVRGBGradientCalc.h"
#include "SSVXYMatrixText.h"

FASTLED_USING_NAMESPACE

#include <SSVTimer.h>
SSVTimer Tmr_effects; 
SSVTimer Tmr_effectChange; 
SSVTimer Tmr_OnBoardLED; 
SSVTimer Tmr_BtnLongPress; 
SSVTimer DelayedResetTimer;

#include <ESP8266WebServer.h>   //WiFi object declared here. Also declared in <ESP8266WiFi.h>
ESP8266WebServer webserver (80);

#include "LittleFS.h" //file system
#include <ArduinoOTA.h>

//HostName is for three things - WiFi.hostname, mDNS hostname(.local) and OTA.setHostname
const char* HostNameTemplate = "LEDMatrix-%06x";
char HostName[20];

#define onboard_led 2 //D4, onboard LED, see FASTLED_ESP8266_RAW_PIN_ORDER above!
#define LED_PIN  D8 //15 //ESP8266 Lolin ESP-12E module

#include "OneButton.h"
#define btn1pin D6  //red PCB
OneButton button1(btn1pin, LOW, false); //OneButton(int pin, int active = LOW, bool pullupActive = true);

#define COLOR_ORDER GRB
#define CHIPSET     WS2811

// Params for width and height
const uint8_t MatrixWidth = 16;
const uint8_t MatrixHeight = 16;

#define NUM_LEDS (MatrixWidth * MatrixHeight)

CRGB leds[NUM_LEDS];

XYMatrix matrix(leds, MatrixWidth, MatrixHeight , true/*ZigZag*/ );
XYMatrixText matrixText(&matrix);
String matrixStr="";

//ahead declarations
//with timesAtOnce ???
void addGlitter( fract8 chanceOfGlitter, CRGB c, uint16_t timesAtOnce=0); //prototype

void TimerEffect1Func();
void TimerEffect2Func();
void TimerEffect3Func();
void TimerEffect4Func();
void TimerEffect5Func();
void TimerEffect6Func();
void TimerEffect7Func();
void TimerEffect8Func();
void TimerEffect9Func();
void TimerEffect10Func();
void TimerEffect11Func();
void TimerEffect12Func();
void TimerEffect13Func();
void TimerEffect14Func();
void TimerEffect15Func();
void TimerEffect16Func();
void TimerEffect17Func();
void TimerEffect18Func();
void TimerEffect19Func();
void TimerEffect20Func();
void TimerEffect21Func();
void TimerEffect22Func();
void TimerEffect23Func();
void TimerEffect24Func();
void TimerEffect25Func();
void TimerEffect26Func();
void TimerEffect27Func();
void TimerEffect28Func();
void TimerEffect29Func();
void TimerEffect30Func();
void TimerEffect31Func();
void TimerEffect32Func();

typedef struct EffectsStruct 
  {timer_callback EffectFunc; const char* EffectName; };

EffectsStruct Effects[] = 
  {
  {TimerEffect1Func,  "#1: 2D Rainbow, 4 Frames"},                //Speed: Done; Style: no
  {TimerEffect2Func,  "#2: 2D Rainbow, Full Matrix"},             //Speed: Done; Style: no
  {TimerEffect3Func,  "#3: Lissajous figure"},                    //Speed: Done; Style: no
  {TimerEffect4Func,  "#4: Kaleidoscope"},                        //Speed: Done 
  {TimerEffect5Func,  "#5: Fade Rectangles"},
  {TimerEffect6Func,  "#6: Fade Circles"},
  {TimerEffect7Func,  "#7: Fade Lines"},
  {TimerEffect8Func,  "#8: Circling Ball"},
  {TimerEffect9Func,  "#9: Rotating Colors"},                     //Speed: Done
  {TimerEffect10Func, "#10: Rotating Triangle"},                  //Speed: Done 
  {TimerEffect11Func, "#11: Pattern Shift"},
  {TimerEffect12Func, "#12: Colors Shift"},
  {TimerEffect13Func, "#13: Colors Shift 2(to debug)"},
  {TimerEffect14Func, "#14: 2D Plasma"},                           //Speed: Done; Style: Done
  {TimerEffect15Func, "#15: 2D Plasma (Random Palette)"},          //Speed: Done; Style: Done
  {TimerEffect16Func, "#16: 2D Plasma (Fade to Random Palette)"},  //Speed: Done; Style: Done
  {TimerEffect17Func, "#17: NTP Clock#1"},                         //Speed: Done
  {TimerEffect18Func, "#18: Confetti"},
  {TimerEffect19Func, "#19: Confetti with Fading Palette"},
  {TimerEffect20Func, "#20: Glitter"},
  {TimerEffect21Func, "#21: Glitter with Fading Palette"},
  {TimerEffect22Func, "#22: Sine with Hor. Wind"},
  {TimerEffect23Func, "#23: Sine with Vert. Wind"},
  {TimerEffect24Func, "#24: Sine (Oscilloscope) Hor."},
  {TimerEffect25Func, "#25: Sine (Oscilloscope) Vert."},
  {TimerEffect26Func, "#26: Fire2018"},
  {TimerEffect27Func, "#27: Fire2018-2"},
  {TimerEffect28Func, "#28: Metaballs"},
  {TimerEffect29Func, "#29: Noise_noise (fade to random Palette)"},  //what is the difference with 2D Plasme? gamma correction?
  {TimerEffect30Func, "#30: 1D inoise (fade to random Palette"},
  {TimerEffect31Func, "#31: Moving 2D Plasma (fade to random Palette"},
  {TimerEffect32Func, "#32: 2D Plasma Zoom (fade to random Palette"},
  };

String HTMLOptionList;
char HTMLOptionTpl[] = "<option value='?Effect=%Effect#%' %EffSel%>%EffName%</option>\n";

//uint8_t EffectNum=0;
uint8_t TotalEffects;//uint8_t arrsize = (sizeof(Effects))/(sizeof(Effects[0]));

char effect_init_str[] = "...effect #%d init.\r\n";

enum  TEffectChangeType {ECT_RANDOM=0, ECT_SEQUENTIAL=1, ECT_NO_CHANGE=2};

//control variables
boolean IsON=false;
boolean EffectInitReq=false;
//unsigned long OFFtimestamp;
//TEffectChangeType EffChangeType = ECT_RANDOM;
//unsigned long EffChangeInterval = 60; //default effect change interval, seconds
CRGBPalette16 Palette1, Palette2;

// NTP Clock
//SSVNTPCore - singleton object
//11-06-2020: new Timezone logic
//11/22/2020: redone NTP clock with another class
#include "SSVNTPCore.h"
//https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
//POSIX format, https://developer.ibm.com/articles/au-aix-posix/
#define MyTZ "EST5EDT,M3.2.0,M11.1.0"  //automatically consider DST 
#define NTPDateFmt "%m/%d/%Y"          // "11/23/2020", to show date on the webpage
#define NTPTimeFmt "%T"                // "15:39:19", to show time on the webpage
#define NTPMatrixShowFmt "%a %H:%M"    // "Mon 15:39", to show on the matrix

/////////////////////// config
struct TConfig 
  {
  uint8_t Brightness; //saving done
  uint8_t Speed;      //no process yet for all effects
  uint8_t EffectNum;  //current effect selected, saving done
  TEffectChangeType EffChangeType;  //EffectChangeType - random, sequential, no_change
  unsigned long EffChangeInterval; //default effect change interval, seconds
  uint8_t Style;
  };
TConfig myConf =   //default config
  {
    100, //Brightness
    25, //Speed
    0,   //CurrentEffectNumber
    ECT_RANDOM, //EffectChangeType - random, sequential, no_change
    60, //default effect change interval, seconds
    127  //default Style
  };
const char *configFile = "/config.dat";
//flag, requesting to save config. 
//if 0 - save not needed, else timestamp when request was posted, used for delayed saving.
unsigned long SaveConfReqTS=0; 
////////

//global unified variable - used with different effects to keep persistent effect-specific data
union TAnyTypeVariable
{
  struct {uint8_t  V1, V2, V3, V4, V5, V6, V7, V8;} U8;
  struct {int8_t   V1, V2, V3, V4, V5, V6, V7, V8;} I8;
  struct {uint16_t V1,     V2,     V3,     V4;} U16;
  struct {int16_t  V1,     V2,     V3,     V4;} I16;
  struct {uint32_t V1,             V2;} U32;
  struct {int32_t  V1,             V2;} I32;
  struct {uint64_t V;} U64;
  struct {int64_t  V;} I64;
};
TAnyTypeVariable gVar;
////////

uint8_t gU8Array1[MatrixWidth][MatrixHeight];  //storage for the array data, like noise array.
uint8_t gU8Array2[MatrixWidth][MatrixHeight];  //another storage for array data
uint8_t gU8Array3[MatrixWidth][MatrixHeight];  //one more storage for array data

#include "SSVLongTime.h"

void setup() 
{
  TotalEffects = (sizeof(Effects))/(sizeof(Effects[0]));
  Serial.begin(115200);
  pinMode(onboard_led, OUTPUT ); //led
  digitalWrite(onboard_led, HIGH); //off
  LittleFS.begin();  
  delay(1000); //for stability
  Serial.println();Serial.println();
  uint8_t errcode = LoadConfigFromFile();
  if (errcode!=0)
    {
     Serial.printf("Error loading config: %d, ", errcode);
     if (errcode==1) Serial.printf("File not found, defaults loaded\r\n");
     if (errcode==2) Serial.printf("Cannot read all file, defaults loaded\r\n");
    }
    else {Serial.printf("Loading config OK\r\n");}
  CombineHTMLOptionList();
  Tmr_effects.SetInterval(20);
  Tmr_effects.SetOnTimer(Effects[0].EffectFunc);
  Tmr_effects.SetEnabled(true);
  Tmr_effectChange.SetOnTimer(Tmr_EffectChangeCB);
  Tmr_effectChange.SetInterval(myConf.EffChangeInterval * 1000); //default is 60 - change once in a minute
  Tmr_effectChange.SetEnabled(false);
  Tmr_OnBoardLED.SetInterval(20);
  Tmr_OnBoardLED.SetOnTimer(Tmr_OnBrdLED);
  Tmr_OnBoardLED.SetEnabled(false);
  Tmr_BtnLongPress.SetInterval(100);
  Tmr_BtnLongPress.SetOnTimer(BtnLongPressCB);
  Tmr_BtnLongPress.SetEnabled(false);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(myConf.Brightness);
  FastLED.clear ();
  FastLED.show();
  WiFi.mode(WIFI_STA); 
  //HostName
  //sprintf(HostName, HostNameTemplate, ESP.getChipId());
  //some cheating :-) with HostName
  if (ESP.getChipId() == 0x27cf08) sprintf(HostName, "LEDMatrixLamp"); //Lamp board
    else sprintf(HostName, HostNameTemplate, ESP.getChipId());

matrixText.setCharSpace(2);
matrixStr="Power UP!";
uint8_t BtnStatus = ScrollTextAndReturnBtnStatus(&matrixStr, CRGB::Yellow, CRGB::Black, 1);
if (BtnStatus==0x80) 
  {
  matrixStr="BUTTON DISABLED...";
  ScrollTextAndReturnBtnStatus(&matrixStr, CRGB::Red, CRGB::Black, 1);
  };


//WiFiManager
if (BtnStatus==0x02) //pressed and released
  {
  Serial.println("WiFiReset!");
  wifiManager.resetSettings();
  } 
   else 
    {
    Serial.println("NO WiFiReset needed...");
    }
//wifiManager.setAPCallback(enterConfigModeCB);   //not in non-blocking mode
wifiManager.setSaveConfigCallback(saveWiFiConfigCB); 
wifiManager.setConfigPortalBlocking(false);
wifiManager.setHttpPort(8080);
//wifiManager.setDebugOutput(false);
//wifiManager.resetSettings();  //for debug only!
wifiManager.setHostname(HostName);
unsigned long ts=millis();
unsigned int cnt=0;
if(wifiManager.autoConnect(HostName, "password"))  //"AutoConnectAP"
  {
  Serial.println("Connected to saved WiFi");
  Serial.print("WiFiMode: "); Serial.println(getWiFiModeString(WiFi.getMode()) );
  WiFi.hostname(HostName);
  //WiFi.begin(ssid, password);
  //connect to wifi
  //ts=millis();
  //cnt=0;
  Serial.print("WiFi connected with ip ");  
  Serial.println(WiFi.localIP());
  matrixStr="IP: "+WiFi.localIP().toString();
  ScrollTextAndReturnBtnStatus(&matrixStr, CRGB::Green, CRGB::Black, 1);
  }
   else 
    {
    Serial.println("Config-portal running");
    Serial.print("WiFiMode: "); Serial.println(getWiFiModeString(WiFi.getMode()) );
    matrixStr="AP: "+String(HostName)+", IP: "+WiFi.softAPIP().toString();
    ScrollTextAndReturnBtnStatus(&matrixStr, CRGB::Fuchsia, CRGB::Black, 1);
    }
  
  digitalWrite(onboard_led, HIGH); //off
  Serial.printf("Host Name: %s\r\n", HostName);
  //web-server
  webserver.on ("/", webSrvHandleRoot);
  webserver.onNotFound (webSrvHandleNotFound);
  webserver.on ("/diag", webSrvHandleDiagInfo);
  webserver.on ("/reset", webSrvHandleReset);
  webserver.begin();
  Serial.println ("HTTP server started");
  //     ArduinoOTA
  //ArduinoOTA.setPort(8266);            // Port defaults to 8266
  ArduinoOTA.setHostname(HostName); // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setPassword("admin");    // No authentication by default
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3"); //not working (S.S.)!!
  ArduinoOTA.onStart(OTA_onStart);
  ArduinoOTA.onEnd(OTA_onEnd);
  ArduinoOTA.onProgress(OTA_onProgress);
  ArduinoOTA.onError(OTA_onError);
  ArduinoOTA.begin();

  //NTP time
  //SSVNTPCore - singleton object
  if (WiFi.status() == WL_CONNECTED)
    {
     SSVNTPCore.setTZString(MyTZ);
     SSVNTPCore.setUpdateInterval(3600000UL * 24); //min is 15000 (15 sec), default 3600000 (1 hour)
     SSVNTPCore.setServerName("pool.ntp.org", "time.nist.gov" /*, "s-ergeskor.no-ip.org"*/);
     SSVNTPCore.begin();
     Serial.println("\nConnecting to NTP Pool");
     bool NTPres = SSVNTPCore.WaitForFirstUpdate(1000);
     if (NTPres) Serial.println("NTP Time sync'ed  OK.");  
            else Serial.println("NTP Time is NOT sync'ed.");  
     }
    
  if (SSVNTPCore.isNeverUpdated()) {Serial.println("NTP not updated...");}
      else 
        {
        Serial.print("NTP update OK. ");
        Serial.print(SSVNTPCore.getFormattedDateTimeString(NTPDateFmt) );
        Serial.print(" ");
        Serial.println(SSVNTPCore.getFormattedDateTimeString(NTPTimeFmt) );
        }
  
  //done, prepare to start main loop
  if (IsON)
    {
    IsON=true;
    Tmr_effects.SetOnTimer(Effects[myConf.EffectNum].EffectFunc);
    Tmr_effectChange.SetEnabled(true);
    EffectInitReq=true;
    Tmr_effects.SetEnabled(false);
    Tmr_OnBoardLED.SetEnabled(false);
    Serial.printf("Starting ON\r\n");
    }
    else 
    {
    IsON=false;
    Tmr_effects.SetOnTimer(TimerOFFFunc1);
    Tmr_effectChange.SetEnabled(false);
     EffectInitReq=true;
    Tmr_OnBoardLED.SetEnabled(true);
    Serial.printf("Starting OFF\r\n");
    }
  Serial.printf("Effect #%d, ", myConf.EffectNum+1);
  Serial.print("Name: "); Serial.println(Effects[myConf.EffectNum].EffectName);

  // link the button 1 functions.
  button1.attachClick(click1);
  button1.attachDoubleClick(doubleclick1);
  button1.attachLongPressStart(longPressStart1);
  button1.attachLongPressStop(longPressStop1);
  button1.attachDuringLongPress(longPress1);
  button1.setDebounceTicks(40); //# of ticks for debounce times. default 50
  button1.setClickTicks(600);   //# of ticks that have to pass by before a click is detected. default is 600. 
  button1.setPressTicks(1000);  //# of ticks that have to pass by before a long button press is detected. default 1000

  //delayed reset timer
  DelayedResetTimer.SetInterval(2000);
  DelayedResetTimer.SetOnTimer(DelayedResetTimerCBFunc);
  DelayedResetTimer.SetEnabled(false);  
} //end of setup()

void loop()
{
  wifiManager.process(); //WiFi Manager
  Tmr_effects.RefreshIt();
  Tmr_effectChange.RefreshIt();
  Tmr_OnBoardLED.RefreshIt();
  webserver.handleClient(); //check httpserver
  SSVLongTime.process();
  ArduinoOTA.handle();
//  boolean b = timeClient.update();  //moved it to mr_OnBrdLED(), when all leds are off.
if ( (SaveConfReqTS!=0) && (millis()-SaveConfReqTS > 1000) )
  {
    //delayed (1 sec) saving config
    SaveConfReqTS=0;
    if (SaveConfigToFile() == 0 ) 
      Serial.printf("Saving config OK\r\n");
       else Serial.printf("Error saving config\r\n");
  }
button1.tick();
Tmr_BtnLongPress.RefreshIt();
DelayedResetTimer.RefreshIt();
}

uint8_t ScrollTextAndReturnBtnStatus(String* S, CRGB TextColor, CRGB BackgoundColor, uint8_t ScrollTimes) //blocking! 
{
//Returns status of button.
// If Button was pressed on the moment when this function has called - high bit is Hihg, otherwise is Low
// The rest seven bits indicate how many times button has changed its state.
//pressed - High!
uint8_t cnt=0;
uint8_t wasPressedBefore=0x00;  
boolean btnStatus = digitalRead(btn1pin);
if (btnStatus) wasPressedBefore=0x80;
//
matrixText.setString(S);
if (ScrollTimes>0)
  {//scroll
    for (uint8_t n=0; n<ScrollTimes; n++) //how many rounds
    {
      for (int16_t x=matrixText.OffsetX_LeftStr_To_RightArea(); x>=matrixText.OffsetX_RightStr_To_LeftArea(); x--)
        {
        fill_solid(leds, NUM_LEDS, BackgoundColor);
        matrixText.drawString(x, matrixText.OffsetY_Center(), TextColor);
        FastLED.show();
        if (digitalRead(btn1pin) != btnStatus)
          {
          btnStatus = !btnStatus;
          if (cnt<0x7F) cnt++;
          }
        delay (40);
        }
    }
  }
   else
    {//not scroll, show in center for 4 seconds
    fill_solid(leds, NUM_LEDS, BackgoundColor);
    matrixText.drawString(matrixText.OffsetX_Center(), matrixText.OffsetY_Center(), TextColor);
    FastLED.show();
    for (uint8_t n=0; n<100; n++) 
      {
      if (digitalRead(btn1pin) != btnStatus)
        {
        btnStatus = !btnStatus;
        if (cnt<126) cnt++;
        }
      delay (40);
      }
    }
//Serial.println(cnt + wasPressedBefore, HEX);
return (cnt + wasPressedBefore);
}

//fill 2D (Width x Height) with value (zero)
void Fill2DWH_U8_Array(uint8_t arr[MatrixWidth][MatrixHeight], uint8_t value=0) 
{
  for (uint8_t y=0; y<MatrixHeight; y++) 
  for (uint8_t x=0; x<MatrixWidth; x++)   
    {arr[x][y] = value;}
}

void saveWiFiConfigCB () 
{ 
  //calse when wifiManager succesfully save wifi ssid
  turnON_OFF(false);
  digitalWrite(onboard_led, HIGH); //off
  //ESP.reset(); 
  matrixStr="IP: "+WiFi.localIP().toString();
  ScrollTextAndReturnBtnStatus(&matrixStr, CRGB::Green, CRGB::Black, 1);
}

void webSrvHandleReset()
{
  String addy = webserver.client().remoteIP().toString();
  Serial.println(addy);
  
  Serial.println("handleReset()");
  if (LittleFS.exists("/redirect.html")) 
    {// If the file exists
      File file = LittleFS.open("/redirect.html", "r");   
      //size_t sent = server.streamFile(file, "text/html");

      String Str1;
      Str1 = file.readString();
      //  replacements
      Str1.replace("$REDIR_DELAY$", String(1) );  //replace
      Str1.replace("$REDIR_URL$",   WiFi.localIP().toString() );  //replace
      //
      webserver.send(200, "text/html", Str1 ); 
      file.close();
    }
     else 
       {
        Serial.print( webserver.uri() );
        webserver.send(404, "text/plain", "404: File NOT FOUND!");
       }
  Tmr_effects.SetEnabled(false); 
  digitalWrite(onboard_led, HIGH); //off     
  FastLED.clear();
  FastLED.show();  
  DelayedResetTimer.SetEnabled(true);
}

void DelayedResetTimerCBFunc()
{  ESP.reset(); }

void TimerEffect1Func()
{
  if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      //use as Q8.8 format BPM for beatsin88()
      gVar.U16.V1=myConf.Speed*256/20;
      gVar.U16.V2=myConf.Speed*256/20*2;
      gVar.U16.V3=myConf.Speed*256/20*3;
    }
  RectangleWH R1 = {Point(0,0), 7, 7};
  matrix.draw_2DRainbow(R1, beatsin88(gVar.U16.V1, 0, 255), Point(7,7), beatsin88(gVar.U16.V2, 0, 64, 0, 0)-32, beatsin88(gVar.U16.V3, 0, 64, 0, 0)-32);
  RectangleWH R2 = {Point(8,0), 7, 7};
  matrix.draw_2DRainbow(R2, beatsin88(gVar.U16.V1, 0, 255), Point(7,7), beatsin88(gVar.U16.V2, 0, 64, 0, 16384)-32, beatsin88(gVar.U16.V3, 0, 64, 0, 16384)-32);
  RectangleWH R3 = {Point(0,8), 7, 7};
  matrix.draw_2DRainbow(R3, beatsin88(gVar.U16.V1, 0, 255), Point(7,7), beatsin88(gVar.U16.V2, 0, 64, 0, 32768)-32, beatsin88(gVar.U16.V3, 0, 64, 0, 32768)-32);
  RectangleWH R4 = {Point(8,8), 7, 7};
  matrix.draw_2DRainbow(R4, beatsin88(gVar.U16.V1, 0, 255), Point(7,7), beatsin88(gVar.U16.V2, 0, 64, 0, 49152)-32, beatsin88(gVar.U16.V3, 0, 64, 0, 49152)-32);
  FastLED.show();
}

void TimerEffect2Func()
{
    if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      //use as Q8.8 format BPM for beatsin88()
      gVar.U16.V1=myConf.Speed*256/20;
      gVar.U16.V2=myConf.Speed*256/20*2;
      gVar.U16.V3=myConf.Speed*256/20*3;
    }
  matrix.draw_2DRainbow(beatsin88(gVar.U16.V1, 0, 255), Point(7,7), beatsin88(gVar.U16.V2, 0, 64)-32, beatsin88(gVar.U16.V3, 0, 64)-32);
  FastLED.show();
}

void TimerEffect3Func()
{
    if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      gVar.U8.V1=myConf.Speed/*/3*/;
      gVar.U8.V2=gVar.U8.V1*1.1;
      Tmr_effects.SetInterval(20);
      //Tmr_effects.SetInterval(myConf.Speed+10);
    }
  fadeToBlackBy( leds, NUM_LEDS, 3);
  //uint8_t posX = triwave8(beat8(37)) /16;
  //uint8_t posY = triwave8(beat8(23)) /16;
  uint8_t posX = triwave8(beat8(gVar.U8.V1)) /16;
  uint8_t posY = triwave8(beat8(gVar.U8.V2)) /16;

  uint8_t hue = beat8(5);
  matrix.setPixelColor(posX,posY, CHSV( hue, 255, 255) );
  FastLED.show();
}

void TimerEffect4Func()
{
    if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      //use as Q8.8 format BPM for beatsin88()
      gVar.U16.V1=myConf.Speed*256/20;
      gVar.U16.V2=myConf.Speed*256/15*2;
      gVar.U16.V3=myConf.Speed*256/18*3;
      gVar.U16.V4=myConf.Speed*256/13*4;
    }
/*  colors layout: 
  c3  c6  c9
  c2  c5  c8
  c1  c4  c7
*/

  CRGB cCorners = CHSV(beatsin88(gVar.U16.V1, 0, 255),     beatsin88(gVar.U16.V3, 16, 255),  beatsin88(gVar.U16.V2, 16, 255));                        
  CRGB cMidSides =CHSV(beatsin88(gVar.U16.V3, 0, 255),     beatsin88(gVar.U16.V1, 16, 255),  beatsin88(gVar.U16.V4, 16, 255));
  CRGB cCenter =  CHSV(beatsin88(gVar.U16.V2+100, 0, 255), beatsin88(gVar.U16.V4, 16, 255),  beatsin88(gVar.U16.V3+80, 16, 255));
  
  CRGB c1 = cCorners;
  CRGB c3 = cCorners;
  CRGB c7 = cCorners;
  CRGB c9 = cCorners;

  CRGB c2 = cMidSides;
  CRGB c4 = cMidSides;
  CRGB c6 = cMidSides;
  CRGB c8 = cMidSides;

  CRGB c5 = cCenter;
  
  RectangleWH R1 = {Point(0,0), 7, 7};
  RectangleWH R2 = {Point(8,0), 7, 7};
  RectangleWH R3 = {Point(0,8), 7, 7};
  RectangleWH R4 = {Point(8,8), 7, 7};

  matrix.draw_2DGradient(R1, c1, c2, c4, c5); // LeftBottomColor, LeftTopColor, RightBottomColor, RightTopColor)
  matrix.draw_2DGradient(R2, c4, c5, c7, c8); // LeftBottomColor, LeftTopColor, RightBottomColor, RightTopColor)
  matrix.draw_2DGradient(R3, c2, c3, c5, c6); // LeftBottomColor, LeftTopColor, RightBottomColor, RightTopColor)
  matrix.draw_2DGradient(R4, c5, c6, c8, c9); // LeftBottomColor, LeftTopColor, RightBottomColor, RightTopColor)
 
  FastLED.show();
}

void TimerEffect5Func()
{
    if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }
  fadeToBlackBy( leds, NUM_LEDS, 10);
  long how_often=250; //mS
  if ((Tmr_effects.GetCounter() % ( how_often / Tmr_effects.GetInterval() ) ) == 0) //every how_often mS
    {
     uint8_t s=random(3,10); //square size
     RectangleWH R = {Point(random(15-s), random(15-s)), s, s};
     if (random(255) > 128) 
       matrix.draw_rect(R, CHSV( random(255), 255, 255));
      else 
       matrix.draw_fillrect(R, CHSV( random(255), 255, 255));
    }
  FastLED.show();
}

void TimerEffect6Func()
{
    if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }
  fadeToBlackBy( leds, NUM_LEDS, 10);
  long how_often=250; //mS
  if ((Tmr_effects.GetCounter() % ( how_often / Tmr_effects.GetInterval() ) ) == 0) //every how_often mS
    {
     uint8_t r=random(2,6); //radius
     uint8_t cx = random (r, MatrixWidth-r);
     uint8_t cy = random (r, MatrixHeight-r);
     if (random(255) > 128) 
       matrix.draw_circle(cx, cy, r, CHSV( random(255), 255, 255));
      else 
       matrix.draw_fillCircle(cx, cy, r, CHSV( random(255), 255, 255));
    }
  FastLED.show();
}

void TimerEffect7Func()
{
    if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }
  fadeToBlackBy( leds, NUM_LEDS, 10);
  long how_often=250; //mS
  if ((Tmr_effects.GetCounter() % ( how_often / Tmr_effects.GetInterval() ) ) == 0) //every how_often mS
    {
     uint8_t x1 = random (MatrixWidth-1);
     uint8_t y1 = random (MatrixHeight-1);
     uint8_t x2 = random (MatrixWidth-1);
     uint8_t y2 = random (MatrixHeight-1);
     matrix.draw_line_gradient(x1, y1, x2, y2, CHSV( random(255), 255, 255), CHSV( random(255), 255, 255));
    }
  FastLED.show();
}

void TimerEffect8Func()
{
    if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int16_t r = 2;
  uint8_t posCX = beatsin8(20, r, MatrixWidth-r, 0, 0 );
  uint8_t posCY = beatsin8(20, r, MatrixHeight-r, 0, 64 ); //90 deg phase shift
  uint8_t hue = beat8(5);
  matrix.draw_circle(posCX, posCY, r, CHSV(hue, 255, 255) );
  FastLED.show();
}

void TimerEffect9Func()
{
    if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      //use as Q8.8 format BPM for beatsin88()
      gVar.U16.V1=myConf.Speed*256/4; //speed

    }
  fadeToBlackBy( leds, NUM_LEDS, 50);
  //external circle
  int16_t r = 1;
  uint8_t CX1 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r,  0, 0 );
  uint8_t CY1 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r, 0, 16383); //90 deg phase shift
  CRGB clr1 = CRGB::Red; //beat8(5);

  uint8_t CX2 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r,  0, 0+21845);  //120 deg phase shift
  uint8_t CY2 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r, 0, 16383+21845); //90+120 deg phase shift
  CRGB clr2 = CRGB::Green; //beat8(5);

  uint8_t CX3 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r,  0,0+43690); //240 deg phase shift
  uint8_t CY3 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r, 0,16383+43690); //90+240 deg phase shift
  CRGB clr3 = CRGB::Blue; //beat8(5);

  matrix.setPixelColor(CX1, CY1, clr1);
  matrix.setPixelColor(CX2, CY2, clr2);
  matrix.setPixelColor(CX3, CY3, clr3);

  //internal circle
  r = 4;
  //another direction!
  CX1 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r, 0, 16383); //90 deg phase shift
  CY1 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r,  0, 0 );
  clr1 = CRGB::Yellow; //beat8(5);

  CX2 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r, 0, 16383+21845); //90+120 deg phase shift
  CY2 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r,  0, 0+21845);  //120 deg phase shift
  clr2 = CRGB::Pink; //beat8(5);

  CX3 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r, 0,16383+43690); //90+240 deg phase shift
  CY3 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r,  0,0+43690); //240 deg phase shift
  clr3 = CRGB::Magenta; //beat8(5);

  matrix.setPixelColor(CX1, CY1, clr1);
  matrix.setPixelColor(CX2, CY2, clr2);
  matrix.setPixelColor(CX3, CY3, clr3);

/*
  //center circle
  //same direction as external circle
  r = 5;
  CX1 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r,  0, 0 );
  CY1 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r, 0, 16383); //90 deg phase shift
  clr1 = CRGB::Red; //beat8(5);

  CX2 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r,  0, 0+21845);  //120 deg phase shift
  CY2 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r, 0, 16383+21845); //90+120 deg phase shift
  clr2 = CRGB::Green; //beat8(5);

  CX3 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r,  0,0+43690); //240 deg phase shift
  CY3 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r, 0,16383+43690); //90+240 deg phase shift
  clr3 = CRGB::Blue; //beat8(5);

  matrix.setPixelColor(CX1, CY1, clr1);
  matrix.setPixelColor(CX2, CY2, clr2);
  matrix.setPixelColor(CX3, CY3, clr3);
*/
  
  FastLED.show();
}

void TimerEffect10Func()  // rotating triangle (one direction, constant speed)
{
    if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      //use as Q8.8 format BPM for beatsin88()
      gVar.U16.V1=myConf.Speed*256/4; //speed
    }
  fadeToBlackBy( leds, NUM_LEDS, 40);
  int16_t r = 1;
  uint8_t CX1 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r,  0, 0 );
  uint8_t CY1 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r, 0, 16383); //90 deg phase shift
  CRGB clr1 = CRGB::Red; //beat8(5);

  uint8_t CX2 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r,  0, 0+21845);  //120 deg phase shift
  uint8_t CY2 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r, 0, 16383+21845); //90+120 deg phase shift
  CRGB clr2 = CRGB::Green; //beat8(5);

  uint8_t CX3 = beatsin88(gVar.U16.V1, r-1, MatrixWidth-r,  0,0+43690); //240 deg phase shift
  uint8_t CY3 = beatsin88(gVar.U16.V1, r-1, MatrixHeight-r, 0,16383+43690); //90+240 deg phase shift
  CRGB clr3 = CRGB::Blue; //beat8(5);

  matrix.draw_line_gradient(CX1, CY1, CX2, CY2, clr1, clr2);
  matrix.draw_line_gradient(CX2, CY2, CX3, CY3, clr2, clr3);
  matrix.draw_line_gradient(CX3, CY3, CX1, CY1, clr3, clr1);

  FastLED.show();
}

void TimerEffect11Func()
{
  static uint8_t dir;
  if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      FastLED.clear();
      matrix.draw_rect(2, 2, 13, 13,  CRGB::Green);
      matrix.draw_fillrect(5, 5, 10, 10,  CRGB::Yellow);
      matrix.draw_line(0, 0,  15, 15, CRGB::Blue,true);
      matrix.draw_line(0, 15, 15, 0,  CRGB::Red, true);
      dir=0;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }

  long how_often=5000; //mS
  if ((Tmr_effects.GetCounter() % ( how_often / Tmr_effects.GetInterval() ) ) == 0) //every how_often mS
    {
     dir = random (8);
    }
    
  how_often=100; //mS
  if ((Tmr_effects.GetCounter() % ( how_often / Tmr_effects.GetInterval() ) ) == 0) //every how_often mS
    {
     matrix.Shift_RectRoundDir(dir);
    }
  FastLED.show();  
}

void TimerEffect12Func()
{
  static uint8_t dir;
  if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      FastLED.clear();
      dir=0;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }

  long how_often=500; //mS
  if ((Tmr_effects.GetCounter() % ( how_often / Tmr_effects.GetInterval() ) ) == 0) //every how_often mS
    {
     dir = random (8);
    }
    
  how_often=75; //mS
  if ((Tmr_effects.GetCounter() % ( how_often / Tmr_effects.GetInterval() ) ) == 0) //every how_often mS
    {

     CRGB c1 =  CHSV( triwave8(beat8(13)), 255, 255); //13
     CRGB c2 =  CHSV( triwave8(beat8(8)),  255, 255); //8
     //fadeToBlackBy( leds, NUM_LEDS, 40);
     matrix.Shift_RectDir(dir, c1, c2);
    }
  FastLED.show();  
}

void TimerEffect13Func()
{
  static uint8_t dir;
  if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      FastLED.clear();
      dir=0;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }

  long how_often=500; //mS
  if ((Tmr_effects.GetCounter() % ( how_often / Tmr_effects.GetInterval() ) ) == 0) //every how_often mS
    {
     dir = random (8);
    }
    
  how_often=75; //mS
  if ((Tmr_effects.GetCounter() % ( how_often / Tmr_effects.GetInterval() ) ) == 0) //every how_often mS
    {

//beatsin8(f, r, MatrixHeight-r, 0, 64+170 ); //90 deg phase shift

     CRGB c1[16]; for( byte i=0; i <= 15; i++) {c1[i]=CHSV(beatsin8(5, i), 255, 255);} //not done yet
     CRGB c2[16]; for( byte i=0; i <= 15; i++) {c2[i]=CHSV(beatsin8(7, i), 255, 255);} //not done yet
     //fadeToBlackBy( leds, NUM_LEDS, 40);
     matrix.Shift_RectDir(dir, c1, c2);
    }
  FastLED.show();  
}

void TimerEffect14Func()
{
  if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      FastLED.clear();
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      gVar.U32.V1 = 0; //used as Z=axis, time coordinate, counter.
      /*
      Two-peaces interpolation used.
      Translations are:
      Slider Position -> TimerInterval[mS]
        1   -> 250
        120 -> 60
        255 -> 15
      Slider Position -> Increment
        1   -> 250
        40  -> 10
        255 -> 20
      Speed [Increments per Sec] = Increment*1000/TimerInterval[mS].  
      So, resulting Speed is (approx.):
      Slider Position -> Speed [Increments per Sec]
        1   -> 4
        64  -> 73.8
        128 -> 245.6
        192 -> 472.2
        255 -> 1333
      */
      uint16_t tmrint=TwoPeacesApprox(myConf.Speed, 1, 120, 255, 250, 60, 15); //timer interval
      Tmr_effects.SetInterval(tmrint);
      gVar.U16.V3 = TwoPeacesApprox(myConf.Speed, 1, 40, 255, 1, 10, 20); //increment
      Serial.printf("Timer Interval: %dmS, increment: %d, Speed: %d\r\n", Tmr_effects.GetInterval(), gVar.U16.V3, gVar.U16.V3*1000/Tmr_effects.GetInterval() );
    }
  gVar.U32.V1 = gVar.U32.V1 + gVar.U16.V3;
  matrix.draw_2DPlasma(0, 0, gVar.U32.V1, myConf.Style/2+4 /*30*/, true, true);
  FastLED.show();  
}

void TimerEffect15Func()
{
  if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Palette1 = GetRandomPalette();
      FastLED.clear();
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      gVar.U32.V1 = 0; //used as Z=axis, time coordinate, counter.
      /*
      Two-peaces interpolation used.
      Translations are:
      Slider Position -> TimerInterval[mS]
        1   -> 250
        120 -> 60
        255 -> 15
      Slider Position -> Increment
        1   -> 250
        40  -> 10
        255 -> 20
      Speed [Increments per Sec] = Increment*1000/TimerInterval[mS].  
      So, resulting Speed is (approx.):
      Slider Position -> Speed [Increments per Sec]
        1   -> 4
        64  -> 73.8
        128 -> 245.6
        192 -> 472.2
        255 -> 1333
      */
      uint16_t tmrint=TwoPeacesApprox(myConf.Speed, 1, 120, 255, 250, 60, 15); //timer interval
      Tmr_effects.SetInterval(tmrint);
      gVar.U16.V3 = TwoPeacesApprox(myConf.Speed, 1, 40, 255, 1, 10, 20); //increment
      Serial.printf("Timer Interval: %dmS, increment: %d, Speed: %d\r\n", Tmr_effects.GetInterval(), gVar.U16.V3, gVar.U16.V3*1000/Tmr_effects.GetInterval() );
    }
  gVar.U32.V1 = gVar.U32.V1 + gVar.U16.V3;  
  matrix.draw_2DPlasmaPal(Palette1, 0, 0, gVar.U32.V1, myConf.Style/2+4 /*30*/, true, true, LINEARBLEND);
  FastLED.show();  
}

void TimerEffect16Func()
{
  if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Palette1 = GetRandomPalette();
      Palette2 = GetRandomPalette(); //target palette
      FastLED.clear();
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      gVar.U32.V1 = 0; //used as Z=axis, time coordinate, counter.
      /*
      Two-peaces interpolation used.
      Translations are:
      Slider Position -> TimerInterval[mS]
        1   -> 250
        120 -> 60
        255 -> 15
      Slider Position -> Increment
        1   -> 250
        40  -> 10
        255 -> 20
      Speed [Increments per Sec] = Increment*1000/TimerInterval[mS].  
      So, resulting Speed is (approx.):
      Slider Position -> Speed [Increments per Sec]
        1   -> 4
        64  -> 73.8
        128 -> 245.6
        192 -> 472.2
        255 -> 1333
      */
      uint16_t tmrint=TwoPeacesApprox(myConf.Speed, 1, 120, 255, 250, 60, 15); //timer interval
      Tmr_effects.SetInterval(tmrint);
      gVar.U16.V3 = TwoPeacesApprox(myConf.Speed, 1, 40, 255, 1, 10, 20); //increment
      Serial.printf("Timer Interval: %dmS, increment: %d, Speed: %d\r\n", Tmr_effects.GetInterval(), gVar.U16.V3, gVar.U16.V3*1000/Tmr_effects.GetInterval() );
    }

if (Tmr_effects.GetCounter() % 40 == 0)
  {
  //check periodically, if palettes are the same - get new random
  if (ArePalettesEqual(Palette1, Palette2))
    { Palette2 = GetRandomPalette();} //change target palette
  }
  nblendPaletteTowardPalette( Palette1, Palette2, 24);  //fade Palette1 to Palette2.

  gVar.U32.V1 = gVar.U32.V1 + gVar.U16.V3;  
  matrix.draw_2DPlasmaPal(Palette1, 0, 0, gVar.U32.V1, myConf.Style/2+4 /*30*/, true, true, LINEARBLEND);
  FastLED.show();  
}

void TimerEffect17Func()
{
  if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      uint16_t tmrint=TwoPeacesApprox(myConf.Speed, 1, 120, 255, 1000, 100, 15); //timer interval
      Tmr_effects.SetInterval(tmrint);
      Serial.printf("Timer Interva: %d\r\n", tmrint);
      gVar.I16.V1 = matrixText.OffsetX_LeftStr_To_LeftArea() + 3; //signed! used as posX
    }
//if (Tmr_effects.GetCounter() % 8 == 0) //slower than timer
//  {
  FastLED.clear();

  matrixStr=SSVNTPCore.getFormattedDateTimeString(NTPMatrixShowFmt);

  //matrixText.setCharRotation(Rotate_CW90);
  //matrixText.setUpdateBackground(true);
  matrixText.setString(&matrixStr);
  if (gVar.I16.V1 > matrixText.OffsetX_RightStr_To_LeftArea()) 
    gVar.I16.V1--; 
      else gVar.U16.V1 = matrixText.OffsetX_LeftStr_To_RightArea();
  CRGB c = CHSV (beatsin88(128, 0, 255), 255, 255);  //Q8.8! 256 - 1 Minute.
  matrixText.drawString(gVar.I16.V1, matrixText.OffsetY_Center(), c);
//  }
FastLED.show();  
}

void TimerEffect18Func() //Confetti
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(50);
    }
fadeToBlackBy( leds, NUM_LEDS, 5);
int pos = random16(NUM_LEDS);
leds[pos] += CHSV( beatsin88(128, 0, 255) + random8(64), 200, 255);
FastLED.show();
}

void TimerEffect19Func() //Confetti with Fading Palette
{
 if (EffectInitReq) 
    {
     //effect initialization is here
      EffectInitReq=false;
      Palette1 = GetRandomPalette();
      Palette2 = GetRandomPalette(); //target palette
      FastLED.clear();
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }

if (Tmr_effects.GetCounter() % 40 == 0)
  {
  //check periodically, if palettes are the same - get new random
  if (ArePalettesEqual(Palette1, Palette2))
    { Palette2 = GetRandomPalette();} //change target palette
  }
nblendPaletteTowardPalette( Palette1, Palette2, 12);  //fade Palette1 to Palette2.

    
fadeToBlackBy( leds, NUM_LEDS, 5);
int pos = random16(NUM_LEDS);
leds[pos] =  RandColorFromPaletteNoBlack(Palette1);
FastLED.show();
}

void TimerEffect20Func() //Glitter
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }
fadeToBlackBy( leds, NUM_LEDS, 245); 
addGlitter(80, CHSV( beatsin88(128, 0, 255), 255, 255)); //+ random8(64), 255, 255));  //CRGB::White
addGlitter(80, CRGB::White);
FastLED.show();
}

void TimerEffect21Func() //Glitter with Fading Palette
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Palette1 = GetRandomPalette();
      Palette2 = GetRandomPalette(); //target palette
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }

if (Tmr_effects.GetCounter() % 40 == 0)
  {
  //check periodically, if palettes are the same - get new random
  if (ArePalettesEqual(Palette1, Palette2))
    { Palette2 = GetRandomPalette();} //change target palette
  }
nblendPaletteTowardPalette( Palette1, Palette2, 12);  //fade Palette1 to Palette2.

    
fadeToBlackBy( leds, NUM_LEDS, 245); 
addGlitter(80, RandColorFromPaletteNoBlack(Palette1), 10);
FastLED.show();
}

void TimerEffect22Func() //Sine with Hor. Wind
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      
      //gVar.U8.V1=random8(5, myConf.Speed/5);
      //gVar.U8.V2=random8(5, myConf.Speed/5);
      //gVar.U8.V3=random8(5, myConf.Speed/5);
      //gVar.U8.V4=random8(5, myConf.Speed/5);
    }
    
fadeToBlackBy( leds, NUM_LEDS, 80); 
matrix.Shift_RectLeft(CRGB::Black);

matrix.setPixelColor(beatsin8(3, 9, 15), beatsin8(13, 0, 15), CRGB::Red);
matrix.setPixelColor(beatsin8(2, 9, 15), beatsin8(15, 0, 15), CRGB::Blue);
matrix.setPixelColor(beatsin8(1, 9, 15), beatsin8(21, 0, 15), CRGB::Green);
matrix.setPixelColor(beatsin8(4, 9, 15), beatsin8(9, 0, 15), CRGB::Yellow);

FastLED.show();
}

void TimerEffect23Func() //Sine with Vert.Wind
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);

      //gVar.U8.V1=random8(5, myConf.Speed/5);
      //gVar.U8.V2=random8(5, myConf.Speed/5);
      //gVar.U8.V3=random8(5, myConf.Speed/5);
      //gVar.U8.V4=random8(5, myConf.Speed/5);
    }
    
fadeToBlackBy( leds, NUM_LEDS, 80); 
matrix.Shift_RectDown(CRGB::Black);

matrix.setPixelColor(beatsin8(31, 0, 15), beatsin8(3, 9, 15), CRGB::Red);
matrix.setPixelColor(beatsin8(27, 0, 15), beatsin8(2, 9, 15), CRGB::Blue);
matrix.setPixelColor(beatsin8(35, 0, 15), beatsin8(1, 9, 15), CRGB::Green);
matrix.setPixelColor(beatsin8(29, 0, 15), beatsin8(4, 9, 15), CRGB::Yellow);

FastLED.show();
}
void TimerEffect24Func() //Sine (Oscilloscope) Hor.
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }
    
fadeToBlackBy( leds, NUM_LEDS, 20); 

uint8_t x=beat8(60)>>4; //linear, 1Hz
//different freq.
matrix.setPixelColor(x, beatsin8(55, 0, 15), CRGB::Blue);
matrix.setPixelColor(x, beatsin8(63, 0, 15), CRGB::Red);

FastLED.show();
}

void TimerEffect25Func() //Sine (Oscilloscope) Vert.
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }
    
fadeToBlackBy( leds, NUM_LEDS, 20); 

uint8_t y=beat8(60)>>4; //linear, 1Hz
//same freq, but phases are ashifted by third circle
matrix.setPixelColor(beatsin8(55, 0, 15, 0, 0), y, CRGB::Yellow);
matrix.setPixelColor(beatsin8(55, 0, 15, 0, 85), y, CRGB::Fuchsia);

FastLED.show();
}

void TimerEffect26Func()  //Fire2018
/*
  FastLED Fire 2018 by Stefan Petrick
  The visual effect highly depends on the framerate.
  In the Youtube video it runs at arround 70 fps.
  https://www.youtube.com/watch?v=SWMu-a9pbyk
  The heatmap movement is independend from the framerate.
  The actual scaling operation is not.
  Check out the code for further comments about the interesting parts
*/
//https://gist.github.com/StefanPetrick/1ba4584e534ba99ca259c1103754e4c5#file-fire2018-ino

//bug: bottom line is not updated - fixed (S.S. 10/20/2020)

{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      Palette1 = HeatColors_p;  // the color palette. Try different?
      //clear arrays, othersise there is some artefacts when switching effects
      Fill2DWH_U8_Array(gU8Array1);
      Fill2DWH_U8_Array(gU8Array2);
    }

  uint32_t x;
  uint32_t y;
  uint32_t z;
  uint32_t scale_x;
  uint32_t scale_y;
  //static uint8_t noise[16][16];  //noise data. Used gU8Array1 instead
  //static uint8_t heat[16][16];   //heatmap data. Used gU8Array2 instead
  uint8_t CentreX = 0; // (matrix.getMatrixWidth() / 2) - 1;  seems like not needed.
  uint8_t CentreY = 0; //(matrix.getMatrixHeight() / 2) - 1;  seems like not needed.
  // get one noise value out of a moving noise space
  uint16_t ctrl1 = inoise16(11 * millis(), 0, 0);
  // get another one
  uint16_t ctrl2 = inoise16(13 * millis(), 100000, 100000);
  // average of both to get a more unpredictable curve
  uint16_t  ctrl = ((ctrl1 + ctrl2) / 2);

  // this factor defines the general speed of the heatmap movement
  // high value = high speed
  uint8_t speed = 27;

  // here we define the impact of the wind
  // high factor = a lot of movement to the sides
  x = 3 * ctrl * speed;

  // this is the speed of the upstream itself
  // high factor = fast movement
  y = 15 * millis() * speed;

  // just for ever changing patterns we move through z as well
  z = 3 * millis() * speed ;

  // ...and dynamically scale the complete heatmap for some changes in the
  // size of the heatspots.
  // The speed of change is influenced by the factors in the calculation of ctrl1 & 2 above.
  // The divisor sets the impact of the size-scaling.
  scale_x = ctrl1 / 2;
  scale_y = ctrl2 / 2;

  // Calculate the noise array based on the control parameters.
  //uint8_t layer = 0;
  for (uint8_t i = 0; i < matrix.getMatrixWidth(); i++) {
    uint32_t ioffset = scale_x * (i - CentreX);
    for (uint8_t j = 0; j < matrix.getMatrixHeight(); j++) {
      uint32_t joffset = scale_y * (j - CentreY);
      uint16_t data = ((inoise16(x + ioffset, y + joffset, z)) + 1);
      gU8Array1[i][j] = data >> 8;
    }
  }

  // Draw the first (lowest) line - seed the fire.
  // It could be random pixels or anything else as well.
  for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {
    // draw
    matrix.setPixelColor(x, matrix.getMatrixHeight()-1, ColorFromPalette( Palette1, gU8Array1[x][0]));
    // and fill the lowest line of the heatmap, too
    gU8Array2[x][matrix.getMatrixHeight()-1] = gU8Array1[x][0];
  }

  // Copy the heatmap one line up for the scrolling.
  for (uint8_t y = 0; y < matrix.getMatrixHeight() - 1; y++) {
    for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {
      gU8Array2[x][y] = gU8Array2[x][y + 1];
    }
  }

  // Scale the heatmap values down based on the independent scrolling noise array.
  for (uint8_t y = 0; y < matrix.getMatrixHeight() - 1; y++) {
    for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {

      // get data from the calculated noise field
      uint8_t dim = gU8Array1[x][y];

      // This number is critical
      // If it´s to low (like 1.1) the fire dosn´t go up far enough.
      // If it´s to high (like 3) the fire goes up too high.
      // It depends on the framerate which number is best.
      // If the number is not right you loose the uplifting fire clouds
      // which seperate themself while rising up.
      dim = dim / 1.4;

      dim = 255 - dim;

      // here happens the scaling of the heatmap
      gU8Array2[x][y] = scale8(gU8Array2[x][y] , dim);
    }
  }

  // Now just map the colors based on the heatmap.
  for (uint8_t y = 0; y < matrix.getMatrixHeight() /*- 1*/; y++) {  //bug with no updating bottom line was here!  
  for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {
    //leds[XY(x, Height-y-1)] = ColorFromPalette( Pal, heat[XY(x, y)]);  //original (upside down): leds[XY(x, y)] = ColorFromPalette( Pal, heat[XY(x, y)]);
    matrix.setPixelColor(x, matrix.getMatrixHeight()-y-1, ColorFromPalette( Palette1, gU8Array2[x][y]));
    }
  }
fadeToBlackBy(leds, 16, 240);//darken bottom line, looks beter
//fadeUsingColor (leds, 16, CRGB(200, 0, 0) );//darken bottom line, looks beter
FastLED.show();
}

void TimerEffect27Func()  //Fire2018-2
//Fire2018-2: https://gist.github.com/StefanPetrick/819e873492f344ebebac5bcd2fdd8aa8
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      //clear arrays, othersise there is some artefacts when switching effects
      Fill2DWH_U8_Array(gU8Array1);
      Fill2DWH_U8_Array(gU8Array2);
    }

uint8_t CentreX =0; //(matrix.getMatrixWidth() / 2) - 1;  seems like not needed
uint8_t CentreY =0; // (matrix.getMatrixHeight() / 2) - 1;   seems like not needed
uint32_t x;
uint32_t y;
uint32_t z;
uint32_t scale_x;
uint32_t scale_y;
//static uint8_t noise[2][16][16];  //heatmap and brightness mask. used gU8Array1, gU8Array2 instead
//static uint8_t heat[16][16];  //used gU8Array3 instead

  // some changing values
  uint16_t ctrl1 = inoise16(11 * millis(), 0, 0);
  uint16_t ctrl2 = inoise16(13 * millis(), 100000, 100000);
  uint16_t  ctrl = ((ctrl1 + ctrl2) / 2);

  // parameters for the heatmap
  uint16_t speed = 25;
  x = 3 * ctrl * speed;
  y = 20 * millis() * speed;
  z = 5 * millis() * speed ;
  scale_x = ctrl1 / 2;
  scale_y = ctrl2 / 2;

  //calculate the noise data
  for (uint8_t i = 0; i < matrix.getMatrixWidth(); i++) {
    uint32_t ioffset = scale_x * (i - CentreX);
    for (uint8_t j = 0; j < matrix.getMatrixHeight(); j++) {
      uint32_t joffset = scale_y * (j - CentreY);
      uint16_t data = ((inoise16(x + ioffset, y + joffset, z)) + 1);
      gU8Array1[i][j] = data >> 8;
    }
  }

  // parameters for the brightness mask
  speed = 25;
  x = 3 * ctrl * speed;
  y = 20 * millis() * speed;
  z = 5 * millis() * speed ;
  scale_x = ctrl1 / 2;
  scale_y = ctrl2 / 2;

  //calculate the noise data
  for (uint8_t i = 0; i < matrix.getMatrixWidth(); i++) {
    uint32_t ioffset = scale_x * (i - CentreX);
    for (uint8_t j = 0; j < matrix.getMatrixHeight(); j++) {
      uint32_t joffset = scale_y * (j - CentreY);
      uint16_t data = ((inoise16(x + ioffset, y + joffset, z)) + 1);
      gU8Array2[i][j] = data >> 8;
    }
  }

  // draw lowest line - seed the fire
  for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {
    gU8Array3[x][15] =  gU8Array1[15 - x][7];
  }


  //copy everything one line up
  for (uint8_t y = 0; y < matrix.getMatrixHeight() - 1; y++) {
    for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {
      gU8Array3[x][y] = gU8Array3[x][y + 1];
    }
  }

  //dim
  for (uint8_t y = 0; y < matrix.getMatrixHeight() - 1; y++) {
    for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {
      uint8_t dim = gU8Array1[x][y];
      // high value = high flames
      dim = dim / 1.7;
      dim = 255 - dim;
      gU8Array3[x][y] = scale8(gU8Array3[x][y] , dim);
    }
  }

  for (uint8_t y = 0; y < matrix.getMatrixHeight(); y++) {
    for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {
      // map the colors based on heatmap
      //matrix.setPixelColor(x, y, CRGB( heat[x][y], 1 , 0));  //upside down?
      matrix.setPixelColor(x, matrix.getMatrixHeight()-y-1, CRGB( gU8Array3[x][y], 1/*?*/, 0));
      
      // dim the result based on 2nd noise layer
      //leds[XY(x, y)].nscale8(noise[1][x][y]);
      matrix.setPixelColor(x, y, matrix.getPixelColor(x, y).nscale8(gU8Array2[x][y]));
    }
  }
FastLED.show();
}

void TimerEffect28Func()  //Metaballs
/*
Metaballs proof of concept by Stefan Petrick
...very rough 8bit math here...

read more about the concept of isosurfaces and metaballs:
https://www.gamedev.net/articles/programming/graphics/exploring-metaballs-and-isosurfaces-in-2d-r2556

*/
// https://gist.githubusercontent.com/StefanPetrick/170fbf141390fafb9c0c76b8a0d34e54/raw/f594df53abb5f9ee90845fb55985a87b219df5a8/Metaballs.INO

{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
    }  

////////////////////
  float speed = 1;

  // get some 2 random moving points
  uint8_t x2 = inoise8(millis() * speed, 25355, 685 ) / 16;
  uint8_t y2 = inoise8(millis() * speed, 355, 11685 ) / 16;

  uint8_t x3 = inoise8(millis() * speed, 55355, 6685 ) / 16;
  uint8_t y3 = inoise8(millis() * speed, 25355, 22685 ) / 16;

  // and one Lissajou function
  uint8_t x1 = beatsin8(23 * speed, 0, 15);
  uint8_t y1 = beatsin8(28 * speed, 0, 15);

  for (uint8_t y = 0; y < matrix.getMatrixHeight(); y++) {
    for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {

      // calculate distances of the 3 points from actual pixel
      // and add them together with weightening
      uint8_t  dx =  abs(x - x1);
      uint8_t  dy =  abs(y - y1);
      uint8_t dist = 2 * sqrt((dx * dx) + (dy * dy));

      dx =  abs(x - x2);
      dy =  abs(y - y2);
      dist += sqrt((dx * dx) + (dy * dy));

      dx =  abs(x - x3);
      dy =  abs(y - y3);
      dist += sqrt((dx * dx) + (dy * dy));

      // inverse result
      byte color = 1000 / dist;

      // map color between thresholds
      if (color > 0 and color < 60) {
        //leds[XY(x, y)] = CHSV(color * 9, 255, 255);
        matrix.setPixelColor(x, y, CHSV(color * 9, 255, 255));
      } else {
        //leds[XY(x, y)] = CHSV(0, 255, 255);
        matrix.setPixelColor(x, y, CHSV(0, 255, 255));
      }
        // show the 3 points, too
        //leds[XY(x1,y1)] = CRGB(255, 255,255);
        //leds[XY(x2,y2)] = CRGB(255, 255,255);
        //leds[XY(x3,y3)] = CRGB(255, 255,255);
        matrix.setPixelColor(x1, y1, CRGB(255, 255,255));
        matrix.setPixelColor(x2, y2, CRGB(255, 255,255));
        matrix.setPixelColor(x3, y3, CRGB(255, 255,255));
    }
  }
FastLED.show();
}

void TimerEffect29Func() //Noise_noise (fading palette)
/*
A FastLED matrix example:
A simplex noise field fully modulated and controlled by itself
written by
Stefan Petrick 2017
Do with it whatever you like and show your results to the FastLED community
https://plus.google.com/communities/109127054924227823508
*/

// https://gist.github.com/StefanPetrick/c856b6d681ec3122e5551403aabfcc68#file-noise_noise-ino

{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      Palette1 = GetRandomPalette();
      Palette2 = GetRandomPalette(); //target palette
      //clear arrays, othersise there is some artefacts when switching effects
      Fill2DWH_U8_Array(gU8Array1);
    }  

if (Tmr_effects.GetCounter() % 10 == 0)
  {
  nblendPaletteTowardPalette( Palette1, Palette2, 40);  //fade Palette1 to Palette2.
  
  //check periodically, if palettes are the same - get new random
  if (ArePalettesEqual(Palette1, Palette2))
    { Palette2 = GetRandomPalette();} //change target palette
  }


    
/////////////
uint8_t CentreX = 0; //(matrix.getMatrixWidth() / 2) - 1;
uint8_t CentreY = 0; //(matrix.getMatrixHeight() / 2) - 1;
static uint32_t x;
static uint32_t y;
static uint32_t z;
//static uint8_t  noise[16][16];  //used gU8Array1 instead
uint32_t scale_x;
uint32_t scale_y;
///////////////////

 

  //modulate the position so that it increases/decreases x
  //(here based on the top left pixel - it could be any position else)
  //the factor "2" defines the max speed of the x movement
  //the "-255" defines the median moving direction
  x = x + (2 * gU8Array1[0][0]) - 255;
  //modulate the position so that it increases/decreases y
  //(here based on the top right pixel - it could be any position else)
  y = y + (2 * gU8Array1[matrix.getMatrixWidth()-1][0]) - 255;
  //z just in one direction but with the additional "1" to make sure to never get stuck
  //in case the movement is stopped by a crazy parameter (noise data) combination
  //(here based on the down left pixel - it could be any position else)
  z += 1 + ((gU8Array1[0][matrix.getMatrixHeight()-1]) / 4);
  //set the scaling based on left and right pixel of the middle line
  //here you can set the range of the zoom in both dimensions
  scale_x = 8000 + (gU8Array1[0][CentreY] * 16);
  scale_y = 8000 + (gU8Array1[matrix.getMatrixWidth()-1][CentreY] * 16);

  //calculate the noise data
  //uint8_t layer = 0;
  for (uint8_t i = 0; i < matrix.getMatrixWidth(); i++) {
    uint32_t ioffset = scale_x * (i - CentreX);
    for (uint8_t j = 0; j < matrix.getMatrixHeight(); j++) {
      uint32_t joffset = scale_y * (j - CentreY);
      uint16_t data = inoise16(x + ioffset, y + joffset, z);
      // limit the 16 bit results to the interesting range
      if (data < 11000) data = 11000;
      if (data > 51000) data = 51000;
      // normalize
      data = data - 11000;
      // scale down that the result fits into a byte
      data = data / 161;
      // store the result in the array
      gU8Array1[i][j] = data;
    }
  }

  //map the colors
  for (uint8_t y = 0; y < matrix.getMatrixHeight(); y++) {
    for (uint8_t x = 0; x < matrix.getMatrixWidth(); x++) {
      //I will add this overlay CRGB later for more colors
      //it´s basically a rainbow mapping with an inverted brightness mask
      CRGB overlay = CHSV(gU8Array1[y][x], 255, gU8Array1[x][y]);
      //here the actual colormapping happens - note the additional colorshift caused by the down right pixel noise[0][15][15]
      //leds[XY(x, y)] = ColorFromPalette( Pal, noise[Width-1][Height-1] + noise[x][y]) + overlay;
      matrix.setPixelColor(x, y, ColorFromPalette( Palette1, gU8Array1[matrix.getMatrixWidth()-1][matrix.getMatrixHeight()-1] + gU8Array1[x][y]) + overlay);
    }
  }

  //make it looking nice
  adjust_gamma();

  //and show it!
  FastLED.show();
}

void TimerEffect30Func() //1D inoise fade to random palette
/* Title: inoise8_pal_demo.ino
 * By: Andrew Tuline
 * Date: August, 2016
 * This short sketch demonstrates some of the functions of FastLED, including:
 * Perlin noise
 * Palettes
 * Palette blending
 * Alternatives to blocking delays
 * Beats (and not the Dr. Dre kind, but rather the sinewave kind)
 * Refer to the FastLED noise.h and lib8tion.h routines for more information on these functions.
 * Recommendations for high performance routines:
 *  Don't use blocking delays, especially if you plan to use buttons for input.
 *  Keep loops to a minimum, and don't use nested loops.
 *  Don't use floating point math. It's slow. Use 8 bit where possible.
 *  Let high school and not elementary school math do the work for you, i.e. don't just count pixels; use sine waves or other math functions instead.
 *  FastLED math functions are faster than built in math functions.
*/
{
if (EffectInitReq) 
  {
  //effect initialization is here
  EffectInitReq=false;
  Serial.printf(effect_init_str, myConf.EffectNum+1);
  Tmr_effects.SetInterval(20);
  Palette1 = GetRandomPalette(); //change target palette
  Palette2 = GetRandomPalette(); //change target palette
  gVar.U16.V1 = random16(12345); // A semi-random number for our noise generator
  gVar.U8.V3 = myConf.Style/2+4; //scale 30;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
  }  

nblendPaletteTowardPalette( Palette1, Palette2, 8);  //Blend Palette1 towards the Palette2.

if (Tmr_effects.GetCounter() % 10 == 0)
  {
  //check periodically, if palettes are the same - get new random
  if (ArePalettesEqual(Palette1, Palette2))
    { 
    Palette2 = GetRandomPalette(); //change target palette
    }
  }

for(int i = 0; i < NUM_LEDS; i++) 
  {
  uint8_t index = inoise8(gVar.U16.V1 + i * gVar.U8.V3); // % 255; ??           // Get a value from the 1D noise function. I'm using both x and y axis.
  leds[i] = ColorFromPalette(Palette1, index, 255, LINEARBLEND);   
  }
gVar.U16.V1++;  // constantly moving. (+beatsin8(10,1, 4))to vary it a bit with a sine wave.
FastLED.show();
}

void TimerEffect31Func() //2D inoise fade to random palette
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      Palette1 = GetRandomPalette(); //change target palette
      Palette2 = GetRandomPalette(); //change target palette
      gVar.U16.V1 = random16(12345); // X position
    }  

nblendPaletteTowardPalette( Palette1, Palette2, 8);  //Blend Palette1 towards the Palette2.

if (Tmr_effects.GetCounter() % 10 == 0)
  {
  //check periodically, if palettes are the same - get new random
  if (ArePalettesEqual(Palette1, Palette2))
    { 
    Palette2 = GetRandomPalette(); //change target palette
    }
  }

  
//  All that 2d loop has been replaced with   matrix.draw_2DPlasmaPal(...) call
//for (int x=0; x<matrix.getMatrixWidth(); x++)
//for (int y=0; y<matrix.getMatrixHeight(); y++)
//  {
//    uint8_t index = inoise8(gVar.U16.V1 + x * gVar.U8.V3, y * gVar.U8.V3);// % 255; ??
//    matrix.setPixelColor(x, y, ColorFromPalette(Palette1, index, 255, LINEARBLEND)); 
//  }
matrix.draw_2DPlasmaPal(Palette1, gVar.U16.V1, 0, 0, myConf.Style/2+4, true, true, LINEARBLEND);

  
gVar.U16.V1++; // constantly moving left. (dist=dist+beatsin8(10,1, 4))to vary it a bit with a sine wave.FastLED.show();
FastLED.show();
}

void TimerEffect32Func() //2D inoise Zoom (fade to random palette)
{
 if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf(effect_init_str, myConf.EffectNum+1);
      Tmr_effects.SetInterval(20);
      Palette1 = GetRandomPalette(); //change target palette
      Palette2 = GetRandomPalette(); //change target palette
      gVar.U16.V1 = random16(12345); //Z position
    }  

uint8_t zoom = beatsin8(5, 5, 80); // zoom (scale) oscilates

if (Tmr_effects.GetCounter() % 10 == 0)
  {
  nblendPaletteTowardPalette( Palette1, Palette2, 24);  //Blend Palette1 towards the Palette2.
  //check periodically, if palettes are the same - get new random
  if (ArePalettesEqual(Palette1, Palette2))
    { 
    Palette2 = GetRandomPalette(); //change target palette
    }
  }

//  All that 2d loop has been replaced with   matrix.draw_2DPlasmaPal(...) call
//for (int x=0; x<matrix.getMatrixWidth(); x++)
//for (int y=0; y<matrix.getMatrixHeight(); y++)
//  {
//    uint8_t index = inoise8(gVar.U16.V1 + x * gVar.U8.V3, y * gVar.U8.V3);// % 255; ??
//    matrix.setPixelColor(x, y, ColorFromPalette(Palette1, index, 255, LINEARBLEND)); 
//  }
matrix.draw_2DPlasmaPal(Palette1, 0-(zoom*8), 0-(zoom*8), gVar.U16.V1, zoom, true, true, LINEARBLEND); // (0-zoom*8) - to keep in center, [8,8] - matrix center

FastLED.show();
}

// cheap correction with gamma 2.0
void adjust_gamma()
{
  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    leds[i].r = dim8_video(leds[i].r);  //?
    leds[i].g = dim8_video(leds[i].g);  //?
    leds[i].b = dim8_video(leds[i].b);  //?
  }
}


void addGlitter( fract8 chanceOfGlitter, CRGB c, uint16_t timesAtOnce) 
{
for (uint16_t i=0; i<=timesAtOnce; i++)
  {
  if( random8() < chanceOfGlitter) 
    {leds[ random16(NUM_LEDS) ] += c;}
  }
}

CRGB RandColorFromPaletteNoBlack(CRGBPalette16 palette)
{
  const uint8_t MAX_ITTERATIONS=15; //to avoid endless do{}while()
  CRGB c;
  uint8_t cnt=0;
  do
    {
    c=ColorFromPalette(palette, random8(255));
    cnt++;
    if (cnt>=MAX_ITTERATIONS) 
      {
      //takes the first color (even if it is black!) and brakes loop
      c=palette[0];
      break;
      }
    }
  while (!(c)); //color comparion is not working
  //if (cnt>5) Serial.println(cnt);  
  return c;
}

void CombineHTMLOptionList()
{
HTMLOptionList = ""; //global
String S;
for (uint8_t i=0; i<TotalEffects; i++) 
  {
  S = HTMLOptionTpl;
  S.replace("%Effect#%", String(i+1));
  S.replace("%EffName%", Effects[i].EffectName);
  S.replace("%EffSel%", (i==myConf.EffectNum)?"selected":"");
  HTMLOptionList = HTMLOptionList + S;
  }
}

void NextEffect()
{
switch (myConf.EffChangeType)
  {
    case ECT_RANDOM: 
      myConf.EffectNum=random8(TotalEffects);
      break;
    
    case ECT_SEQUENTIAL: 
    case ECT_NO_CHANGE: //if no auto-change - change to next manually still alowed
      myConf.EffectNum++; 
      if (myConf.EffectNum >= TotalEffects) myConf.EffectNum=0; 
      break;  
   
    default: 
      myConf.EffectNum=random8(TotalEffects); //default random
      break;
  }
Tmr_effects.SetOnTimer(Effects[myConf.EffectNum].EffectFunc);
EffectInitReq=true;
//Combine HTMLOptionList to use in HTTP resp, only when effect changed for optimization
CombineHTMLOptionList();
Tmr_effectChange.SetEnabled(false); Tmr_effectChange.SetEnabled(true); //to restart timer
Serial.printf("Change effect to #%d, ", myConf.EffectNum+1);
Serial.print("Name: "); Serial.println(Effects[myConf.EffectNum].EffectName);
//SaveConfReqTS=millis(); //request saving config, moved to processHTTPReq()
}

void Tmr_EffectChangeCB()
{
if (!IsON) return; //not active
if (myConf.EffChangeType == ECT_NO_CHANGE) return; //no auto-change
NextEffect();
}

boolean IsAllLedsOFF()
{
//check if all leds are black
//color comparison is not working
boolean res=true;
for (uint16_t i=0; i<NUM_LEDS; i++) 
  {
    if (leds[i]) 
      {
      res=false; 
      break;
      }
  }
return res;
}

void Tmr_OnBrdLED()
{
  static uint32_t phaseshift; //to start always from zero brightness
  if ( IsAllLedsOFF() )
    {
    //update NTP client. Moved it here from loop() such as it an take long time, to not affect effects dynamic.
    // if (WiFi.getMode() == WIFI_STA) timeClient.update(); //do not update if AP mode
    //
    if (Tmr_effects.GetEnabled()) 
      {//first time when started
      phaseshift=millis()-500; //quoter cycle shift. If beat-rate is 30 (2 Hz) - full cycle is 2000, quoter is 500
      Tmr_effects.SetEnabled(false);
      }
    accum88 beatrate;
    if (WiFi.getMode() == WIFI_STA) {beatrate=30;} else {beatrate=120;}
    analogWrite(onboard_led, beatsin16(beatrate/*beat-rate*/, 0, 1023, phaseshift ) ); //sin, conflicting with FastLED.show()!!! 
    }
}


void webSrvHandleNotFound()
{
  String addy = webserver.client().remoteIP().toString();
  Serial.println(addy);
  
  Serial.print("handleNotFound(): "); Serial.println( webserver.uri() );
  webserver.send(404, "text/plain", "404: Not found");
}

void webSrvHandleRoot()
{
  String addy = webserver.client().remoteIP().toString();
  Serial.println(addy);

  Serial.println("handleRoot()");
  
  processHTTPReq();  //process request parameters
  //webserver.send(200, "text/html", "OK" );
  if (LittleFS.exists("/index.html")) 
    {// If the file exists
      File file = LittleFS.open("/index.html", "r");   
      //size_t sent = webserver.streamFile(file, "text/html");

      String Str1 = file.readString();
      //  replacements
      Str1.replace("%bONen%",  IsON ? "disabled" : "");
      Str1.replace("%bOFFen%", IsON ? "" : "disabled");
      Str1.replace("%OptionList%", HTMLOptionList);

      Str1.replace("%EffChRand%", (myConf.EffChangeType==ECT_RANDOM)     ? "checked" : "" );
      Str1.replace("%EffChSeq%",  (myConf.EffChangeType==ECT_SEQUENTIAL) ? "checked" : "" );
      Str1.replace("%EffChNone%", (myConf.EffChangeType==ECT_NO_CHANGE)  ? "checked" : "" );
      //
      Str1.replace("%ChngeInterv%", String(myConf.EffChangeInterval));
      Str1.replace("%BriVal%", String(myConf.Brightness)); 
      Str1.replace("%SpeedVal%", String(myConf.Speed)); 
      Str1.replace("%StyleVal%", String(myConf.Style)); 
      //
      Str1.replace("$NTPDate$", SSVNTPCore.getFormattedDateTimeString(NTPDateFmt) );  //replace
      Str1.replace("$NTPTime$", SSVNTPCore.getFormattedDateTimeString(NTPTimeFmt) );  //replace
      //
      webserver.send(200, "text/html", Str1 ); 
      file.close();
    }
     else 
       {
        Serial.print( webserver.uri() );
        webserver.send(404, "text/plain", "404: File NOT FOUND!");
       }
}

String getWiFiModeString(WiFiMode_t wifimode)  //WIFI_OFF=0, WIFI_STA=1, WIFI_AP=2, WIFI_AP_STA=3
{
  String wifimodestr;
  switch (wifimode)  
    {
      case WIFI_OFF:
        wifimodestr = String("WIFI_OFF");
        break;
      case WIFI_STA:
        wifimodestr = String("WIFI_STA");
        break;
      case WIFI_AP:
        wifimodestr = String("WIFI_AP");
        break;
      case WIFI_AP_STA:
        wifimodestr = String("WIFI_AP_STA");
        break;
      default:
        wifimodestr = String("Unknown??");
    }
  return wifimodestr;  
}

String GetNTPServersList()
{
  String s="";
  if (SSVNTPCore.getServerName(0) == NULL) s=s+String("1: N/A <br>"); else s=s+String("1: ")+SSVNTPCore.getServerName(0)+String("; <br>");
  if (SSVNTPCore.getServerName(1) == NULL) s=s+String("2: N/A <br>"); else s=s+String("2: ")+SSVNTPCore.getServerName(1)+String("; <br>");
  if (SSVNTPCore.getServerName(2) == NULL) s=s+String("3: N/A <br>"); else s=s+String("3: ")+SSVNTPCore.getServerName(2);
  return s;
}

String PalToString(CRGBPalette16 aPal)
{
  /*  sample:
  <b style="color:#007F00; background-color:#0000FF;">0</b> 
  <b style="color:#00007F; background-color:#00FF00;">1</b> 
  <b style="color:#7F0000; background-color:#FF0000;">2</b> 
  */
  String s = ""; //accumulator
  uint8_t buff_size = 80;
  char buff[buff_size];  //buffer for one color string represenation
  
  for (uint8_t i=0; i<16; i++)
    {
    CRGB c = aPal[i]; //get color
    snprintf(buff, buff_size, "<b style='color:#7f7f7f; background-color:#%02X%02X%02X;'>%X</b>", c.r,  c.g,  c.b,  i); //make one-color string
    s=s+String(buff); //accumulate strings
    }
  return s;
}

void webSrvHandleDiagInfo()
{
  String addy = webserver.client().remoteIP().toString();
  Serial.println(addy);
  
  Serial.println("handleDiagInfo()");
  if (LittleFS.exists("/diag-info.html")) 
    {// If the file exists
      File file = LittleFS.open("/diag-info.html", "r");   
      //size_t sent = server.streamFile(file, "text/html");

      String Str1;
      Str1 = file.readString();
      //  replacements

//ESP.getResetReason()  //more details: https://arduino-esp8266.readthedocs.io/en/latest/libraries.html
      
      Str1.replace("$WiFiMode$",  getWiFiModeString(WiFi.getMode()) );  //replace

      if (WiFi.getMode()==WIFI_STA) 
          Str1.replace("$SSID$",      WiFi.SSID()                  );  //replace
        else
          Str1.replace("$SSID$",      wifiManager.getConfigPortalSSID()   );  //replace
      
      Str1.replace("$RSSI$",      String(WiFi.RSSI())          );  //replace
      Str1.replace("$MAC$",       WiFi.macAddress()            );  //replace
      Str1.replace("$BSSID$",     WiFi.BSSIDstr()              );  //replace
      
      if (WiFi.getMode()==WIFI_STA) 
        Str1.replace("$IP$",        WiFi.localIP().toString()    );  //replace
      else   
        Str1.replace("$IP$",        WiFi.softAPIP().toString()    );  //replace
        
      Str1.replace("$SubMask$",   WiFi.subnetMask().toString() );  //replace
      Str1.replace("$GateWayIP$", WiFi.gatewayIP().toString()  );  //replace
      Str1.replace("$DNSIP$",     WiFi.dnsIP().toString()      );  //replace
      Str1.replace("$HostName$",  WiFi.hostname()              );  //replace
      Str1.replace("$FreeHeapSize$", String(ESP.getFreeHeap()) );  //replace
      Str1.replace("$Uptime$",   Sec_To_Age_Str(SSVLongTime.getUpTimeSec()) );  //replace
      Str1.replace("$LoadedFW$",       String(__FILE__) );  //replace
      Str1.replace("$CompiledOn$",String(__DATE__)+String(" ")+String(__TIME__) );  //replace
      Str1.replace("$CoreVersion$", String(ESP.getCoreVersion()) );  //replace
      Str1.replace("$SdkVersion$", String(ESP.getSdkVersion()) );  //replace
      Str1.replace("$ChipID$",    String(ESP.getChipId(), HEX) );  //replace
      Str1.replace("$FlashChipID$",       String(ESP.getFlashChipId(), HEX) );  //replace
      Str1.replace("$FlashChipSize$",     String(ESP.getFlashChipSize()) );  //replace
      Str1.replace("$FlashChipRealSize$", String(ESP.getFlashChipRealSize()) );  //replace
      Str1.replace("$FlashChipSpeed$",    String(ESP.getFlashChipSpeed()) );  //replace
      Str1.replace("$FlashChipMode$",     String(ESP.getFlashChipMode()) );  //replace
      Str1.replace("$NTPServer$",         GetNTPServersList() );  //replace
      Str1.replace("$NTPDate$",           SSVNTPCore.getFormattedDateTimeString(NTPDateFmt) );  //replace
      Str1.replace("$NTPTime$",           SSVNTPCore.getFormattedDateTimeString(NTPTimeFmt) );  //replace
      Str1.replace("$NTPUpdInterval$",    Sec_To_Age_Str(SSVNTPCore.getUpdateInterval()/1000) );  //replace
      
      if (SSVNTPCore.isNeverUpdated())
             Str1.replace("$NTPUpdAge$",   String("NEVER"));  //replace
        else 
          {
          String sss=Sec_To_Age_Str((millis()-SSVNTPCore.getLastUpdate())/1000);
          sss = sss + String(" (")+String(SSVNTPCore.UpdateCNT) +String(" times, ");
          sss = sss + String(SSVNTPCore.UpdFixCNT)+String(" fixes)");
          Str1.replace("$NTPUpdAge$",   sss);  //replace
          }
      //
      
      Str1.replace("$NTPTimeZoneString$", SSVNTPCore.getTZString() );  //replace
      Str1.replace("$NTPTimeZone$", SSVNTPCore.getTimeZone() );  //replace
      Str1.replace("$NTPDST$",      (SSVNTPCore.isDST()) ? "YES (Now is Summer)" : "NO (Now is Winter)" );  //replace

      Str1.replace("$Palette1$", PalToString(Palette1) );  //replace
      Str1.replace("$Palette2$", PalToString(Palette2)  );  //replace
      
      webserver.send(200, "text/html", Str1 ); 
      file.close();
    }
     else 
       {
        Serial.print( webserver.uri() );
        webserver.send(404, "text/plain", "404: File NOT FOUND!");
       }
};

void SelectEffect(uint8_t Num)
{
  if (myConf.EffectNum == Num-1) return; //alreay selected
  myConf.EffectNum = Num-1;
  if (IsON)
    {
    Tmr_effects.SetOnTimer(Effects[myConf.EffectNum].EffectFunc);
    EffectInitReq=true;
    }
  //Combine HTMLOptionList to use in HTTP resp, only when effect changed for optimization
  CombineHTMLOptionList();
  Tmr_effectChange.SetEnabled(false); Tmr_effectChange.SetEnabled(true); //to restart timer
  Serial.printf("Change effect to #%d, ", myConf.EffectNum+1);
  Serial.print("Name: "); Serial.println(Effects[myConf.EffectNum].EffectName);
  //SaveConfReqTS=millis(); //request saving config, moved to processHTTPReq()
}

void processHTTPReq()
{
  String S;
  uint16_t N;
  //Active buttons (ON, OFF)
  S = webserver.arg("Active");
  if (S.equalsIgnoreCase("ON")) turnON_OFF(true);
    else if (S.equalsIgnoreCase("OFF")) turnON_OFF(false);
   
  //NextEffect 
  if (webserver.hasArg("NextEffect")) {NextEffect();}
    
  //Select Effect
  N = webserver.arg("Effect").toInt();
  if (N>0) {SelectEffect(N);}
  
  //Effect Change (Random, Sequential, NoChange)
  S = webserver.arg("effectchange");
  if (S.equalsIgnoreCase("RANDOM")) {ChangeEffectChangeType(ECT_RANDOM);}
   else if (S.equalsIgnoreCase("SEQUENTIAL")) {ChangeEffectChangeType(ECT_SEQUENTIAL);}
    else if (S.equalsIgnoreCase("NONE")) {ChangeEffectChangeType(ECT_NO_CHANGE);}

  //Effect Change Interval  changeinterval=61
  N = webserver.arg("changeinterval").toInt();
  if (N>0) {ChangeEffChangeInterval(N);}

  //Brightness
  N = webserver.arg("brightness").toInt();
  if ((N>0) and (N<=255)) {SetBrightness(N);}

  //Speed
  N = webserver.arg("speed").toInt();
  if ((N>0) and (N<=255)) {SetSpeed(N);}

  //Style
  N = webserver.arg("style").toInt();
  if ((N>0) and (N<=255)) {SetStyle(N);}

  SaveConfReqTS=millis(); //request saving config  
}

void SetSpeed(uint8_t newSpeed)
{
  if (myConf.Speed == newSpeed) return; //already set
  Serial.printf("Change Speed from %d to %d\r\n", myConf.Speed, newSpeed);
  myConf.Speed = newSpeed;
   EffectInitReq=true;
  //SaveConfReqTS=millis();  //request saving config, moved to processHTTPReq()
}

void SetStyle(uint8_t newStyle)
{
  if (myConf.Style == newStyle) return; //already set
  Serial.printf("Change Style from %d to %d\r\n", myConf.Style, newStyle);
  myConf.Style = newStyle;
   EffectInitReq=true;
  //SaveConfReqTS=millis();  //request saving config, moved to processHTTPReq()
}

void SetBrightness(uint8_t newBrightness)
{
  if (myConf.Brightness == newBrightness) return; //already set
  Serial.printf("Change Brightness from %d to %d\r\n", myConf.Brightness, newBrightness);
  myConf.Brightness = newBrightness;
  FastLED.setBrightness(myConf.Brightness);
  //SaveConfReqTS=millis();  //request saving config, moved to processHTTPReq()
}

void ChangeEffChangeInterval(uint16_t NewEffectChangeInterval)
{  
  if (myConf.EffChangeInterval == NewEffectChangeInterval) return; //already set
  Serial.printf("Change EffectChangeInterval from %dSec to %dSec\r\n", myConf.EffChangeInterval, NewEffectChangeInterval);
  myConf.EffChangeInterval = NewEffectChangeInterval;
  Tmr_effectChange.SetInterval(myConf.EffChangeInterval * 1000); 
}

void ChangeEffectChangeType(TEffectChangeType NewEffectChangeType)
{
  if (myConf.EffChangeType == NewEffectChangeType) return; //already set
  Serial.printf("Change ECT from %d to %d\r\n", myConf.EffChangeType, NewEffectChangeType);
  myConf.EffChangeType = NewEffectChangeType;
}

void turnON_OFF(boolean on)
{
  if (on==IsON) return; //already in this mode
  IsON=on;
  Tmr_effectChange.SetEnabled(on);
  Tmr_effects.SetInterval(20); //set to default
  EffectInitReq=true;
  if (on)
    {
    Tmr_OnBoardLED.SetEnabled(false);  
    digitalWrite(onboard_led, HIGH); //off
    Tmr_effects.SetOnTimer(Effects[myConf.EffectNum].EffectFunc);
    Tmr_effects.SetEnabled(true);
    Serial.printf("Turn ON\r\n");
    }
    else 
    {
    Tmr_OnBoardLED.SetEnabled(true);
    Tmr_effects.SetOnTimer(TimerOFFFunc1);
    Serial.printf("Turn OFF\r\n");
    }
}

void OTA_onStart()
{
  String type;
  switch (ArduinoOTA.getCommand())
    {
    case U_FLASH: type = "SKETCH"; break;
    case U_FS:    type = "FILESYSTEM"; break; // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    default:      type = "UNKNOWN";
    } 
  Serial.println("Start updating " + type);
  digitalWrite(onboard_led, HIGH); //off 
  FastLED.clear();
  FastLED.show();     
};

boolean g_OTAError=false;;

void OTA_onEnd()
{
  Serial.println("\r\nEnd");
  CRGB c;
  if (g_OTAError) c=CRGB::Red; else c=CRGB::Green; //Lime
  FastLED.clear();
  for (uint16_t i=0; i<NUM_LEDS; i++) 
    {
      if ( (i % 2) == 0) { leds[i]=c; } //do not change the rest, stays after onProgress
    }
  FastLED.show();
  //not needed, leds stay until restart
  //delay(250);
  //FastLED.clear();
  //FastLED.show();
};

void OTA_onProgress(unsigned int progress, unsigned int total)
{
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  uint16_t nn = (progress / (total / NUM_LEDS));
  fill_solid(leds, nn, CRGB::Blue);
  FastLED.show();
};

void OTA_onError(ota_error_t error)
{
  g_OTAError = true; //save in global
  Serial.printf("Error[%u]: ", error);
  switch (error)
    {
    case OTA_AUTH_ERROR:    Serial.println("Auth Failed");    break;
    case OTA_BEGIN_ERROR:   Serial.println("Begin Failed");   break;
    case OTA_CONNECT_ERROR: Serial.println("Connect Failed"); break;
    case OTA_RECEIVE_ERROR: Serial.println("Receive Failed"); break;
    case OTA_END_ERROR:     Serial.println("End Failed");     break;
    default:                Serial.println("UNKNOWN Failure"); 
    } 
  FastLED.clear();
  FastLED.show();                     
};

void TimerOFFFunc1()
{
  if (EffectInitReq) 
    {
      //effect initialization is here
      EffectInitReq=false;
      Serial.printf("Turning OFF tmr\r\n");
    }
  fadeToBlackBy( leds, NUM_LEDS, 32);
  FastLED.show();  
}

CRGBPalette16 GetRandomPalette()
{
uint8_t rndpal=random8(14); //MAX switch number!
CRGBPalette16 res;
switch (rndpal)
  {
  // Pre-defined palettes: CloudColors_p, LavaColors_p, OceanColors_p, ForestColors_p, RainbowColors_p, RainbowStripeColors_p, PartyColors_p, HeatColors_p
  case 0: {res = CloudColors_p; break;}
  case 1: {res = LavaColors_p; break;}
  case 2: {res = OceanColors_p; break;}
  case 3: {res = ForestColors_p; break;}
  case 4: {res = RainbowColors_p; break;}
  case 5: {res = RainbowStripeColors_p; break;}
  case 6: {res = PartyColors_p; break;}
  case 7: {res = HeatColors_p; break;}
  case 8:
    { 
    //random palette - four random colors 
    res = CRGBPalette16(CHSV(random8(), 255, 32), 
                        CHSV(random8(), 255, 255), 
                        CHSV(random8(), 128, 255), 
                        CHSV(random8(), 255, 255));  
    break; 
    }
        
  case 9: 
    {
    //Black And White Striped Palette
    fill_solid( res, 16, CRGB::Black);  // 'black out' all 16 palette entries... 
    res[0] = CRGB::White;         //and set every fourth one to white.
    res[4] = CRGB::White;
    res[8] = CRGB::White;
    res[12] = CRGB::White; 
    break;
    }
        
  case 10: 
    {
    //Yellow-Blue Palette
    CRGB c1     = CHSV(HUE_YELLOW, 255, 255);
    CRGB c2     = CHSV(HUE_BLUE, 255, 255);
    CRGB black  = CRGB::Black;
    res = CRGBPalette16(c1,  c1,  black,  black,
                        c2,  c2,  black,  black,
                        c1,  c1,  black,  black,
                        c2,  c2,  black,  black);
    break;
    }

  case 11:
    {
    //three neibor color palette
    fill_solid( res, 16, CRGB::Black);  // 'black out' all 16 palette entries... 
    uint8_t c = random8();
    res[7] = CHSV(c-16, 255, 128);
    res[8] = CHSV(c,    255, 255);
    res[9] = CHSV(c+16, 255, 128);
    break;
    }

  case 12:
    {
    res = CRGBPalette16(CHSV(random8(), 255, random8(128,255)), 
                        CHSV(random8(), 255, random8(128,255)), 
                        CHSV(random8(), 192, random8(128,255)), 
                        CHSV(random8(), 255, random8(128,255)));
    }


  case 13: 
    {
    /*
    Pallette from here:  https://plus.google.com/communities/109127054924227823508
                         https://gist.github.com/StefanPetrick/c856b6d681ec3122e5551403aabfcc68#file-noise_noise-ino
    first column - palette index
    next three columns - color at this index in palette
    DEFINE_GRADIENT_PALETTE( pit ) {
    0,        3,   3,   3,
    64,       13,   13, 255,  //blue
    128,      3,   3,   3,
    192,      255, 130,   3 ,  //orange
    255,      3,   3,   3
    };
    */
    res = CRGBPalette16(CRGB(3,3,3),     CRGB(5,5,66),    CRGB(8,8,129),  CRGB(10,10,192),
                        CRGB(13,13,255), CRGB(13,13,255), CRGB(9,9,171),  CRGB(6,6,87),
                        CRGB(3,3,3),     CRGB(3,3,3),     CRGB(87,45,3),  CRGB(171,87,3),
                        CRGB(255,129,3), CRGB(255,130,3), CRGB(129,66,3), CRGB(3,3,3) );
    break;
    }

  default: 
    {
    //one random color palette
    fill_solid( res, 16, CRGB::Black);  // 'black out' all 16 palette entries... 
    res[8] = CHSV(random8(), 255, 255);
    break;
    }     
  }//end of switch  
Serial.printf("Random Palette selector: %d\r\n", rndpal);  
return res;  
}

boolean ArePalettesEqual ( CRGBPalette16 &pal1, CRGBPalette16 &pal2)
{
if (pal1==pal2) return true; //same references
for(uint8_t i=0; i <= 15; i++)
  {
  if ( pal1[i] != pal2[i] ) {return false;}
  }
return true;
}

/////////////////////// Save/Load config /////////////////
uint8_t SaveConfigToFile()
{
  //Save myConf to configFile, return 0 if no errors
  File myFile = LittleFS.open(configFile, "w");
  unsigned int readSize = myFile.write((byte *)&myConf, sizeof(myConf));
  if (readSize != sizeof(myConf)) return 2; //cannot save all bytes
  myFile.close();  
  return 0;
}

uint8_t LoadConfigFromFile()
{
  //Load myConf from configFile, return 0 if no errors
  if (!LittleFS.exists(configFile)) return 1; //Could not find config file
  File myFile = LittleFS.open(configFile, "r");
  unsigned int readSize = myFile.read((byte *)&myConf, sizeof(myConf));
  if (readSize != sizeof(myConf)) return 2; //cannot read all bytes
  myFile.close();
  return 0;
}

///////////two peaces linear approximation
uint16_t TwoPeacesApprox(uint8_t FromX, 
                         uint8_t From1st, uint8_t From2nd, uint8_t From3rd,
                         uint16_t To1st,  uint16_t To2nd,  uint16_t To3rd )
{
  if (FromX<From2nd)
    {
    //first peace
    return map(FromX, From1st, From2nd, To1st, To2nd);
    }
    else
      {
        //second peace
        return map(FromX, From2nd, From3rd, To2nd, To3rd);
      }
}

// ----- button 1 callback functions
// This function will be called when the button1 was pressed 1 time (and no 2. button press followed).
void click1() 
{
  Serial.println("Button 1 click.");
  turnON_OFF(not IsON);
} // click1

// This function will be called when the button1 was pressed 2 times in a short timeframe.
void doubleclick1() 
{
  if (not IsON) exit; //do nothing
  Serial.println("Button 1 doubleclick.");
  NextEffect();
  SaveConfReqTS=millis(); //request saving config 
} // doubleclick1

// This function will be called once, when the button1 is pressed for a long time.
void longPressStart1() 
{
  if (not IsON) exit; //do nothing
  Tmr_BtnLongPress.SetEnabled(true);
  Serial.println("Button 1 longPress start");
} // longPressStart1

// This function will be called often, while the button1 is pressed for a long time.
void longPress1() 
{
if (not IsON) exit; //do nothing
//  Serial.println("Button 1 longPress...");
} // longPress1


boolean BriIncrease=true;  //flag for brightness change direction
void BtnLongPressCB()
{
if (not IsON) exit; //do nothing  
uint8_t NewBr=myConf.Brightness;
const uint8_t BriStep=5;
if (BriIncrease) 
  {
  if (NewBr<=(255-BriStep)) NewBr+=BriStep; else NewBr=255;
  } 
   else 
    {
    if (NewBr>=(1+BriStep)) NewBr-=BriStep; else NewBr=1;
    }
SetBrightness(NewBr);
}

// This function will be called once, when the button1 is released after beeing pressed for a long time.
void longPressStop1() 
{
  if (not IsON) exit; //do nothing
  Tmr_BtnLongPress.SetEnabled(false);
  BriIncrease=not BriIncrease; //change direction
  SaveConfReqTS=millis(); //request saving config 
  Serial.println("Button 1 longPress stop");
} // longPressStop1
