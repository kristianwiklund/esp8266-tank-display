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

/////////////////////////////////
// configurations

const int broadcast_interval=10; // seconds between transmits

int pins[] = {D1,D2,D3}; // array of input pins
const int numpins = sizeof(pins)/sizeof(int);

int inputs[numpins];



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

/////////////////////////////////////////////////////////
// calculate tank level based on which sensor was trigged

float calclevel(int value, unsigned int sensor) {

  Serial.print("calclevel called with value ");
  Serial.print(value);
  Serial.print(" from pin id ");
  Serial.println(sensor);
  
  inputs[sensor]=value;

  Serial.println("inputs array set successfully, going to iterator");
  
  for (int i=numpins;i>0;i--) {
    Serial.println(i);
    if(inputs[i-1]) {
      Serial.print("Returning value=");
      Serial.println((1.0*i)/numpins);
      Serial.println("=======================");
      return ((1.0*i)/numpins);
    }
  }
  Serial.println("Returning 0.0");
  return 0.0;
}

///////////////////////////////
// SensEsp app below

ReactESP app([]() {
  float lvl;
  
  u8g2.begin();



#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif

  sensesp_app = new SensESPApp();
  
  // sensor to send percentages to signalk, send the volume once per minute
  // (the freshwater tank is not very time-sensitive, but we need an update more
  // often than the sensors trigger the measurements)
  
  FloatSensor* water_level = new FloatSensor(broadcast_interval*1000); 
  water_level->connect_to(new SKOutputNumber("tanks.freshWater.0.currentLevel"));	

  /////////////////////////////////////////////////////////////
  // start at zero then go up once we start reading the sensors
  Serial.println("Clearing display and old readings in SignalK");
  water_level->set(0.0);
  handle_oled(0);
  
  ////////////////////////////////////////////
  // instantiate sensors to read the input pins
  // store the sensors in an array of sensors watching the pins

  
  DigitalInputChange *bws[numpins];

  for (unsigned int i=0;i<numpins;i++) {    
    int read_delay = 10;
    String read_delay_config_path = "/button_watcher/read_delay";
    bws[i] = new DigitalInputChange(pins[i], INPUT_PULLUP, CHANGE, read_delay, read_delay_config_path);
    inputs[i]=digitalRead(pins[i]);
    lvl = calclevel(inputs[i],i);
  }
  // and update with the current level
  
  handle_oled((int)(100*lvl));
  water_level->set(lvl);
  
    
  ///////////////////////////////////////
  // do things with the received pins
  /** Modified from SensESP example code:
   *
   * Create a LambdaConsumer that calls the update of the tank ratio, Because DigitalInputChange
   * outputs an int, the version of LambdaConsumer we need is LambdaConsumer<int>.
   */
  // create another array of consumers

  LambdaConsumer<int> *bcs[numpins];
  
  for (unsigned int i=0;i<numpins;i++) {

    // https://stackoverflow.com/questions/9554102/in-lambda-functions-syntax-what-purpose-does-a-capture-list-serve
    bcs[i] = new LambdaConsumer<int>([=,water_level,i](int input) {
      float lvl;
      Serial.print("Got something on pin id=");
      Serial.print(i);
      Serial.print(" which is pin ");
      Serial.print(pins[i]);
      
      lvl = calclevel(input,i);
      water_level->set(lvl);
      handle_oled((int)(100*lvl));
    });

    // connect the input to the function that do the magic
    bws[i]->connect_to(bcs[i]);
  }
    
  sensesp_app->enable();
});
