/********************************************************
 * Project:     LED_SignBoard
 * Created by:  Chanartip Soonthonrwan
 * Email:       Chanartip.Soonthornwan@gmail.com
 * Purpose:     Iluminate light patterns in a hallway
 *               when there is an object moving within
 *               its PIR sensor range
 * 
 ********************************************************/
#include "Tlc5940.h"

// VARIABLE
#define NUM_LEDS  16
#define NUM_FUNC   6

// Pins definitions
const int PIRpin = 2;   // D2 GPIO Digital Input

// Global Variable
byte led_state = LOW;

// Function Prototypes
void Motion_Detect();
void Blink();
void Breathing();
void Inward();
void Knight_Rider();
void L_to_R();
void Star();

/*****************************************************************************
 * Motion_Detect
 *  - an Intterupt Service Routine(ISR) actives when PIR sensor sees
 *  an object movement within range.
 *****************************************************************************/
void Motion_Detect() {

  if(digitalRead(PIRpin) == HIGH)
    led_state = HIGH;
  else
    led_state = LOW;
     
//  // Toggle state at rising and falling edges
//  led_state = !led_state;

  // Display led_state status
  if (led_state)
    Serial.println(">>>>>>> Motion Detected! <<<<<<<");
  else
    Serial.println(">>>>>>> No Motion Detected <<<<<<<");
}

/*****************************************************************************
 * Blinking pattern
 *****************************************************************************/
void Blink() {
  Serial.println("Blink is ON.");
  
  while (led_state) {
    // Turn off all LEDs
    Tlc.clear();
    Tlc.update();
    delay(500);

    // Turn on all LEDs
    Tlc.setAll(4095);
    Tlc.update();
    delay(500);
  }
  
  Serial.println("Blink is OFF.");
}

/*****************************************************************************
 * Breathing pattern
 *****************************************************************************/
void Breathing() {
  Serial.println("Breathing is ON.");

  int direction = 100;
  int dim = 0;

  while(led_state){
    
    Tlc.setAll(dim);
    Tlc.update();
    delay(50);

    dim += direction;     // Update dim value
  
    if (dim >= 4095) {    // Already bright; make a return
      dim = 4095;
      direction *= -1;
    }
    else if (dim <= 0) {  // Already dim; make a return
      dim = 0;
      direction *= -1;
    }  
  }

//  for (int dim = 0; led_state == HIGH; dim += direction) {
//    if (dim >= 4095) {
//      dim = 4095;
//      direction *= -1;
//    }
//    else if (dim <= 0) {
//      dim = 0;
//      direction *= -1;
//    }
//
//    Tlc.setAll(dim);
//    Tlc.update();
//    delay(50);
//  }
  
  Serial.println("Breathing is OFF.");
}

/*****************************************************************************
 * L_to_R pattern
 *****************************************************************************/
void L_to_R() {
  Serial.println("L_to_R is ON.");
  
  int direction = 0;
  int channel = 0;
  int randVal = random(2);    // random pattern for L_to_R or R_to_L

  if (randVal == 1) {
    direction = 1;
    channel = 0;              // Start from Left to Right
    Serial.println("Left to Right direction");
  }
  else {
    direction = -1;
    channel = NUM_LEDS - 1;  // Start from Right to Left
    Serial.println("Right to Left direction");
  }

  while (led_state) {
    // Clear all LED values
    Tlc.clear();

    // Assigning the left LED
    if (direction == 1 && channel != 0) {
      Tlc.set(channel - 1, 1000);
    }
    else if (direction == -1 && channel != NUM_LEDS - 1) {
      Tlc.set(channel - 1, 1000);
    }

    // Assigning the middle LED
    Tlc.set(channel, 4095);

    // Assigning the right LED
    if (direction == 1 && channel != NUM_LEDS - 1) {
      Tlc.set(channel + 1, 1000);
    }
    else if (direction == -1 && channel != 0) {
      Tlc.set(channel + 1, 1000);
    }

    // Update middle LED position
    channel += direction;

    // Make a stop and the boundaries
    if (channel >= NUM_LEDS)
      channel = 0;
    else if (channel <= 0)
      channel = NUM_LEDS - 1;

    Tlc.update();
    delay(50);
  }
  Serial.println("L_to_R is OFF.");
}

/*****************************************************************************
 * Inward pattern
 *****************************************************************************/
void Inward() {
  Serial.println("Inward is ON.");

  int randVal = random(2);
  int direction = 0;
  int channel_L = 0, channel_R = 0;

  // Setting up starting points
  if (randVal == 1) {
    direction = 1;
    channel_L = 0;
    channel_R = NUM_LEDS - 1;
    Serial.println("Inward direction");
  }
  else {
    direction = -1;
    channel_L = (NUM_LEDS - 1) / 2;
    channel_R = NUM_LEDS / 2;
    Serial.println("Outward direction");
  }

  while (led_state) {
    Tlc.clear();

    // Inward direction
    if (direction) {
      if (channel_L != 0)                   // Setting left LED for Left part
        Tlc.set(channel_L - 1, 1000);
      if (channel_L != (NUM_LEDS - 1) / 2)  // Setting right LED for Left part
        Tlc.set(channel_L + 1, 1000);

      if (channel_R != NUM_LEDS / 2)        // Setting left LED for Right part
        Tlc.set(channel_R - 1, 1000);
      if (channel_R != NUM_LEDS - 1)        // Setting right LED for Right part
        Tlc.set(channel_R + 1, 1000);
    }
    // Outward direction
    else { // if(direction == -1)
      if (channel_L != (NUM_LEDS - 1) / 2)  // Setting left LED for Left part
        Tlc.set(channel_L - 1, 1000);
      if (channel_L != 0)                   // Setting right LED for Left part
        Tlc.set(channel_L + 1, 1000);

      if (channel_R != NUM_LEDS / 2)        // Setting left LED for Right part
        Tlc.set(channel_R - 1, 1000);
      if (channel_R != NUM_LEDS - 1)        // Setting right LED for Right part
        Tlc.set(channel_R + 1, 1000);
    }
    
    Tlc.set(channel_L, 4095);               // Setting middle LED
    Tlc.set(channel_R, 4095);               // Setting middle LED

    channel_L += direction;                 // Update left position
    channel_R -= direction;                 // Update right position

    // Checking positions at boundaries
    if ((channel_L >= (NUM_LEDS - 1) / 2) && (channel_R <= NUM_LEDS / 2)) {
      channel_L = 0;
      channel_R = NUM_LEDS - 1;
    }
    else if ((channel_L <= 0) && (channel_R >= NUM_LEDS - 1)) {
      channel_L = (NUM_LEDS - 1) / 2;
      channel_R = NUM_LEDS / 2;
    }

    Tlc.update();
    delay(75);
  }

  Serial.println("Inward is OFF.");
}

/*****************************************************************************
 * Knight pattern
 *****************************************************************************/
void Knight_Rider() {
  Serial.println("Knight Rider is ON.");
  
  int direction = 1;
  for (int channel = 0; (led_state == HIGH) && (channel < NUM_LEDS); channel += direction) {

    /* Tlc.clear() sets all the grayscale values to zero, but does not send
       them to the TLCs.  To actually send the data, call Tlc.update() */
    Tlc.clear();

    /* Tlc.set(channel (0-15), value (0-4095)) sets the grayscale value for
       one channel (15 is OUT15 on the first TLC, if multiple TLCs are daisy-
       chained, then channel = 16 would be OUT0 of the second TLC, etc.).

       value goes from off (0) to always on (4095).

       Like Tlc.clear(), this function only sets up the data, Tlc.update()
       will send the data. */
    if (channel == 0) {
      direction = 1;
    } else {
      Tlc.set(channel - 1, 1000);
    }
    Tlc.set(channel, 4095);
    if (channel != NUM_TLCS * 16 - 1) {
      Tlc.set(channel + 1, 1000);
    } else {
      direction = -1;
    }

    /* Tlc.update() sends the data to the TLCs.  This is when the LEDs will
       actually change. */
    Tlc.update();
    delay(75);
  }
  
  Serial.println("Knight Rider is OFF.");
}

/*****************************************************************************
 * Star pattern
 *****************************************************************************/
void Star() {
  Serial.println("Star is ON.");
  
  int direction = 200;
  int dim = 0;

  // Create a random number in an array
  int num_star = random(1, NUM_LEDS / 4)+1;
  int pin[NUM_LEDS];

  // Setting all element in the array to low
  for (int i = 0; i < NUM_LEDS; i++) {
    pin[i] = 0;
  }
  // Setting the pins to high
  for (int i = 0; i < num_star; i++) {
    int randNum = random(NUM_LEDS);
    while (pin[randNum] == HIGH) {
      randNum = random(NUM_LEDS);
    }
    pin[randNum] = HIGH;
  }

  // Displaying elements in the array.
  for (int i = 0; i < NUM_LEDS; i++) {
    Serial.print(pin[i]);
    if ((i == NUM_LEDS / 2 - 1) || (i == NUM_LEDS - 1))
      Serial.println("");
    else
      Serial.print(" ");
  }

  // Clearing all LEDs
  Tlc.clear();

  while (led_state) {
    for (int index = 0; led_state && (index < NUM_LEDS); index++) {
      if (pin[index] == HIGH) {       // Setting the LED to display
        Tlc.set(index, dim);
      }
      else {
        Tlc.set(index, 0);            // Turning off the rest
      }
    }
    dim += direction;                 // Updating dim

    // Setting boundaries
    if (dim >= 4095) {                // Making a return at the most brightness
      dim = 4095;
      direction *= -1;
    }
    else if (dim <= 0) {              // Making a return at the most dim
      dim = 0;
      direction *= -1;

      // Generating new random LEDs
      num_star = random(NUM_LEDS / 4) +1;
      for (int i = 0; i < NUM_LEDS; i++) {
        pin[i] = 0;
      }
      for (int i = 0; i < num_star; i++) {
        int randNum = random(NUM_LEDS);
        while (pin[randNum] == HIGH) {
          randNum = random(NUM_LEDS);
        }
        pin[randNum] = HIGH;
      }

      // Displaying the array
      for (int i = 0; i < NUM_LEDS; i++) {
        Serial.print(pin[i]);
        if ((i == NUM_LEDS / 2 - 1) || (i == NUM_LEDS - 1))
          Serial.println("");
        else
          Serial.print(" ");
      }
      Tlc.clear();
    }

    Tlc.update();
    delay(50);

  }

  Serial.println("Star is OFF.");
}

/*****************************************************************************
 * Setting up Pins and Register
 *  - Setting Tlc pins out 
 *    - VPRG to GND
 *    - SIN to D11
 *    - SCLK to D13
 *    - XLAT to D9
 *    - BLANK to D10 and 10k ohm pull-up resistor
 *    - GND
 *    - VCC
 *    - IREF to GND with 2k-4k ohm pull-down resistor
 *    - DCPRG to GND
 *    - SOUT
 *    - XERR
 *  - PIRpin to D2 (Interrupt at both rising and falling edge)
 *  - randomSeed
 *****************************************************************************/
void setup() {
  Tlc.init();
  Serial.begin(9600);

  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  noInterrupts();
  pinMode(PIRpin, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIRpin), Motion_Detect, CHANGE);

  while(digitalRead(PIRpin) == HIGH){
    Serial.println("PIR is still HIGH");  
  }
  delay(1000);
  interrupts();
  Serial.println("==== System is ready ====");
}

/*****************************************************************************
 * Infinite Loop
 *  - perform base on led_state
 *      while led_state is on, random a pattern,
 *      when led_state is off, turn off leds.
 *****************************************************************************/
void loop() {
  
  static byte prev_state = LOW;
  
  if (led_state == HIGH) {
        int pattern = random(NUM_FUNC);
        prev_state = led_state;
        switch(pattern){
          case 0: { Knight_Rider(); break;}
          case 1: {        Blink(); break;}
          case 2: {    Breathing(); break;}
          case 3: {       L_to_R(); break;}
          case 4: {       Inward(); break;}
          case 5: {         Star(); break;}
          default:{    Tlc.clear();
                      Tlc.update(); break;
                  }
        }
  }
  else if(led_state == LOW && prev_state == HIGH){  // the fallin edge moment of led_state
    // Turn off all LEDs
    Tlc.clear();
    Tlc.update();
    prev_state = led_state;
  }
  else{
    // nothing happen when led_state is LOW and prev_state is LOW.  
  }
}

