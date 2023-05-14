#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <base64.h>
#include <ArduinoJson.h>

const char* ssid = "Serenity";
const char* password = "Barracud@2016!";

const char* httpUser = "pi";
const char* httpPassword = "V7phYaUUFYqCpLpFqzLTpA3DEodG";

const int analogPin = A0;
const int ledPin = D2;
unsigned long below700StartTime = 0;
unsigned long above700StartTime = 0;
bool prevState = HIGH;

const char* token = "11418ce6-e4d5-4a86-9f11-fc954c35c8c0";

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // Set D2 to high when the NodeMCU turns on

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Wifi Status: " + String(WiFi.status()) + " (IP: " + WiFi.localIP().toString() + ")");
  }

  //WiFiClient client;

  Serial.println("");
  Serial.println("Wi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void sendHttpRequest(const char* url) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    http.begin(client, url);

    String auth = String(httpUser) + ':' + String(httpPassword);
    String base64Auth = base64::encode(auth);
    http.addHeader("Authorization", "Basic " + base64Auth);

    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("HTTP Response code: " + String(httpCode));
      Serial.println(payload);
    } else {
      Serial.println("Error in HTTP request");
    }

    http.end();
  }
}

void loop() {
  int analogValue = analogRead(analogPin);
  unsigned long currentTime = millis();


  Serial.println(analogValue);

  Serial.println("Token: " + String(token));

  if (analogValue < 700) {
    Serial.println("Sensor is WET");
    if (above700StartTime != 0) {
      above700StartTime = 0;
    }
    if (below700StartTime == 0) {
      below700StartTime = currentTime;
    }
    if (currentTime - below700StartTime >= 10000) {
      if (prevState == HIGH) {
        digitalWrite(ledPin, LOW);
        Serial.println("Turning pump on");
        makeApiCall("http://serenity-tweed.ddns.net:1880/lights", token, 2, 0, "on");
        prevState = LOW;
      }
    }
  } else {
    Serial.println("Sensor is DRY");
    if (below700StartTime != 0) {
      below700StartTime = 0;
    }
    if (above700StartTime == 0) {
      above700StartTime = currentTime;
    }
    if (currentTime - above700StartTime >= 5000) {
      if (prevState == LOW) {
        digitalWrite(ledPin, HIGH);
        Serial.println("Turning pump off");
        makeApiCall("http://serenity-tweed.ddns.net:1880/lights", token, 2, 0, "off");
        prevState = HIGH;
      }
    }
  }

  delay(1000);
}

void makeApiCall(const char* apiUrl, const char* bearerToken, const int dimmer_id, const int channel_number, const char* turn) {
  // Create an HTTPClient object
  HTTPClient http;
  WiFiClient client;

  // Set the authorization header with the Bearer token
  String authHeader = "Bearer ";
  authHeader += String(bearerToken);

  // Create a JSON payload
  const size_t capacity = JSON_OBJECT_SIZE(5);
  DynamicJsonDocument jsonDoc(capacity);
  jsonDoc["dimmer_id"] = dimmer_id;
  jsonDoc["channel_number"] = channel_number;
  jsonDoc["turn"] = turn;
  jsonDoc["brightness"] = 100;
  jsonDoc["transition"] = 2000;

  // Serialize the JSON payload to a string
  String requestBody;
  serializeJson(jsonDoc, requestBody);

  // Send the API request with the JSON payload
  http.begin(client, apiUrl);


  http.addHeader("Authorization", authHeader);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  // Close the connection
  http.end();
}
