/*
 * IRMux - Multiplexes control of 4 IR devices using a custom Arduino shield
 * Version 1.0 January, 2014
 */

#include <IRremote.h>
//#define DEBUG
int RECV_PIN = 11;

void convert50toUS(unsigned int * buf, int len);
#ifdef DEBUG
void printraw(decode_results * results);
#endif
IRrecv irrecv(RECV_PIN);
IRsend irsend;
decode_results results;

#define LED0_PIN 0
#define LED1_PIN 1
#define LED2_PIN 4
#define LED3_PIN 5
int leds[4] = {
  LED0_PIN,
  LED1_PIN,
  LED2_PIN,
  LED3_PIN
};
#define DET0_PIN 6
#define DET1_PIN 7
#define DET2_PIN 8
#define DET3_PIN 10
int detects[4] = {
  DET0_PIN,
  DET1_PIN,
  DET2_PIN,
  DET3_PIN
};
unsigned long control;
int transmit;
#define SEL0_PIN 12
#define SEL1_PIN 13
int selects[2] = {
  SEL0_PIN,
  SEL1_PIN
};
volatile int learn;
#define BUTTON_PIN 2
unsigned long reattachtime;

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
#endif
#ifndef DEBUG
  // Initialize LEDs
  for (int i = 0; i < 4; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }
#endif
  // Initialize plug detects
  for (int i = 0; i < 4; i++) {
    pinMode(detects[i], INPUT_PULLUP);
  }
  // Initialize Mux select lines
  for (int i = 0; i < 2; i++) {
    pinMode(selects[i], OUTPUT);
    digitalWrite(selects[i], LOW);
  }
  irrecv.enableIRIn(); // Start the receiver
  control = 0;
  learn = 0;
  transmit = 0;
  attachInterrupt(0, ButtonInterrupt, RISING);
  reattachtime = 0;
  nextTransmit();
}

void loop() {
  if (irrecv.decode(&results)) {
    if (results.value == control) {
#ifdef DEBUG
      Serial.println("Got control signal");
#endif
      nextTransmit();
    } else if (learn) {
#ifdef DEBUG
      Serial.println("Learning");
#endif
      learnControl(results.value);
      reattachtime = millis() + 1000;
    } else {
      transmitSignal();
      // Required in order to receive the next signal
      irrecv.enableIRIn();
    }
#ifdef DEBUG
    Serial.print("Got signal: ");
    Serial.println(results.value, HEX);
#endif
    irrecv.resume(); // Receive the next value
  }
  if (reattachtime != 0 && millis() >= reattachtime) {
#ifdef DEBUG
    Serial.println("Reattaching Interrupt");
#endif
    // Clear the pending interrupt flag. Otherwise, the ISR will be called as
    // soon as it is reattached.
    EIFR |= 0x1;
    attachInterrupt(0, ButtonInterrupt, RISING);
    reattachtime = 0;
  }
  // This delay time is purely empirical in nature, change however you see fit.
  delay(500);
}

// Comcast values have to be used in the raw state and come in as a number of
// 50us intervals. multiply each number by 50 to get a raw microsecond value
// that can be fed into sendRaw.
void convert50toUS(unsigned int * buf, int len)
{
  for (int i = 1; i < len; i++)
  {
    buf[i] = buf[i] * 50;
  }
}

void transmitSignal() {

  if (transmit >= 0 && transmit <= 3) {
    sendSignal(transmit);
  } else if (transmit == 4) {
    if (!digitalRead(leds[0])) sendSignal(0);
    delay(10);
    if (!digitalRead(leds[1])) sendSignal(1);
    delay(10);
    if (!digitalRead(leds[2])) sendSignal(2);
    delay(10);
    if (!digitalRead(leds[3])) sendSignal(3);
  }
}

void sendSignal(int select) {
#ifdef DEBUG
  Serial.print("Sending signal to output ");
  Serial.println(select);
#endif
  digitalWrite(selects[0], (select >> 0) & 0x1);
  digitalWrite(selects[1], (select >> 1) & 0x1);

  convert50toUS((unsigned int *)results.rawbuf, results.rawlen);
#ifdef DEBUG
  printraw(&results);
#endif
  irsend.sendRaw((unsigned int *)results.rawbuf + 1, results.rawlen, 38);
}

void learnControl(unsigned long newcontrol) {
  learn = 0;

  control = newcontrol;
}

void nextTransmit()
{
  char notconnected;
  int newtransmit = -1;
  int start;

  start = transmit > 3 ? -1 : transmit;
  // Check for connected
#ifdef DEBUG
  Serial.print("transmit was ");
  Serial.println(transmit);
#endif
  for (int i = 1; i < 5; i++) {
    // Connected pins are asserted low
    notconnected = digitalRead(detects[(start + i) % 4]);
#ifdef DEBUG
    Serial.print("Pin ");
    Serial.print(detects[(start + i) % 4]);
    Serial.print(" (i=");
    Serial.print(i);
    Serial.print("): ");
    Serial.println(digitalRead(detects[(start + i) % 4]), HEX);
#endif
    if (!notconnected) {
      newtransmit = (start + i) % 4;
      break;
    }
  }
#ifdef DEBUG
  Serial.print("calculated newtransmit: ");
  Serial.println(newtransmit);
#endif
  if (newtransmit == -1) {
    // No emitters detected
    transmit = 4;
  }
  else if (newtransmit <= transmit && transmit != 4) {
    // Select all
    transmit = 4;
  } else {
    transmit = newtransmit;
  }
#ifndef DEBUG
  if (transmit == 4) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(leds[i], HIGH);
    }
  } else {
    for (int i = 0; i < 4; i++) {
      if (i == transmit) digitalWrite(leds[i], HIGH);
      else digitalWrite(leds[i], LOW);
    }
  }
#endif
#ifdef DEBUG
  Serial.print("Transmit set to: ");
  Serial.println(transmit);
#endif
}

#ifdef DEBUG
void printraw(decode_results * results) {
  for (int i = 0; i < results->rawlen; i++)
  {
    Serial.print(results->rawbuf[i] * 50);
    Serial.print(", ");
    if (i % 10 == 0) {
      Serial.println();
    }
  }
}
#endif

void ButtonInterrupt() {
  // disable the interrupt for debouncing the switch
  detachInterrupt(0);
  learn = 1;

}
