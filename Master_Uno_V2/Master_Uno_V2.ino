#include "SoftwareSerial.h"     // Library for using multiple Serial
#include "SimpleTimer.h"
#include "PCD8544.h"

#define PIR_LED 8
#define IR_LED A0
#define LCD_LED A1
#define DESK_1_LED A2
#define DESK_2_LED A3
#define DESK_3_LED A4

#define MAX_CHAR 15
#define IR_IN_RANGE 100
#define SERIAL_DATA 1
#define BT_DATA 2

SoftwareSerial masterBT_1(12, 13); // Master_1 RX & TX Pins
SoftwareSerial masterBT_2(10, 11); // Master_2 RX & TX Pins

SimpleTimer timer;
SimpleTimer lcd_timer;
SimpleTimer irTimer;

PCD8544 lcd;

volatile bool PIR_state = false;
volatile bool completeTX = false;
int timeId = 0;
int lcdTimeId = 0;
int irTimeId = 0;

void isr_500ms() {
  char userInput[MAX_CHAR] = "";
  char BlueToothInput[MAX_CHAR] = "";
  int isSerialInput = checkSerialInput(userInput);
  bool isBlueToothInput = checkBlueToothInput(BlueToothInput);

  if (isSerialInput == SERIAL_DATA) {
    processUserCommand(userInput);
  }
  else if (isSerialInput == BT_DATA) {
    sendBlueToothData(userInput);
  }

  if (isBlueToothInput) {
    processBlueToothData(BlueToothInput);
  }
}

void isr_200ms() {
  displayLCD();
}

void isr_50ms() {
  volatile int IR_value = 0;
  static int count = 0;

  IR_value = analogRead(IR_LED);

  if (IR_value > IR_IN_RANGE) {
    digitalWrite(LCD_LED, HIGH);
    count = 99;
  }
  else if (count > 0) {
    count--;
  }
  else if (count == 0) {
    digitalWrite(LCD_LED, LOW);
    count = -1;
  }
}

/**********************************************************************************************
    checkSerialInput
 **********************************************************************************************/
int checkSerialInput(char s[]) {
  int source = -1;

  if (Serial.peek() > 0) {
    char ch = Serial.read();

    if (ch == '+') source = BT_DATA;
    else if (ch == '-') source = SERIAL_DATA;

    if ((ch == '+') || (ch == '-')) {
      int i = 0;

      // Collecting data from User's input
      //   began with '+' and end with '#'
      //   At the end print out confirmation
      //   Limit 15 characters(MAX_CHAR)
      noInterrupts();             // enter critical timimg
      while (Serial.available()) {
        ch = (char)Serial.read(); // collect input

        if (ch == '#') {          // check if it's the end of command
          if (source == SERIAL_DATA) {
            Serial.print("User's command: ");
            Serial.println(s);
          }
          else if (source = BT_DATA) {
            Serial.print("Sending data: ");
            Serial.println(s);
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
          break;
        }
      } // end while()
      interrupts();               // exit critical timing
    } // end ch == '+' || ch == '-'
  } // end if Serial.available()

  return source;
}

/**********************************************************************************************
    checkBlueToothInput
 **********************************************************************************************/
bool checkBlueToothInput(char s[]) {
  bool completeData = false;

  if (masterBT_1.isListening()) {
    if (masterBT_1.read() == '/') {

      // Collecting data from User's input
      //   begin with '/' and end with '#'
      //   At the end print out confirmation
      //   Limit 15 characters(MAX_CHAR)
      if (masterBT_1.peek() == '#') {
        Serial.println("Received Acknowledgement");
        completeTX = true;
        completeData = false;
      }
      else {
        noInterrupts();             // enter critical timimg
        for (int i = 0; (i < MAX_CHAR) && (masterBT_1.available() > 0) && (masterBT_1.peek() != '#'); i++) {
          s[i] = (char)masterBT_1.read();
        }
        interrupts();               // exit critical timing
        Serial.print("Received Data: ");
        Serial.println(s);
        completeData = true;
      }
    } // end ch == '/'
    
    masterBT_2.listen();
    return completeData;
  }
  else if (masterBT_2.isListening()) {
    if (masterBT_2.peek() != -1) {
      noInterrupts();
      while (masterBT_2.available() > 0) {
        Serial.write(masterBT_2.read());
      }
      interrupts();
    }
    else {
      Serial.write('.');
    }
    masterBT_1.listen();
    return false;
  }
}

/**********************************************************************************************
    sendBlueToothData
 **********************************************************************************************/
void sendBlueToothData(char s[]) {
  masterBT_1.write('+');
  masterBT_1.write(s);
  masterBT_1.write('#');
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
    digitalWrite(DESK_1_LED, HIGH);
  }
  else if (strcmp(s, "DESK_1 OFF") == 0) {
    digitalWrite(DESK_1_LED, LOW);
  }
  else if (strcmp(s, "DESK_2 ON") == 0) {
    digitalWrite(DESK_2_LED, HIGH);
  }
  else if (strcmp(s, "DESK_2 OFF") == 0) {
    digitalWrite(DESK_2_LED, LOW);
  }
  else if (strcmp(s, "DESK_3 ON") == 0) {
    digitalWrite(DESK_3_LED, HIGH);
  }
  else if (strcmp(s, "DESK_3 OFF") == 0) {
    digitalWrite(DESK_3_LED, LOW);
  }
  else if (strcmp(s, "HELP") == 0) {
    displayCommand();
  }
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

void displayCommand() {
  Serial.println("-------------------------------------------");
  Serial.println("Communicating through Bluetooth: ");
  Serial.println("+# checking");
  Serial.println("+LED ON# turn on the led on NodeMCU");
  Serial.println("+LED OFF# turn off the led on NodeMCU");
  Serial.println("\nCommunicating through Serial Terminal: ");
  Serial.println("-LCD ON# turn on LCD background light");
  Serial.println("-LCD OFF# turn off LCD background light");
  Serial.println("-DESK_1 ON# turn on white led strip");
  Serial.println("-DESK_1 OFF# turn off white led strip");
  Serial.println("-DESK_2 ON# turn on warm led strip");
  Serial.println("-DESK_2 OFF# turn off warm led strip");
  Serial.println("-DESK_3 ON# turn on under desk led strip");
  Serial.println("-DESK_3 OFF# turn off under desk led strip");
  Serial.println("-HELP# display command list");
  Serial.println("-------------------------------------------");
}

/**********************************************************************************************
    Setup
 **********************************************************************************************/
void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Starting System\r\nWaiting for input...");
  displayCommand();

  masterBT_1.begin(38400);
  masterBT_2.begin(9600);
  masterBT_1.listen();

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
  pinMode(DESK_1_LED, OUTPUT);
  pinMode(DESK_2_LED, OUTPUT);
  pinMode(DESK_3_LED, OUTPUT);

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
