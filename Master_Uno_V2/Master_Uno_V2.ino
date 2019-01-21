#include "SoftwareSerial.h"     // Library for using multiple Serial
#include "SimpleTimer.h"
#include "PCD8544.h"

#define MAX_CHAR 15
#define PIR_LED 8
#define IR_LED A0
#define LCD_LED A1
#define IR_IN_RANGE 100

SoftwareSerial masterBT_1(12, 13); // Master_1 RX & TX Pins
//SoftwareSerial masterBT_2(8,9); // Master_2 RX & TX Pins

SimpleTimer timer;
SimpleTimer lcd_timer;
SimpleTimer irTimer;

PCD8544 lcd;

volatile bool PIR_state = false;

int timeId = 0;
int lcdTimeId = 0;
int irTimeId = 0;

void isr_500ms() {
  char userInput[MAX_CHAR] = "";
  char BlueToothInput[MAX_CHAR] = "";
  bool isSerialInput = checkSerialInput(userInput);
  bool isBlueToothInput = checkBlueToothInput(BlueToothInput);

  if (isSerialInput) {
    sendBlueToothData(userInput);
    isSerialInput = false;
  }

  if (isBlueToothInput) {
    processBlueToothData(BlueToothInput);
    isBlueToothInput = false;
  }
}

void isr_200ms() {
  displayLCD();
}

void isr_50ms() {
  volatile int IR_value = 0;
  static int count = 0;
  
  IR_value = analogRead(IR_LED);
  
  if(IR_value > IR_IN_RANGE){
    digitalWrite(LCD_LED, HIGH);
    count = 99;
  }
  else if(count > 0){
    count--;
  }
  else if(count == 0){
    digitalWrite(LCD_LED, LOW);
    count = -1;
  }
}

/**********************************************************************************************
    checkSerialInput
 **********************************************************************************************/
bool checkSerialInput(char s[]) {
  bool completeData = false;

  if (Serial.available()) {
    char ch = Serial.read();

    if (ch == '+') {
      int i = 0;

      // Collecting data from User's input
      //   began with '+' and end with '#'
      //   At the end print out confirmation
      //   Limit 15 characters(MAX_CHAR)
      timer.disable(timeId);  // enter critical timimg
      while (Serial.available()) {
        ch = (char)Serial.read(); // collect input

        if (ch == '#') {          // check if it's the end of command
          Serial.print("Sending data: ");
          completeData = true;
          break;                  // finish collecting input
        }

        s[i++] = ch;              // append into the string

        // **** This condition hasn't been met yet
        // since received data are validated on the other side
        // by using the same MAX_CHAR to limit the characters
        // to transmit.
        if (i == MAX_CHAR) {      // check if it's out of bound
          Serial.print("Input is more than ");
          Serial.print(MAX_CHAR);
          Serial.println(" characters");
          completeData = false;
          break;
        }

      } // end while()
      timer.enable(timeId);       // exit critical timing
    } // end ch == '+'
    //    else if (ch == '-') {
    //      // On development process
    //    }
  } // end if Serial.available()

  return completeData;
}

/**********************************************************************************************
    checkBlueToothInput
 **********************************************************************************************/
bool checkBlueToothInput(char s[]) {
  bool completeData = false;

  if (masterBT_1.available()) {
    char ch = masterBT_1.read();

    if (ch == '/') {
      int i = 0;

      // Collecting data from User's input
      //   begin with '/' and end with '#'
      //   At the end print out confirmation
      //   Limit 15 characters(MAX_CHAR)
      timer.disable(timeId);  // enter critical timimg
      while (masterBT_1.available()) {
        ch = (char)masterBT_1.read(); // collect input

        if (ch == '#') {          // check if it's the end of command
          if (i == 0) {
            Serial.println("Received Acknowledgement");
            completeData = false;
          }
          else {
            Serial.print("Received data: ");
            completeData = true;
          }
          break;                  // finish collecting input
        }

        s[i++] = ch;              // append into the string

        // **** This condition hasn't been met yet
        // since received data are validated on the other side
        // by using the same MAX_CHAR to limit the characters
        // to transmit.
        if (i == MAX_CHAR) {      // check if it's out of bound
          Serial.print("Input is more than ");
          Serial.print(MAX_CHAR);
          Serial.println(" characters");
          completeData = false;
          break;
        }
      } // end while()
      timer.enable(timeId);       // exit critical timing

    } // end ch == '/'
  } // if master.available()

  return completeData;
}

/**********************************************************************************************
    sendBlueToothData
 **********************************************************************************************/
void sendBlueToothData(char s[]) {
  Serial.println(String(s));
  masterBT_1.write('+');
  masterBT_1.write(s);
  masterBT_1.write('#');
}

/**********************************************************************************************
    processBlueToothData
 **********************************************************************************************/
void processBlueToothData(char s[]) {

  if (strcmp(s, "PIR ON") == 0) {
    digitalWrite(PIR_LED, HIGH);
    PIR_state = true;
  }
  else if (strcmp(s, "PIR OFF") == 0) {
    digitalWrite(PIR_LED, LOW);
    PIR_state = false;
  }
  // Because char s[] doesn't have null terminal at the end
  // therefore String(s) would append '\n'
  Serial.println(String(s));
  masterBT_1.write("+#");

}

/**********************************************************************************************
    displayLCD
 **********************************************************************************************/
void displayLCD() {

  lcd.setCursor(10, 1);
  if (PIR_state)
    lcd.print("PIR:  ON");
  else
    lcd.print("PIR: OFF");
}

/**********************************************************************************************
    Setup
 **********************************************************************************************/
void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Starting System\r\nWaiting for input...");

  masterBT_1.begin(38400);

  // Draw LCD Layout
  lcd.begin(84, 48);
  lcd.setCursor(0, 0);
  lcd.drawColumn(6, 48);
  lcd.setCursor(83, 0);
  lcd.drawColumn(6, 48);
  lcd.setCursor(1, 0);

  // Setup I/O Pins
  pinMode(IR_LED, INPUT);
  pinMode(PIR_LED, OUTPUT);
  pinMode(LCD_LED, OUTPUT);

  // Set Timers
  timeId = timer.setInterval(500, isr_500ms);
  lcdTimeId = lcd_timer.setInterval(200, isr_200ms);
  irTimeId = irTimer.setInterval(50, isr_50ms);
}

/**********************************************************************************************
    loop
 **********************************************************************************************/
void loop() {
  timer.run();
  lcd_timer.run();
  irTimer.run();

}
