#include "SoftwareSerial.h"     // Library for using multiple Serial
#include "SimpleTimer.h"
#include "PCD8544.h"

#define MAX_CHAR 15
#define PIR_LED 8
#define IR_LED A0
#define LCD_LED A1

SoftwareSerial masterBT_1(12, 13); // Master_1 RX & TX Pins
//SoftwareSerial masterBT_2(8,9); // Master_2 RX & TX Pins

SimpleTimer timer;
SimpleTimer lcd_timer;
SimpleTimer irTimer;

PCD8544 lcd;

volatile bool checkingTime = false;
volatile bool displayLCDTime = false;
volatile bool irTime = false;
volatile bool PIR_state = false;

int timeId = 0;
int lcdTimeId = 0;
int irTimeId = 0;


void isr_500ms() {
  checkingTime = true;
}

void isr_200ms(){
  displayLCDTime = true;
}

void isr_50ms(){
  irTime = true;
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
  else if(ch == '-'){
    int i=0;
    while(Serial.available()){
      ch = (char)Serial.read();
      if(ch == '#'){
        Serial.print("User command: ");
        completeData = true;
        break;
      }
      s[i++] = ch;
      if (i == MAX_CHAR) {      // check if it's out of bound
        Serial.print("Input is more than ");
        Serial.print(MAX_CHAR);
        Serial.println(" characters");
        completeData = false;
        break;
      }
    }
    
  }
  
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

void lcd_isr(){
  static int count = 0;
  static int lcd_pwm_val = 0;
  static int direction = 1;
  if(count == 499){
    count = 0;
    lcd_dimming = false;
  }
  else if(count >= 250){
    direction = -1;
  }
  else{
    direction = 1;
  }

  lcd_pwm_val += direction;

  analogWrite(LCD_LED, lcd_pwm_val);
  
  count++;
}

void displayLCD(){

  lcd.setCursor(10,1);
  if(PIR_state) lcd.print("PIR:  ON");
  else          lcd.print("PIR: OFF");

}


void setup() {
  Serial.begin(9600);   // Arduino Uno Serial
  while (!Serial);      // Wait until the Serial is ready.
  Serial.println("Starting System\r\nWaiting for input...");

  masterBT_1.begin(38400);
  lcd.begin(84, 48);
  lcd.setCursor(0,0);
  lcd.drawColumn(6,48);
  lcd.setCursor(83,0);
  lcd.drawColumn(6,48);

  pinMode(IR_LED, INPUT);
  pinMode(PIR_LED, OUTPUT);
  pinMode(LCD_LED, OUTPUT);
  
  timeId = timer.setInterval(500, isr_500ms);   // call ISR every 500ms
  lcdTimeId = lcd_timer.setInterval(200, isr_200ms);
  irTimeId = irTimer.setInterval(50, isr_50ms);
}

void loop() {

  timer.run();
  lcd_timer.run();
  irTimer.run();
  
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
