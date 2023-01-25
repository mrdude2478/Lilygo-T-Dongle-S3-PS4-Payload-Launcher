/*
original code before mod & help info
https://github.com/stooged/PS4-Server-900u/blob/main/PS4_Server_900u/PS4_Server_900u.ino
https://github.com/stooged/ESP32-Server-900u/blob/main/ESP32_Server_900u/ESP32_Server_900u.ino
https://github.com/Xinyuan-LilyGO/T-Dongle-S3
https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
https://github.com/Bodmer/TFT_eSPI
https://github.com/mathertel/OneButton/tree/master/examples/FunctionalButton
https://techtutorialsx.com/2019/02/24/esp32-arduino-formatting-the-spiffs-file-system/
https://github.com/espressif/arduino-esp32/blob/master/libraries/SD_MMC/examples/SDMMC_Test/SDMMC_Test.ino
*/
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long start2Millis;  //global for scrolling text
unsigned long start3Millis;  //global for cycling leds text
unsigned long currentMillis;
unsigned long nowMillis;
int logodelay = 10; //amount of seconds to show startup logo for.
const unsigned long scrolldelay = 25;  //the value is a number of milliseconds
int32_t tcount;
boolean colour_cycle = true; //set true so when power is applied the onboard led starts colour cycling
boolean blink_led = false; //trigger when usb is enabled
boolean scroller = false; //enable scrolling text on the tft screen
boolean runonce = true; //don't change this
boolean resetconf = false; //don't change this
String mcuType = CONFIG_IDF_TARGET;
String ip; //used for lcd screen info
String sid; //used for lcd screen info
boolean installgoldhen = false; //install default goldhen to filesys if hard reset is activated

#define usefat false //don't use yet as it breaks formatting...fix fatfs format code later.

#define TFT_W 160 //set tft screen width
#define TFT_H 80 //set tft screen height

#include <OneButton.h>
#include <FS.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "esp_task_wdt.h"
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "USB.h"
#include "USBMSC.h"
#include "exfathax.h"
#include "loader.h"
#include "jzip.h"
#include "logo.h"
#if usefat
#include "FFat.h"
#define FILESYS FFat
#else
#include "SPIFFS.h"
#define FILESYS SPIFFS
#endif
#include "goldhen_2.2.5b8_900.h"
#include "pages.h"
#include <FastLED.h> // https://github.com/FastLED/FastLED
#include"TFT_eSPI.h"
#include "pin_config.h" //include pins for Lilygo Tdongle-S3
CRGB leds;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite stext2 = TFT_eSprite(&tft); // Sprite object stext2
OneButton button(BTN_PIN, true);
uint8_t btn_press = 0;

const char compile_date[] = __DATE__ " " __TIME__;

//set a default payload for payloads.html config page
String Default_Payload = "goldhen.bin"; //don't change unless you also mod pages.h/payload code
String Payload_Name = "GoldHEN"; //don't change unless you also mod pages.h/payload code

//create access point
boolean startAP = true;
String AP_SSID = "PS4-Hack";
String AP_PASS = "";
IPAddress Server_IP(1, 2, 3, 4);
IPAddress Subnet_Mask(255, 255, 255, 0);

//connect to wifi
boolean connectWifi = false;
String WIFI_SSID = "Home_WIFI";
String WIFI_PASS = "password";
String WIFI_HOSTNAME = "PS4-Local";

//server port
int WEB_PORT = 80;

//Auto Usb Wait(milliseconds)
int USB_WAIT = 2000; //don't change unless you also mod loader.h

// Displayed firmware version
String firmwareVer = "1.00";

//ESP sleep after x minutes
boolean espSleep = true; //enable on by default
int TIME2SLEEP = 10; // minutes
//-----------------------------------------------------//

DNSServer dnsServer;
AsyncWebServer server(WEB_PORT);
boolean hasEnabled = false;
boolean isFormating = false;
long enTime = 0;
long bootTime = 0;
File upFile;
USBMSC dev;

String split(String str, String from, String to) {
  String tmpstr = str;
  tmpstr.toLowerCase();
  from.toLowerCase();
  to.toLowerCase();
  int pos1 = tmpstr.indexOf(from);
  int pos2 = tmpstr.indexOf(to, pos1 + from.length());
  String retval = str.substring(pos1 + from.length(), pos2);
  return retval;
}

bool instr(String str, String search) {
  int result = str.indexOf(search);
  if (result == -1) {
    return false;
  }
  return true;
}

String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + " KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + " MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
  }
}

//convert byte array to a hex string output
String mac2String(byte ar[]){
  String s;
  for (byte i = 0; i <= sizeof(ar)+1; ++i)
  {
    char buf[3];
    sprintf(buf, "%02X", ar[i]);
    s += buf;
    if (i <= sizeof(ar)) s += ':';
  }
  return s;
}

//get esp2 mac address from long long int and convert into a byte array
String MacAddress() {
  uint64_t mac = ESP.getEfuseMac();
  char arrayOfByte[]={};
  memcpy(arrayOfByte, &mac, 6);
  char one = arrayOfByte[0];
  char two = arrayOfByte[1];
  char three = arrayOfByte[2];
  char four = arrayOfByte[3];
  char five = arrayOfByte[4];
  char six = arrayOfByte[5];
  byte buf[] = {one, two, three, four, five, six};
  String s = mac2String(buf);
  return (s);
}

String chipID(){
  String s = String(ESP.getEfuseMac());
  return (s);
}

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield(); //https://www.arduino.cc/reference/en/libraries/scheduler/yield/
  }
  encodedString.replace("%2E", ".");
  return encodedString;
}

void sendwebmsg(AsyncWebServerRequest * request, String htmMsg) {
  String tmphtm = "<!DOCTYPE html><html><head><link rel=\"stylesheet\" href=\"style.css\"></head><center><br><br><br><br><br><br>" + htmMsg + "</center></html>";
  request -> send(200, "text/html", tmphtm);
}

void handleFwUpdate(AsyncWebServerRequest * request, String filename, size_t index, uint8_t * data, size_t len, bool final) {
  if (!index) {
    String path = request -> url();
    if (path != "/update.html") {
      request -> send(500, "text/plain", "Internal Server Error");
      return;
    }
    if (!filename.equals("fwupdate.bin")) {
      sendwebmsg(request, "Invalid update file: " + filename);
      return;
    }
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
      Update.printError(Serial);
      sendwebmsg(request, "Update Failed: " + String(Update.errorString()));
    }
  }
  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
      sendwebmsg(request, "Update Failed: " + String(Update.errorString()));
    }
  }
  if (final) {
    if (Update.end(true)) {
      String tmphtm = "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"8; url=/info.html\"><style type=\"text/css\">body {background-color: #1451AE; color: #ffffff; font-size: 20px; font-weight: bold; margin: 0 0 0 0.0; padding: 0.4em 0.4em 0.4em 0.6em;}</style></head><center><br><br><br><br><br><br>Update Success, Rebooting.</center></html>";
      request -> send(200, "text/html", tmphtm);
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
    }
  }
}

void handleDelete(AsyncWebServerRequest * request) {
  if (!request -> hasParam("file", true)) {
    request -> redirect("/fileman.html");
    return;
  }
  String path = request -> getParam("file", true) -> value();
  if (path.length() == 0) {
    request -> redirect("/fileman.html");
    return;
  }
  if (FILESYS.exists("/" + path) && path != "/" && !path.equals("config.ini")) {
    FILESYS.remove("/" + path);
  }
  request -> redirect("/fileman.html");
}

void handleFileMan(AsyncWebServerRequest * request) {
  File dir = FILESYS.open("/");
  String output = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>File Manager</title><link rel=\"stylesheet\" href=\"style.css\"><style>body{overflow-y:auto;} th{border: 1px solid #dddddd; background-color:gray;padding: 8px;}</style><script>function statusDel(fname) {var answer = confirm(\"Are you sure you want to delete \" + fname + \" ?\");if (answer) {return true;} else { return false; }} </script></head><body><br><table id=filetable></table><script>var filelist = [";
  int fileCount = 0;
  while (dir) {
    File file = dir.openNextFile();
    if (!file) {
      dir.close();
      break;
    }
    String fname = String(file.name());
    if (fname.length() > 0 && !fname.equals("config.ini") && !file.isDirectory()) {
      fileCount++;
      fname.replace("|", "%7C");
      fname.replace("\"", "%22");
      output += "\"" + fname + "|" + formatBytes(file.size()) + "\",";
    }
    file.close();
    esp_task_wdt_reset(); //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html

  }
  if (fileCount == 0) {
    output += "];</script><center>No files found<br>You can upload files using the <a href=\"/upload.html\" target=\"mframe\"><u>File Uploader</u></a> page.</center></p></body></html>";
  } else {
    output += "];var output = \"\";filelist.forEach(function(entry) {var splF = entry.split(\"|\"); output += \"<tr>\";output += \"<td><a href=\\\"\" +  splF[0] + \"\\\">\" + splF[0] + \"</a></td>\"; output += \"<td>\" + splF[1] + \"</td>\";output += \"<td><a href=\\\"/\" + splF[0] + \"\\\" download><button type=\\\"submit\\\">Download</button></a></td>\";output += \"<td><form action=\\\"/delete\\\" method=\\\"post\\\"><button type=\\\"submit\\\" name=\\\"file\\\" value=\\\"\" + splF[0] + \"\\\" onClick=\\\"return statusDel('\" + splF[0] + \"');\\\">Delete</button></form></td>\";output += \"</tr>\";}); document.getElementById(\"filetable\").innerHTML = \"<tr><th colspan='1'><center>File Name</center></th><th colspan='1'><center>File Size</center></th><th colspan='1'><center><a href='/dlall' target='mframe'><button type='submit'>Download All</button></a></center></th><th colspan='1'><center><a href='/format.html' target='mframe'><button type='submit'>Delete All</button></a></center></th></tr>\" + output;</script></body></html>";
  }
  request -> send(200, "text/html", output);
}

void handleDlFiles(AsyncWebServerRequest * request) {
  File dir = FILESYS.open("/");
  String output = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>File Downloader</title><link rel=\"stylesheet\" href=\"style.css\"><style>body{overflow-y:auto;}</style><script type=\"text/javascript\" src=\"jzip.js\"></script><script>var filelist = [";
  int fileCount = 0;
  while (dir) {
    File file = dir.openNextFile();
    if (!file) {
      dir.close();
      break;
    }
    String fname = String(file.name());
    if (fname.length() > 0 && !fname.equals("config.ini") && !file.isDirectory()) {
      fileCount++;
      fname.replace("\"", "%22");
      output += "\"" + fname + "\",";
    }
    file.close();
    esp_task_wdt_reset();
  }
  if (fileCount == 0) {
    output += "];</script></head><center>No files found to download<br>You can upload files using the <a href=\"/upload.html\" target=\"mframe\"><u>File Uploader</u></a> page.</center></p></body></html>";
  } else {
    output += "]; async function dlAll(){var zip = new JSZip();for (var i = 0; i < filelist.length; i++) {if (filelist[i] != ''){var xhr = new XMLHttpRequest();xhr.open('GET',filelist[i],false);xhr.overrideMimeType('text/plain; charset=x-user-defined'); xhr.onload = function(e) {if (this.status == 200) {zip.file(filelist[i], this.response,{binary: true});}};xhr.send();document.getElementById('fp').innerHTML = 'Adding: ' + filelist[i];await new Promise(r => setTimeout(r, 50));}}document.getElementById('gen').style.display = 'none';document.getElementById('comp').style.display = 'block';zip.generateAsync({type:'blob'}).then(function(content) {saveAs(content,'esp_files.zip');});}</script></head><body onload='setTimeout(dlAll,100);'><center><br><br><br><br><div id='gen' style='display:block;'><div id='loader'></div><br><br>Generating ZIP<br><p id='fp'></p></div><div id='comp' style='display:none;'><br><br><br><br>Complete<br><br>Downloading: esp_files.zip</div></center></body></html>";
  }
  request -> send(200, "text/html", output);
}

void handleConfig(AsyncWebServerRequest * request) {
  if (request -> hasParam("ap_ssid", true) && request -> hasParam("ap_pass", true) && request -> hasParam("web_ip", true) && request -> hasParam("web_port", true) && request -> hasParam("subnet", true) && request -> hasParam("wifi_ssid", true) && request -> hasParam("wifi_pass", true) && request -> hasParam("wifi_host", true) && request -> hasParam("usbwait", true) && request -> hasParam("payload", true)) {
    AP_SSID = request -> getParam("ap_ssid", true) -> value();
    if (!request -> getParam("ap_pass", true) -> value().equals("********")) {
      AP_PASS = request -> getParam("ap_pass", true) -> value();
    }
    WIFI_SSID = request -> getParam("wifi_ssid", true) -> value();
    if (!request -> getParam("wifi_pass", true) -> value().equals("********")) {
      WIFI_PASS = request -> getParam("wifi_pass", true) -> value();
    }
    String tmpip = request -> getParam("web_ip", true) -> value();
    String tmpwport = request -> getParam("web_port", true) -> value();
    String tmpsubn = request -> getParam("subnet", true) -> value();
    String WIFI_HOSTNAME = request -> getParam("wifi_host", true) -> value();
    String Default_Payload = request -> getParam("payload", true) -> value();
    String Payload_Name = request -> getParam("payload_name", true) -> value();
    String tmpua = "false";
    String tmpcw = "false";
    String tmpslp = "false";
    if (request -> hasParam("useap", true)) {
      tmpua = "true";
    }
    if (request -> hasParam("usewifi", true)) {
      tmpcw = "true";
    }
    if (request -> hasParam("espsleep", true)) {
      tmpslp = "true";
    }
    if (tmpua.equals("false") && tmpcw.equals("false")) {
      tmpua = "true";
    }
    int USB_WAIT = request -> getParam("usbwait", true) -> value().toInt();
    int TIME2SLEEP = request -> getParam("sleeptime", true) -> value().toInt();
    File iniFile = FILESYS.open("/config.ini", "w");
    if (iniFile) {
      iniFile.print("\r\nAP_SSID=" + AP_SSID + "\r\nAP_PASS=" + AP_PASS + "\r\nWEBSERVER_IP=" + tmpip + "\r\nWEBSERVER_PORT=" + tmpwport + "\r\nSUBNET_MASK=" + tmpsubn + "\r\nWIFI_SSID=" + WIFI_SSID + "\r\nWIFI_PASS=" + WIFI_PASS + "\r\nWIFI_HOST=" + WIFI_HOSTNAME + "\r\nUSEAP=" + tmpua + "\r\nCONWIFI=" + tmpcw + "\r\nUSBWAIT=" + USB_WAIT + "\r\nESPSLEEP=" + tmpslp + "\r\nSLEEPTIME=" + TIME2SLEEP + "\r\npayload=" + Default_Payload + "\r\npayload_name=" + Payload_Name + "\r\n");
      iniFile.close();
    }
    String htmStr = "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"8; url=/info.html\"><style type=\"text/css\">#loader {z-index: 1;width: 50px;height: 50px;margin: 0 0 0 0;border: 6px solid #f3f3f3;border-radius: 50%;border-top: 6px solid #3498db;width: 50px;height: 50px;-webkit-animation: spin 2s linear infinite;animation: spin 2s linear infinite; } @-webkit-keyframes spin {0%{-webkit-transform: rotate(0deg);}100%{-webkit-transform: rotate(360deg);}}@keyframes spin{0%{ transform: rotate(0deg);}100%{transform: rotate(360deg);}}body {background-color: #1451AE; color: #ffffff; font-size: 20px; font-weight: bold; margin: 0 0 0 0.0; padding: 0.4em 0.4em 0.4em 0.6em;} #msgfmt {font-size: 16px; font-weight: normal;}#status {font-size: 16px; font-weight: normal;}</style></head><center><br><br><br><br><br><p id=\"status\"><div id='loader'></div><br>Config saved<br>Rebooting</p></center></html>";
    request -> send(200, "text/html", htmStr);

    writepage(); //needs esp32 s2 to reboot before it takes effect!
    
    delay(1000);
    ESP.restart();
  }
}

void handleReboot(AsyncWebServerRequest * request) {
  AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/html", rebooting_gz, sizeof(rebooting_gz));
  response -> addHeader("Content-Encoding", "gzip");
  request -> send(response);
  delay(1000);
  ESP.restart();
}

void handleConfigHtml(AsyncWebServerRequest * request) {
  String tmpUa = "";
  String tmpCw = "";
  String tmpSlp = "";
  if (startAP) {
    tmpUa = "checked";
  }
  if (connectWifi) {
    tmpCw = "checked";
  }
  if (espSleep) {
    tmpSlp = "checked";
  }

  String htmStr = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>Config Editor</title><style type=\"text/css\">body {background-color: #1451AE; color: #ffffff; font-size: 14px;font-weight: bold;margin: 0 0 0 0.0;padding: 0.4em 0.4em 0.4em 0.6em;}input[type=\"submit\"]:hover {background: #ffffff;color: green;}input[type=\"submit\"]:active{outline-color: green;color: green;background: #ffffff; }table {font-family: arial, sans-serif;border-collapse: collapse;}td {border: 1px solid #dddddd;text-align: left;padding: 8px;}th {border: 1px solid #dddddd; background-color:gray;text-align: center;padding: 8px;}</style></head><body><form action=\"/config.html\" method=\"post\"><center><table><tr><th colspan=\"2\"><center>Access Point</center></th></tr><tr><td>AP SSID:</td><td><input name=\"ap_ssid\" value=\"" + AP_SSID + "\"></td></tr><tr><td>AP PASSWORD:</td><td><input name=\"ap_pass\" value=\"********\"></td></tr><tr><td>AP IP:</td><td><input name=\"web_ip\" value=\"" + Server_IP.toString() + "\"></td></tr><tr><td>SUBNET MASK:</td><td><input name=\"subnet\" value=\"" + Subnet_Mask.toString() + "\"></td></tr><tr><td>START AP:</td><td><input type=\"checkbox\" name=\"useap\" " + tmpUa + "></td></tr><tr><th colspan=\"2\"><center>Web Server</center></th></tr><tr><td>WEBSERVER PORT:</td><td><input name=\"web_port\" value=\"" + String(WEB_PORT) + "\"></td></tr><tr><th colspan=\"2\"><center>Wifi Connection</center></th></tr><tr><td>WIFI SSID:</td><td><input name=\"wifi_ssid\" value=\"" + WIFI_SSID + "\"></td></tr><tr><td>WIFI PASSWORD:</td><td><input name=\"wifi_pass\" value=\"********\"></td></tr><tr><td>WIFI HOSTNAME:</td><td><input name=\"wifi_host\" value=\"" + WIFI_HOSTNAME + "\"></td></tr><tr><td>CONNECT WIFI:</td><td><input type=\"checkbox\" name=\"usewifi\" " + tmpCw + "></td></tr><tr><th colspan=\"2\"><center>Auto USB Wait</center></th></tr><tr><td>WAIT TIME(ms):</td><td><input name=\"usbwait\" value=\"" + USB_WAIT + "\"></td></tr><tr><th colspan=\"2\"><center>ESP Sleep Mode</center></th></tr><tr><td>ENABLE SLEEP:</td><td><input type=\"checkbox\" name=\"espsleep\" " + tmpSlp + "></td></tr><tr><td>TIME TO SLEEP(minutes):</td><td><input name=\"sleeptime\" value=\"" + TIME2SLEEP + "\"></td></tr><tr><th colspan=\"2\"><center>Default Payload</center></th></tr><tr><td>PAYLOAD NAME:</td><td><input name=\"payload_name\" value=\"" + Payload_Name + "\"></td></tr><tr><td>PAYLOAD:</td><td><input name=\"payload\" value=\"" + Default_Payload + "\"></td></tr></table><br><input id=\"savecfg\" type=\"submit\" value=\"Save Config\"></center></form></body></html>";
  request -> send(200, "text/html", htmStr);
}

void handleFileUpload(AsyncWebServerRequest * request, String filename, size_t index, uint8_t * data, size_t len, bool final) {
  if (!index) {
    String path = request -> url();
    if (path != "/upload.html") {
      request -> send(500, "text/plain", "Internal Server Error");
      return;
    }
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    if (filename.equals("/config.ini")) {
      return;
    }
    upFile = FILESYS.open(filename, "w");
  }
  if (upFile) {
    upFile.write(data, len);
  }
  if (final) {
    upFile.close();
  }
}

void handleConsoleUpdate(String rgn, AsyncWebServerRequest * request) {
  String Version = "05.050.000";
  String sVersion = "05.050.000";
  String lblVersion = "5.05";
  String imgSize = "0";
  String imgPath = "";
  String xmlStr = "<?xml version=\"1.0\" ?><update_data_list><region id=\"" + rgn + "\"><force_update><system level0_system_ex_version=\"0\" level0_system_version=\"" + Version + "\" level1_system_ex_version=\"0\" level1_system_version=\"" + Version + "\"/></force_update><system_pup ex_version=\"0\" label=\"" + lblVersion + "\" sdk_version=\"" + sVersion + "\" version=\"" + Version + "\"><update_data update_type=\"full\"><image size=\"" + imgSize + "\">" + imgPath + "</image></update_data></system_pup><recovery_pup type=\"default\"><system_pup ex_version=\"0\" label=\"" + lblVersion + "\" sdk_version=\"" + sVersion + "\" version=\"" + Version + "\"/><image size=\"" + imgSize + "\">" + imgPath + "</image></recovery_pup></region></update_data_list>";
  request -> send(200, "text/xml", xmlStr);
}

void handleCacheManifest(AsyncWebServerRequest * request) {
  String output = "CACHE MANIFEST\r\n";
  File dir = FILESYS.open("/");
  while (dir) {
    File file = dir.openNextFile();
    if (!file) {
      dir.close();
      break;
    }
    String fname = String(file.name());
    if (fname.length() > 0 && !fname.equals("config.ini") && !file.isDirectory()) {
      if (fname.endsWith(".gz")) {
        fname = fname.substring(0, fname.length() - 3);
      }
      output += urlencode(fname) + "\r\n";
    }
    file.close();
  }
  if (!instr(output, "index.html\r\n")) {
    output += "index.html\r\n";
  }
  if (!instr(output, "menu.html\r\n")) {
    output += "menu.html\r\n";
  }
  if (!instr(output, "loader.html\r\n")) {
    output += "loader.html\r\n";
  }
  if (!instr(output, "payloads.html\r\n")) {
    output += "payloads.html\r\n";
  }
  if (!instr(output, "style.css\r\n")) {
    output += "style.css\r\n";
  }
  request -> send(200, "text/cache-manifest", output);
}

void handleInfo(AsyncWebServerRequest * request) {
  float flashFreq = (float) ESP.getFlashChipSpeed() / 1000.0 / 1000.0;
  FlashMode_t ideMode = ESP.getFlashChipMode();
  String output = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>System Information</title><style type=\"text/css\">body { background-color: #1451AE;color: #ffffff;font-size: 14px;font-weight: bold; margin: 0 0 0 0.0; padding: 0.4em 0.4em 0.4em 0.6em;}</style></head>";
  output += "<hr><center><p>*** Code Mod By MrDude ***</center>";
  output += "<hr>###### MCU information ######<br><br>";
  output += "MCU: " + mcuType + "<br>";
  output += "Firmware version " + firmwareVer + "<br>";
  output += "SDK version: " + String(ESP.getSdkVersion()) + "<br>";
  output += "MAC Address: " + MacAddress() + "<br>";
  output += "Cycle Count: " + String(ESP.getCycleCount()) + "<br><hr>";
  output += "###### CPU ######<br><br>";
  output += "Chip Id: " + String(ESP.getChipModel()) + "<br>";
  output += "CPU frequency: " + String(ESP.getCpuFreqMHz()) + "MHz<br>";
  output += "Chip Revision: " + String(ESP.getChipRevision()) + "<br>";
  output += "Cores: " + String(ESP.getChipCores()) + "<br><hr>";
  output += "###### Flash chip information ######<br><br>";
  output += "Flash chip Id: " + chipID() + "<br>";
  output += "Estimated Flash size: " + formatBytes(ESP.getFlashChipSize()) + "<br>";
  output += "Flash frequency: " + String(flashFreq) + " MHz<br>";
  output += "Flash write mode: " + String((ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN")) + "<br><hr>";
  
  if (usefat == true) {
    output += "###### File system (FatFs) ######<br><br>";
  }
  else {
    output += "###### File system (SPIFFS) ######<br><br>";
  }
  output += "Total Size: " + formatBytes(FILESYS.totalBytes()) + "<br>";
  output += "Used Space: " + formatBytes(FILESYS.usedBytes()) + "<br>";
  output += "Free Space: " + formatBytes(FILESYS.totalBytes() - FILESYS.usedBytes()) + "<br><hr>";
  output += "###### PSRam information ######<br><br>";
  output += "Psram Size: " + formatBytes(ESP.getPsramSize()) + "<br>";
  output += "Free psram: " + formatBytes(ESP.getFreePsram()) + "<br>";
  output += "Max alloc psram: " + formatBytes(ESP.getMaxAllocPsram()) + "<br><hr>";
  output += "###### Ram information ######<br><br>";
  output += "Ram size: " + formatBytes(ESP.getHeapSize()) + "<br>";
  output += "Free ram: " + formatBytes(ESP.getFreeHeap()) + "<br>";
  output += "Min free ram since boot: " + formatBytes(ESP.getMinFreeHeap()) + "<br>";
  output += "Max alloc ram: " + formatBytes(ESP.getMaxAllocHeap()) + "<br><hr>";
  output += "###### Sketch information ######<br><br>";
  output += "Build date: " + String(compile_date) + "<br>";
  output += "Sketch hash: " + ESP.getSketchMD5() + "<br>";
  output += "Sketch size: " + formatBytes(ESP.getSketchSize()) + "<br>";
  output += "Free space available: " + formatBytes(ESP.getFreeSketchSpace() - ESP.getSketchSize()) + "<br><hr>";
  output += "<center><p>*** Greetings to all the scene hackers out there ***</center><hr>";
  output += "</html>";
  request -> send(200, "text/html", output);
}

void writeConfig() {
  File iniFile = FILESYS.open("/config.ini", "w");
  if (iniFile) {
    String tmpua = "false";
    String tmpcw = "false";
    String tmpslp = "false";
    if (startAP) {
      tmpua = "true";
    }
    if (connectWifi) {
      tmpcw = "true";
    }
    if (espSleep) {
      tmpslp = "true";
    }
    iniFile.print("\r\nAP_SSID=" + AP_SSID + "\r\nAP_PASS=" + AP_PASS + "\r\nWEBSERVER_IP=" + Server_IP.toString() + "\r\nWEBSERVER_PORT=" + String(WEB_PORT) + "\r\nSUBNET_MASK=" + Subnet_Mask.toString() + "\r\nWIFI_SSID=" + WIFI_SSID + "\r\nWIFI_PASS=" + WIFI_PASS + "\r\nWIFI_HOST=" + WIFI_HOSTNAME + "\r\nUSEAP=" + tmpua + "\r\nCONWIFI=" + tmpcw + "\r\nUSBWAIT=" + USB_WAIT + "\r\nESPSLEEP=" + tmpslp + "\r\nSLEEPTIME=" + TIME2SLEEP + "\r\npayload=" + Default_Payload + "\r\n");
    iniFile.close();
  }
}

void handleFormat()
{
  //complete wipe, removes all cached files and settings
  if (usefat == true) {
    //FILESYS.format is not supported in Fat - so call another function here to wipe the files.
  }
  else {
    //https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
    bool formatted = FILESYS.format();
    if (formatted) {
    ESP.restart();
    }
  }
}

void line(){
  int x = 1;
  int y;
  for (int i = 1; i < TFT_W; i++)
  {
    tft.drawPixel(x, y+30, TFT_GOLD);
    tft.drawPixel(x, y+31, 0xFDA0);
    tft.drawPixel(x, TFT_H-1, TFT_GOLD);
    tft.drawPixel(x, TFT_H-2, 0xFDA0);
    x++;
  }
}

void setup() {
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,0); //1 = High, 0 = Low
  startMillis = millis();  //initial start time
  start2Millis = millis();  //initial start time
  start3Millis = millis();  //initial start time
  FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(&leds, 1);
  leds = 0x000000; //set onboard led to black (basically the same as turning it off) - in case chip has artefact colour set on boot.
  FastLED.setBrightness(255);
  FastLED.show();

  //set tft screen backlight off during boot, (we can turn it on when we want to use it).
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  tft.pushImage(0, 0,  TFT_W, TFT_H, logo); //added background image
  pinMode(TFT_LEDA_PIN, OUTPUT);
  digitalWrite(TFT_LEDA_PIN, 0); //1 for off, 0 for on

  mcuType.toUpperCase(); //string for later display use

  //short click
  button.attachClick([] {
    btn_press = 1;
  });
  
  //double click
  button.attachDoubleClick([] {
    btn_press = 2;
  });

  //long click
  button.attachLongPressStart([] {
    btn_press = 3;
  });
  
  if (FILESYS.begin(true)) {

    if (FILESYS.exists("/config.ini")) {
      File iniFile = FILESYS.open("/config.ini", "r");
      if (iniFile) {
        String iniData;
        while (iniFile.available()) {
          char chnk = iniFile.read();
          iniData += chnk;
        }
        iniFile.close();

        if (instr(iniData, "SSID=")) {
          AP_SSID = split(iniData, "SSID=", "\r\n");
          AP_SSID.trim();
        }

        if (instr(iniData, "PASSWORD=")) {
          AP_PASS = split(iniData, "PASSWORD=", "\r\n");
          AP_PASS.trim();
        }

        if (instr(iniData, "WEBSERVER_IP=")) {
          String strwIp = split(iniData, "WEBSERVER_IP=", "\r\n");
          strwIp.trim();
          Server_IP.fromString(strwIp);
        }

        if (instr(iniData, "SUBNET_MASK=")) {
          String strsIp = split(iniData, "SUBNET_MASK=", "\r\n");
          strsIp.trim();
          Subnet_Mask.fromString(strsIp);
        }

        if (instr(iniData, "WIFI_SSID=")) {
          WIFI_SSID = split(iniData, "WIFI_SSID=", "\r\n");
          WIFI_SSID.trim();
        }

        if (instr(iniData, "WIFI_PASS=")) {
          WIFI_PASS = split(iniData, "WIFI_PASS=", "\r\n");
          WIFI_PASS.trim();
        }

        if (instr(iniData, "WIFI_HOST=")) {
          WIFI_HOSTNAME = split(iniData, "WIFI_HOST=", "\r\n");
          WIFI_HOSTNAME.trim();
        }

        if (instr(iniData, "USEAP=")) {
          String strua = split(iniData, "USEAP=", "\r\n");
          strua.trim();
          if (strua.equals("true")) {
            startAP = true;
          } else {
            startAP = false;
          }
        }

        if (instr(iniData, "CONWIFI=")) {
          String strcw = split(iniData, "CONWIFI=", "\r\n");
          strcw.trim();
          if (strcw.equals("true")) {
            connectWifi = true;
          } else {
            connectWifi = false;
          }
        }

        if (instr(iniData, "USBWAIT=")) {
          String strusw = split(iniData, "USBWAIT=", "\r\n");
          strusw.trim();
          USB_WAIT = strusw.toInt();
        }

        if (instr(iniData, "payload=")) {
          Default_Payload = split(iniData, "payload=", "\r\n");
          Default_Payload.trim();
        }
        
        if (instr(iniData, "payload_name=")) {
          Payload_Name = split(iniData, "payload_name=", "\r\n");
          Payload_Name.trim();
        }

        if (instr(iniData, "ESPSLEEP=")) {
          String strsl = split(iniData, "ESPSLEEP=", "\r\n");
          strsl.trim();
          if (strsl.equals("true")) {
            espSleep = true;
          } else {
            espSleep = false;
          }
        }

        if (instr(iniData, "SLEEPTIME=")) {
          String strslt = split(iniData, "SLEEPTIME=", "\r\n");
          strslt.trim();
          TIME2SLEEP = strslt.toInt();
        }
      }
      
    } else {
      writeConfig();
    }
  }

  if (startAP) {
    WiFi.softAPConfig(Server_IP, Server_IP, Subnet_Mask);
    WiFi.softAP(AP_SSID.c_str(), AP_PASS.c_str());
    dnsServer.setTTL(30);
    dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
    dnsServer.start(53, "*", Server_IP);
    ip = Server_IP.toString();
    sid = AP_SSID;
  }

  if (connectWifi && WIFI_SSID.length() > 0 && WIFI_PASS.length() > 0) {
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.hostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {} else {
      IPAddress LAN_IP = WiFi.localIP();
      if (LAN_IP) {
        String mdnsHost = WIFI_HOSTNAME;
        mdnsHost.replace(".local", "");
        MDNS.begin(mdnsHost.c_str());
        if (!startAP) {
          dnsServer.setTTL(30);
          dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
          dnsServer.start(53, "*", LAN_IP);
        }
      }
    }
    ip = WiFi.localIP().toString();
    sid = WIFI_SSID;
  }

  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send(200, "text/plain", "Microsoft Connect Test");
  });
  
  server.on("/cache.manifest", HTTP_GET, [](AsyncWebServerRequest *request){
   handleCacheManifest(request);
  });

  server.on("/config.ini", HTTP_ANY, [](AsyncWebServerRequest * request) {
    request -> send(404);
  });

  server.on("/upload.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/html", upload_gz, sizeof(upload_gz));
    response -> addHeader("Content-Encoding", "gzip");
    request -> send(response);
  });

  server.on("/upload.html", HTTP_POST, [](AsyncWebServerRequest * request) {
    request -> redirect("/fileman.html");
  }, handleFileUpload);

  server.on("/fileman.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    handleFileMan(request);
  });

  server.on("/delete", HTTP_POST, [](AsyncWebServerRequest * request) {
    handleDelete(request);
  });

  server.on("/config.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    handleConfigHtml(request);
  });

  server.on("/config.html", HTTP_POST, [](AsyncWebServerRequest * request) {
    handleConfig(request);
  });

  server.on("/admin.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/html", admin_gz, sizeof(admin_gz));
    response -> addHeader("Content-Encoding", "gzip");
    request -> send(response);
  });

  server.on("/reboot.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/html", reboot_gz, sizeof(reboot_gz));
    response -> addHeader("Content-Encoding", "gzip");
    request -> send(response);
  });

  server.on("/rebooting.html", HTTP_POST, [](AsyncWebServerRequest * request) {
    handleReboot(request);
  });

  server.on("/update.html", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", update_gz, sizeof(update_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/update.html", HTTP_POST, [](AsyncWebServerRequest *request){
  }, handleFwUpdate);

  server.on("/info.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    handleInfo(request);
  });

  server.on("/usbon", HTTP_POST, [](AsyncWebServerRequest * request) {
    enableUSB();
    request -> send(200, "text/plain", "ok");
  });

  server.on("/usboff", HTTP_POST, [](AsyncWebServerRequest * request) {
    disableUSB();
    request -> send(200, "text/plain", "ok");
  });

  server.on("/payloadinject", HTTP_POST, [](AsyncWebServerRequest * request) {
    payloadinject();
    request -> send(200, "text/plain", "ok");
  });

  server.on("/deepsleep", HTTP_POST, [](AsyncWebServerRequest * request) {
    deepsleep();
    request -> send(200, "text/plain", "ok");
  });

  server.on("/format.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/html", format_gz, sizeof(format_gz));
    response -> addHeader("Content-Encoding", "gzip");
    request -> send(response);
  });

  server.on("/format.html", HTTP_POST, [](AsyncWebServerRequest * request) {
    isFormating = true;
    request -> send(304);
  });

  server.on("/dlall", HTTP_GET, [](AsyncWebServerRequest * request) {
    handleDlFiles(request);
  });

  server.on("/jzip.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/javascript", jzip_gz, sizeof(jzip_gz));
    response -> addHeader("Content-Encoding", "gzip");
    request -> send(response);
  });

  server.serveStatic("/", FILESYS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest * request) {
    String path = request -> url();
    if (instr(path, "/update/ps4/")) {
      String Region = split(path, "/update/ps4/list/", "/");
      handleConsoleUpdate(Region, request);
      return;
    }
    if (instr(path, "/document/") && instr(path, "/ps4/")) {
      request -> redirect("http://" + WIFI_HOSTNAME + "/index.html");
      return;
    }
    if (path.endsWith("index.html") || path.endsWith("index.htm") || path.endsWith("/")) {
      AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/html", index_gz, sizeof(index_gz));
      response -> addHeader("Content-Encoding", "gzip");
      request -> send(response);
      return;
    }
    if (path.endsWith("style.css")) {
      AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/css", style_gz, sizeof(style_gz));
      response -> addHeader("Content-Encoding", "gzip");
      request -> send(response);
      return;
    }
    if (path.endsWith("payloads.html")) {
      AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/html", payload, sizeof(payload));
      request -> send(response);
      return;
    }
    if (path.endsWith("loader.html")) {
      AsyncWebServerResponse * response = request -> beginResponse_P(200, "text/html", loader_gz, sizeof(loader_gz));
      response -> addHeader("Content-Encoding", "gzip");
      request -> send(response);
      return;
    }
    //use goldhen stored in the sketch
    if (path.endsWith("goldhen.bin"))
    {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "application/octet-stream", goldhen, sizeof(goldhen));
        request->send(response);
        return;
    }
    request -> send(404);
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
  if (TIME2SLEEP < 5) {
    TIME2SLEEP = 5;
  } //min sleep time
  bootTime = millis();
}

static int32_t onRead(uint32_t lba, uint32_t offset, void * buffer, uint32_t bufsize) {
  if (lba > 130) {
    lba = 130;
  }
  memcpy(buffer, exfathax[lba] + offset, bufsize);
  return bufsize;
}

void enableUSB() {
  colour_cycle = false;
  blink_led = true;
  dev.vendorID("PS4");
  dev.productID("ESP32 Server");
  dev.productRevision("1.0");
  dev.onRead(onRead);
  dev.mediaPresent(true);
  dev.begin(1024, 512);
  USB.begin();
  enTime = millis();
  hasEnabled = true;
}

void disableUSB() {
  ESP.restart();
}

void deepsleep() {
  pinMode(TFT_LEDA_PIN, OUTPUT);
  digitalWrite(TFT_LEDA_PIN, 1); //turn off lcd screen
  leds = 0x000000; //set onboard led to black - turns them off
  FastLED.show();
  FastLED.setBrightness(0);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_light_sleep_start();
  //esp_deep_sleep_start(); //don't use deep sleep or the lcd backlight will stay on
  return;
}

void writepage(){
  FILESYS.end();
  FILESYS.begin();
  String strusw;
  //read the config again because it's been updated....
  if (FILESYS.exists("/config.ini")) {
      File iniFile = FILESYS.open("/config.ini", "r");
      if (iniFile) {
        String iniData;
        while (iniFile.available()) {
          char chnk = iniFile.read();
          iniData += chnk;
        }
        iniFile.close();

        if (instr(iniData, "payload=")) {
          Default_Payload = split(iniData, "payload=", "\r\n");
          Default_Payload.trim();
        }
        
        if (instr(iniData, "payload_name=")) {
          Payload_Name = split(iniData, "payload_name=", "\r\n");
          Payload_Name.trim();
        }

        if (instr(iniData, "USBWAIT=")) {
          strusw = split(iniData, "USBWAIT=", "\r\n");
          strusw.trim();
        }
      }
   }
  //end of reading the bit of the config we care about.
  
  String filename = "/payloads.html";
  bool file_exists = FILESYS.exists(filename);
  if (file_exists) {
    FILESYS.remove(filename);
  }

  //write payload html page to buffer.
  char buffer[sizeof(payload)];
  memcpy(buffer, payload, sizeof(payload));
  
  //convert the above array to a string
  unsigned int a_size = sizeof(buffer) / sizeof(char);
  String s_a = convertToString(buffer, a_size);

  //replace part of string
  s_a.replace("goldhen.bin", Default_Payload); //don't change unless you also mod loader.h
  s_a.replace("GoldHEN", Payload_Name); //don't change unless you also mod loader.h
  s_a.replace("2000", strusw); //don't change unless you also mod loader.h

  //write modded string to a file
  File newfile = FILESYS.open(filename, "w");
  newfile.print(s_a);
  newfile.close();
}

String convertToString(char* a, unsigned int size)
{
    unsigned int i;
    String s = "";
    for (i = 0; i < size; i++) {
        s = s + a[i];
    }
    return s;
}

void colourcycle(){
  int period = 60;
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - start3Millis >= period)  //test whether the period has elapsed
  {
    static uint8_t hue = 0;
    leds = CHSV(hue++, 0XFF, 100);
    FastLED.show();
    start3Millis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
}

void blinkled(){
  currentMillis = millis();
  int delaytime = (currentMillis/350);
  if(delaytime % 2 == 0)leds = 0xff0000; //red
  if(delaytime % 2 > 0)leds = 0x0000ff; //blue
  FastLED.show();
}

void reinstall() {
  File file = FILESYS.open("/Reset-triggered.txt", "w");
  file.print("***Coded by MrDude***\n\nYou triggered an emergency reset, goldhen has been installed and all the default settings have been restored.");
  file.close();
  reinstall_goldhen();
}

void reinstall_goldhen(){

  File file = FILESYS.open("/goldhen.bin", "ab");  //ab for append binary
  unsigned char buffer[1];
  unsigned int goldsize = sizeof(goldhen);

  for (unsigned int i = 0; i < goldsize; i++) {
    memcpy(buffer, goldhen + i, 1);
    file.write(buffer, 1);
  }
  file.close();
}

void hardreset(){
  //display red led and set led screen red
  tft.fillScreen(TFT_RED);
  tft.setTextColor(0xFFFF, TFT_RED); //RGB foreground, background
  tft.setTextSize(2);
  tft.setCursor(12, 15); //x,y
  tft.println("Please Wait");
  tft.setCursor(18, 33); //x,y
  tft.println("Hard Reset");
  tft.setCursor(22, 51); //x,y
  tft.println("Triggered");
  FastLED.setBrightness(255);
  leds = 0xff0000; //red
  FastLED.show();
}

void scrolltext(){
  nowMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  stext2.setTextWrap(false);  // Don't wrap text to next line
  stext2.setTextSize(2);  // larger letters
  stext2.setTextColor(TFT_GOLD, 0x0000); //RGB foreground, background
  
  //***************************************limit the text length of the sprite drawn or we will run out of memory and break the code....
  String text = "SSID: " + sid + " - ";
  text += "IP: " + ip + " ";
  //text += mcuType + " ";
  int32_t scrollsize = (text.length()*14); //mod this if txt doesn't fit on the screen properly.
  stext2.createSprite(scrollsize+TFT_W, 26); // Sprite wider than the display plus the text to allow text to scroll from the right.
  //***************************************limit the text length of the sprite drawn or we will run out of memory and break the code....
      
  if (nowMillis - start2Millis >= scrolldelay)
  {
    stext2.pushSprite(0, 38); //location to put the scrolling text
    stext2.scroll(-1); // scroll stext 1 pixel left, up/down default is 0
  
    tcount--;
    if (tcount <=0)
    {
      tcount = scrollsize; //once this pixel count is reached redraw the text
      stext2.drawString(text, TFT_W, 0, 2); // draw at 160,0 in sprite, font 2
    }
    start2Millis = nowMillis;
  }
}

void payloadinject(){
  //show when the payload is being loaded
  colour_cycle = false;
  blink_led = false;
  leds = CRGB::Green;
  FastLED.show();
}

void resetconfig() {
  FILESYS.remove("/config.ini");
  FILESYS.remove("/payloads.html");
  resetconf = true;
}

void testcode(){
  btn_press = 0; //reset button to original state
}

void loop() {
  button.tick();

  //short button click
  if (btn_press == 1) {
    testcode();
  }

  //double button click - trigger hard reset.
  if (btn_press == 2) {
    hardreset(); //show visual warning
    resetconfig();
    if (resetconf == true) {
      if (installgoldhen == true) {
        reinstall(); //reinstall goldhen before rebooting
      }
    }
    ESP.restart();
  }

  //long button click
  if (btn_press == 3) {
    ESP.restart();
  }
  
  if (espSleep) {
    if (millis() >= (bootTime + (TIME2SLEEP * 60000))) {
      deepsleep();
    }
  }
  if (hasEnabled && millis() >= (enTime + 15000)) {
    disableUSB();
  }
  if (isFormating == true) {
    handleFormat();
  }
  dnsServer.processNextRequest();

  if (colour_cycle == true){
    FastLED.setBrightness(250);
    colourcycle(); //start onboard led colour cycle
  }

  if (blink_led == true){
    FastLED.setBrightness(255);
    blinkled(); //start blinking onboard led
  }

  if (scroller == true){
    scrolltext();
  }

  nowMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)

  if (runonce == true){
    if (nowMillis > (logodelay * 1000)) {
      tft.fillScreen(TFT_BLACK);
      tft.setSwapBytes(false);
      tft.setTextSize(1);
      tft.fillRoundRect(0, 2, TFT_W, 22, 4, TFT_RED);
      tft.setTextColor(0xFFFF, TFT_RED); //RGB foreground, background
      tft.setCursor(13, 10); //x,y
      tft.println("MAC: " + MacAddress());
      line(); //draw a line
      scroller = true; //enable scrolling text on the tft screen
      runonce=false; //don't change - we only need this code to run once...
    }
  }
}
