#include <U8g2lib.h>

#define OLED_RESET     U8X8_PIN_NONE // Reset pin (not used)
#define OLED_SDA 14                  // D6
#define OLED_SCL 12                  // D5

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RESET, OLED_SCL, OLED_SDA);

#define USE_LIB_WEBSOCKET true

#include "sensesp_app.h"
#include "sensors/sht31.h"
#include "sensors/digital_input.h"
#include "transforms/frequency.h"
#include "signalk/signalk_output.h"
#include "sensors/analog_input.h"
#include "transforms/debounce.h"

//////////////////////////////////////////
// draw a bar indicating the tank volume
// progress bar code from https://github.com/upiir/arduino_oled_lopaka/tree/main

void handle_oled(int progress) {
  char buffer[32];

  u8g2.clearBuffer();
  u8g2.setBitmapMode(1);
  u8g2.drawFrame(12, 21, 104, 20);

  u8g2.drawBox(14, 23, progress, 16); // draw the progressbar fill
  u8g2.setFont(u8g2_font_helvB08_tr);
  sprintf(buffer, "Level: %d%%", progress); // construct a string with the progress variable
  u8g2.drawStr(33, 53, buffer); // display the string
  u8g2.setFont(u8g2_font_haxrcorp4089_tr);
  u8g2.drawStr(0, 7, "Freshwater Tank");
  u8g2.drawLine(0, 9, 127, 9);
  u8g2.sendBuffer();
}


//////////////////////////////////////////
// sensor class to send a float to signalk
// * sends the value on "set", then
//   broadcasts the value on the specified
//   interval

class FloatSensor: public NumericSensor {

 public:
  FloatSensor();
  FloatSensor(unsigned int);
  
   void set(float);

 private:
   float thevalue;
  unsigned int resend_interval;
};

FloatSensor::FloatSensor() {
  this->resend_interval=0;
}

FloatSensor::FloatSensor(unsigned int resend_interval) {
  this->resend_interval=resend_interval;

  if (resend_interval) {
    app.onRepeat(resend_interval, [this]() {this->emit(this->thevalue);});
  }
}

void FloatSensor::set(float ap) {
  this->thevalue=ap;
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
  
  // sensor to send percentages to signalk, send the volume once per minute
  // (the freshwater tank is not very time-sensitive, but we need an update more
  // often than the sensors trigger the measurements)
  
  FloatSensor* water_level = new FloatSensor(60000); 
  water_level->connect_to(new SKOutputNumber("tanks.freshWater.0.currentLevel"));	
  
  ////////////////////////////////////////////
  // instantiate sensors to read the input pins
  // store the sensors in an array of sensors watching the pins
  
  int pins[] = {D3}; // array of input pins  
  DigitalInputChange *bws[sizeof(pins)];

  for (unsigned int i=0;i<sizeof(pins);i++) {    
    int read_delay = 10;
    String read_delay_config_path = "/button_watcher/read_delay";
    bws[i] = new DigitalInputChange(pins[i], INPUT_PULLUP, CHANGE, read_delay, read_delay_config_path); 
  }
    
  ///////////////////////////////////////
  // do things with the received pins
  /** Modified from SensESP example code:
   *
   * Create a LambdaConsumer that calls the update of the tank ratio, Because DigitalInputChange
   * outputs an int, the version of LambdaConsumer we need is LambdaConsumer<int>.
   */
  // create another array of consumers

  LambdaConsumer<int> *bcs[sizeof(pins)];
  
  for (unsigned int i=0;i<sizeof(pins);i++) {
    
    bcs[i] = new LambdaConsumer<int>([water_level](int input) {
      if (input==1) {
	water_level->set(1.0);
	handle_oled(100);
      } else {
	water_level->set(0.0);
	handle_oled(0);
      }
    });

    // connect the input to the function that do the magic
    bws[i]->connect_to(bcs[i]);
  }
  
  // display something
  handle_oled(0);
  
  sensesp_app->enable();
});
