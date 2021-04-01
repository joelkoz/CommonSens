#include <Arduino.h>

#include "sensesp_app.h"
#include "sensesp_app_builder.h"
#include "sensors/digital_input.h"
#include "sensors/digital_output.h"
#include "controllers/smart_switch_controller.h"
#include "signalk/signalk_listener.h"
#include "signalk/signalk_output.h"
#include "signalk/signalk_put_request_listener.h"
#include "signalk/signalk_value_listener.h"
#include "transforms/click_type.h"
#include "transforms/repeat_report.h"
#include "transforms/press_repeater.h"


#ifdef SHELLY1
// Set up the GPIO pin mappings specific to the Shelly 1
int pin_button[1] = {  5  };
int pin_relay[1] = { 4  };
int pin_mode[1] = { INPUT };
const int CHANNEL_COUNT = 1;
const int OFF_VALUE = 0;
const bool auto_init_controller = true;
const char* HOST_NAME = "sk-shelly1";

// Define the SK Path that represents the load this channel controls.
// This device will report its status on this path, as well as
// respond to PUT requests to change its status.
const char* sk_path_t = "electrical.switches.shelly1.light%d.state";
#endif

#ifdef SHELLY25
// Set up the GPIO pin mappings specific to the Shelly 2.5
int pin_button[2] = {  5, 13  };
int pin_relay[2] = { 4, 15  };
int pin_mode[2] = { INPUT, INPUT };
const int CHANNEL_COUNT = 2;
const int OFF_VALUE = 0;
const bool auto_init_controller = true;
const char* HOST_NAME = "sk-shelly25";
const char* sk_path_t = "electrical.switches.shelly25.light%d.state";
#endif

#ifdef SONOFF4CH
int pin_button[4] = {  0, 9, 10, 14 };
int pin_relay[4] = { 12, 5, 4, 15 };
int pin_mode[4] = { INPUT, INPUT, INPUT, INPUT };
const int CHANNEL_COUNT = 4;
const int OFF_VALUE = 1;
const bool auto_init_controller = true;
const char* HOST_NAME = "sk-4chpro";
const char* sk_path_t = "electrical.switches.sonoff4ch.light%d.state";
#endif


#ifdef M5STICK
#include "display/m5_display_bool.h"

// Set up the GPIO pin mappings specific to the Shelly 1
int pin_button[1] = {  M5_BUTTON_HOME  };
int pin_relay[1] = { -1 };
int pin_mode[1] = { INPUT };
const int CHANNEL_COUNT = 1;
const int OFF_VALUE = 1;
const bool auto_init_controller = false;
const char* HOST_NAME = "sk-m5stick";

// Define the SK Path that represents the load this channel controls.
// This device will report its status on this path, as well as
// respond to PUT requests to change its status.
const char* sk_path_t = "electrical.switches.shelly1.light%d.state";
#endif


#ifdef COMMON_SENSE_D1_MINI
// Set up the GPIO pin mappings specific to the CommonSense Smart Switch
int pin_button[2] = {  D7, D5  };
int pin_relay[2] = { D8, D6  };
int pin_mode[2] = { INPUT_PULLUP, INPUT_PULLUP };
const int CHANNEL_COUNT = 2;
const int OFF_VALUE = 1;
const bool auto_init_controller = true;
const char* HOST_NAME = "sk-cs-switch";
const char* sk_path_t = "electrical.switches.commonsense.load%d.state";
#endif



// A helper function that formats a parameter template to
// a unique channel specific String. This allows each of
// multiple channels to have their own unique configuration path
String get_param(const char* param_template, int channel) {
    char str[1024];
    sprintf(str, param_template, channel);
    return String(str);
};


// SensESP builds upon the ReactESP framework. Every ReactESP application
// defines an "app" object (vs. defining a "main()" method).
ReactESP app([]() {

#ifdef M5STICK
   M5.begin();
   M5.Lcd.setRotation(3);
#endif

// Some initialization boilerplate when in debug mode...
#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif

  // Create a builder object
  SensESPAppBuilder builder;


  // Create the global SensESPApp() object.
  sensesp_app = builder.set_hostname(HOST_NAME)
//                    ->set_sk_server("sk-server.local", 3000)
//                    ->set_wifi("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD")
                    ->set_sk_server("192.168.0.147", 80)
                    ->set_wifi("eSeaStreet", "rrrybgdts")
                    ->set_standard_sensors(StandardSensors::NONE)
                    ->get_app();



  // "Configuration paths" are combined with "/config" to formulate a URL
  // used by the RESTful API for retrieving or setting configuration data.
  // It is ALSO used to specify a path to the SPIFFS file system
  // where configuration data is saved on the MCU board.  It should
  // ALWAYS start with a forward slash if specified.  If left blank,
  // that indicates a sensor or transform does not have any
  // configuration to save.
  const char* config_path_button_t = "/button%d/clicktime";
  const char* config_path_sk_output_t = "/button%d/signalk/path";
  const char* config_path_sk_sync_t = "/button%d/signalk/sync";
  const char* config_path_repeat_t = "/button%d/signalk/repeat";

  for (int ch = 0; ch < CHANNEL_COUNT; ch++) {

      // Create a switch controller to handle the user press logic and 
      // connect it to the load switch...
      const char* sk_sync_paths[] = { "sk.path1", "sk.path2", "" };
      SmartSwitchController* controller = new SmartSwitchController(auto_init_controller, get_param(config_path_sk_sync_t, ch), sk_sync_paths);


      // Connect the channel's physical button that will feed manual click types into the controller...
      DigitalInputState* btn = new DigitalInputState(pin_button[ch], pin_mode[ch], CHANGE, 100);
      PressRepeater* pr = new PressRepeater("", OFF_VALUE);
      btn->connect_to(pr);
      pr->connect_to(new ClickType(get_param(config_path_button_t, ch)))
        ->connect_to(controller);


      if (pin_relay[ch] >= 0) {
        // Create a digital output that is connected to the relay for this channel.
        auto* load_switch = new DigitalOutput(pin_relay[ch]);
        controller->connect_to(load_switch);

        // Connect the relay to an SKOutput so it reports its state 
        // to the Signal K server.  Since the load switch only reports its state 
        // whenever it changes (and switches like light switches change infrequently), 
        // send it through a `RepeatReport` transform, which will cause the state 
        // to be reported to the server every 10 seconds, regardless of whether 
        // or not it has changed.  That keeps the value on the server fresh and 
        // lets the server know the switch is still alive.
        auto* sk_output = new SKOutputBool(get_param(sk_path_t, ch), get_param(config_path_sk_output_t, ch));
        load_switch->connect_to(new RepeatReport<bool>(10000, get_param(config_path_repeat_t, ch)))
                  ->connect_to(sk_output);


        // In addition to the manual button "click types", a 
        // SmartSwitchController accepts explicit state settings via 
        // any boolean producer as well as any "truth" values in human readable 
        // format via a String producer.
        // Here, we set up a SignalK PUT request listener to handle
        // requests made to the Signal K server to set the switch state.
        // This allows any device on the SignalK network that can make
        // such a request to also control the state of our switch.
        auto* sk_listener = new SKStringPutRequestListener(sk_output->get_sk_path());
        sk_listener->connect_to(controller);
      }
      else {
        // This "channel" is a virtual (remote) switch.
        
        // Output of the controller sends a PUT request to the remote switch...
        SKBooleanPutRequest* pRemoteSwitch = new SKBooleanPutRequest(get_param(sk_path_t, ch), get_param(config_path_sk_output_t, ch));
        controller->connect_to(pRemoteSwitch);

        // The remote switch's reported value sets the current state of the controller...
        auto* sk_listener = new SKValueListener<bool>(pRemoteSwitch->get_sk_path());
        sk_listener->connect_to(controller);        
      }

#ifdef M5STICK
      // Also connect the controller to an onboard m5Display...
      controller->connect_to(new M5DisplayBool());
#endif

  }

  // Start the SensESP application running
  sensesp_app->enable();

});
