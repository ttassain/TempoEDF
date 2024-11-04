#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#define ONBOARD_LED  2

// GPIOs
const int TEMPO_NOW_RED = 12;
const int TEMPO_NOW_GREEN = 13;
const int TEMPO_NOW_BLUE = 14;
const int TEMPO_TOMORROW_RED = 25;
const int TEMPO_TOMORROW_GREEN = 32;
const int TEMPO_TOMORROW_BLUE = 33;

const int REFRESH_TIME_IN_MILLI = 60 * 60 * 1000;
const int BLINK_TIME_IN_MILLI =  500;
unsigned long lastWork = millis();
unsigned long lastBlink = millis();
int blinkUnknowStep = 1;
int blinkErrorStep = 1;
bool watchDog;

// WiFi
const char *ssid = "stargate";
const char *password = "69696969696969696969696969";

String couleurJourJ = "";
String couleurJourJ1 = "";

// EDF
// ID Client et ID Secret en base 64, créées sur le site de RTE avec le bouton "Copier en base 64"
#define identificationRTE   "ZTNmNWFlNGMtOTI3ZS00MT....................................1hOGQyLTM1NTgwZWVjN2FhNA=="

const char * idRTE = "Basic " identificationRTE;

// Certificat racine (format PEM) de https://digital.iservices.rte-france.com
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\n" \
"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\n" \
"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\n" \
"MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\n" \
"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n" \
"hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\n" \
"RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\n" \
"gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\n" \
"KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\n" \
"QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\n" \
"XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\n" \
"DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\n" \
"LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\n" \
"RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\n" \
"jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\n" \
"6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\n" \
"mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\n" \
"Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\n" \
"WD9f\n" \
"-----END CERTIFICATE-----\n";

String errorDescription(int code, HTTPClient& http)
// Liste des codes d'erreurs spécifique à l'API RTE ou message général en clair
{
  switch (code) 
  {
    case 400: return "Erreur dans la requête";
    case 401: return "L'authentification a échouée";
    case 403: return "L’appelant n’est pas habilité à appeler la ressource";
    case 413: return "La taille de la réponse de la requête dépasse 7Mo";
    case 414: return "L’URI transmise par l’appelant dépasse 2048 caractères";
    case 429: return "Le nombre d’appel maximum dans un certain laps de temps est dépassé";
    case 509: return "L‘ensemble des requêtes des clients atteint la limite maximale";
    default: break;
  }
  return http.errorToString(code);
}

HTTPClient httpClientTempoColor, httpClient, httpClientDate;

String currentDate;

WebServer server(80);

void initLed() {
  pinMode(TEMPO_NOW_RED, OUTPUT);
  pinMode(TEMPO_NOW_GREEN, OUTPUT);
  pinMode(TEMPO_NOW_BLUE, OUTPUT);

  pinMode(TEMPO_TOMORROW_RED, OUTPUT);
  pinMode(TEMPO_TOMORROW_GREEN, OUTPUT);
  pinMode(TEMPO_TOMORROW_BLUE, OUTPUT);

  pinMode(ONBOARD_LED, OUTPUT);
}

void clearLed() {
  digitalWrite(TEMPO_NOW_RED, LOW);
  digitalWrite(TEMPO_NOW_GREEN, LOW);
  digitalWrite(TEMPO_NOW_BLUE, LOW);

  digitalWrite(TEMPO_TOMORROW_RED, LOW);
  digitalWrite(TEMPO_TOMORROW_GREEN, LOW);
  digitalWrite(TEMPO_TOMORROW_BLUE, LOW);
}

void errorWifiLed() {
    digitalWrite(TEMPO_NOW_RED, HIGH);
    digitalWrite(TEMPO_TOMORROW_RED, HIGH);
    delay(250);
    digitalWrite(TEMPO_NOW_RED, LOW);
    digitalWrite(TEMPO_TOMORROW_RED, LOW);
    delay(250);
}

void initWifi() {
  // Connexion au Wifi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    errorWifiLed();
    if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_CONNECTION_LOST) {
      WiFi.begin(ssid, password);
    }
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
  if (couleurJourJ == "BLEU") {
    digitalWrite(TEMPO_NOW_BLUE, HIGH);
  } else if (couleurJourJ == "ROUG") {
    digitalWrite(TEMPO_NOW_RED, HIGH);
  } else if (couleurJourJ == "BLAN") {
    digitalWrite(TEMPO_NOW_RED, HIGH);
    digitalWrite(TEMPO_NOW_GREEN, HIGH);
    digitalWrite(TEMPO_NOW_BLUE, HIGH);
  }

    // TOMORROW
  if (couleurJourJ1 == "BLEU") {
    digitalWrite(TEMPO_TOMORROW_BLUE, HIGH);
  } else if (couleurJourJ1 == "ROUG") {
    digitalWrite(TEMPO_TOMORROW_RED, HIGH);
  } else if (couleurJourJ1 == "BLAN") {
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
      digitalWrite(TEMPO_TOMORROW_BLUE, HIGH);
      blinkUnknowStep = 2;
    } else if (blinkUnknowStep == 2) {
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

  if (watchDog) {
  digitalWrite(ONBOARD_LED, HIGH);
  } else {
    digitalWrite(ONBOARD_LED, LOW);
  }
  watchDog = !watchDog;
}

/**
 * @brief Récupère la date du jour sur mon synology, c'est juste un script php :
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

String addDaysToDate(String dateStr, int daysToAdd) {
    // Extraire les composants de la date
    int year = dateStr.substring(0, 4).toInt();
    int month = dateStr.substring(5, 7).toInt();
    int day = dateStr.substring(8, 10).toInt();

    // Ajouter les jours
    day += daysToAdd;

    // Gérer les débordements de jours
    while (true) {
        int daysInMonth;
        if (month == 2) {
            // Année bissextile
            daysInMonth = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
        } else {
            daysInMonth = (month == 4 || month == 6 || month == 9 || month == 11) ? 30 : 31;
        }

        if (day > daysInMonth) {
            day -= daysInMonth;
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
        } else {
            break;
        }
    }

    // Reformater la date en chaîne
    String newDateStr = String(year) + "-" + String(month < 10 ? "0" + String(month) : month) + "-" + String(day < 10 ? "0" + String(day) : day);
    return newDateStr;
}

bool getRTEData() {
  int HTTPcode;
  const char* access_token;
  bool requeteOK = true;
  const char* oauthURI =   "https://digital.iservices.rte-france.com/token/oauth/";

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFI non disponible. Requête impossible");
    return false;
  }
  WiFiClientSecure client;     // on passe en connexion sécurisée (par https...)
  HTTPClient http;
  client.setCACert(root_ca);   // permet la connexion sécurisée en vérifiant le certificat racine

// ************** Première des deux requêtes pour obtenit un token **************
  http.begin(client, oauthURI);

  // Headers nécessaires à la requête
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", idRTE);

  // Send HTTP POST request     
  HTTPcode = http.POST(nullptr,0);     

  if (HTTPcode == HTTP_CODE_OK)
  {
    String oauthPayload = http.getString();
    Serial.println("------------ Contenu renvoyé par la requête 1 : ------------");
    Serial.println(oauthPayload);
    Serial.println("------------------------------------------------------------\n");
    StaticJsonDocument<192> doc;
    DeserializationError error = deserializeJson(doc, oauthPayload);
    if (error)     // cas où il y a un problème dans le contenu renvoyé par la requête
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      access_token = "";
      requeteOK = false;
    }
    else           // cas où le contenu renvoyé par la requête est valide et exploitable
    {
      access_token = doc["access_token"];
    }
  } 
  else 
  {
    Serial.print("erreur HTTP POST: ");
    Serial.println(errorDescription(HTTPcode, http));
    requeteOK = false;
  }
  http.end();
  if (!requeteOK) return false;

  // ***** Deuxième des deux requêtes pour obtenir la couleur des jours, nécessitant le token *****

  // REMARQUES : l'adresse pour la requête est sous la forme :
  // https://digital.iservices.rte-france.com/open_api/tempo_like_supply_contract/v1/tempo_like_calendars?start_date=2015-06-08T00:00:00%2B02:00&end_date=2015-06-11T00:00:00%2B02:00
  // avec (dans notre cas) "start_date" la date du jour et "end_date" la date du jour + 2
  // Après les "%2B" (signe +) on a le décalage horaire par rapport au temps UTC. On doit obligatoirement avoir "02:00" en heures d'été et "01:00" en heure d'hiver
  // Les heures de début et de fin peuvent rester à "T00:00:00" pour la requête mais doivent être présentes !
  // Pour les mois et les jours, les "0" au début peuvent être omis dans le cas de nombres inférieurs à 10

  String requete = "https://digital.iservices.rte-france.com/open_api/tempo_like_supply_contract/v1/tempo_like_calendars";
  // YYYY-MM-DDThh:mm:sszzzzzz1
  
  requete +=  "?start_date=";
  requete += currentDate;
  requete += "T00:00:00+01:00";
  
  requete +=  "&end_date=";
  requete += addDaysToDate(currentDate, 2);
  requete += "T00:00:00+01:00";
  
  
  Serial.println("---------------- Adresse pour la requête 2 : ---------------");
  Serial.println(requete);
  Serial.println("------------------------------------------------------------\n");
  http.begin(client, requete);

    String signalsAutorization = "Bearer "+ String(access_token);
    
    // Headers nécessaires à la requête
    http.addHeader("Authorization", signalsAutorization.c_str());
    http.addHeader("Accept", "application/xml");
    // Mettre la ligne précédente en remarque pour avoir le résultat en json plutôt qu'en xml

    // On envoie la requête HTTP GET
    HTTPcode = http.GET();

    if (HTTPcode == HTTP_CODE_OK) {
      String recup = http.getString();              // "recup" est une chaîne de caractères au format xml
      Serial.println("------------ Contenu renvoyé par la requête 2 : ------------");
      Serial.println(recup);
      Serial.println("------------------------------------------------------------\n");

      // Récupération des couleurs
      int posi = recup.indexOf("<Couleur>",100);    // Recherche de la première occurence de la chaîne "<Couleur>"
                                                    // à partir du 100ème caractère de "recup"
      if (recup.length() > 200)                     // Si la couleur J+1 est connue le String "recup" fait plus de 200 caractères 
      {
        couleurJourJ1 = (recup.substring(posi+9,posi+13)); // Récupération du substring des 4 caractères contenant couleur du lendemain
                                                    // peut être "BLEU", "BLAN" ou "ROUG"
        posi = recup.indexOf("<Couleur>",230);      // Recherche de la deuxième  occurence de la chaîne "<Couleur>"
                                                    // à partir du 230ème caractère de "recup"
        couleurJourJ = (recup.substring(posi+9,posi+13));  // Récupération du substring des 4 caractères contenant la couleur du jour
                                                    // peut être "BLEU", "BLAN" ou "ROUG"
      }
      else                                          // cas où la couleur de J+1 n'est pas encore connue
      {
        couleurJourJ = (recup.substring(posi+9,posi+13));  // Récupération du substring des 4 caractères contenant la couleur du jour
                                                    // peut être "BLEU", "BLAN" ou "ROUG"
        couleurJourJ1 = "NON_DEFINI";
      }
      Serial.print("Couleur Tempo du jour : ");
      Serial.println(couleurJourJ);
      Serial.print("Couleur Tempo de demain : ");
      Serial.println(couleurJourJ1 + "\n");
    } 
    else
    {
      Serial.print("erreur HTTP GET: ");
      Serial.print(HTTPcode);
      Serial.print(" => ");
      Serial.println(errorDescription(HTTPcode, http));
      requeteOK = false;
    }
  http.end();
  return requeteOK;
}

void checkTempo() {
  clearLed();
  checkWifi();
  getDate();
  getRTEData();
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
  while (!Serial) {
    delay(20);
  }

  initLed();
  clearLed();
  initWifi();
  initWebServer();

  checkTempo();
}

void loop() {
  if (millis() - lastWork > REFRESH_TIME_IN_MILLI) {
    lastWork = millis();

    checkTempo();
  }

  if (millis() - lastBlink > BLINK_TIME_IN_MILLI) {
    lastBlink = millis();
    blinkLed();
  }

  server.handleClient();
}