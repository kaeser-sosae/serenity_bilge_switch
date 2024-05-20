#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <base64.h>
#include <ArduinoJson.h>

const char* ssid = "SSID";
const char* password = "Password";

const char* httpUser = "pi";
const char* httpPassword = "Password";

const int analogPin = A0;
const int ledPin = D2;
unsigned long below700StartTime = 0;
unsigned long above700StartTime = 0;
bool prevState = HIGH;

const char* token = "Token";

void makeApiCall(const char* apiUrl, const char* bearerToken, String requestString = "") {
  // Create an HTTPClient object
  HTTPClient http;
  WiFiClient client;

  // Set the authorization header with the Bearer token
  String authHeader = "Bearer ";
  authHeader += String(bearerToken);

  // Send the API request with the JSON payload
  http.begin(client, apiUrl);

  http.addHeader("Authorization", authHeader);

  if (requestString != ""){ http.addHeader("Content-Type", "application/json"); }

  int httpResponseCode = http.POST(requestString);

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

void loop() {
  int analogValue = analogRead(analogPin);
  unsigned long currentTime = millis();

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
        
        // Create the JSON payload
        const size_t capacity = JSON_OBJECT_SIZE(5);
        DynamicJsonDocument jsonDoc(capacity);
        jsonDoc["bilge_status"] = "on";

        // Serialize the JSON payload to a string
        String requestBody;
        serializeJson(jsonDoc, requestBody);

        // Make the API call
        makeApiCall("http://192.168.1.161:1880/bilgeswitch", token, requestBody);

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
        
        // Create the JSON payload
        const size_t capacity = JSON_OBJECT_SIZE(5);
        DynamicJsonDocument jsonDoc(capacity);
        jsonDoc["bilge_status"] = "off";

        // Serialize the JSON payload to a string
        String requestBody;
        serializeJson(jsonDoc, requestBody);

        // Make the API call
        makeApiCall("http://192.168.1.161:1880/bilgeswitch", token, requestBody);

        prevState = HIGH;
      }
    }
  }
  delay(1000);
}
