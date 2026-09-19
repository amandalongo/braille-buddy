// Define constants
const int numCol = 2;
const int numRow = 3;

const int colPins[numCol] = {D0, D1};
const int rowPins[numRow] = {D2, D3, D4};

// States
bool prevState[numCol][numRow];
bool currState[numCol][numRow];

// Function prototypes
void scanBoard();
void detectStateChange();

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

  Serial.println("Setup Completed");
}

void loop() {
  scanBoard();
  detectStateChange();
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
