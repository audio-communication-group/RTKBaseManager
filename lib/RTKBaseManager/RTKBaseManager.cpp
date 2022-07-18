#include <RTKBaseManager.h>

/********************************************************************************
*                             WiFi
* ******************************************************************************/

void RTKBaseManager::setupStationMode(const char* ssid, const char* password, const char* deviceName) {
  WiFi.mode(WIFI_STA);
  WiFi.begin( ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    // TODO:  - count reboots and stop after 3 times (save in SPIFFS)
    //        - display state
    DEBUG_SERIAL.println("WiFi Failed! Reboot in 10 s as AP!");
    delay(10000);
    ESP.restart();
  }
  DEBUG_SERIAL.println();

  if (!MDNS.begin(deviceName)) {
      DEBUG_SERIAL.println("Error starting mDNS, use local IP instead!");
  } else {
    DEBUG_SERIAL.print(F("Starting mDNS, find me under <http://www."));
    DEBUG_SERIAL.print("DEVICE_NAME");
    DEBUG_SERIAL.println(F(".local>"));
  }

  DEBUG_SERIAL.print(F("Wifi client started: "));
  DEBUG_SERIAL.println(WiFi.getHostname());
  DEBUG_SERIAL.print(F("IP Address: "));
  DEBUG_SERIAL.println(WiFi.localIP());
}

void RTKBaseManager::setupAPMode(const char* apSsid, const char* apPassword) {
    DEBUG_SERIAL.print("Setting soft-AP ... ");
    WiFi.mode(WIFI_AP);
    DEBUG_SERIAL.println(WiFi.softAP(apSsid, apPassword) ? "Ready" : "Failed!");
    DEBUG_SERIAL.print("Access point started: ");
    DEBUG_SERIAL.println(AP_SSID);
    DEBUG_SERIAL.print("IP address: ");
    DEBUG_SERIAL.println(WiFi.softAPIP());
}

bool RTKBaseManager::savedNetworkAvailable(const String& ssid) {
  if (ssid.isEmpty()) return false;

  uint8_t nNetworks = (uint8_t) WiFi.scanNetworks();
  DEBUG_SERIAL.print(nNetworks);  DEBUG_SERIAL.println(F(" networks found."));
    for (uint8_t i=0; i<nNetworks; i++) {
    if (ssid.equals(String(WiFi.SSID(i)))) {
      DEBUG_SERIAL.print(F("A known network with SSID found: ")); 
      DEBUG_SERIAL.print(WiFi.SSID(i));
      DEBUG_SERIAL.print(F(" (")); 
      DEBUG_SERIAL.print(WiFi.RSSI(i)); 
      DEBUG_SERIAL.println(F(" dB), connecting..."));
      return true;
    }
  }
  return false;
}

/********************************************************************************
*                             Web server
* ******************************************************************************/

void RTKBaseManager::startServer(AsyncWebServer *server) {
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML, processor);
  });

  server->on("/actionUpdateData", HTTP_POST, actionUpdateData);
  server->on("/actionWipeData", HTTP_POST, actionWipeData);
  server->on("/actionRebootESP32", HTTP_POST, actionRebootESP32);

  server->onNotFound(notFound);
  server->begin();
}
  
void RTKBaseManager::notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void RTKBaseManager::actionRebootESP32(AsyncWebServerRequest *request) {
  DEBUG_SERIAL.println("ACTION actionRebootESP32!");
  request->send_P(200, "text/html", REBOOT_HTML, RTKBaseManager::processor);
  delay(3000);
  ESP.restart();
}

void RTKBaseManager::actionWipeData(AsyncWebServerRequest *request) {
  DEBUG_SERIAL.println(F("ACTION actionWipeData!"));

  int params = request->params();
  DEBUG_SERIAL.print(F("params: "));
  DEBUG_SERIAL.println(params);

  for (int i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    DEBUG_SERIAL.printf("%d. POST[%s]: %s\n", i+1, p->name().c_str(), p->value().c_str());
    if (strcmp(p->name().c_str(), "wipe_button") == 0) {
      if (p->value().length() > 0) {
        DEBUG_SERIAL.printf("wipe command received: %s",p->value().c_str());
        wipeSpiffsFiles();
      } 
     }
    } 

  DEBUG_SERIAL.print(F("Data in SPIFFS was wiped out!"));
  request->send_P(200, "text/html", INDEX_HTML, processor);
}

void RTKBaseManager::actionUpdateData(AsyncWebServerRequest *request) {
  DEBUG_SERIAL.println("ACTION: actionUpdateData!");

  int params = request->params();
  for (int i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    DEBUG_SERIAL.printf("%d. POST[%s]: %s\n", i+1, p->name().c_str(), p->value().c_str());

    if (strcmp(p->name().c_str(), PARAM_WIFI_SSID) == 0) {
      if (p->value().length() > 0) {
        writeFile(SPIFFS, PATH_WIFI_SSID, p->value().c_str());
     } 
    }

    if (strcmp(p->name().c_str(), PARAM_WIFI_PASSWORD) == 0) {
      if (p->value().length() > 0) {
        writeFile(SPIFFS, PATH_WIFI_PASSWORD, p->value().c_str());
     } 
    }

    if (strcmp(p->name().c_str(), PARAM_RTK_LOCATION_METHOD) == 0) {
      if (p->value().length() > 0) {
        writeFile(SPIFFS, PATH_RTK_LOCATION_METHOD, p->value().c_str());
     } 
    }

    if (strcmp(p->name().c_str(), PARAM_RTK_LOCATION_SURVEY_ACCURACY) == 0) {
      if (p->value().length() > 0) {
        writeFile(SPIFFS, PATH_RTK_LOCATION_SURVEY_ACCURACY, p->value().c_str());
     } 
    }

    if (strcmp(p->name().c_str(), PARAM_RTK_LOCATION_LATITUDE) == 0) {
      if (p->value().length() > 0) {
        String deconstructedValAsCSV = getDeconstructedValAsCSV(p->value());
        writeFile(SPIFFS, PATH_RTK_LOCATION_LATITUDE, deconstructedValAsCSV.c_str());
     } 
    }

    if (strcmp(p->name().c_str(), PARAM_RTK_LOCATION_LONGITUDE) == 0) {
      if (p->value().length() > 0) {
        String deconstructedValAsCSV = getDeconstructedValAsCSV(p->value());
        writeFile(SPIFFS, PATH_RTK_LOCATION_LONGITUDE, deconstructedValAsCSV.c_str());
     } 
    }

    if (strcmp(p->name().c_str(), PARAM_RTK_LOCATION_ALTITUDE) == 0) {
      if (p->value().length() > 0) {
        String deconstructedValAsCSV = getDeconstructedValAsCSV(p->value());
        writeFile(SPIFFS, PATH_RTK_LOCATION_ALTITUDE, deconstructedValAsCSV.c_str());
     } 
    }
  }

  DEBUG_SERIAL.println(F("Data saved to SPIFFS!"));
  request->send_P(200, "text/html", INDEX_HTML, RTKBaseManager::processor);
}

String RTKBaseManager::getDeconstructedValAsCSV(const String& doubleStr) {
    double dVal = doubleStr.toDouble();
    int32_t lowerPrec = getLowerPrecisionPartFromDouble(dVal);
    int8_t highPrec = getHighPrecisionPartFromDouble(dVal);
    String deconstructedCSV = String(lowerPrec) + "," + String(highPrec);
    return deconstructedCSV;
}

String RTKBaseManager::getDoubleStringFromCSV(const String& csvStr) { 
  if (csvStr.isEmpty()) return String();
  int32_t lowerPrec = getValueAsStringFromCSV(csvStr, ',', 0).toInt();
  int8_t highPrec = getValueAsStringFromCSV(csvStr, ',', 1).toInt();
  double reconstructedVal = getDoubleFromIntegerParts(lowerPrec, highPrec);
  String reconstructedValStr = String(reconstructedVal, 9);
  return reconstructedValStr;
}

// Replaces placeholder with stored values
String RTKBaseManager::processor(const String& var) 
{
  if (var == PARAM_WIFI_SSID) {
    String savedSSID = readFile(SPIFFS, PATH_WIFI_SSID);
    return (savedSSID.isEmpty() ? String(PARAM_WIFI_SSID) : savedSSID);
  }
  else if (var == PARAM_WIFI_PASSWORD) {
    String savedPassword = readFile(SPIFFS, PATH_WIFI_PASSWORD);
    return (savedPassword.isEmpty() ? String(PARAM_WIFI_PASSWORD) : "*******");
  }
  else if (var == PARAM_RTK_LOCATION_METHOD) {
    String savedLocationMethod = readFile(SPIFFS, PATH_RTK_LOCATION_METHOD);
    return (savedLocationMethod.isEmpty() ? String(PARAM_RTK_SURVEY_ENABLED) : savedLocationMethod);
  }
  else if (var == PARAM_RTK_LOCATION_SURVEY_ACCURACY) {
    String savedSurveyAccuracy = readFile(SPIFFS, PATH_RTK_LOCATION_SURVEY_ACCURACY);
    return (savedSurveyAccuracy.isEmpty() ? String(PARAM_RTK_LOCATION_SURVEY_ACCURACY) : savedSurveyAccuracy);
  }
  else if (var == PARAM_RTK_LOCATION_LATITUDE) {
    String savedLatitude = readFile(SPIFFS, PATH_RTK_LOCATION_LATITUDE);
    String savedLatitudeStr = getDoubleStringFromCSV(savedLatitude);
    return (savedLatitude.isEmpty() ? String(PARAM_RTK_LOCATION_LATITUDE) : savedLatitudeStr);
  }
  else if (var == PARAM_RTK_LOCATION_LONGITUDE) {
    String savedLongitude = readFile(SPIFFS, PATH_RTK_LOCATION_LONGITUDE);
    String savedLongitudeStr = getDoubleStringFromCSV(savedLongitude);
    return (savedLongitude.isEmpty() ? String(PARAM_RTK_LOCATION_LONGITUDE) : savedLongitudeStr);
  }
  else if (var == PARAM_RTK_LOCATION_ALTITUDE) {
    String savedAlt = readFile(SPIFFS, PATH_RTK_LOCATION_ALTITUDE);
    String altitudeDoubleStr = getDoubleStringFromCSV(savedAlt);
    return (altitudeDoubleStr.isEmpty() ? String(PARAM_RTK_LOCATION_ALTITUDE) : altitudeDoubleStr);
  }
  else if (var == "next_addr") {
    String savedSSID = readFile(SPIFFS, PATH_WIFI_SSID);
    String savedPW = readFile(SPIFFS, PATH_WIFI_PASSWORD);
    if (savedSSID.isEmpty() || savedPW.isEmpty()) {
      return String(IP_AP);
    } else {
      String clientAddr = String(DEVICE_NAME);
      clientAddr += ".local";
      return clientAddr;
    }
  }
  else if (var == "next_ssid") {
    String savedSSID = readFile(SPIFFS, PATH_WIFI_SSID);
    return (savedSSID.isEmpty() ? String(AP_SSID) : savedSSID);
  }
  return String();
}

/********************************************************************************
*                             SPIFFS
* ******************************************************************************/

void RTKBaseManager::setupSPIFFS(void) {
  #ifdef ESP32
    if (!SPIFFS.begin(true)) {
      DEBUG_SERIAL.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if (!SPIFFS.begin()) {
      DEBUG_SERIAL.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif
}

String RTKBaseManager::readFile(fs::FS &fs, const char* path) 
{
  DEBUG_SERIAL.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");

  if (!file || file.isDirectory()) {
    DEBUG_SERIAL.println("- empty file or failed to open file");
    return String();
  }
  DEBUG_SERIAL.println("- read from file:");
  String fileContent;

  while (file.available()) {
    fileContent += String((char)file.read());
  }
  file.close();
  DEBUG_SERIAL.println(fileContent);

  return fileContent;
}

void RTKBaseManager::writeFile(fs::FS &fs, const char* path, const char* message) 
{
  DEBUG_SERIAL.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, "w");
  if (!file) {
    DEBUG_SERIAL.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    DEBUG_SERIAL.println("- file written");
  } else {
    DEBUG_SERIAL.println("- write failed");
  }
  file.close();
}

void RTKBaseManager::listFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
 
  while (file) {
      DEBUG_SERIAL.print("FILE: ");
      DEBUG_SERIAL.println(file.name());
      file = root.openNextFile();
  }
  file.close();
  root.close();
}

void RTKBaseManager::wipeSpiffsFiles() 
{
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  DEBUG_SERIAL.println(F("Wiping: "));

  while (file) {
    DEBUG_SERIAL.print("FILE: ");
    DEBUG_SERIAL.println(file.path());
    SPIFFS.remove(file.path());
    file = root.openNextFile();
  }
}

int32_t RTKBaseManager::getLowerPrecisionPartFromDouble(double input) 
{
 // We work with 7 + 2 post dot places, (max 0.11 mm accuracy)
  double intp, fracp;
  fracp = modf(input, &intp);
  String output = String((int)intp);
  String fracpStr = (fracp < 0) ? String(abs(fracp), 9) : String(fracp, 9);
  output += fracpStr.substring(2,9);
  return atoi(output.c_str());
}

int8_t RTKBaseManager::getHighPrecisionPartFromDouble(double input) 
{
  // We work with 7 + 2 post dot places, (max 0.11 mm accuracy)
  double intp, fracp;
  fracp = abs(modf(input, &intp));
  String fracpStr = String(fracp, 9);
  String output = fracpStr.substring(9, 11);
  return atoi(output.c_str());
}

double RTKBaseManager::getDoubleFromIntegerParts(int32_t commonPrecisionInt, int8_t highPrecisionInt) 
{
  double commonPrecisionDouble, highPrecisionDouble;
  commonPrecisionDouble = (double)commonPrecisionInt * 10e-8;
  highPrecisionDouble = (double)highPrecisionInt * 10e-10;

  return (commonPrecisionDouble + highPrecisionDouble);
}

  // Function to parse lora string message
String RTKBaseManager::getValueAsStringFromCSV(const String &data, char separator, int index)
{
  int idx = data.indexOf(separator);
  int length = data.length();
  String result = (index==0) ? data.substring(0, idx) : data.substring(idx+1, length);
  return  result;
}
