#include "SoftwareSerial.h"     // Library for using multiple Serial
#include "SimpleTimer.h"
#include "PCD8544.h"

#define MAX_CHAR 15
#define PIR_LED 13
#define LCD_LED 9

SoftwareSerial masterBT_1(10, 11); // Master_1 RX & TX Pins
//SoftwareSerial masterBT_2(8,9); // Master_2 RX & TX Pins

SimpleTimer timer;
SimpleTimer lcd_timer;

PCD8544 lcd;

volatile bool checkingTime = false;
volatile bool displayLCDTime = false;
int timeId = 0;
int lcdTimeId = 0;

volatile bool PIR_state = false;

void isr_500ms() {
  checkingTime = true;
}

void isr_10ms(){
  displayLCDTime = true;
}

bool gettingUserInput(char s[]) {
  bool completeData = false;
  char ch = Serial.read();  // get the first char
  if (ch == '+') {          // if '+', continue. stop, otherwise.
    int i = 0;
    timer.disable(timeId);  // enter critical timimg

    // Collecting data from User's input
    //   began with '+' and end with '#'
    //   At the end print out confirmation
    //   Limit 15 characters(MAX_CHAR)
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

  return completeData;
}

bool gettingBTInput(char s[]) {
  bool completeData = false;
  char ch = masterBT_1.read();
  if (ch == '/') {
    int i = 0;
    timer.disable(timeId);  // enter critical timimg

    // Collecting data from User's input
    //   begin with '/' and end with '#'
    //   At the end print out confirmation
    //   Limit 15 characters(MAX_CHAR)
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
  
  return completeData;
}

void displayLCD(){

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("01234567890123");
  lcd.setCursor(0,3);
  static unsigned int LCD_val = 0;
  static int direction = 1;
  analogWrite(LCD_LED,map(LCD_val,0,1023,0,255));
  
  if(LCD_val >= 1022){
    direction = -1;
  }
  else if(LCD_val <= 1){
    direction = 1;
  }
  LCD_val += direction;
  lcd.print(LCD_val);

  lcd.setCursor(0,4);
  if(PIR_state) lcd.print("PIR is ON");
  else          lcd.print("PIR is OFF");
  
}


void setup() {
  Serial.begin(9600);   // Arduino Uno Serial
  while (!Serial);      // Wait until the Serial is ready.
  Serial.println("Starting System\r\nWaiting for input...");

  masterBT_1.begin(38400);
  lcd.begin(84, 48);

  pinMode(PIR_LED, OUTPUT);
  pinMode(LCD_LED, OUTPUT);
  
  timeId = timer.setInterval(500, isr_500ms);   // call ISR every 500ms
  lcdTimeId = lcd_timer.setInterval(1000, isr_10ms);
}

void loop() {

  timer.run();
  lcd_timer.run();

  if (checkingTime) {
    char receivedInput = false;
    char userInput[MAX_CHAR] = "";
    char BTInput[MAX_CHAR] = "";

    // Getting input from Serial
    if (Serial.available()) {
      receivedInput = gettingUserInput(userInput);
      if (receivedInput) {
        Serial.println(userInput);
        masterBT_1.write('+');
        masterBT_1.write(userInput);
        masterBT_1.write('#');
        receivedInput = false;
      }
    } // end Serial.available()

    // Getting input from Bluetooth
    //  including Acknowledge and data
    if (masterBT_1.available()) {
      receivedInput = gettingBTInput(BTInput);
      if (receivedInput) {
//        if(strcmp(BTInput,"")==0){
//          Serial.println("Received Acknowledgement");
//        }
        if(strcmp(BTInput,"PIR ON")==0){
          digitalWrite(PIR_LED, HIGH);
          PIR_state = true;
        }
        else if(strcmp(BTInput, "PIR OFF")==0){
          digitalWrite(PIR_LED, LOW);
          PIR_state = false;
        }
        Serial.println(BTInput);
        masterBT_1.write("+#");
        receivedInput = false;
      }
    } // end masterBT_1.available()
    
      checkingTime = false;
  } // end checkingTime

  if(displayLCDTime){
    displayLCD();
    displayLCDTime = false;
  }
  
}
