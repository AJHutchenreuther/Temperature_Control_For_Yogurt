
/*
    TemperatureControl_MSP430_20Oct
    Copyright (C) 2014  Alan Hutchenreuther

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
 The purpose of this application is to control the yogurt brewing temperature.
 This is done by maintaining a 105 degF chamber temperature through simple ON/OFF control of an incandescent lamp.
 A histeresis band is set about the setpoint temperature to reduce the 
 number of heating/cooling cycles and preserve life of the lamp and relay.   

  This is a conversion of an Arduino application to run on a Launchpad v1.4 with MSP430G2553.
  Development environment is Energia 0101E0012 or later.
  
  Temperature measurement adapted from SD card datalogger.
   created  24 Nov 2010
    modified 9 Apr 2012
    by Tom Igoe
 
 This example code is part of the public domain 
*/
#include <LiquidCrystal.h>

#define TEMP_PIN A0
#define HEATER_PIN P1_7
#define MEASUREMENTS_TO_SUM 10
#define ANALOG_REF 1.5  // ajh141023 
#define DELAY_MS 100
#define TSP 105              // degF
#define TTOL 2
#define HIGH_T_LIMIT 140.0  // degF
#define LOW_T_LIMIT 10.0

// lcd arguments:  RS, EN, DB4, DB5, DB6, DB7 - signals on LCD pins 4, 6, 11, 12, 13, 14 respectively.
LiquidCrystal lcd(P2_0, P2_1, P2_2, P2_3, P2_4, P2_5); //ajh20141020 - Rewire LCD to free up P1_0 (A0, TEMP_PIN), P1_6 (GREEN_LED), P1_7 (HEATER_PIN)

float tempK, tempC, tempF, temp;
float thisTempF = 0.0;
float sumT = 0.0;
long count = 0;
float maxT = -10.0;
float minT = 150.0;
float timeSinceStart = 0.0;
float heaterONtime = 0.0;
boolean TSPreached = false;
long cycleCount= 0;
boolean heaterOFF;
boolean errorFlag = false;
float heaterDutyCycle = 0.0;
boolean loopstate = true; // Green LED control.
boolean debugging = false;

void setup() {
  analogReference( INTERNAL1V5);  // ajh141023 
  // set up the sensor pin as an input
  pinMode(TEMP_PIN, INPUT);
  // initialize control pin as output
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite( HEATER_PIN, LOW);
  heaterOFF = true;
  
  // Liquid Crystal //ajh20141019
  lcd.begin(16,2);  // using 16 x 2 LCD 
  lcd.setCursor( 0, 0);
  lcd.print("Temperature");
  lcd.setCursor( 1, 1);
  lcd.print("Controller v4.1");  // Update version number here!
}

void loop() {
 // This loop inputs sensorVal every DELAY_MS ms
 // thisTempF contains the average of the previous 10 measurements in degF.
 float voltage;
 int sensorVal;
 float elapsedMinutes;
 
   // Read temperature sensor
   sensorVal = analogRead(TEMP_PIN);
   voltage = sensorVal * ANALOG_REF/1024.0;
   tempC = (voltage - 0.5) * 100.0; // TMP36 Sensor has 0.5V offset to allow negative T readings on unipolar supply.
   tempF = ((tempC * 9.0)/5.0) + 32.0; 
   sumT += tempF;
   elapsedMinutes = (1.0 * count * DELAY_MS)/60000;
   count++;
   
   if (count >= MEASUREMENTS_TO_SUM ) {
     // Calculate current average reading.
     thisTempF = sumT/MEASUREMENTS_TO_SUM;
     count = 0;
     sumT = 0.0;
     // Detect sensor failure and disable heater.
     if( (thisTempF >= HIGH_T_LIMIT) || (thisTempF <= LOW_T_LIMIT)) {
       errorFlag = true;
       heaterOFF = true; 
       digitalWrite( HEATER_PIN, LOW);
     }
     
     // Record Max and Min T for display purposes
     timeSinceStart += elapsedMinutes;
     if (thisTempF > maxT) {
         maxT = thisTempF;
     }
     else if (thisTempF < minT) {
         minT = thisTempF;
     }
   
     // Update heater control.
     if( !errorFlag){
       if (thisTempF < (TSP - TTOL)) {
         // Turn heater ON
         digitalWrite( HEATER_PIN, HIGH);
         // Statistics
         if( heaterOFF) {
           cycleCount++;
           heaterOFF = false;
           }
         }
       else if ( thisTempF > (TSP + TTOL)) {
         // Turn heater OFF
         digitalWrite( HEATER_PIN, LOW);
         heaterOFF = true;
       }
     } // !ErrorFlag
     
     // Statistical analysis
     // When Temperature setpoint is reached, reset the min Temp to current temp.
     if( !TSPreached & (thisTempF >= TSP)) {
       TSPreached = true;
       minT = thisTempF;
     };
     // Update duty cycle 
     if( !heaterOFF) {
        heaterONtime += elapsedMinutes;
     }
     heaterDutyCycle = 100.0 * (heaterONtime / timeSinceStart);
     
     // Blink GREEN_LED  
     digitalWrite( GREEN_LED, loopstate);
     loopstate = !loopstate;
     
     // Update LCD
     lcd.clear();
     lcd.setCursor( 0, 0);

      // First line
      lcd.print( thisTempF); lcd.print( ", ");
      if ( !errorFlag) {
        if( debugging) {
            lcd.print( voltage * 1000); 
        }
        else {
            lcd.print( int(heaterDutyCycle)); lcd.print( ", ");
            lcd.print( cycleCount);
        }
      }
      else {
        lcd.print( " Error");
      }
      // Second line 
      lcd.setCursor( 0, 1);
      if( debugging) {   // ajh141023 - for determination of reference voltage.
        lcd.print( sensorVal);
      }
      else {
        lcd.print( minT); lcd.print( ", ");
        lcd.print( maxT);
      } 
    };  // count >= MEASUREMENTS_TO_SUM
  
    delay( DELAY_MS);
}// loop.
