#include <SoftwareSerial.h>
#include <Ticker.h>

SoftwareSerial bt(13, 15); //Rx,Tx
Ticker blinker;

#define LED_PIN 5          //GPIO5
#define PIR_PIN 4          //GPIO4
#define MAX_CHAR 15        //Maximum input characters
#define MS_500 2500000

volatile bool checkingTime = false;
volatile bool updatePIR = false;
volatile bool led_state = LOW;
volatile bool pir_state = LOW;
/*******************************************************************************
   A blinker ISR
   called by Ticker (software timer)
 *******************************************************************************/
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
    Note: 5 ticks/us since using DIV16
 *******************************************************************************/
void ICACHE_RAM_ATTR onTimerISR() {
  checkingTime = true;
  timer1_write(MS_500);
}

/*******************************************************************************
   pir_isr

 *******************************************************************************/
void pir_isr(void) {
  updatePIR = true;
  pir_state = digitalRead(PIR_PIN);
}

/*******************************************************************************
   Bluetooth_RX_Flush

 *******************************************************************************/
void Bluetooth_RX_Flush() {
  while (bt.available() > 0) {
    bt.read();
  }
}

/*******************************************************************************
   sendBluetoothData

 *******************************************************************************/
void sendBluetoothData(char s[]) {
  bt.write('/');
  bt.write(s);
  bt.write('#');
  bt.flush();
}

/*******************************************************************************
    Getting Bluetooth Input
      when bt.available (or when there is incoming data from bluetooth)
      checking first character. if it is '+', then continue receiving data.
      otherwise, ignore the rest of data.
 *******************************************************************************/
bool gettingBluetoothInput(char s[]) {

  if (bt.read() == '+') {
    if (bt.peek() == '#') {
      Bluetooth_RX_Flush();
      Serial.println("Received Acknowledge");
      Serial.flush();
      return true;
    }
    else {
      for (int i = 0; (i < MAX_CHAR) && (bt.available() > 0 ); i++) {
        char ch = bt.read();

        if (ch == '#') {
          Bluetooth_RX_Flush();
        }
        else if ((i == MAX_CHAR - 1) && (ch != '#')) {
          Bluetooth_RX_Flush();
          Serial.print("Invalid input: ");
          Serial.println(s);
          Serial.flush();
          return false;
        }
        else {
          s[i] = ch;
        }
      }
      Serial.print("Received Data: ");
      Serial.println(s);
      Serial.flush();
      return false;
    }
  }
  else {
    Bluetooth_RX_Flush();
    //    Serial.println("Input is not '+'");
    //    Serial.flush();
    return false;
  }

}

/*******************************************************************************
   processBluetooth_Data

 *******************************************************************************/
void processBluetooth_Data(char s[]) {
  if (strcmp(s, "LED OFF") == 0) {
    digitalWrite(LED_PIN, LOW);
    blinker.detach();
    Serial.println("LED should be off");
    Serial.flush();
  }
  else if (strcmp(s, "LED ON") == 0) {
    digitalWrite(LED_PIN, HIGH);
    blinker.attach(2, blink_led);
    Serial.println("LED should be on");
    Serial.flush();
  }

  bt.write("/#");
  bt.flush();
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
  Serial.begin(115200);
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
  timer1_write(MS_500);
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
    char gotAck = false;                   // Flag to check if received data
    char BlueToothInput[MAX_CHAR] = "";            // An empty character array
    char BlueToothOutput[MAX_CHAR] = "";

    // There is an incoming input
    //  Collect data into BlueToothInput
    if (bt.available() > 0) {
      bool gotAck = gettingBluetoothInput(BlueToothInput);
      if (!gotAck) processBluetooth_Data(BlueToothInput);
    }
    else if (updatePIR) {
      bool completeTX = false;
      unsigned long previousmillis = millis();

      if (pir_state) {
        strcpy(BlueToothOutput, "PIR ON");
      }
      else {
        strcpy(BlueToothOutput, "PIR OFF");
      }
      Serial.println(BlueToothOutput);

      for(int i=0; i<3 && (!completeTX); i++){
        unsigned long prev_ms = millis();
        sendBluetoothData(BlueToothOutput);
        while((millis()-prev_ms < 50) && (bt.available() == 0));
        completeTX = gettingBluetoothInput(BlueToothInput);
      }

      updatePIR = false;
    }
    else {
//      Serial.print(".");
    }

    checkingTime = false;
  }

}
