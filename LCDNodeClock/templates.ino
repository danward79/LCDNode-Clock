#include "utility/font_helvB24.h"
#include "utility/font_helvB12.h"
#include "utility/font_clR4x6.h"
#include "utility/font_clR6x8.h"

//------------------------------------------------------------------
// Draws a page showing a single power and energy value in big font
//------------------------------------------------------------------
void draw_main_page(int hourval, int minval, int sec)
{ 
  glcd.clear();
  glcd.fillRect(0,0,128,64,0);
  


  char str[8], str2[2];
  
  itoa(hourval/10, str, 10);
  itoa(hourval%10, str2, 10);
  strcat(str, str2);
  strcat(str, "   ");
  itoa(minval/10, str2, 10);
  strcat(str, str2);
  itoa(minval%10, str2, 10);
  strcat(str, str2);
  
  glcd.setFont(font_helvB24);
  glcd.drawString(15, 5,str);
  
  glcd.fillCircle(62, 10, 2, sec & 1);
  glcd.fillCircle(62, 25, 2, sec & 1);
      
}


//------------------------------------------------------------------
// Draws a footer showing GLCD temperature and the time
//------------------------------------------------------------------
void draw_temperature_time_footer(int temp, int mintemp, int maxtemp, double localTemp)
{
  
  glcd.setFont(font_clR6x8);      
  glcd.drawString(0,42,"  OUTSIDE     INSIDE");
  glcd.drawLine(77, 39, 77, 65, WHITE);
  glcd.drawLine(0, 39, 128, 39, WHITE);     //middle horizontal line 

  char str[8], str2[2];
  // GLCD Temperature
  glcd.setFont(font_helvB12);  
  itoa(temp/10, str, 10);
  strcat(str, ".");
  itoa(temp%10, str2, 10);
  strcat(str, str2);
  strcat(str,"C");
  glcd.drawString(0,50,str);  
  
  // Minimum and maximum GLCD temperature
  glcd.setFont(font_clR4x6);             
  itoa(mintemp/10, str, 10);
  strcat(str,"C");
  glcd.drawString_P(46,51,PSTR("MIN"));
  glcd.drawString(62,51,str);
               
  itoa(maxtemp/10, str, 10); 
  strcat(str,"C");
  glcd.drawString_P(46,59,PSTR("MAX"));
  glcd.drawString(62,59,str);
  
  // Local Temp
  dtostrf(localTemp,0,1,str);
  strcat(str,"C");
  glcd.setFont(font_helvB12);
  glcd.drawString(82,50,str);

}


void draw_weather_page(int light, int humidity, int temperature, int dewpoint, int cloudbase, int32_t pressure)
{
  glcd.clear();
  glcd.setFont(font_clR6x8);
  glcd.drawString(40, 0, "WEATHER");
  
  glcd.setFont(font_clR4x6);           

  glcd.drawString_P(2,9,PSTR("Temperature: "));
  glcd.drawString_P(2,16,PSTR("Humidity: "));
  glcd.drawString_P(2,23,PSTR("Light: "));
  glcd.drawString_P(2,30,PSTR("Pressure: "));
  glcd.drawString_P(2,37,PSTR("Cloudbase: "));
  glcd.drawString_P(2,44,PSTR("Dewpoint: "));
  
  char str[21];
 
  sprintf(str, "%d.%d C", temperature/10, temperature%10);
  glcd.drawString(55, 9, str);
  
  sprintf(str, "%d", humidity/10);
  strcat(str, " %");
  glcd.drawString(55, 16, str);

  sprintf(str, "%d", light);
  strcat(str, " %");
  glcd.drawString(55, 23, str);
  
  sprintf(str, "%d hPa", pressure/100);
  glcd.drawString(55, 30, str);
  
  sprintf(str, "%d ft", cloudbase);
  glcd.drawString(55, 37, str);
  
  sprintf(str, "%d C", dewpoint);
  glcd.drawString(55, 44, str);

}