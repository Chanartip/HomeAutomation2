/*******************************************************************************
    Home Automation Project
    Board: Arduino Uno(Atmega328)
    By     Chanartip.Soonthornwan
    Email: Charles.s.aim@gmail.com
 *******************************************************************************/
#include "SoftwareSerial.h"     // Library for using multiple Serial
#include "SimpleTimer.h"        // Library for timers
#include "PCD8544.h"            // Library for Nokia5110 display

// Microcontroller Definition Pins
#define PIR_LED       8         // LED for responding to PIR sensor from slaveBT
#define IR_LED       A0         // IR Receiver LED analog input
#define LCD_LED      A1         // LED backlight on Nokia5110
#define DESK_1_LED   A2         // White LED Strip
#define DESK_2_LED   A3         // Warm White LED Strip
#define DESK_3_LED   A4         // Under desk LED Strip

// Other Definitions
#define MAX_CHAR     15         // Lenght of Serial I/O Buffer
#define IR_IN_RANGE 125         // Calibrated IR Analog Threshold value
#define NO_SOURCE    -1
#define SERIAL_SOURCE 0
#define BT_SOURCE     1
#define TIMEOUT       4

/*******************************************************************************
    SoftwareSerial Instances
    provide multiple ways of communication
    - Serial
    - masterBT_1 communicates to SlaveBT_1 on NodeMCU(another device)
    - slaveBT_2 communicate to HP_Laptop (or another smartdevice)
 *******************************************************************************/
SoftwareSerial masterBT_1(12, 13); // Master_1 RX & TX Pins
SoftwareSerial slaveBT_2(10, 11);  // Master_2 RX & TX Pins

/*******************************************************************************
    Other Instnaces
 *******************************************************************************/
SimpleTimer timer;                // Checking Input Timer
SimpleTimer lcd_timer;            // Updating Nokia5110 Timer
SimpleTimer irTimer;              // Receiving IR sensor value Timer

PCD8544 lcd;                      // Nokia5110 Instance

/*******************************************************************************
    Global Variables
  Note: volatile variable is a variable that could be changed outside the scope
       of programming in the time. In other word, its value could be often
       changed because of hardware in real time.
 *******************************************************************************/
volatile int IR_value = 0;
volatile bool checkingInputTime = false;
volatile bool repeat = false;
volatile bool PIR_state = false;
volatile bool Desk_1_state = false;
volatile bool Desk_2_state = false;
volatile bool Desk_3_state = false;

/*******************************************************************************
    Interrupt Service Routines (ISR)
 *******************************************************************************/
// Checking input time
void isr_500ms() {
  checkingInputTime = true;
  repeat = true;
}

// Updating LCD time
void isr_200ms() {
  displayLCD();
}

// Getting IR value time
//  - constantly receiving input from IR_LED every 50ms.
//    If there is an object in range of the IR sensor,
//    it will turn on the Nokia5110 LED for 100*50ms = 5 seconds
//    before turning off the LED.
void isr_50ms()  {
  static int count = 0;
  IR_value = analogRead(IR_LED);

  if ((IR_value > IR_IN_RANGE) && (count == -1)) {   // When an object is closer
    digitalWrite(LCD_LED, HIGH);
    count = 99;                     // reset the counter value
  }
  else if (count > 0) {             // keep counting down
    count--;
  }
  else if (count == 0) {            // reach 5000ms (5 seconds)
    digitalWrite(LCD_LED, LOW);     // turn off the led
    count = -1;
  }
}

/**********************************************************************************************
    displayLCD
 **********************************************************************************************/
void displayLCD() {

  lcd.setCursor(3, 1);
  if (PIR_state)
    lcd.print("PIR   :  ON");
  else
    lcd.print("PIR   : OFF");

  lcd.setCursor(3, 2);
  if (Desk_1_state)
    lcd.print("Desk_1:  ON");
  else
    lcd.print("Desk_1: OFF");

  lcd.setCursor(3, 3);
  if (Desk_2_state)
    lcd.print("Desk_2:  ON");
  else
    lcd.print("Desk_2: OFF");

  lcd.setCursor(3, 4);
  if (Desk_3_state)
    lcd.print("Desk_3:  ON");
  else
    lcd.print("Desk_3: OFF");

  lcd.setCursor(3, 5);
  lcd.print(IR_value);
  lcd.print("\001");
}

/**********************************************************************************************
    displayCommand
 **********************************************************************************************/
void displayCommand() {
  Serial.println("-------------------------------------------");
//  Serial.println("Communicating through Bluetooth: ");
//  Serial.println("+# checking");
//  Serial.println("+LED ON#");
//  Serial.println("+LED OFF#");
//  Serial.println("\nCommunicating through Serial Terminal: ");
//  Serial.println("-LCD ON#");
//  Serial.println("-LCD OFF#");
//  Serial.println("-DESK_1 ON#");
//  Serial.println("-DESK_1 OFF#");
//  Serial.println("-DESK_2 ON#");
//  Serial.println("-DESK_2 OFF#");
//  Serial.println("-DESK_3 ON#");
//  Serial.println("-DESK_3 OFF#");
//  Serial.println("-DESK ON#");
//  Serial.println("-DESK OFF#");
//  Serial.println("-HELP#");
  Serial.println("-------------------------------------------");
}

/*******************************************************************************
    Serial_RX_Flush
      - to erase remaning data on Serial Buffer by
        keep reading the serial until Serial.available() == 0
 *******************************************************************************/
void Serial_RX_Flush() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

void Bluetooth_RX_Flush() {
  if (masterBT_1.isListening()) {
    while (masterBT_1.available() > 0) {
      masterBT_1.read();                 // Flushing the RX buffer
    }
  }
  else if (slaveBT_2.isListening()) {
    while (slaveBT_2.available() > 0) {
      slaveBT_2.read();                  // Flushing the RX buffer
    }
  }
}

/**********************************************************************************************
    sendBlueToothData
 **********************************************************************************************/
void sendBlueToothData(char s[]) {
  if (masterBT_1.isListening()) {
    masterBT_1.write('+');
    masterBT_1.write(s);
    masterBT_1.write('#');
    masterBT_1.flush();
  }
  else if (slaveBT_2.isListening()) {
    slaveBT_2.write('@');
    slaveBT_2.write('#');
    slaveBT_2.flush();
  }
}
/**********************************************************************************************
    processUserCommand
 **********************************************************************************************/
void processUserCommand(char s[]) {
  if (strcmp(s, "LCD ON") == 0) {
    digitalWrite(LCD_LED, HIGH);
  }
  else if (strcmp(s, "LCD OFF") == 0) {
    digitalWrite(LCD_LED, LOW);
  }
  else if (strcmp(s, "DESK_1 ON") == 0) {
    Desk_1_state = true;
    digitalWrite(DESK_1_LED, HIGH);
  }
  else if (strcmp(s, "DESK_1 OFF") == 0) {
    Desk_1_state = false;
    digitalWrite(DESK_1_LED, LOW);
  }
  else if (strcmp(s, "DESK_2 ON") == 0) {
    Desk_2_state = true;
    digitalWrite(DESK_2_LED, HIGH);
  }
  else if (strcmp(s, "DESK_2 OFF") == 0) {
    Desk_2_state = false;
    digitalWrite(DESK_2_LED, LOW);
  }
  else if (strcmp(s, "DESK_3 ON") == 0) {
    Desk_3_state = true;
    digitalWrite(DESK_3_LED, HIGH);
  }
  else if (strcmp(s, "DESK_3 OFF") == 0) {
    Desk_3_state = false;
    digitalWrite(DESK_3_LED, LOW);
  }
  else if (strcmp(s, "DESK ON") == 0) {
    Desk_1_state = true;
    Desk_2_state = true;
    Desk_3_state = true;
    digitalWrite(DESK_1_LED, HIGH);
    digitalWrite(DESK_2_LED, HIGH);
    digitalWrite(DESK_3_LED, HIGH);
  }
  else if (strcmp(s, "DESK OFF") == 0) {
    Desk_1_state = false;
    Desk_2_state = false;
    Desk_3_state = false;
    digitalWrite(DESK_1_LED, LOW);
    digitalWrite(DESK_2_LED, LOW);
    digitalWrite(DESK_3_LED, LOW);
  }
  else if (strcmp(s, "HELP") == 0) {
    displayCommand();
  }
}

/*******************************************************************************
    checkingInputSource
      - Checking if there is a data available from which communication
      - If there isn't incoming data from masterBT_1, swiching turn to slaveBT_2
      and vise versa.
      - Disadvantage is everytime listen() is called, it erases info in
      SoftSerial's buffer.
 *******************************************************************************/
int checkingInputSource() {
  if (Serial.available() > 0) {
    return SERIAL_SOURCE;
  }
  else if (masterBT_1.isListening()) {
    if (masterBT_1.available() > 0) {
      return BT_SOURCE;
    }
    else {
      slaveBT_2.listen();
    }
  }
  else if (slaveBT_2.isListening()) {
    if (slaveBT_2.available() > 0) {
      return BT_SOURCE;
    }
    else {
      masterBT_1.listen();
    }
  }

  return NO_SOURCE;
}

/*******************************************************************************
    gettingUserInput
    - This function could be called when Serial.available() > 0, or there is
    a data in Serial Buffer.
    Input: char s[] - a character string container
    Output: char s[] - data after chopping header and tailer of String
                       data on Serial.
 *******************************************************************************/
int gettingUserInput(char s[]) {
  int source = NO_SOURCE;

  // Checking source of header
  char ch = Serial.read();      // Read the header

  if (ch == '+') {              // Incoming input is for masterBT_1
    source = BT_SOURCE;
    masterBT_1.listen();
  }
  else if (ch == '-') {         // Incoming input is for the microcontroller itself
    source = SERIAL_SOURCE;
  }
  else {                        // Invalid input
    Serial_RX_Flush();          // eliminate the data on the buffer
    return NO_SOURCE;           // exit the function
  }

  /*
    Receiving data from the character after the header until
    the MAX_CHAR -1 character.
    - Characters beyond MAX_CHAR-1 will be discarded.
    - Characters beyond '#' will be discarded since '#' is the tailer.
  */
  for (int i = 0; (i < MAX_CHAR) && (Serial.available() > 0); i++) {
    ch = Serial.read();
    if (ch == '#') {
      Serial_RX_Flush();
    }
    else if ((i == MAX_CHAR - 1) && (ch != '#')) {
      Serial_RX_Flush();
      Serial.println("Invalid input\n Type -HELP# for more information");
      Serial.flush();
    }
    else {
      s[i] = ch;
    }
  }

  Serial.print("Serial Input is: ");
  Serial.println(s);
  Serial.flush();
  return source;
}

/*******************************************************************************
    gettingBluetoothInput
 *******************************************************************************/
bool gettingBluetoothInput(char s[]) {
  if (masterBT_1.isListening()) {

    if (masterBT_1.read() == '/') {           // Find the Header
      if (masterBT_1.peek() == '#') {         // Find the Tailer (Acknowledge)
        Bluetooth_RX_Flush();
        Serial.println("Received Acknowledgement");
        Serial.flush();
        return true;
      }
      else {
        for (int i = 0; (i < MAX_CHAR) && (masterBT_1.available() > 0); i++) {
          char ch = masterBT_1.read();

          if (ch == '#') {
            Bluetooth_RX_Flush();
          }
          else if ((i == MAX_CHAR - 1) && (ch != '#')) {
            Bluetooth_RX_Flush();
            Serial.println("Invalid input\n Type -HELP# for more information");
            Serial.flush();
          }
          else {
            s[i] = ch;
          }
        }
        Serial.print("Received BT_1 Data: ");
        Serial.println(s);
        Serial.flush();
        return false;
      }
    }
    else {
      Bluetooth_RX_Flush();
//      Serial.println("Header from BT_1 is not '/'");
//      Serial.flush();
      return false;
    }
  }
  else if (slaveBT_2.isListening()) {

    if (slaveBT_2.read() == '/') {
      Bluetooth_RX_Flush();
      return false;
    }
    else {
      for (int i = 0; (i < MAX_CHAR) && (slaveBT_2.available() > 0); i++) {
        s[i] = (char)slaveBT_2.read();
      }
      Bluetooth_RX_Flush();
      Serial.print("Received BT_2 Data: ");
      Serial.println(s);
      Serial.flush();
      return false;
    }

  }
}

/**********************************************************************************************
    processBlueToothData
 **********************************************************************************************/
void processBlueTooth_Data(char s[]) {
  if (masterBT_1.isListening()) {
    if (strcmp(s, "PIR ON") == 0) {
      digitalWrite(PIR_LED, HIGH);
      PIR_state = true;
    }
    else if (strcmp(s, "PIR OFF") == 0) {
      digitalWrite(PIR_LED, LOW);
      PIR_state = false;
    }

    masterBT_1.write("+#");
    masterBT_1.flush();

  }
  else if (slaveBT_2.isListening()) {
    if (strcmp(s, "AimAim") == 0) {
      Serial.println("We got AimAim");
      Serial.flush();
    }

    slaveBT_2.write("@#");
    slaveBT_2.flush();

  }
}

/**********************************************************************************************
    Setup
 **********************************************************************************************/
void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Starting System...");
  displayCommand();

  masterBT_1.begin(38400);
  slaveBT_2.begin(9600);
  masterBT_1.listen();

  // Draw LCD Layout
  lcd.begin(84, 48);
  lcd.setCursor(0, 0);
  lcd.drawColumn(6, 48);
  lcd.setCursor(83, 0);
  lcd.drawColumn(6, 48);

  // Setup I/O Pins
  pinMode(IR_LED, INPUT);
  pinMode(PIR_LED, OUTPUT);
  pinMode(LCD_LED, OUTPUT);
  pinMode(DESK_1_LED, OUTPUT);
  pinMode(DESK_2_LED, OUTPUT);
  pinMode(DESK_3_LED, OUTPUT);

  // Set Timers
  timer.setInterval(500, isr_500ms);
  lcd_timer.setInterval(200, isr_200ms);
  irTimer.setInterval(50, isr_50ms);
}

/**********************************************************************************************
    loop
 **********************************************************************************************/
void loop() {
  timer.run();
  lcd_timer.run();
  irTimer.run();

  if (checkingInputTime) {
    char userInput[MAX_CHAR] = "";
    char BlueToothInput[MAX_CHAR] = "";
    int inputFrom = checkingInputSource();

    switch (inputFrom) {
      case SERIAL_SOURCE: {
          int cmd = gettingUserInput(userInput);

          if (cmd == SERIAL_SOURCE) {
            processUserCommand(userInput);
          }
          else if (cmd == BT_SOURCE) {
            bool completeTX = false;
            unsigned currentmillis = millis();
            while (!completeTX) {
              if(millis() - currentmillis >= 1000){
                Serial.println("timeout");
                break;
              }
              sendBlueToothData(userInput);
              completeTX = gettingBluetoothInput(BlueToothInput);
            }
          }

          break;
        }
      case BT_SOURCE: {
          gettingBluetoothInput(BlueToothInput);
          processBlueTooth_Data(BlueToothInput);
          break;
        }
      default: {
          Serial.print(".");
          break;
        }
    }

    checkingInputTime = false;
  }
}
