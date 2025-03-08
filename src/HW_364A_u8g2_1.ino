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

ReactESP app([]() {

  u8g2.begin();

  app.onRepeat(1000,[] () {
  handle_oled(c);
  c++;
  });


#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif

  sensesp_app = new SensESPApp();

  // Everything above this line is boilerplate, except for the #include statements

  // This is the beginning of the code to read the SHT31 temperature and humidity
  // sensor.

  auto* sht31 = new SHT31(0x45);
  const uint sht31_read_delay = 1000;

  auto* sht31_temperature =
      new SHT31Value(sht31, SHT31Value::temperature, sht31_read_delay, "/fridge/temperature");
  sht31_temperature->connect_to(
      new SKOutputNumber("environment.inside.refrigerator.temperature"));

  auto* sht31_humidity =
      new SHT31Value(sht31, SHT31Value::humidity, sht31_read_delay, "/fridge/humidity");
  sht31_humidity->connect_to(
      new SKOutputNumber("environment.inside.refrigerator.humidity"));

   // This is the end of the SHT31 code and the beginning of the RPM counter code. Notice
   // that nothing special has to be done - just start writing the code for the next sensor.
   // You'll notice slight differences in code styles - for example, the code above
   // defines the config paths "in place", and the code below defines them with a variable,
   // then uses the variable as the function parameter. Either approach works.  

  const char* sk_path = "propulsion.main.revolutions";
  const char* config_path_calibrate = "/sensors/engine_rpm/calibrate";
  const char* config_path_skpath = "/sensors/engine_rpm/sk";
  const float multiplier = 1.0 / 97.0;
  const uint rpm_read_delay = 500;

#ifdef ESP8266
  uint8_t pin = D5;  
#elif defined(ESP32)
  uint8_t pin = 4;
#endif
  auto* dic = new DigitalInputCounter(pin, INPUT_PULLUP, RISING, rpm_read_delay);

  dic
      ->connect_to(new Frequency(multiplier, config_path_calibrate)) 
      ->connect_to(new SKOutputNumber(sk_path, config_path_skpath));

    // This is the end of the RPM counter code.

    // If you wanted to add a third, fourth, or more sensor, you would do that
    // here. An ESP9266 should easily handle four or five sensors, and an ESP32
    // should handle eight or ten, or more.
    
  sensesp_app->enable();
});
