#include <EEPROM.h>
#include "Arduino.h"
#include "Pages.h"
#include "Comms.h"
#define N_PAGES 6
#define BUTTON_HOLD_TIME 500

#define PUMP_OUT 3
#define LED_PUMP 10
#define STATUS_LED_R 5
#define STATUS_LED_G 6
#define STATUS_LED_B 9
#define FAN_OUT 4
#define PWR_HOLD 8
#define IGN_ON 7

#define WARM_UP_TEMP 60
#define ENGINE_TARGET_TEMP 77
#define MAX_PUMP_SPEED 216 //36-216 = 0-100% pumpspeed
#define FAN_START 80
#define FAN_HYS 2
#define RUN_TIME 60 //seconds


uint8_t pageNum = EEPROM.read(0);

//variables
float sensorValue = 0.0;  // analog read of pot
float get_CLT = 0.0;
float get_TPS = 0.0;
float get_RPM = 0.0;

uint8_t dis_pumpspeed = 0;
uint8_t led_pumpspeed = 0;

uint16_t pump_target = 0;
  
bool fan_run = false;
bool after_run = false;
bool system_hot = false;

void setup()
{
  Serial.begin(115200);
  initDisplay();
  if (pageNum >= N_PAGES)
  {
    pageNum = 0;
    EEPROM.update(0, pageNum);
  }
   // declare output pins
  pinMode(PUMP_Out, OUTPUT); //Pump
  pinMode(LED_PUMP, OUTPUT); //Led indicator pump
  pinMode(STATUS_LED_R, Output); //Status led red
  pinMode(STATUS_LED_G, Output); //Status led green
  pinMode(STATUS_LED_B, Output); //Status led blue
  pinMode(FAN_OUT, Output); //Cooling fan
  pinMode(PWR_HOLD, Output); //Power output for afterrun
  
  // declare input pins
  pinMode(A0,Input); //reference speed
  pinMode(IGN_ON, Input); //signal from ignition key
  
  // initiate pump wakeup 
  analogWrite(PUMP_Out, 0);
  analogWrite(PUMP_Out, 255);
  delay(300);
  analogWrite(PUMP_Out, 0);
  delay(100);
  // initate reset and standby
  analogWrite(PUMP_Out, 25);
  delay(300);
  analogWrite(PUMP_Out, 0);
  delay(100);
  // pump is now ready to use
  
  // initiate Power
  digitalWrite(PWR_HOLD, HIGH);
  
}

void loop()
{
  // button and page number operations
  static bool buttonLast = false;
  static bool buttonHeld = false;
  static uint32_t timePressed = 0;

  bool buttonNow = !digitalRead(2);
  //digitalWrite(LED_BUILTIN, buttonNow);

  // button pressed
  if (!buttonLast & buttonNow)
  {
    timePressed = millis();
  }
  // button held
  if (buttonNow & !buttonHeld & ((millis() - timePressed) > BUTTON_HOLD_TIME))
  {
    pageNum = EEPROM.read(0);
    pageNum--;
    if (pageNum >= N_PAGES)
      pageNum = N_PAGES - 1;
    EEPROM.update(0, pageNum);
    buttonHeld = true; // prevent rerunning button held action.
  }
  // button released
  if (buttonLast & !buttonNow)
  {
    if (!buttonHeld) // do not perform if button hold action executed.
    {
      pageNum = EEPROM.read(0);
      pageNum++;
      if (pageNum >= N_PAGES)
        pageNum = 0;
      EEPROM.update(0, pageNum);
    }
    buttonHeld = false;
  }
  buttonLast = buttonNow;

  // serial operation, frequency based request
  static uint32_t lastUpdate = millis();
  if (millis() - lastUpdate > 10)
  {
    requestData(50);
    lastUpdate = millis();
  }

  // get refresh rate
  static uint32_t lastRefresh = millis();
  uint16_t refreshRate = 1000 / (millis() - lastRefresh);
  lastRefresh = millis();

  // display pages
  switch (pageNum)
  {
  case 0:
    show2Bar(F("Oil Pressure (psi)"), getWord(104), 0, 100, 0,
             F("Oil Temperatur (\367C)"), getWord(103), 0, 150, 0); //fuelpress for Oiltempsensor??? muss ich noch klären
      
      // showBar(F("Engine Speed (RPM)"), getWord(14), 0, 6000);
    break;
  case 1:
    showNumeric(F("Gear"), getWord(102), 0, 6);
      
      //showBar(F("MAP (kPa)"), getWord(4), 0, 101);
    break;
  case 2:
    show4Numeric(F("RPM"), getWord(14), 0, 6000, 0,
                 F("MAP (kpa)"), getWord(4), 0, 120, 0,
                 F("AFR"), getByte(10), 80, 210, 1,
                 F("Battery (V)"), getByte(9), 60, 160, 1);
      
      //showBar(F("Coolant Temp (\367C)"), (int16_t)getByte(7) - 40, 0, 120);
    break;
  case 3:
    showFlags(F("crank"), getBit(2, 1),
              F("RUN"), getBit(2, 0),
              F("sync"), getBit(31, 7),
              F("warm"), getBit(2, 3),
              F("Fan"), fan_run,
              F("IGN Error"), getBit(31, 5),
              F("Oil Error"), getBit(83, 2),
              F("AFR Error"), getBit(83, 3),
         
      
    //showBar(F("Battery (V)"), getByte(9), 60, 160, 1);
    break;
  case 4:
     show4Numeric(F("Pumpspeed %"), dis_pumpspeed, 0, 100, 0,
                  F("Coolant Temp (\367C)"), (int16_t)getByte(7) - 40, 0, 120)
                  F("Min Pumpspeed %"), dis_sensor_value, 0, 100, 0,
                  F(""), false);

      //showNumeric(F("Speed (kph)"), getWord(100), 0, 300);
    break;
  case 5:
    show4Numeric(F("Cycles/sec"), getWord(25), 0, 1000, 0,
                 F("Mem (b)"), getWord(27), 0, 2048, 0,
                 F("FPS"), refreshRate, 0, 100, 0,
                 F(""), 0, 0, 100, 0);
      
      //show2Bar(F("Oil Pressure (psi)"), getByte(104), 0, 100, 0,
    //         F("Engine Speed (RPM)"), getWord(14), 0, 6000, 0);
    break;
  case 6:
    show4Numeric(F("RPM"), getWord(14), 0, 6000, 0,
                 F("MAP (kpa)"), getWord(4), 0, 120, 0,
                 F("AFR"), getByte(10), 80, 210, 1,
                 F("Adv (deg)"), (int8_t)getByte(23), -10, 40, 0);
    break;
  case 7:
    show4Numeric(F("CLT (\367C)"), (int16_t)getByte(7) - 40, 0, 120, 0, // temp offset by 40
                 F("EOP (psi)"), getByte(104), 0, 100, 0,
                 F("Batt (V)"), getByte(9), 60, 160, 1,
                 F("Dwell (ms)"), (int8_t)getByte(3), 0, 200, 1);
    break;
  case 8:
    showFlags(F("crank"), getBit(2, 1),
              F("RUN"), getBit(2, 0),
              F("sync"), getBit(31, 7),
              F("warm"), getBit(2, 3),
              F(""), false,
              F(""), false,
              F(""), false,
              F(""), false);
    break;
  case 9:
    show4Numeric(F("Cycles/sec"), getWord(25), 0, 1000, 0,
                 F("Mem (b)"), getWord(27), 0, 2048, 0,
                 F("FPS"), refreshRate, 0, 100, 0,
                 F(""), 0, 0, 100, 0);
    break;
  default:
    showSplash(F("Coming Soon!"));
    break;
  }
  
  //Pump control
  //Get reference pot
  sensorValue = map(analogRead(A0), 0, 1023, 36, 150);  // rescale 0-1024 to required range
  dis_sensorValue = map(sensorValue, 36, 150, 0, 63);
  //get coolant temp + TPS + RPM for Pump Speed calculation
  get_CLT = getByte(7) - 40, 0, 120, 0;  // temp offset by 40
  get_TPS = getByte(24) 0, 100, 0;
  get_RPM = getWord(14), 0, 9000;
  
  if (get_CLT <=WARM_UP_TEMP) {
    //Engine Warm Up Pumpspeed = pot read
    pump_target = sensorValue;
    analogWrite(STATUS_LED_R, 0);
    analogWrite(STATUS_LED_G, 0);
    analogWrite(STATUS_LED_B, 255);
    after_run = false;
    system_hot = false;
  }
  else if (get_CLT > WARM_UP_TEMP && get_CLT <= ENGINE_TARGET_TEMP) {
    //pumpspeed ramp till target temp. Target temp some °C over thermostat opening temp
    pump_target = map(pump_target, WARM_UP_TEMP, ENGINE_TARGET_TEMP, sensorValue, MAX_PUMP_SPEED);
    analogWrite(STATUS_LED_R, 0);
    analogWrite(STATUS_LED_G, 255);
    analogWrite(STATUS_LED_B, 0);
    after_run = true;
    system_hot = false;
  }
  else if (get_CLT > ENGINE_TARGET_TEMP) {
    //pumpspeed allways 100%
    pump_target = 216;
    analogWrite(STATUS_LED_R, 255);
    analogWrite(STATUS_LED_G, 0);
    analogWrite(STATUS_LED_B, 0);
    after_run = true;
    system_hot = true;
  }
  
  //Pump Output
  analogWrite(PUMP_OUT, pump_target);
  //convert Pump Output to 0-100% for displaying
  dis_pumpspeed = map(PUMP_OUT, 36,  216, 0, 100);
  //convert Pump Output to 0-255 for LED
  led_pumpspeed = map(PUMP_OUT, 36, 216, 0, 255);
  //LED Pump
  analogwrite(LED_PUMP, led_pumpspeed);
  
  
  //Fan Control
  if (get_CLT >= FAN_START) {
    digitalWrite(FAN_OUT, HIGH);
    fan_run = true;
  }
  
  if (get_CLT <= (FAN_START - FAN_HYS) && fan_run == true) {
    ditalWrite(FAN_OUT, LOW);
    fan_run = false;
  }
  
  //Afterrun control
  bool ignon = !digitalRead(IGN_ON);
  if (ignon == false && after_run == true) {
    pump_target = 125; //pumpspeed 50%
    if (system_hot == true) {
      pump_target = 216; //pumpspeed 100%
      digitalWrite(FAN_OUT, HIGH);
      fan_run = true;
    }
    // Nachlaufzeit erstellen
    
    
    
  }
    
    
  

  
}
