#include <U8g2lib.h>

#define OLED_RESET     U8X8_PIN_NONE // Reset pin (not used)
#define OLED_SDA 14                  // D6
#define OLED_SCL 12                  // D5

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RESET, OLED_SCL, OLED_SDA);

int c = 0;

void handle_oled(int c) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "Display is working!");
  u8g2.drawStr(0, 30, "Have fun with it");
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "Uptime: %ds", c);
  u8g2.drawStr(0, 50, buffer);
  u8g2.sendBuffer();
}

/* void setup() {
  u8g2.begin();
}*/

/* void loop() {
  handle_oled(c);
  c++;
  delay(1000);
} */


#define USE_LIB_WEBSOCKET true

#include "sensesp_app.h"
#include "sensors/sht31.h"
#include "sensors/digital_input.h"
#include "transforms/frequency.h"
#include "signalk/signalk_output.h"
#include "sensors/analog_input.h"

float waterlevel=0.0;

class FulSensor: public NumericSensor {

 public:
   FulSensor();

   void set(float);

 private:
   float thevalue;

};

FulSensor::FulSensor() {
}

void FulSensor::set(float ap) {
  thevalue=ap;
  this->emit(ap);
  }
  

ReactESP app([]() {


  u8g2.begin();



#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif

  sensesp_app = new SensESPApp();

FulSensor* water_level = new FulSensor(); 
water_level->connect_to(	     
   new SKOutputNumber("tanks.freshWater.0.currentLevel"));	
	
    // If you wanted to add a third, fourth, or more sensor, you would do that
    // here. An ESP9266 should easily handle four or five sensors, and an ESP32
    // should handle eight or ten, or more.

  app.onRepeat(1000,[water_level] () {
     handle_oled(c);
     c++;
     water_level->set(c);
  });


  sensesp_app->enable();
});
