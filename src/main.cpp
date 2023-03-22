#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// GPIOs
const int TEMPO_NOW_RED = 12;
const int TEMPO_NOW_GREEN = 13;
const int TEMPO_NOW_BLUE = 14;
const int TEMPO_TOMORROW_RED = 25;
const int TEMPO_TOMORROW_GREEN = 32;
const int TEMPO_TOMORROW_BLUE = 33;

const int REFRESH_TIME_IN_MILLI = 30 * 60 * 1000;
const int BLINK_TIME_IN_MILLI =  1000;
unsigned long lastRefresh = millis();
unsigned long lastBlink = millis();
int blinkUnknowStep = 1;
int blinkErrorStep = 1;

// WiFi
const char *ssid = "*** your ssid ***";
const char *password = "*** password ***";

String couleurJourJ = "";
String couleurJourJ1 = "";
String currentDate = "";
int daysWhite = -1;
int daysRed = -1;
int dayBlue = -1;

HTTPClient httpClientTempoColor, httpClientTempoDays, httpClientDate;

WebServer server(80);

void initLed() {
  pinMode(TEMPO_NOW_RED, OUTPUT);
  pinMode(TEMPO_NOW_GREEN, OUTPUT);
  pinMode(TEMPO_NOW_BLUE, OUTPUT);

  pinMode(TEMPO_TOMORROW_RED, OUTPUT);
  pinMode(TEMPO_TOMORROW_GREEN, OUTPUT);
  pinMode(TEMPO_TOMORROW_BLUE, OUTPUT);
}

void clearLed() {
  digitalWrite(TEMPO_NOW_RED, LOW);
  digitalWrite(TEMPO_NOW_GREEN, LOW);
  digitalWrite(TEMPO_NOW_BLUE, LOW);

  digitalWrite(TEMPO_TOMORROW_RED, LOW);
  digitalWrite(TEMPO_TOMORROW_GREEN, LOW);
  digitalWrite(TEMPO_TOMORROW_BLUE, LOW);
}

void initWifi() {
  // Connexion au Wifi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("ok !");
  Serial.printf("SSID: %s, IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void checkWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    initWifi();
  }
}

void updateLed() {
  clearLed();

  // NOW
  if (couleurJourJ == "TEMPO_BLEU") {
    digitalWrite(TEMPO_NOW_BLUE, HIGH);
  } else if (couleurJourJ == "TEMPO_ROUGE") {
    digitalWrite(TEMPO_NOW_RED, HIGH);
  } else if (couleurJourJ == "TEMPO_BLANC") {
    digitalWrite(TEMPO_NOW_RED, HIGH);
    digitalWrite(TEMPO_NOW_GREEN, HIGH);
    digitalWrite(TEMPO_NOW_BLUE, HIGH);
  }

    // TOMORROW
  if (couleurJourJ1 == "TEMPO_BLEU") {
    digitalWrite(TEMPO_TOMORROW_BLUE, HIGH);
  } else if (couleurJourJ1 == "TEMPO_ROUGE") {
    digitalWrite(TEMPO_TOMORROW_RED, HIGH);
  } else if (couleurJourJ1 == "TEMPO_BLANC") {
    digitalWrite(TEMPO_TOMORROW_RED, HIGH);
    digitalWrite(TEMPO_TOMORROW_GREEN, HIGH);
    digitalWrite(TEMPO_TOMORROW_BLUE, HIGH);
  }
}

void blinkLed() {
  if (couleurJourJ1 == "NON_DEFINI") {
    if (blinkUnknowStep == 1) {
      digitalWrite(TEMPO_TOMORROW_RED, HIGH);
      digitalWrite(TEMPO_TOMORROW_GREEN, LOW);
      digitalWrite(TEMPO_TOMORROW_BLUE, LOW);
      blinkUnknowStep = 2;
    } else if (blinkUnknowStep == 2) {
      digitalWrite(TEMPO_TOMORROW_RED, LOW);
      digitalWrite(TEMPO_TOMORROW_GREEN, HIGH);
      digitalWrite(TEMPO_TOMORROW_BLUE, LOW);
      blinkUnknowStep = 3;
    } else if (blinkUnknowStep == 3) {
      digitalWrite(TEMPO_TOMORROW_RED, LOW);
      digitalWrite(TEMPO_TOMORROW_GREEN, LOW);
      digitalWrite(TEMPO_TOMORROW_BLUE, HIGH);
      blinkUnknowStep = 4;
    } else if (blinkUnknowStep == 4) {
      digitalWrite(TEMPO_TOMORROW_RED, LOW);
      digitalWrite(TEMPO_TOMORROW_GREEN, LOW);
      digitalWrite(TEMPO_TOMORROW_BLUE, LOW);
      blinkUnknowStep = 1;
    }
  }
  if (couleurJourJ == "ERROR" || currentDate == "ERROR") {
    if (blinkErrorStep == 1) {
      digitalWrite(TEMPO_NOW_RED, HIGH);
      digitalWrite(TEMPO_TOMORROW_RED, LOW);
      blinkErrorStep = 2;
    } else if (blinkErrorStep == 2) {
      digitalWrite(TEMPO_NOW_RED, LOW);
      digitalWrite(TEMPO_TOMORROW_RED, HIGH);
      blinkErrorStep = 1;
    }
  }
}

/**
 * @brief Récupère la date du jour sur mon synology
 * 
 * <?php
 * date_default_timezone_set('Europe/Paris');
 * echo @date("Y-m-d|H:i:s");
 * ?>
 * 
 */
void getDate() {
  String url = "http://192.168.1.150/datetime.php";
  httpClientDate.begin(url);
  int httpCode = httpClientDate.GET();

  if (httpCode == 200) {
    String payload = httpClientDate.getString();

    currentDate = payload.substring(0, 10);
    Serial.println(currentDate);
  } else {
    currentDate == "ERROR";
    Serial.println("Http error :" + httpCode);
  }
  httpClientDate.end();
}

/**
 * @brief Récupération des jours restant par couleur
 * 
 */
void getTempoDays() {
  String url = "https://particulier.edf.fr/services/rest/referentiel/getNbTempoDays?TypeAlerte=TEMPO";
  httpClientTempoDays.begin(url);
  httpClientTempoDays.addHeader("Accept", "application/json");
  int httpCode = httpClientTempoDays.GET();

  if (httpCode == 200) {
    String payload = httpClientTempoDays.getString();

    DynamicJsonDocument doc(100);
    deserializeJson(doc, payload);

     daysWhite = doc["PARAM_NB_J_BLANC"];
     daysRed = doc["PARAM_NB_J_ROUGE"];
     dayBlue = doc["PARAM_NB_J_BLEU"];

     Serial.println(String(daysRed) + " / " + String(daysWhite) + " / " + String(dayBlue));
  }

}

/**
 * @brief Récupération de la couleur du jour et du lendemain
 * 
 */
void getTempoColor() {
  String url = "https://particulier.edf.fr/services/rest/referentiel/searchTempoStore?dateRelevant=" + currentDate;
  httpClientTempoColor.begin(url);
  httpClientTempoColor.addHeader("Accept", "application/json");
  int httpCode = httpClientTempoColor.GET();

  if (httpCode == 200) {
    String payload = httpClientTempoColor.getString();

    DynamicJsonDocument doc(100);
    deserializeJson(doc, payload);

    String tmpJ = doc["couleurJourJ"];
    String tmpJ1 = doc["couleurJourJ1"];

    couleurJourJ = tmpJ;
    couleurJourJ1 = tmpJ1;

    Serial.println(couleurJourJ + " / " + couleurJourJ1);
  } else {
    couleurJourJ = "ERROR";
    couleurJourJ1 = "ERROR";
    Serial.println("Http error :" + httpCode);
  }
  httpClientTempoColor.end();
}

void checkTempo() {
  clearLed();
  checkWifi();
  getDate();
  getTempoColor();
  getTempoDays();
  updateLed();
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

void handleRefresh() {
  checkTempo();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleRoot() {
  String page = "<!DOCTYPE html>";
  page += "<head>";
  page += "    <title>Serveur Tempo</title>";
  page += "    <meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8'/>";
  page += "</head>";
  page += "<body lang='fr'>";
  page += "    <h1>Info Tempo</h1>";
  page += "    <p>Aujourd'hui : " + couleurJourJ + "</p>";
  page += "    <p>Demain : " + couleurJourJ1 + "</p>";
  page += "    <p>Rouge restant: " + String(daysRed) + "</p>";
  page += "    <p>Blanc restant: " + String(daysWhite) + "</p>";
  page += "    <p>Bleu restant: " + String(dayBlue) + "</p>";
    page += "    <a href='/refresh'>REFRESH</a>";
  page += "</body>";
  page += "</html>";

  server.setContentLength(page.length()); 
  server.send(200, "text/html", page);
}

void initWebServer() {
  server.on("/", handleRoot);
  server.on("/refresh", handleRefresh);
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
  {
    delay(20);
  }

  initLed();
  clearLed();
  initWifi();
  initWebServer();
  checkTempo();
}

void loop() {

  if (millis() - lastRefresh > REFRESH_TIME_IN_MILLI) {
    lastRefresh = millis();
    checkTempo();
  }

  if (millis() - lastBlink > BLINK_TIME_IN_MILLI) {
    lastBlink = millis();
    blinkLed();
  }

  server.handleClient();
}