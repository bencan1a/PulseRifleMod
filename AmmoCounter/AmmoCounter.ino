#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

// Create an instance of the Adafruit AlphaNum4 display
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

// Define constants and variables
const int MAX_AMMO = 95;  // Maximum ammo count
const int RND_AMT = 1;  //how many shots per trigger pull
int counter = MAX_AMMO;  // Current ammo count
const int triggerPin = 3;  // Pin for trigger button (normally open)
const int reloadPin = 2;  // Pin for reload button (normally closed)
const int rstPin = 4;  // Pin for soundboard reset
const int txPin = 5;  // Pin for soundboard TX
const int rxPin = 6;  // Pin for soundboard RX
bool resetTriggered = false;  // Flag to indicate if reset has been triggered
bool debugEnabled = true;  // Flag to enable or disable debug output

// Create a software serial connection to communicate with the soundboard
// Create an instance of the Adafruit Soundboard
SoftwareSerial ss = SoftwareSerial(txPin, rxPin);

// pass the software serial to Adafruit_soundboard, the second
// argument is the debug port (not used really) and the third 
// arg is the reset pin
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, rstPin);


void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  Serial.println("Init Ammo Counter");

  ss.begin(9600);
  if (!sfx.reset()) {
    Serial.println("Not found");
    while (1);
  }
  Serial.println("SFX board found");

  // Set pin modes for trigger and reload buttons
  pinMode(triggerPin, INPUT_PULLUP);
  pinMode(reloadPin, INPUT_PULLUP);  // Normally closed switch

  // Initialize the LED display
  alpha4.begin(0x70);  // Default I2C address for the LED backpack is 0x70

  // Update the display with the initial counter value
  updateDisplay();

 // Seed the random function to get different results each time (used by playSound)
  randomSeed(analogRead(0));

  if (debugEnabled) Serial.println("Setup complete. Counter initialized to 99.");
}

void loop() {
  static unsigned long lastDebounceTime = 0;  // Last debounce time for reload pin
  static bool lastReloadState = HIGH;  // Last known state of reload pin

  // Read the current state of the reload pin
  int currentReloadState = digitalRead(reloadPin);

  // Debounce logic for reload pin
  if (currentReloadState != lastReloadState) {
    lastDebounceTime = millis();
  }

  // If the reload pin state is stable for more than 50 ms, process the change
  if ((millis() - lastDebounceTime) > 50) { // 50 ms debounce delay
    if (currentReloadState == HIGH && !resetTriggered) {  // Reload pin is opened
      if (debugEnabled) Serial.println("Reload pin opened. Resetting counter to 0.");
      resetCounter();  // Reset counter to 0
      resetTriggered = true;  // Set reset flag to true
    } else if (currentReloadState == LOW && resetTriggered) {  // Reload pin is closed again
      if (debugEnabled) Serial.println("Reload pin closed. Resetting counter to MAX_AMMO.");
      counter = MAX_AMMO;  // Reset counter to maximum ammo
      updateDisplay();  // Update the display
      resetTriggered = false;  // Clear reset flag
    }
  }

  // Update last reload state
  lastReloadState = currentReloadState;

  // If reset has not been triggered, handle trigger pin and serial input
  if (!resetTriggered) {
    // Check for serial input to decrement counter
    if (Serial.available() > 0) {
      char input = Serial.read();
      if (input == '\n' && counter > 1) {
        if (debugEnabled) Serial.println("Serial input received. Decrementing counter.");
        decrementCounter();
      }
    }

    // Check if the trigger button is pressed
    if (digitalRead(triggerPin) == LOW) {
      delay(50); // Debounce delay
      if (debugEnabled) Serial.println("Trigger pin pressed. Starting decrement loop.");
      // Continue decrementing while trigger button is held and reload is not triggered
      while (digitalRead(triggerPin) == LOW && !resetTriggered) {
        decrementCounter();
        delay(250); // Delay to control decrement speed while button is held
      }
    }
  }
}

// Function to decrement the counter
void decrementCounter() {
  if (counter > 1) {
    counter = counter - RND_AMT;
    //counter--;
    if (debugEnabled) {
      Serial.print("Counter decremented to: ");
      Serial.println(counter);
    }
    updateDisplay();  // Update the display with the new counter value
    playSound();  // Play sound on the soundboard
  }
}

// Function to reset the counter to 0
void resetCounter() {
  counter = 0;
  if (debugEnabled) Serial.println("Counter reset to 0.");
  updateDisplay();  // Update the display with the reset value
}

// Function to update the LED display with the current counter value
void updateDisplay() {
  int tens = counter / 10;  // Calculate tens digit
  int ones = counter % 10;  // Calculate ones digit

  // Write the tens and ones digits to the display
  alpha4.writeDigitAscii(0, '0' + tens);  // Write tens place to second block
  alpha4.writeDigitAscii(1, '0' + ones);  // Write ones place to first block
  alpha4.writeDisplay();  // Update the display
  
  if (debugEnabled) {
    Serial.print("Display updated: ");
    Serial.print(tens);
    Serial.println(ones);
  }
}

// Function to play sound on the soundboard
void playSound() {
  uint8_t track = random(0, 5); // Generate a random number between 0 and 4 (5 tracks total)
  
  if (!sfx.playTrack(track)) {
    if (debugEnabled) Serial.print("Failed to play sound on track ");
    Serial.println(track);
  } else {
    if (debugEnabled) Serial.print("Playing sound on track ");
    Serial.println(track);
  }
}
