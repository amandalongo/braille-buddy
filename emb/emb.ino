#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Matrix Dimensions
const int col = 2;
const int row = 3;

// Pin Assignments
const int colPins[col] = {D0, D1};
const int rowPins[row] = {D2, D3, D4};
const int btnPin = D7;

// Hardware States
bool currState[col][row];
bool lastButtonState = HIGH; // Active-LOW button default state

// Bluetooth Infrastructure
BLECharacteristic *pBrailleCharacteristic;
bool isAppConnected = false;

// Transmitted 6-dot state string ("d1d2d3d4d5d6")
char brailleCell[7] = "000000";

// Function Declarations
void scanBrailleMatrix();
void serializeCellState(char *brailleCell);
void processSubmitButton();

class BrailleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    isAppConnected = true;
    Serial.println("BLE Success: Mobile App Connected.");
  }
  void onDisconnect(BLEServer *pServer) {
    isAppConnected = false;
    Serial.println("BLE Alert: App Disconnected. Restarting advertising...");
    pServer->startAdvertising();
  }
};

void setup() {
  Serial.begin(115200);

  while (!Serial && millis() < 3000);
  delay(1000);

  Serial.println("=================");
  Serial.println("Starting BrailleBuddy Setup...");

  // Configure Submit Button (Active LOW)
  pinMode(btnPin, INPUT_PULLUP);
  Serial.println("Configured Submit Button Pin");

  // Configure Column Pins (Outputs)
  for (int c = 0; c < col; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], LOW);
  }
  Serial.println("Configured Column Pins");

  // Configure Row Pins (Inputs with Pull-Downs)
  for (int r = 0; r < row; r++) {
    pinMode(rowPins[r], INPUT_PULLDOWN);
  }
  Serial.println("Configured Row Pins");

  // Clear initial matrix state
  for (int c = 0; c < col; c++) {
    for (int r = 0; r < row; r++) {
      currState[c][r] = false;
    }
  }

  // Initialize BLE Stack
  BLEDevice::init("BrailleBuddy");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BrailleServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pBrailleCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  
  pBrailleCharacteristic->addDescriptor(new BLE2902());
  pBrailleCharacteristic->setValue((uint8_t *)brailleCell, 6);
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Setup Completed Successfully");
}

void loop() {
  processSubmitButton();
  delay(20);
}

void processSubmitButton() {
  int currentButtonState = digitalRead(btnPin);

  // Trigger on button press edge (HIGH -> LOW transition)
  if (currentButtonState == LOW && lastButtonState == HIGH) {    
    scanBrailleMatrix();
    serializeCellState(brailleCell);

    pBrailleCharacteristic->setValue((uint8_t *)brailleCell, 6);

    if (isAppConnected) {
      pBrailleCharacteristic->notify();
    }
    
    Serial.print("Submit Triggered -> Transmitted State: ");
    Serial.println(brailleCell);

    delay(50); // Button contact debounce
  }

  lastButtonState = currentButtonState;
}

void scanBrailleMatrix() {
  for (int c = 0; c < col; c++) {
    digitalWrite(colPins[c], HIGH);
    delayMicroseconds(10);

    for (int r = 0; r < row; r++) {
      currState[c][r] = (digitalRead(rowPins[r]) == HIGH);
    }

    digitalWrite(colPins[c], LOW);
  }
}

void serializeCellState(char *brailleCell) {
  for (int c = 0; c < col; c++) {
    for (int r = 0; r < row; r++) {
      brailleCell[c * row + r] = currState[c][r] ? '1' : '0';
    }
  }
  brailleCell[6] = '\0';
}