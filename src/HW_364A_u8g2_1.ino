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
#include "transforms/debounce.h"

//////////////////////////////////////////
// sensor class to send a float to signalk

class FloatSensor: public NumericSensor {

 public:
   FloatSensor();

   void set(float);

 private:
   float thevalue;

};

FloatSensor::FloatSensor() {
}

void FloatSensor::set(float ap) {
  thevalue=ap;
  this->emit(ap);
  }

///////////////////////////////
// SensEsp app below

ReactESP app([]() {


  u8g2.begin();



#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif

  sensesp_app = new SensESPApp();

// sensor to read pin and do something with the pin
   int read_delay = 10;
   String read_delay_config_path = "/button_watcher/read_delay";
   auto* button_watcher = new DigitalInputChange(D3, INPUT_PULLUP, CHANGE, read_delay, read_delay_config_path); 


/** 
 * Create the LambdaConsumer that calls reset_function, Because DigitalInputChange
 * outputs an int, the version of LambdaConsumer we need is LambdaConsumer<int>.
 * 
 * While this approach - defining the lambda function (above) separate from the
 * LambdaConsumer (below) - is simpler to understand, there is a more concise approach:
 */ 
  auto* button_consumer = new LambdaConsumer<int>([](int input) {
    if (1) {
      c++;
      handle_oled(c);
    }
  });


//auto* button_consumer = new LambdaConsumer<int>(reset_function);

/** 
 * Create a DebounceInt to make sure we get a nice, clean signal from the button.
 * Set the debounce delay period to 15 ms, which can be configured at debounce_config_path
 * in the Config UI.
*/

int debounce_delay = 15;
String debounce_config_path = "/debounce/delay";
auto* debounce = new DebounceInt(debounce_delay, debounce_config_path);


/* Connect the button_watcher to the debounce to the button_consumer. */
button_watcher->connect_to(button_consumer);


// sensor to send percentages to signalk

FloatSensor* water_level = new FloatSensor(); 
water_level->connect_to(	     
   new SKOutputNumber("tanks.freshWater.0.currentLevel"));	
	
    // If you wanted to add a third, fourth, or more sensor, you would do that
    // here. An ESP9266 should easily handle four or five sensors, and an ESP32
    // should handle eight or ten, or more.


// update display with liveness info

//  app.onRepeat(1000,[water_level] () {
//     handle_oled(c);
//     c++;
     //     water_level->set(c);
//  });

 handle_oled(0);

  sensesp_app->enable();
});
