#include <SoftwareSerial.h>
#include <Ticker.h>

SoftwareSerial bt(13, 15); //Rx,Tx
Ticker blinker;

#define LED_PIN 5          //GPIO5
#define PIR_PIN 4          //GPIO4
#define MAX_CHAR 15        //Maximum input characters

volatile bool led_state = LOW;
volatile bool checkingTime = false;

// A blinker ISR
//  called by Ticker (software timer)
void blink_led() {
  led_state = !led_state;
  digitalWrite(LED_PIN, led_state);
}

/*******************************************************************************
   An ISR for checking time
    by setting checkingTime flag
    every 500ms.
    500ms = 500,000us
    such that 500,000us * 5(ticks/us)
      = 2,500,000 ticks/us
 *  *Note: 5 ticks/us since using DIV16
 *******************************************************************************/
void ICACHE_RAM_ATTR onTimerISR() {
  checkingTime = true;
  timer1_write(2500000);
}

void pir_isr(void) {
  volatile char pir_state = digitalRead(PIR_PIN);
  Serial.print("Sending: ");
  bt.write('/');
  if (pir_state == HIGH){
    bt.write("PIR ON");
    Serial.println("PIR ON");
  }
  else{
    bt.write("PIR OFF");
    Serial.println("PIR OFF");
  }
  bt.write('#');

}

/*******************************************************************************
    Getting Bluetooth Input
      when bt.available (or when there is incoming data from bluetooth)
      checking first character. if it is '+', then continue receiving data.
      otherwise, ignore the rest of data.
 *******************************************************************************/
bool gettingBTInput(char s[]) {
  bool completeData = false;
  char ch = bt.read();    // get the first char
  if (ch == '+') {        // if '+', continue. stop, otherwise.
    int i = 0;
    noInterrupts();       // enter critical timing

    // Collecting data from bt's input
    //  begin with '+' and end with '#'
    //  At the end, print out confirmation
    //  Limit 15 characters(MAX_CHAR)
    while (bt.available() > 0) {
      ch = bt.read();
      if (ch == '#') {
        if (i == 0) {
          Serial.println("Received Acknowledge");
          completeData = false;
        }
        else {
          Serial.print("Received Data: ");
          completeData = true;
        }
        break;
      }
      s[i++] = ch;

      // **** This condition hasn't been met yet
      // since received data are validated on the other side
      // by using the same MAX_CHAR to limit the characters
      // to transmit.
      if (i >= MAX_CHAR) {
        Serial.println("Invalid Input");
        completeData = false;
        break;
      }

    } // end while()
    interrupts();       // exit critical timing

  } // end ch == '+'

  return completeData;
}

/*******************************************************************************
   Setting Function
    - Set up Serial Terminal with 9600 baud rate
    - Set up BT Serial with 38400 baud rate
    - Set up two timers
      - One is blinker by utilizing Ticker OOP
      - Two is NodeMCU's on-board timer1 (because timer0 is taken for Wi-Fi)
 *******************************************************************************/
void setup() {
  // Setting Serial terminal
  //  with 9600 baud rate
  //  and wait until it's complete initiated.
  Serial.begin(9600);
  while (!Serial);
  Serial.println("NodeMCU Serial Port is ready.");

  // Setting Bluetooth terminal
  //  with 38400 baud rate.
  bt.begin(38400);

  // Setting LED_PIN as output
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), pir_isr, CHANGE);

  // Setting Timers
  //  Ticker timer is for blinker calling blink_led every 2 seconds
  //  Timer1 is for Bluetooth communication
  //    calling onTimerISR every 500ms (2500000 ticks)
  //  **more detail in Arduino.h**
  noInterrupts();
  blinker.attach(2, blink_led);
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(2500000);
  interrupts();

  Serial.println("System is ready.");
}

/*******************************************************************************
   Infinite Loop
    - Update system every 500ms (2Hz)
      - Checking BT input
      - Process the input
      - Update a flag.
 *******************************************************************************/
void loop() {

  // Only get input every 500 ms
  //  checkingTime is update by onTimerISR set in setup()
  if (checkingTime) {
    char gotData = false;                   // Flag to check if received data
    char BTInput[MAX_CHAR] = "";            // An empty character array

    // There is an incoming input
    //  Collect data into BTInput
    if (bt.available()) {
      gotData = gettingBTInput(BTInput);
    }

    // Acknowledge the transmission
    //  by sending '/' back to master Bluetooth
    //  and then working on the input
    if (gotData) {
      Serial.println(BTInput);
      bt.write('/');

      if (strcmp(BTInput, "LED OFF") == 0) {
        digitalWrite(LED_PIN, LOW);
        blinker.detach();
        Serial.println("LED should be off");
      }
      else if (strcmp(BTInput, "LED ON") == 0) {
        digitalWrite(LED_PIN, HIGH);
        blinker.attach(2, blink_led);
        Serial.println("LED should be on");
      }

      bt.write('#');
    }

    checkingTime = false;
  }

}
