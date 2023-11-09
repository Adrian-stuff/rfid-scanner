#include <MFRC522.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoWebsockets.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define RST_PIN 2
#define SS_PIN 15
#define ssid "DESKTOP-ADRIAN"
#define pass "connectkana"
#define BUTTON_PIN 16

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522::StatusCode status;
websockets::WebsocketsClient wsClient;

byte firstHash[18];
byte secondHash[18];
byte hash[34];
byte firstHashSize = sizeof(firstHash);
byte secondHashSize = sizeof(secondHash);

int lastMode = LOW;
int buttonMode;

String currentText = "Waiting card...";
String mode = "in";

DynamicJsonDocument response(1024);

String concatenateBytes(byte *value, byte size) {
  return hexToText(byteArrayToString(value, size));
}

String byteArrayToString(byte *array, byte size)
{
    String str = "";
    for (int i = 0; i < size; i++)
    {
        str += String(array[i], HEX);
    }

    return str;
}

String hexToText(const String &hexString)
{
  String result = "";
  int strLen = hexString.length();
  for (int i = 0; i < strLen; i += 2)
  {
      String hexByte = hexString.substring(i, i + 2);
      char charByte = (char)strtol(hexByte.c_str(), NULL, 16);
      result += charByte;
  }
  
  return result;
}

void ws_message_callback(websockets::WebsocketsMessage message) {
  Serial.print("Got Message: ");
  Serial.println(message.data());
}

void setup() {
  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();
  lcd.clear();
  lcd.init();
  lcd.backlight();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while ((WiFi.status() != WL_CONNECTED)) {
    changeLcdText("Connecting...");
    delay(150);
  }

  if ((WiFi.status() == WL_CONNECTED)) {
    changeLcdText("Connected!");
    Serial.println(WiFi.localIP());
  }

  for (int keyCount = 0; keyCount < MFRC522::MF_KEY_SIZE; keyCount++) {
    key.keyByte[keyCount] = 0xFF;
  }

  wsClient.addHeader("Authorization", "Basic ZXNwMzI6MTIzNA==");
  wsClient.connect("ws://roundhouse.proxy.rlwy.net:42552/websocket/scanner/in");
  wsClient.setInsecure();
  wsClient.onMessage(ws_message_callback);

  // pin mode
  pinMode(BUTTON_PIN, INPUT);
  buttonMode = digitalRead(BUTTON_PIN);
}

void changeLcdText(String value) {
  if (currentText != value) {
    lcd.clear();
    lcd.print(currentText);
    currentText = value;
  }
}


void loop() {
  buttonMode = digitalRead(BUTTON_PIN);

  if (buttonMode == HIGH) {
    if (mode == "in") {
      mode = "out";
      changeLcdText("Check in na");
    } else {
      mode = "in";
      changeLcdText("Check out na");
    }
    delay(100);
    return;
  }

  // Websocket client
  wsClient.poll();
  if (!rfid.PICC_IsNewCardPresent()) {
    changeLcdText("Scanning...");
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    changeLcdText("No card found.");
    return;
  }

  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(rfid.uid));

  if (status == MFRC522::STATUS_OK) {
    // Read the first hash.
    status = rfid.MIFARE_Read(4, firstHash, &firstHashSize);

    if (status == MFRC522::STATUS_OK) {
      // Proceed getting the second hash.
      status = rfid.MIFARE_Read(5, secondHash, &secondHashSize);
      if (status == MFRC522::STATUS_OK) {
        changeLcdText("Read Success");
        // If success, concatenate firstHash and secondHash to hash variable.
        for (int character = 0; character < sizeof(firstHash) - 2; character++) {
          hash[character] = firstHash[character];
        }

        for (int character = 0; character < sizeof(secondHash) - 2; character++) {
          hash[(sizeof(hash) / 2) - 1 + character] = secondHash[character];
        }

        // Debug, print hash value
        // for (int character = 0; character < sizeof(hash) - 2; character++) {
        //   Serial.print(hash[character]);
        // }

        String hashString = concatenateBytes(hash, sizeof(hash) - 2);
        String jsonData = "{\"mode\": \"" + mode + "\", \"hashedLrn\": \"" + hashString + "\"}";
        Serial.println("Sending to websocket: " + hashString);
        wsClient.send(hashString);
      } else {
        changeLcdText("Scan again...");
      }
    } else {
      changeLcdText(String(rfid.GetStatusCodeName(status)));
      Serial.println(rfid.GetStatusCodeName(status));
    }
  } else {
    changeLcdText("Invalid auth");
  }
  
  // changeLcdText("Card scanned!");


  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(500);
}