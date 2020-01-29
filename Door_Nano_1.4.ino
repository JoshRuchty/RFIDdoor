

/**************************************************************************/
/*
   Josh notes: 


  Note that you need the baud rate to be 115200 because we need to print
  out the data and read from the card at the same time!

  But also, BT code and tutorial from  https://howtomechatronics.com/tutorials/arduino/arduino-and-hc-05-bluetooth-module-tutorial/
  so leave baud at 9600 for HC05. choose.

  This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
  This library works with the Adafruit NFC breakout
  ----> https://www.adafruit.com/products/364
  https://www.youtube.com/watch?v=rATC1tcn9BM
*/

//==============================================================================
//Libraries & Objects

#include <Servo.h>
Servo myservo;  // create servo object to control a servo
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

//==============================================================================
//Variables

//Constants
#define ledPin 2
#define servospeed 2    // Define replaces the text before uploading, so uses no variable space. 
#define servodelay 10    // seem max ratio of servospeed/servodelay = 1/2
#define intBut A2   // interior lock button
#define timerBut A3
#define servoPin 3
#define FETPin 4
#define Buzzer 13 // Pin 3 connected to + pin of the Buzzer

//Servo & State Variables
int pos = 0;    // variable to store the servo position
bool locked;
bool activateTimer = 0;
//need to write code for a toggle

//BT variables
char BTChar; // to use... BTChar = Serial.read() // which will add one character to
String BTString = "test setup"; // to consolidate Serial.read characters, use 'BTString += BTChar'
//String helloWorld = "Hello 2nd world";
//char extractChar = helloWorld.charAt(6);

//Other Variables
int oldMillis;
byte GoodTagSerialNumber[] = {0xF7, 0xF3, 0xA1, 0x39}; // The Tag Serial number we are looking for (currently white card in wallet)
byte viTag[] = {0x4, 0x1A, 0x11, 0xBB, 0x20, 0x49, 0x80};
int loopCounter = 0;


//==============================================================================
//==============================================================================
//PN 532 original code

#define PN532_IRQ   (6) //i.e. the pin number
#define PN532_RESET (5)  // Not connected by default on the NFC Shield
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
// also change #define in Adafruit_PN532.cpp library file
#define Serial SerialUSB
#endif

//==============================================================================
//Functions
void buzzerPass(bool x) {
  if (x == 1) {
    for (int y = 0; y < 3; y++) {

      digitalWrite (Buzzer, HIGH) ;// Buzzer On
      delay (50) ;// Delay 1ms
      digitalWrite (Buzzer, LOW) ;// Buzzer Off
      delay (50) ;// delay 1ms
    }
  }
  if (x == 0) {
    for (int y = 0; y < 15; y++) {
      digitalWrite (Buzzer, HIGH) ;// Buzzer On
      delay (3) ;// Delay 1ms
      digitalWrite (Buzzer, LOW) ;// Buzzer Off
      delay (8) ;// delay 1ms
    }
  }
}

void unlock() {
  digitalWrite(FETPin, HIGH);
  for (pos = 40; pos <= 150; pos += servospeed) { // goes from 0 degrees to 180 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(servodelay);                       // waits 15ms for the servo to reach the position
  }
  digitalWrite(FETPin, LOW);
}
void lock() {
  digitalWrite(FETPin, HIGH);
  for (pos = 150; pos >= 40; pos -= servospeed) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(servodelay);                       // waits 15ms for the servo to reach the position
  }
  digitalWrite(FETPin, LOW);
}

void servoLock(bool i) {
  if (i == 1) { // LOCK it
    locked = 1;
    lock();
    Serial.println("locked");
    //delay(1000);
  }
  if (i == 0) { // unlock it
    locked = 0;
    unlock();
    Serial.println("UNlocked");
    //delay(1000);
  }
}

void blink(int x) { // x argument is how many 100 ms flashes should happen
  for (int i = 0; i < x; i++) {
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);
    delay(50);
  }
}


void receiveBT() {

  BTString = ""; //clears the BTString var
  while (Serial.available() > 0) {
    BTChar = Serial.read();
    BTString += BTChar; // adds one character at a time to BTString
    delay(10); //this delay exists because the arduino will cycle faster than the serial buffer empties, appearing to empty the buffer when some is there
  }
} //end receiveBT

void BTCommands() { //SEE project_spartan HC05 for string reading examples

  if (BTString == "hello") Serial.println("back atcha");
  if (BTString == "1") servoLock(1), Serial.println("BTstring = 1");
  if (BTString == "0") servoLock(0), Serial.println("BTstring = 0");


  //if(BTString.indexOf("wh") >= 0) Serial.println("Search worked!"); // this is how to search a string!
} // end BTCommands

//==============================================================================
//==============================================================================
//==============================================================================

void setup(void) {

  //==============================================================================
  //==============================================================================
  // PN532

  /*#ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
    #endif */
  //Serial.begin(115200);
  Serial.begin(9600);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(10);
  //nfc.setPassiveActivationRetries(0xFF); //original line provided in example: default wait forever


  // configure board to read RFID tags
  nfc.SAMConfig(); //this lines stops the door duino from working.

  Serial.println("Waiting for an ISO14443A card");

  //==============================================================================
  //==============================================================================
  //Door nano pin setup

  //Serial.begin(9600); // Default communication rate of the Bluetooth module is 38400 (according to this tutorial); conflicts with 115200 for PN532; and only seems to work at 9600
  pinMode(ledPin, OUTPUT);
  pinMode(intBut, INPUT_PULLUP);
  pinMode(timerBut, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT); // Set buzzer pin to an Output pin
  pinMode(FETPin, OUTPUT);
  myservo.attach(servoPin);  // attaches the servo on pin 9 to the servo object
  digitalWrite(FETPin, LOW);
  digitalWrite(ledPin, LOW);
  digitalWrite(Buzzer, LOW); // Buzzer Off at startup
  analogWrite(ledPin, 0); // starts LED off
  servoLock(1);  // locks door on reset
  bool toggle = 1;


  /*Serial.println(BTString);
    int firstS = BTString.indexOf('s');
    Serial.println("index with var: " + BTString + " is " + firstS);
    Serial.println("same line index" + BTString + " is " + BTString.indexOf('s'));
    Serial.println("2 found here");
  */

}

//==============================================================================
//==============================================================================
//==============================================================================

void loop(void) {
  //Serial.println(loopCounter);
  //loopCounter++;

  //==============================================================================
  //PN532 read process

  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };	// Buffer to store the returned UID
  uint8_t uidLength;				// Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success) {
    Serial.println("Found a card!");
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      Serial.print(" 0x"); Serial.print(uid[i], HEX);
    }
  }
  else
  {
    // PN532 probably timed out waiting for a card
    //Serial.println("Timed out waiting for a card");
  }

  //End of example PN532 code

  //==============================================================================
  //check the detected card for a match, exectute commands

  String GoodTag = "False"; // Variable used to confirm good Tag Detected

  // Check if detected Tag has the right Serial number we are looking for
  for (int i = 0; i < 4; i++) {
    if (GoodTagSerialNumber[i] != uid[i]) {
      break; // if not equal, then break out of the "for" loop
    }
    if (i == 3) { // if we made it to 4 loops for each of the array positions, then all 4 Tag Serial numbers sections are matching
      GoodTag = "TRUE";
    }
  }

  for (int i = 0; i < uidLength; i++) {
    if (viTag[i] != uid[i]) {
      break; // if not equal, then break out of the "for" loop
    }
    if (i == uidLength) { // if we made it to 4 loops for each of the array positions, then all 4 Tag Serial numbers sections are matching
      GoodTag = "TRUE";
    }
  }

  if (GoodTag == "TRUE") {
    Serial.println("Success!!!!!!!"); Serial.println();
    buzzerPass(1);
    if (locked == 1) servoLock(0);
    else if (locked == 0) servoLock(1);
  }

  //==============================================================================
  //BUTTONS

  if (digitalRead(timerBut) == LOW) {
    Serial.println("timerbut low");
    servoLock(0);
    buzzerPass(1);
    delay(7000);
    buzzerPass(1);
    delay(600);
    servoLock(1); //when unpressed, state = HIGH; when pressed, state = low = run lock function to on (1)
  }
  if (digitalRead(intBut) == LOW && locked == 0)    {
    servoLock(1);
    Serial.println("intbutlow locked0");
  }
  if (digitalRead(intBut) == LOW && locked == 1) {
    servoLock(0);
    Serial.println("intbutlow locked1");
  }


  if (digitalRead(intBut) == LOW && locked == 0)    {
    servoLock(1);
    Serial.println("intbutlow locked0");
  }
  if (digitalRead(intBut) == LOW && locked == 1) {
    servoLock(0);
    Serial.println("intbutlow locked1");
  }

  //==============================================================================
  //INCOMING DATA
  receiveBT();
  BTCommands();

}
