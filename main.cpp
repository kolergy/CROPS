/*******************************************************************************

                                C R O P S - V0.3
                      CRoissance Optimale des Plantes en Sol


Colecte des donnees environementales pour unbe croissance optimale des plantes
et stockage de ces donnees dans une base de donmnee de type couchDB

 - Le capteur DHT22 permet la mesure de la temperature ambiante
   ainsi que du niveau d'humiditee ambiante
 - Le capteur CSMS V1.2 permet la mesure de la quantitee d'eau dans le sol.
   CSMS: 'Capacitive Soil Moisture Sensor'
 - Le captteur BMP085 permet la mesure de la pression et de la temperature
   (de maniere semblet'il plus precise aue le DHT mais je ne l'ai pas verifie)
 - Le capteur TSL2561 Permet de mesurer la lumiere recue par les plantes

Auteur: kolergy{@}gmail.com
*******************************************************************************/

/************** Declaration des librairies utilisees ***************/
#include <Adafruit_BMP085.h>        // Par Adafruit
#include <Adafruit_GFX.h>           // Par Adafruit
#include <Adafruit_Sensor.h>        // Par Adafruit
#include <Adafruit_TSL2561_U.h>     // Par Adafruit
#include <ArduinoJson.h>            // Par Benoit Blanchon
#include "DHTesp.h"                 // Par Bernd Giesecke
#include <LOLIN_EPD.h>              // Par Wemos
#include <HTTPClient.h>             // Par ?
#include <NTPClient.h>              // Par Fabrice Weinberg
#include <TimeLib.h>                // Par Paul Stoffregen
#include <SPI.h>                    // Par ?
#include <WiFi.h>                   // Par ?

/*************************** Definitions **************************************/
/* Definition des annees: il y a une annee NON bisextile !
      quand l'annee est superieure a zero
                                      |  ET PAS les multiples de quatres ans
                                      |         |     et tous les sciecles
                                      |         |            |    sauf tous les 4 sciecles
                                      |         |            |           |                */
#define ANNEE_NON_BISEXTILE(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ) )

/******************** Declaration des constantes *******************/
/* Utiliser des constantes plustot que des #define permet au compileur
de mieux optimiser la memoire                                          */
/********* Definition des Broches valide pour le Lolin D32 Pro *********/
const int   BROCHE_LIGHT  =    2;   // broche de Controlant l'eclerage
const int   BROCHE_LED    =    5;   // broche controlant la LED
const int   BROCHE_DHT    =    4;   // broche de comunication avec le DHT
const int   BROCHE_CSMS1  =   32;   // broche de measure du premier CSMS
const int   BROCHE_CSMS2  =   34;   // broche de measure du second  CSMS
const int   BROCHE_BAT    =   35;   // broche de measure de la tension baterie

const int   EPD_CS        =   14;   // Broches Pour le e-Paper Lolin 2.13 250x122
const int   EPD_DC        =   27;   //
const int   EPD_RST       =   33;   // can set to -1 and share with microcontroller Reset!
const int   EPD_BUSY      =   -1;   // signal Busy n'est pas conecte donc defini a -1 (Attente de delais fixes)

/********* Autres Definitions *********/
const int   TIME_DIV    =   10;                        // Minutes
const int   DAY_DIV     =   60*24/TIME_DIV;            // 144 divisions fo a 10 mins divisions
const int   SLEEP_SEC   =   60*2;
const float WATER_LOW   = 3050.0;
const float WATER_HIGH  =  771.0;
const float LIGHT_START =    6.0;
const float LIGHT_END   =    0.0;      // end < Start
const char* versionStr  = "CROPS_V0.3";
const char* sensorID    = "Crops01";
const char* WiFiSsid    = "MyWiFiName";                    // wifi config
const char* WiFiPass    = "MyWiFiPassword";
const char* DBUser      = "MDBUserName";
const char* DBPass      = "MDBPassword";
const char* DBName      = "crops";
const char* DBPort      = "5984";
const char* DBIp        = "999.999.999.999";

/************** declaration des Variables Globales a minimiser ****************/
bool          couchFail = false;
bool          lights    = false;
bool          WiFiCon   = false;
char          deviceid[21];
char          ipstr[20];
int           results[16];
int           coreID;
uint32_t      CPUFreq;
long          rssi;
unsigned long ts0;
time_t        tt0;                       // temp de demarage des activitees
uint64_t      chipid;
float         tWif;
float         tNTP;
float         tSet;
float         tTot;
float         tJso;

/******************** Instance des Objets ***************************/
DHTesp        dht;                       // instance du capteur de temperature et d'humiditee
HTTPClient    http;                      // instance du client NTP
WiFiUDP       ntpUDP;                    // instance UDP
NTPClient     timeClient(ntpUDP, "europe.pool.ntp.org", 3600*2, 60000);

LOLIN_IL3897             EPD(250, 122, EPD_DC, EPD_RST, EPD_CS, EPD_BUSY); //hardware SPI pour la conection a l'e-Paper
Adafruit_BMP085          bmp;                                              // Instance du capteur de pression & temperature
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 26872);

StaticJsonBuffer<1000> jsonBuffer;                         // Memoire pour l'arbre JSON. ATTENTION: Augmenter la valeur quand on augmente la taille du JSON
JsonObject& root      = jsonBuffer.createObject();         // Creation de la racine de l'arbre
const long int period = dht.getMinimumSamplingPeriod()*2;  // Le capteur DHT done sa periode d'echantillonage mais ca ne fonctione par allors on multiplie par 2

/******** declaration des Variables Globales en memoire RTC plus necessaire
maintenant qu'il n'y as plus de deep sleep a passer en constantes ********/
RTC_DATA_ATTR float    tempT[DAY_DIV];   // Stocke une valeur de temperature toute les 10 minutes pendant 24h
RTC_DATA_ATTR float    humiT[DAY_DIV];   // Stocke une valeur d'humiditee toute les 10 minutes pendant 24h
RTC_DATA_ATTR long     epoch  = 0;       // nombres de cycles depuis le dernier reset
RTC_DATA_ATTR float    tSum   = 0;
RTC_DATA_ATTR float    hSum   = 0;
RTC_DATA_ATTR int      eSum   = 0;
RTC_DATA_ATTR int      divN   = 0;
RTC_DATA_ATTR bool     light  = false;   // permet de conaitre l'etat precedent de l'eclerage
RTC_DATA_ATTR time_t   tm1    = 0;       // Previous epoch time

/******************** declaration prealable des Fonctions *********************
   Normalement ce n'est pas necessaire excepte si vous utilisez l'environement
   platform.io qui le nececite
*********************************************************************/
void   displayESP32Info();
bool   connectToWifi();
void   connectToCouch();
void   storeToCouch(String);
void   connectToNTP();
void   fillTables(unsigned, float, float);
String getFormattedDate();
void   configureSensor(void);
void   displaySensorDetails(void);

/********* Fonction Set-up Mise en place de l'environement *********/
void setup() {
  //esp_sleep_enable_timer_wakeup(DPSLEEP_SEC * S_TO_US);
  ts0 = millis();                        // Recupere le temps de la carte
  pinMode(     BROCHE_LIGHT, OUTPUT);    // Declaration les sorties
  pinMode(     BROCHE_LED  , OUTPUT);
  //pinMode(     BROCHE_BAT  , INPUT );  // Declaration les Entrees
  digitalWrite(BROCHE_LED  , LOW   );    // Alumage de la LED

  Serial.begin(115200);                  // Initialise la comunication serie

  EPD.begin(                     );      // Initialisation de l'e-Paper
  EPD.clearBuffer(               );
  EPD.fillScreen(  EPD_WHITE     );
  EPD.setTextColor(EPD_BLACK     );
  EPD.setTextSize( 2             );
  EPD.setCursor(   2, 0          );
  EPD.println(     versionStr    );
  EPD.println(     "Set-up"      );
  EPD.display(                   );      // Affichage du contenu


  /* Initialise the sensors */
  dht.setup(BROCHE_DHT, DHTesp::DHT22);  // Connecte le Capteur DHT22 sur la broche definie
  while(!bmp.begin()) {
    Serial.println("Ooops, no BMP085 sensor dtected... check your wiring");
    delay(500);
  }
  //use tsl.begin() to default to Wire,  tsl.begin(&Wire2) directs api to use Wire2, etc.
  while(!tsl.begin()) {
    Serial.print("Ooops, no TSL2561 sensor detected... Check your wiring");
    delay(500);
  }

  configureSensor();          // Setup the light sensor gain and integration time (necessaire)
  displaySensorDetails();     // Display some basic information on the light sensor (pas necessaire)
  displayESP32Info();         // Afiche les donnees de l'ESP sur le port serie (pas necessaire)
  WiFiCon = connectToWifi();  // Se conecte au Wifi
  if(WiFiCon) {               // Pas de wifi pas de couch
    connectToCouch();         // Se connect a CouchDB
    connectToNTP();           // Se conecte au serveur NTP
  } else {
    Serial.println( "No Wifi skiping connections");
    EPD.clearBuffer(                             );
    EPD.fillScreen( EPD_WHITE                    );
    EPD.println(    "No Wifi skiping connections");
    EPD.display();
  }

  EPD.println(   "Set-up Complete"      );
  EPD.display();
  Serial.println("Starting measurements");
  Serial.println("Measurement Period: " + String(period) + " ms");
  Serial.println();
  tSet = (millis()-ts0)/1000.0;
}

void loop() {
  String s = "Loop " + String(epoch);
  char ch[99];
  s.toCharArray(ch,99);
  delay(period);
  digitalWrite(BROCHE_LED, LOW);
  bool NTPok = timeClient.update();                                 //
  String dateStamp    = getFormattedDate();
  String timeStamp    = timeClient.getFormattedTime();
  time_t tt           = timeClient.getEpochTime();     // get the current time from NTP time
  time_t t            = now();                         // get the current time from board
  time_t dt           = tt-t;                          // error between board time & NTP
  time_t uptime       =  t-tt0;                        // seconds since start of monitoring

  //s = dateStamp + " " + timeStamp;
  s = "Loop " + String(epoch) + " " + timeStamp;
  s.toCharArray(ch,99);
  EPD.clearBuffer(             );
  EPD.fillScreen(  EPD_WHITE   );
  EPD.setTextColor(EPD_BLACK   );
  EPD.setTextSize( 2           );
  EPD.setCursor(   0, 0        );
  EPD.println(ch);
  EPD.display();
  int      hour  = timeStamp.substring(0,2).toInt();
  int      mins  = timeStamp.substring(3,5).toInt();
  unsigned tmins = hour * 60 + mins;
  if(NTPok){
    Serial.println("Time OK");
    tm1 = tt;
    if(hour >= LIGHT_END && hour < LIGHT_START) {
      Serial.println("Les lumieres sont Etintes ");
      digitalWrite(BROCHE_LIGHT, LOW);
      lights    = false;
      EPD.println("Lights OFF");
      EPD.display();
    } else {
      Serial.println("Les lumieres sont Allumees");
      digitalWrite(BROCHE_LIGHT, HIGH);
      lights    = true;
      EPD.println("Lights ON");
      EPD.display();
    }
  } else {
    Serial.println("Time KO");
  }
  float waterlevel1   =  0;
  float waterlevel2   =  0;
  float powerlevel    =  0;
  int   n             = 30;
  Serial.print(   n                  );
  Serial.println( " Measurement loop");
  for(int i =0;i<n;i++) {
      waterlevel1    += analogRead(BROCHE_CSMS1);
      waterlevel2    += analogRead(BROCHE_CSMS2);
      powerlevel     += analogRead(BROCHE_BAT  );
      delay(10);
  }
  waterlevel1         = waterlevel1 / n;     // average a set of measurements
  waterlevel2         = waterlevel2 / n;     // average a set of measurements
  powerlevel          = powerlevel  / n;

  sensors_event_t event;
  tsl.getEvent(&event);   // Get a new sensor event

  float waterlevelCor1= 100*(1-((waterlevel1 - WATER_HIGH) / (WATER_LOW-WATER_HIGH)));
  float waterlevelCor2= 100*(1-((waterlevel2 - WATER_HIGH) / (WATER_LOW-WATER_HIGH)));
  float vBatRaw       = 3.3 * powerlevel/4096;
  float vBat          = vBatRaw * 1.44;
  //for(int i=0;i<15;i++) {
  //  results[i] = analogRead(BROCHES_ANA[i]);
  //}
  float  humidity      = dht.getHumidity();             // read the sensor humidity
  float  temperature   = dht.getTemperature();          // read the sensor temperature
  float  tempBaro      = bmp.readTemperature();
  float  pressure      = bmp.readPressure();
  float  pressureSL    = bmp.readSealevelPressure(141+1);
  float  light         = event.light;
  uint16_t broadband = 0;
  uint16_t infrared  = 0;

  /* Populate broadband and infrared with the latest values */
  tsl.getLuminosity (&broadband, &infrared);

  fillTables(tmins, temperature, humidity);

  for(int i=0;i<DAY_DIV;i++) {
    if(i%6==0) {
      Serial.println("");
      Serial.print("Hour:" +String(i/6) + "  ");
    }
    Serial.print(String(tempT[i]) + "   ");
  }
  Serial.println("");
  for(int i=0;i<DAY_DIV;i++) {
    if(i%6==0) {
      Serial.println("");
      Serial.print("Hour:" +String(i/6) + "  ");
    }
    Serial.print(String(humiT[i]) + "   ");
  }
  Serial.println("");
  s = "T =" + String(temperature) + "C  H =" + String(humidity) + "%";
  s.toCharArray(ch,99);
  EPD.println(     ch          );
  s = "P =" + String(pressure) + " Pa";
  s.toCharArray(ch,99);
  EPD.println(     ch          );
  s = "W1=" + String(waterlevelCor1) + "%";
  s.toCharArray(ch,99);
  EPD.println(     ch          );
  s = "W2=" + String(waterlevelCor2) + "%";
  s.toCharArray(ch,99);
  EPD.println(     ch          );
  s = "L =" + String(light) + " Lux";
  s.toCharArray(ch,99);
  EPD.println(     ch          );
  EPD.display();
  EPD.display();

  Serial.println("Measures OK -> generation de la StringA pour le Json");
  root.set(          "Sensor"  , sensorID      );      // put the data into the json
  root.set(          "DeviceID", deviceid      );
  root.set(          "CPUFreq" , CPUFreq       );
  root.set(          "RSSI"    , rssi          );
  root.set(          "IPadress", ipstr         );
  root.set(          "Date"    , dateStamp     );
  root.set(          "Hour"    , timeStamp     );
  root.set(         "ThisBoard", ARDUINO_BOARD );
  root.set<bool>(   "couchFail", couchFail     );
  root.set<bool>(    "NTPok"   , NTPok         );
  root.set<bool>(    "lights"  , lights        );
  root.set<time_t>(  "Time"    , t             );
  root.set<time_t>(  "UpTime"  , uptime        );
  root.set<time_t>(  "DTim"    , dt            );
  root.set<time_t>(  "tt0"     , tt0           );
  root.set<float>(   "tWif"    , tWif          );
  root.set<float>(   "tNTP"    , tNTP          );
  root.set<float>(   "tSet"    , tSet          );
  root.set<float>(   "vBatRaw" , vBatRaw       );
  root.set<float>(   "vBat"    , vBat          );
  root.set<float>(   "Temp"    , temperature   );
  root.set<float>(   "Hum"     , humidity      );
  root.set<float>(   "Press"   , pressure      );
  root.set<float>(   "PressSL" , pressureSL    );
  root.set<float>(   "TempBaro", tempBaro      );
  root.set<float>(   "Light"   , light         );
  root.set<float>(   "Brdband" , broadband     );
  root.set<float>(   "Infrared", infrared      );
  root.set<float>(   "Water1%" , waterlevelCor1);
  root.set<float>(   "Water1"  , waterlevel1   );
  root.set<float>(   "Water2%" , waterlevelCor2);
  root.set<float>(   "Water2"  , waterlevel2   );
  root.set<float>(   "Power"   , powerlevel    );
  root.set<unsigned long>("Epoch"   , epoch    );
  Serial.println("StringA  OK");

  String jsonStr;
  tJso = (millis()-ts0)/1000.0;
  root.set<float>(   "tJso"    , tJso          );
  root.printTo(jsonStr);                              // put the json in a string
  Serial.println("StringB  OK");

  storeToCouch(jsonStr);

  //delay(60000);
  digitalWrite(BROCHE_LED, HIGH);
  tTot      = now()-t;
  float slp = max(float(0.0),(SLEEP_SEC-tTot));
  Serial.println("After " +String(tTot)+ "s This is the END of Epoch"+String(epoch)+", sleepinng for " + String(slp) + "s");
  epoch     = epoch + 1;
  delay(slp*1000);
  //esp_sleep_enable_timer_wakeup((DPSLEEP_SEC-tTot) * S_TO_US);
  //esp_deep_sleep_start();
}

// Based on https://github.com/PaulStoffregen/Time/blob/master/Time.cpp
// currently assumes UTC timezone, instead of using this->_timeOffset
String getFormattedDate() {
  unsigned long rawTime = (timeClient.getEpochTime()) / 86400L;  // in days
  unsigned long days    =    0;
  unsigned long year    = 1970;
  uint8_t month;
  static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

  while((days += (ANNEE_NON_BISEXTILE(year) ? 366 : 365)) <= rawTime) {year++;}
  rawTime -= days - (ANNEE_NON_BISEXTILE(year) ? 366 : 365); // now it is days in this year, starting at 0
  days=0;
  for (month=0; month<12; month++) {
    uint8_t monthLength;
    if (month==1) { monthLength = ANNEE_NON_BISEXTILE(year) ? 29 : 28; } // february
    else          { monthLength = monthDays[month];}
    if (rawTime < monthLength) break;
    rawTime -= monthLength;
  }
  String monthStr = ++month   < 10 ? "0" + String(month  ) : String(month  ); // jan is month 1
  String dayStr   = ++rawTime < 10 ? "0" + String(rawTime) : String(rawTime); // day of month
  return String(year) + "-" + monthStr + "-" + dayStr;
}

void displayESP32Info() {
  chipid              = ESP.getEfuseMac();    // some infos about the board
  CPUFreq             = ESP.getCpuFreqMHz();
  coreID              = xPortGetCoreID();
  sprintf(deviceid, "%" PRIu64, chipid);
  Serial.println("--------------------------------------");
  Serial.println("Board       : " + String(ARDUINO_BOARD));
  Serial.println("DeviceID    : " + String(deviceid     ));
  Serial.println("CPUFreqency : " + String(CPUFreq      ));
  Serial.println("Used Core ID: " + String(coreID       ));
  Serial.println("--------------------------------------");
}

bool connectToWifi() {
  Serial.print("Connecting to the WiFi: ");
  WiFi.begin(WiFiSsid , WiFiPass);
  int WTry = 1;
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(100);
    Serial.print(".");
    EPD.print(".");
    EPD.display();
    unsigned long ts1   = millis();           // get the current time from board
    tWif                = (ts1-ts0)/1000.0;   // calculate the time to get a wifi fix
    if(tWif > 10*WTry) {
      WTry++;
      Serial.println("WiFi retry:");
      EPD.println("Wifi retry");
      EPD.display();
      WiFi.reconnect();
    }
    if(WTry > 10) {
      EPD.println("Wifi Wifi failed after 10 attempts");
      return(false);
    }
  }
  EPD.println("Wifi reconect");
  EPD.display();

  rssi          = WiFi.RSSI();
  IPAddress ip  = WiFi.localIP();
  Serial.print(  "Conected! in: "                      );
  Serial.println(tWif                                  );
  Serial.print(  "IP number assigned by DHCP is: "     );
  sprintf(ipstr, "%d:%d:%d:%d", ip[0],ip[1],ip[2],ip[3]);
  Serial.println(ipstr);
  return(true);
}

void connectToCouch() {
  EPD.println(   "Couch Wait"                                      );
  EPD.display();
  Serial.print(  "Conection to couchdb: "                          );
  String s= "http://" +String(DBUser)+ ":" + String(DBPass)+ "@" +String(DBIp)+ ":" +String(DBPort)+ "/" + String(DBName);
  Serial.println(s);
  http.begin(s);
  int httpCode = http.GET();
  Serial.println(httpCode);
  if(httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    Serial.println(response);
    couchFail = false;
    Serial.println("Connection to couchDB OK!");
    EPD.println(   "Couch OK  "                                      );
    EPD.display();
  } else {
    couchFail = true;
    Serial.println("Connection to couchDB FAILED!"                   );
    EPD.println(   "Couch Fail"                                      );
    EPD.display();
  }
}

void storeToCouch(String jsonStr) {
  if(WiFiCon) {
    http.addHeader("Content-Type", "application/json"); //Specify content-type header
    int httpCode     = http.POST(jsonStr);              //Send the request
    Serial.println("HTTP code: " + String(httpCode));
    if(httpCode == HTTP_CODE_CREATED) {
      String payload = http.getString();                //Get the response payload
      //Serial.println(payload);
      couchFail      = false;
      Serial.println("Upload to couchDB OK!");
    } else {
      Serial.println("Upload to couchDB FAILED!");
      couchFail      = true;
      root.set<bool>(   "couchFail", couchFail     );
      root.printTo(jsonStr);                            // put the json in a string
      http.POST(jsonStr);                               //resend the request
    }
    Serial.println(jsonStr);
  }
}

void connectToNTP() {
  timeClient.begin();
  Serial.println("NTP time client started: Synchronising with NTP");
  unsigned long ts2 = millis();                   // get the current time from board
  tNTP = (millis()-ts2)/1000.0;                   // initialise delta time
  while(!timeClient.update() && tNTP<60.0*4) {
    delay(100);
    Serial.print(".");
    tNTP = (millis()-ts2)/1000.0;
  }
  if(tNTP > 60.0*4) Serial.println("Failed to Synchronize");                        //
  tt0 = timeClient.getEpochTime();                // get the ntp time
  setTime(tt0);
  delay(100);                                     // set the board time to the ntp time
}

void fillTables(unsigned tmins, float temperature, float humidity) {
  Serial.println("Temp & Hum Tables");
  if(tmins > (divN+1)*TIME_DIV) {
    Serial.println("New time div");
    if(eSum>0){
      tempT[divN] = tSum/eSum;
      humiT[divN] = hSum/eSum;
    }
    divN = tmins/TIME_DIV;
    eSum = 0;
    tSum = 0;couchFail = true;

    hSum = 0;
  } else if(divN >= DAY_DIV-1 && tmins < divN*TIME_DIV) {  // just pased the end of the day
    Serial.println("New day & time div");
    if(eSum>0){
      tempT[divN] = tSum/eSum;
      humiT[divN] = hSum/eSum;
    }
    divN = 0;
    eSum = 0;
    tSum = 0;
    hSum = 0;
  }
  eSum++;
  tSum+=temperature;
  hSum+=humidity;
  Serial.println("eSum:" + String(eSum) );
}

/**************************************************************************/
/*
    Configures the gain and integration time for the TSL2561
*/
/**************************************************************************/
void configureSensor(void) {
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */

  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  //tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
   tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("402ms");
  Serial.println("------------------------------------");
}

void displaySensorDetails(void) {
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" lux");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" lux");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" lux");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}
