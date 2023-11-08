#include <MFRC522.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN 2
#define SS_PIN 15

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522::StatusCode status;

byte firstHash[18];
byte secondHash[18];
byte hash[34];
byte firstHashSize = sizeof(firstHash);
byte secondHashSize = sizeof(secondHash);

String currentText = "Scanning new card...";

void newLine(String value) {
  Serial.println(value);
}

void setup() {
  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();
  lcd.clear();
  lcd.init();
  lcd.backlight();

  for (int keyCount = 0; keyCount < MFRC522::MF_KEY_SIZE; keyCount++) {
    key.keyByte[keyCount] = 0xFF;
  }

  lcd.print("Scan card...");
}

void changeLcdText(String value) {
  if (currentText != value) {
    lcd.clear();
    lcd.print(currentText);
    currentText = value;
  }

}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) {
    changeLcdText("Scanning new card...");
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    changeLcdText("No card found.");
    return;
  }

  byte firstHash[18];
  byte secondHash[18];
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(rfid.uid));

  if (status == MFRC522::STATUS_OK) {
    status = rfid.MIFARE_Read(4, firstHash, &firstHashSize);

    

    if (status == MFRC522::STATUS_OK) {
      // Proceed getting the second hash.
      for (int i = 0; i < sizeof(firstHash) - 2; i++) {
        Serial.print(firstHash[i]);
      }
      status = rfid.MIFARE_Read(5, secondHash, &secondHashSize);
      if (status == MFRC522::STATUS_OK) {
        changeLcdText("Block 5 Read Success");
        // If success, concatenate firstHash and secondHash to hash variable.
        for (int character = 0; character < sizeof(firstHash) - 2; character++) {
          hash[character] = firstHash[character];
        }

        newLine("");

        for (int character = 0; character < sizeof(secondHash) - 2; character++) {
          hash[(sizeof(hash) / 2) - 1 + character] = secondHash[character];
        }

        // Debug, print hash value
        // for (int character = 0; character < sizeof(hash) - 2; character++) {
        //   Serial.print(hash[character]);
        // }
      } else {
        changeLcdText("Block 5 Read Failed");
      }
    } else {
      changeLcdText("Status: " + String(rfid.GetStatusCodeName(status)));
      Serial.println(rfid.GetStatusCodeName(status));
    }
  } else {
    changeLcdText("Invalid auth");
  }
  
  // changeLcdText("Card scanned!");
  newLine(rfid.GetStatusCodeName(status));


  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

String concatenateHashes(String val1, String val2) {
  return "";
}

String byteArrayToString(byte *array, int size)
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