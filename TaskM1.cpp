#include <avr/interrupt.h>
const int tempSensorPin   = 2;
const int doorSensorPin   = 8;
const int windowSensorPin = 9;
const int fanLedPin       = 12;
const int alarmLedPin     = 13;
const int heartbeatLedPin = 11;

volatile bool tempFlag       = false;
volatile bool pciFlag        = false;
volatile bool timerTickFlag  = false;
volatile byte secondCounter  = 0;
const byte PERIODIC_INTERVAL_SEC = 5;

bool tempState   = false;
bool doorState   = false;
bool windowState = false;

byte lastPinBState = 0; 

void tempISR() {
  tempFlag = true;   
}

void pciSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));
  PCIFR  |= bit(digitalPinToPCICRbit(pin));
  PCICR  |= bit(digitalPinToPCICRbit(pin));
}

ISR(PCINT0_vect) {
  pciFlag = true;
}

void setupTimer1() {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 15624;                         
  TCCR1B |= (1 << WGM12);                
  TCCR1B |= (1 << CS12) | (1 << CS10);   
  TIMSK1 |= (1 << OCIE1A);               
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  secondCounter++;
  if (secondCounter >= PERIODIC_INTERVAL_SEC) {
    secondCounter = 0;
    timerTickFlag = true;
  }
}

void updateAlarm() {
  if (tempState && (doorState || windowState)) {
    digitalWrite(alarmLedPin, HIGH);
  } else {
    digitalWrite(alarmLedPin, LOW);
  }
}

void setup() {
  pinMode(tempSensorPin, INPUT_PULLUP);
  pinMode(doorSensorPin, INPUT_PULLUP);
  pinMode(windowSensorPin, INPUT_PULLUP);
  pinMode(fanLedPin, OUTPUT);
  pinMode(alarmLedPin, OUTPUT);
  pinMode(heartbeatLedPin, OUTPUT);

  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(tempSensorPin), tempISR, CHANGE);

  pciSetup(doorSensorPin);
  pciSetup(windowSensorPin);
  lastPinBState = PINB;

  setupTimer1();

  Serial.println("System started. Monitoring temperature, door, window, heartbeat...");
}

void loop() {

  if (tempFlag) {
    tempFlag = false;
    tempState = !digitalRead(tempSensorPin); 
    digitalWrite(fanLedPin, tempState ? HIGH : LOW);
    Serial.println(tempState ? "Temperature HIGH. Fan ON." : "Temperature normal. Fan OFF.");
    updateAlarm();
  }

  if (pciFlag) {
    pciFlag = false;
    byte currentPinBState = PINB;
    byte changedBits = currentPinBState ^ lastPinBState;

    if (changedBits & bit(doorSensorPin - 8)) {
      doorState = !(currentPinBState & bit(doorSensorPin - 8));
      Serial.println(doorState ? "Door OPENED." : "Door CLOSED.");
    }
    if (changedBits & bit(windowSensorPin - 8)) {
      windowState = !(currentPinBState & bit(windowSensorPin - 8));
      Serial.println(windowState ? "Window OPENED." : "Window CLOSED.");
    }

    lastPinBState = currentPinBState;
    updateAlarm();
  }

  if (timerTickFlag) {
    timerTickFlag = false;
    digitalWrite(heartbeatLedPin, !digitalRead(heartbeatLedPin));
    Serial.print("Heartbeat - system alive. Door=");
    Serial.print(doorState);
    Serial.print(" Window=");
    Serial.print(windowState);
    Serial.print(" Temp=");
    Serial.println(tempState);
  }
}