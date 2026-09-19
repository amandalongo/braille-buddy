#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Define constants
const int numCol = 2;
const int numRow = 3;

const int colPins[numCol] = {D0, D1};
const int rowPins[numRow] = {D2, D3, D4};

// States
bool prevState[numCol][numRow];
bool currState[numCol][numRow];

// Bluetooth setup
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// Make sure board is stable, before app gets the message
char lastScan[7] = "000000";
char lastSent[7] = "000000";
unsigned long lastScanChangeMs = 0;
const unsigned long STABLE_MS = 150;

// Function prototypes
void scanBoard();
void detectStateChange();
void buildBoardString(char *out);
void notifyIfStable();

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Success! App Connected.");
  }
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("App Disconnected. Restarting advertising...");
    pServer->startAdvertising();
  }
};

void setup() {
  Serial.begin(115200);

  // Wait for serial to load
  while (!Serial && millis() < 3000);
  
  delay(1000);

  Serial.println("=================");
  Serial.println("Starting Setup...");

  // Set col pins into output pins
  for (int c = 0; c < numCol; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], LOW);
  }
  Serial.println("Set Column Pins");

  // Set row pins to input pull-down pins
  for (int r = 0; r < numRow; r++) {
    pinMode(rowPins[r], INPUT_PULLDOWN);
  }
  Serial.println("Set Row Pins");

  // Set an Empty Board
  for (int c = 0; c < numCol; c++) {
    for (int r = 0; r < numRow; r++) {
      prevState[c][r] = false;
      currState[c][r] = false;
    }
  }

  // BLE
  BLEDevice::init("BrailleBuddy");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());   // lets the phone subscribe to notifications
  pCharacteristic->setValue((uint8_t *)lastSent, 6);  // a READ always returns the current board
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Setup Completed");
}

void loop() {
  scanBoard();
  detectStateChange();
  notifyIfStable(); // BLE update
  delay(50);
}

void scanBoard() {
  for (int c = 0; c < numCol; c++) {

    // Turn on col
    digitalWrite(colPins[c], HIGH);
    delayMicroseconds(10);

    // Read row
    for (int r = 0; r < numRow; r++) {
      currState[c][r] = (digitalRead(rowPins[r]) == HIGH);
    }

    // Turn off col
    digitalWrite(colPins[c], LOW);
  }
}

/*
void detectStateChange() {
  // Only check column 0 and row 0
  int c = 0;
  int r = 0;

  // Check if square (0,0) changed
  if (currState[c][r] != prevState[c][r]) {


    if (currState[c][r] == true) {
      Serial.println("PLACED");
    } else {
      Serial.println("REMOVED");
    }


    // Update state for (0,0)
    prevState[c][r] = currState[c][r];
  }
}
*/


void detectStateChange() {
  for (int c = 0; c < numCol; c++) {
    for (int r = 0; r < numRow; r++) {

      // Check if something is different
      if (currState[c][r] != prevState[c][r]) {
        
        char buffer[60];

        if (currState[c][r] == true) {
          sprintf(buffer, "Piece Placed on Col: %d | Row: %d", c, r);
        } else {
          sprintf(buffer, "Piece Removed on Col: %d | Row: %d", c, r);
        }

        Serial.println(buffer);

        // Update state
        prevState[c][r] = currState[c][r];
      }

    }
  }
}


// Turns currState into "d1 d2 d3 d4 d5 d6" as '0'/'1' characters.
void buildBoardString(char *out) {
  for (int c = 0; c < numCol; c++) {
    for (int r = 0; r < numRow; r++) {
      out[c * numRow + r] = currState[c][r] ? '1' : '0';
    }
  }
  out[6] = '\0';
}

void notifyIfStable() {
  char now[7];
  buildBoardString(now);

  // Pattern changed since last scan: restart the stability timer.
  if (strcmp(now, lastScan) != 0) {
    strcpy(lastScan, now);
    lastScanChangeMs = millis();
    return;
  }

  // Stable long enough and different from what the app last saw: send it.
  if (millis() - lastScanChangeMs >= STABLE_MS && strcmp(now, lastSent) != 0) {
    strcpy(lastSent, now);
    pCharacteristic->setValue((uint8_t *)now, 6);   //always update, so READ works too
    if (deviceConnected) {
      pCharacteristic->notify();
    }
    Serial.print("Board -> ");
    Serial.println(now);
  }
  // test to see if I can push
}