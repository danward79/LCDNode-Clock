/*
//------------------------------------------------------------------------------
// emonGLCD Based LCD Node
// Based on original code from Open Energy Monitor and Libraries from Jeelabs

// Desired functionality
// - LEDs
// - Clock Page
// - Weather Page

// RTC is implemented is software. 
// Correct time is updated via RF Packet which gets time from internet
// Temperature recorded on the emonglcd is also broadcasts

// THIS SKETCH REQUIRES:
// Libraries in the standard arduino libraries folder:
//	- OneWire library	http://www.pjrc.com/teensy/td_libs_OneWire.html
//	- DallasTemperature	http://download.milesburton.com/Arduino/MaximTemperature
//                           or https://github.com/milesburton/Arduino-Temperature-Control-Library
//	- JeeLib		https://github.com/jcw/jeelib
//	- RTClib		https://github.com/jcw/rtclib
//	- GLCD_ST7565		https://github.com/jcw/glcdlib
//
// Other files in project directory (should appear in the arduino tabs above)
//	- templates.ino
//------------------------------------------------------------------------------
*/

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
//------------------------------------------------------------------------------
// RFM12B Settings
//------------------------------------------------------------------------------
#define MYNODE 11 
#define freq RF12_433MHZ  
#define group 212 

#define ONE_WIRE_BUS 5  

unsigned long fast_update, slow_update, backLightOverrideTime, buttonPressTime;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

double localTemp;
int temp, maxtemp, mintemp;
int hour = 12, minute = 0;
boolean backLightOverride = false;
int page;
boolean firstTempReading = false;

//---------------------------------------------------
// Data structures for transfering data between units
//---------------------------------------------------
//Node ID 16
typedef struct {byte light; int humidity; int temperature; byte vcc;} PayloadOutdoor;
PayloadOutdoor outdoornode;

//Node ID 17
typedef struct {byte light; int16_t temperature; int32_t pressure; int battery;} PayloadBaro;
PayloadBaro barotx;

//Node ID 11
typedef struct {int temperature; byte light;} PayloadGLCD;
PayloadGLCD emonglcd;

#define greenLED 6      // Green bi-color LED
#define redLED 9        // Red bi-color LED
#define LDRpin 4        // analog pin of onboard lightsensor 
#define switchEnter 15
#define switchUp 16
#define switchDown 19
#define backLightOffHour 22
#define backLightOnHour 07


//------------------------------------------------------------------------------
// Setup
//------------------------------------------------------------------------------
void setup()
{
  //delay(300); 				   //wait for power to settle before firing up the RF
  rf12_initialize(MYNODE, freq, group);
  delay(400);				   //wait for RF to settle befor turning on display
  
  glcd.begin(0x19);
  glcd.backLight(200);
  
  sensors.begin();                // start up the DS18B20 temp sensor onboard  
  sensors.requestTemperatures();
  
  pinMode(greenLED, OUTPUT); 
  pinMode(redLED, OUTPUT); 
  
  enter.attach(switchEnter);
  enter.interval(5);
  up.attach(switchUp);
  up.interval(5);
  down.attach(switchDown);
  down.interval(5);
  
  RTC.begin(DateTime(__DATE__, __TIME__));
  
  Serial.begin(57600);
  Serial.println("\nLCD Node");
}

//------------------------------------------------------------------------------
// Loop
//------------------------------------------------------------------------------
void loop()
{
  //Button control & page control
  boolean enterStateChanged = enter.update();
  boolean upStateChanged = up.update();
  boolean downStateChanged = down.update();
  
  if (enterStateChanged){ //Detect enter button long or short press.
    if (enter.read())
    {
      buttonPressTime = millis();
    } else {
      if (millis() - buttonPressTime > 1500){
        Serial.print(">1.5s ");
        backLightOverride = !backLightOverride;
        Serial.println(backLightOverride);
      }
      else{
        page += 1;
        if (page > 1) page = 0;
      }
    }
  }
  
  
  
  // Up and Down Button backlight override
  bool switch_state = up.read() or down.read();  
  
  if (switch_state) // Backlight override
  {
    backLightOverride = true;
    backLightOverrideTime = millis();
  }
  
  
  


  

  
  
  
  
  
  
  
  
  // RF Reception
  if (rf12_recvDone())
  {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no rf errors
    {
      int node_id = (rf12_hdr & 0x1F);
      
      if (node_id == 16)
      {
        outdoornode = *(PayloadOutdoor*) rf12_data;
        if (!firstTempReading){
          mintemp = maxtemp = outdoornode.temperature; 
          firstTempReading = true;
        };
      }
      
      if (node_id == 17)
      {
        barotx = *(PayloadBaro*) rf12_data;
      }
      
      if (node_id == 31)			//Time transmission
      {
        RTC.adjust(DateTime(2014, 1, 1, rf12_data[1], rf12_data[2], rf12_data[3]));
      } 
    }
  }

//------------------------------------------------------------------------------
// Display update every 200ms
//------------------------------------------------------------------------------
  if ((millis()-fast_update)>200){
    fast_update = millis();
    
    DateTime now = RTC.now();
    int last_hour = hour;
    hour = now.hour();
    minute = now.minute();
    
    int LDR = analogRead(LDRpin); // Read the LDR Value
    int LDRbacklight = map(LDR, 0, 1023, 50, 250); // Map the LDR from 0-1023 (Max seen 1000) to var GLCDbrightness min/max
    LDRbacklight = constrain(LDRbacklight, 0, 255); // Constrain PWM value 0-255
  
    // Backlight time control
    if ((hour >= backLightOffHour) ||  (hour < backLightOnHour)) {
      Serial.print("bckliteoverride ");   Serial.println(backLightOverride);
 
      if (!backLightOverride) {
        glcd.backLight(0); 
      } else {
        glcd.backLight(LDRbacklight);
      }
    
      if ((millis() - backLightOverrideTime) > 6000) {
        glcd.backLight(0);
        backLightOverride = false;
      }
    
    } else {
    
      glcd.backLight(LDRbacklight);
    
    } 
    
    if (page == 0) //Standard Power Page
    {
      draw_main_page(hour, minute, now.second());
      draw_temperature_time_footer(temp, mintemp, maxtemp, localTemp);
      glcd.refresh();
    }
    else if (page == 1) //Weather Page
    {
      int dp = calculateDewpoint(outdoornode.humidity, outdoornode.temperature);
      draw_weather_page((outdoornode.light * 100/255), outdoornode.humidity, outdoornode.temperature, dp, calculateCloudbase (outdoornode.temperature, dp), barotx.pressure);
      glcd.refresh();
    }
    else if (page == 2){ // Settings page
 
    } 
      
  } 
  
  //Slow page updates
  if ((millis()-slow_update)>10000)
  {
    slow_update = millis();
    
    //Update Min/Max Temp
    temp = outdoornode.temperature;
    
    if ((temp > -500) && (temp < 700))
    {
      if (temp > maxtemp) maxtemp = temp;
      if (temp < mintemp) mintemp = temp;
    }
    
    // Get local temp
    sensors.requestTemperatures();
    localTemp = (sensors.getTempCByIndex(0));
   
    // set emonglcd payload
     emonglcd.temperature = (int) (localTemp * 100); 
     emonglcd.light = 255 - analogRead(LDRpin) / 4;
                     
    // Broadcast temp and light reading
     rf12_sendNow(0, &emonglcd, sizeof emonglcd);
     rf12_sendWait(2);    
  }
}

//cloudBase calculation
int calculateCloudbase (int t, int d)
{ 
  return ( ((t * 0.1) - d) * 400 );
}

//dewpoint Calculation
int calculateDewpoint(int h, int t)
{
	float k = (log10(h * 0.1)-2)/0.4343 + (17.62*(t*0.1))/(243.12+(t*0.1)); 
	
	return 243.12*k/(17.62-k); 
}
