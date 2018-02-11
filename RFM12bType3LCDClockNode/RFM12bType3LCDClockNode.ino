/*
  GLCD Based LCD Clock Node
  Based on original code from Open Energy Monitor and Libraries from Jeelabs

  Desired functionality
  - Clock Page
  - Weather Page
  - Ability to override timed backlight, reset at next on or off time
  - LEDs TODO
  - ?

  RTC is implemented is software. 
  Correct time is updated via RF Packet which gets time from internet
  Temperature recorded on the emonglcd is also broadcasts

  THIS SKETCH REQUIRES:
  Libraries in the standard arduino libraries folder:
    - OneWire library	http://www.pjrc.com/teensy/td_libs_OneWire.html
    - DallasTemperature	http://download.milesburton.com/Arduino/MaximTemperature
                            or https://github.com/milesburton/Arduino-Temperature-Control-Library
    - JeeLib		https://github.com/jcw/jeelib
    - RTClib		https://github.com/jcw/rtclib
    - GLCD_ST7565		https://github.com/jcw/glcdlib

  Other files in project directory (should appear in the arduino tabs above)
    - templates.ino

*/

// User settings
#define NODE_ID 11         		
#define GROUP 212  
#define NODE_LABEL "\n[Clock Node]"
#define BACKLIGHTOFFHOUR 23
#define BACKLIGHTONHOUR 05
#define SEND_PERIOD 60000	// Minimum transmission period of sensor values
#define SLOW_UPDATE_INT 10000 // Interval at which footer parameters are updated

// General parameters which may need to be changed
#define FREQ RF12_433MHZ        	// Frequency of RF12B module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
#define SENSORTYPE 3
#define SERIALBAUD 9600

// Parameters that should not be changed for the standard EMONLCD
#define ONE_WIRE_BUS 5  
#define GREENLED 6      // Green bi-color LED
#define REDLED 9        // Red bi-color LED
#define LDR_PORT 4        // analog pin of onboard lightsensor 
#define SWITCHENTER 15
#define SWITCHUP 16
#define SWITCHDOWN 19


#include <JeeLib.h>
#include <GLCD_ST7565.h>
#include <avr/pgmspace.h>
GLCD_ST7565 glcd;

#include <OneWire.h>		    
#include <DallasTemperature.h>     

#include <RTClib.h>                 
#include <Wire.h>                 
RTC_Millis RTC;

#include <Bounce2.h>
Bounce enter = Bounce(); 
Bounce up = Bounce(); 
Bounce down = Bounce(); 

unsigned long fast_update, slow_update, backLightOverrideTime, buttonPressTime;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int maxtemp, mintemp, page;
int hour = 12, minute = 0;
byte sendCounter;
boolean backLightOverride = false;
boolean firstTempReading = false;
boolean forceBacklight = false;

//---------------------------------------------------
// Data structures for transfering data between units

//Node ID 16
typedef struct {byte sensortype; byte light; int temperature; long pressure; byte vcc;} PayloadOutdoor;
PayloadOutdoor outdoornode;

//Node ID 17
typedef struct {byte light; int16_t temperature; int32_t pressure; int battery;} PayloadBaro;
PayloadBaro barotx;

//Node ID 11
typedef struct {byte sensortype; int temperature; byte light;} PayloadGLCD;
PayloadGLCD emonglcd;

// ------------------------------------------------------------------------------
// Setup
void setup()
{
  rf12_initialize(NODE_ID, FREQ, GROUP);
  delay(100);
  
  glcd.begin(0x19);
  
  sensors.begin();               
  
  pinMode(GREENLED, OUTPUT); 
  pinMode(REDLED, OUTPUT); 
  
  enter.attach(SWITCHENTER); enter.interval(5);
  up.attach(SWITCHUP); up.interval(5);
  down.attach(SWITCHDOWN); down.interval(5);
  
  //RTC.begin(DateTime(__DATE__, __TIME__));
  RTC.begin(DateTime(2018, 1, 20, 00, 00, 00));
  
  Serial.begin(SERIALBAUD);
  Serial.println(NODE_LABEL);
}

// Loop
void loop()
{
  //Button control & page control
  boolean enterStateChanged = enter.update();
  boolean upStateChanged = up.update();
  boolean downStateChanged = down.update();
  
  if (enterStateChanged){ //Detect enter button long or short press.
    if (enter.read()){
      buttonPressTime = millis();
    } else {
      if (millis() - buttonPressTime > 1500){
        forceBacklight = !forceBacklight;
      }
      else {
        page += 1;
        if (page > 1) page = 0;
      }
    }
  }    
  
  // Up and Down Button backlight override
  boolean switch_state = up.read() or down.read();  
  
  // Backlight override
  if (switch_state){
    backLightOverride = true;
    backLightOverrideTime = millis();
  }
  
  // RF Reception
  if (rf12_recvDone()){

    flashGreen();

    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0){ // and no rf errors
      int node_id = (rf12_hdr & 0x1F);
      
      if (node_id == 16){
        outdoornode = *(PayloadOutdoor*) rf12_data;
        if (!firstTempReading){
          mintemp = maxtemp = outdoornode.temperature; 
          firstTempReading = true;
        };
      }
      
      if (node_id == 17){
        barotx = *(PayloadBaro*) rf12_data;
      }

      if (node_id == 31){		//Time transmission
        if (rf12_data[0] == 116){
          flashRed();
          RTC.adjust(DateTime(2014, 1, 1, rf12_data[1], rf12_data[2], rf12_data[3]));
        }
      } 
    }
  }

//------------------------------------------------------------------------------
// Display update every 200ms
  if ((millis()-fast_update)>200) {
    fast_update = millis();
    
    DateTime now = RTC.now();
    int last_hour = hour;
    hour = now.hour();
    minute = now.minute();
    
    //reset min/max temp at midnight
    if (last_hour == 23 && hour == 00) maxtemp = mintemp = outdoornode.temperature;
    
    int LDR = analogRead(LDR_PORT); // Read the LDR Value
    int LDRbacklight = map(LDR, 0, 1023, 2, 255); // Map the LDR from 0-1023 to 0-255 
    
    //reset backlight forcing, on the next entry or exit to time boundary
    if (forceBacklight){
      if ((last_hour == (BACKLIGHTOFFHOUR - 1)) && (hour == BACKLIGHTOFFHOUR)) forceBacklight = !forceBacklight;
      if ((last_hour == (BACKLIGHTONHOUR - 1)) && (hour == BACKLIGHTONHOUR)) forceBacklight = !forceBacklight;  
    }
    
    // Backlight time control
    if ((hour >= BACKLIGHTOFFHOUR) ||  (hour < BACKLIGHTONHOUR)){
      
      if (backLightOverride){
        glcd.backLight(LDRbacklight);
        
        if ((millis() - backLightOverrideTime) > 6000){
          glcd.backLight(0);
          backLightOverride = false;
        }
      } else {
        if(forceBacklight){
          glcd.backLight(LDRbacklight);
        } else {
          glcd.backLight(0); 
        }
      }
    
    } else {
      if (forceBacklight){
        glcd.backLight(0);
      } else {
        glcd.backLight(LDRbacklight);
      }
    } 
    
    if (page == 0){ //Standard Power Page
      draw_main_page(hour, minute, now.second());
      draw_temperature_time_footer(outdoornode.temperature, mintemp, maxtemp, sensors.getTempCByIndex(0));
      glcd.refresh();
    }
    else if (page == 1){ //Weather Page
      int dp = calculateDewpoint(0, outdoornode.temperature); // TODO: Add humidity back
      // TODO: Add humidity back
      draw_weather_page((outdoornode.light * 100/255), 0, outdoornode.temperature, dp, calculateCloudbase (outdoornode.temperature, dp), barotx.pressure);
      glcd.refresh();
    }
      
  } 
  
  //Slow page updates
  if ((millis()-slow_update)>SLOW_UPDATE_INT) {
    slow_update = millis();
    
    //Update Min/Max Temp    
    if ((outdoornode.temperature > -500) && (outdoornode.temperature < 700)) {
      if (outdoornode.temperature > maxtemp) maxtemp = outdoornode.temperature;
      if (outdoornode.temperature < mintemp) mintemp = outdoornode.temperature;
    }
  
    // Get local temp
    sensors.requestTemperatures();

    // set emonglcd payload
    emonglcd.sensortype = SENSORTYPE;
    emonglcd.temperature = (int) (sensors.getTempCByIndex(0) * 100); 
    emonglcd.light = 255 - analogRead(LDR_PORT) / 4;

    if (sendCounter >= SEND_PERIOD/SLOW_UPDATE_INT) {
       sendCounter = 0;

      // Broadcast temp and light reading
      rf12_sendNow(0, &emonglcd, sizeof emonglcd);
      rf12_sendWait(2);  
    }
    sendCounter++;
  }
}


// cloudBase calculation
int calculateCloudbase (int t, int d) { 
  return ( ((t * 0.1) - d) * 400 );
}

// dewpoint Calculation
int calculateDewpoint(int h, int t) {
	float k = (log10(h * 0.1)-2)/0.4343 + (17.62*(t*0.1))/(243.12+(t*0.1)); 
	
	return 243.12*k/(17.62-k); 
}

// flashRed LED
void flashRed() {
    flashLED(REDLED, 20);  
}

// flashGreen LED
void flashGreen() {
  flashLED(GREENLED, 20);     
}

// flashLED
void flashLED (int pin, int period) {
  digitalWrite(pin, HIGH);     
  delay(period);                 
  digitalWrite(pin, LOW); 
}