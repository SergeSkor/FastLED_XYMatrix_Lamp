# FastLED_XYMatrix_Lamp

Lamp Based on ESP8266 module and WS8212b 16x16 Full Color LED Matrix.
Features:
1) Multiple effects based on FastLED library;
2) Ability to select WiFi to connect to (thanks to WiFiManager library);
3) Ability to work with no connection to any SSID (creates own AP, no NTP support in this case);
4) Templates-based Web-interface to control;
5) LittleFS filesystem;
6) OTA support;
7) One touch-sensitive button control (short touch: ON/OFF, double touch: next effect, long hold: brightness control);
8) Effect changes: none, random, sequential;
9) Speed change (not for all effects done yet, control from web-page only);
10) Style change (not for all effects done yet, control from web-page only);
11) NTP support, can show time/date;
12) NTP supports auto-DST changes;
13) Save all settings in flash when turned off, restore it on power up and turn on.

See video here: https://www.youtube.com/watch?v=zFcnNPhO3xU

List of used libraries:
  1) FastLED (https://github.com/FastLED/FastLED)
  2) WiFiManager (https://github.com/tzapu/WiFiManager)
  3) OneButton (https://github.com/mathertel/OneButton)
  4) SSVXYMatrix (my library, https://github.com/SergeSkor/SSVXYMatrix)
  5) SSVRGBGradientCalc (my library, https://github.com/SergeSkor/SSVRGBGradientCalc
  6) SSVXYMatrixText (my library, https://github.com/SergeSkor/SSVXYMatrixText)
  7) SSVTimer (my library, https://github.com/SergeSkor/SSVTimer)
  8) SSVNTPCore (my library, https://github.com/SergeSkor/SSVNTPCoreClass)
  9) SSVLongTime (my library, https://github.com/SergeSkor/SSVLongTime)
