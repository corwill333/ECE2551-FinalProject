#include <Arduino.h>
#include <LiquidCrystal.h>
#include <RF24.h>
#include "Entropy.h"

/* ---------- hardware pins ---------- */
const int BUZZER_PIN = 3;
const int CE_PIN  = A1;
const int CSN_PIN = A2;

/* ---------- objects ---------- */
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);   // LCD Shield pins
RF24 radio(CE_PIN, CSN_PIN);           // Radio object
byte uuid[5];                          // 5-byte unique ID

/* ---------- helper ---------- */
void beep(int f, int d) { tone(BUZZER_PIN, f, d); }

/* ---------- setup ---------- */
void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  lcd.begin(16, 2);

  //  Say hello on LCD
  lcd.print("Hello!");
  lcd.setCursor(0,1);
  lcd.print("Let's Begin :)");
  beep(1000,200);
  delay(1500);
  lcd.clear();

  // Make a random 5-byte UUID
  Entropy.initialize();
  for (int i=0;i<5;i++) uuid[i] = Entropy.randomByte();

  Serial.begin(115200);
  Serial.print("My UUID: 0x");
  for (int i=0;i<5;i++){ if(uuid[i]<0x10)
Serial.print('0'); 
Serial.print(uuid[i],HEX); }
  Serial.println();
  lcd.print("UUID:");
  lcd.setCursor(0,1);
  for (int i=0;i<5;i++){ 
if(uuid[i]<0x10) 
lcd.print('0'); 
lcd.print(uuid[i],HEX); }
  delay(2000);
  lcd.clear();

  //  Test Radio power-up
  lcd.print("Radio check...");
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  lcd.setCursor(0,1);
  lcd.print("OK!");
  beep(1200,150);
  delay(1500);
  lcd.clear();

  //  Button & buzzer play
  lcd.print("Press buttons!");
  lcd.setCursor(0,1);
  lcd.print("A0 read test");
}

/* ---------- loop ---------- */
void loop() {
  int v = analogRead(A0);   // read button ladder
  lcd.setCursor(0,1);

  if (v < 50)      { lcd.print("RIGHT "); beep(1000,80); }
  else if (v <250) { lcd.print("UP    "); beep(1200,80); }
  else if (v <450) { lcd.print("DOWN  "); beep(800,80); }
  else if (v <650) { lcd.print("LEFT  "); beep(600,80); }
  else if (v <900) { lcd.print("SELECT"); beep(1500,80); }
  else             { lcd.print("NONE  "); }

  delay(200);
} 
