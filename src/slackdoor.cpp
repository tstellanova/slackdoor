/*
 * Project slackdoor
 * Description: Publishes to a Particle webhook when a doorbell is pressed.
 * Author: Todd Stellanova
 */

#include "Particle.h"
#include "DiagnosticsHelperRK.h"
#include "system_power.h"

// We select this system mode in order to control _when_ the potentially
// lengthy network connection process runs.
SYSTEM_MODE(SEMI_AUTOMATIC);

const uint16_t USER_LED_PIN = D7; 
const uint16_t BUTTON_PIN = D2;
const String EMPTY_EVT_DATA = String(2);
static PinState g_cur_led_state = LOW;
volatile bool g_button_pressed = false;

void publish_doorbell_event() {
  g_cur_led_state = HIGH;
  digitalWrite(USER_LED_PIN, HIGH);
  // provide a little time to catch a DFU over USB
  delay(100);

  // Regardless of the network technology, it takes some time to connect to the Particle cloud:
  // blink the LED while connecting
  if (!Particle.connected()) {
    Particle.connect();
    while (!Particle.connected()) {
      g_cur_led_state = (HIGH == g_cur_led_state) ? LOW : HIGH;
      digitalWrite(USER_LED_PIN, g_cur_led_state);
      Particle.process();
      delay(100);
    }
  }

  // Trigger the webhook integration
  Particle.publish("evt_door", EMPTY_EVT_DATA, PRIVATE);
  delay(50);

  // blink the LED quickly to indicate the bell has been rung
  for (int i = 0; i < 30; i++) {
    Particle.process();
    delay(500);
    g_cur_led_state = (HIGH == g_cur_led_state) ? LOW : HIGH;
    digitalWrite(USER_LED_PIN, g_cur_led_state);
  }

  // gracefully shutdown the cloud connection
  Particle.disconnect();
}
// setup() runs once, when the device is first turned on.
void setup() {
  // Configure button input pin as pullup-- when switch is clicked it will fall to ground
  pinMode(USER_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

// Runs forever after setup()
void loop() {
  g_cur_led_state = LOW;
  digitalWrite(USER_LED_PIN, g_cur_led_state);
  
  int32_t powerSource = DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_POWER_SOURCE);
  if (POWER_SOURCE_BATTERY == powerSource) {
    // sleep until the doorbell is pressed
    SystemSleepConfiguration sleep_cfg;
    sleep_cfg.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .gpio(BUTTON_PIN, FALLING);
    SystemSleepResult result = System.sleep(sleep_cfg);

    // check whether we awoke specifically due to button click
    if (SystemSleepWakeupReason::BY_GPIO == result.wakeupReason()) {
      publish_doorbell_event();
    }
  }
  else {
    if (g_button_pressed) {
      publish_doorbell_event();
    }
    else {
      g_cur_led_state = HIGH;
      digitalWrite(USER_LED_PIN, g_cur_led_state);
      delay(1000);
      g_cur_led_state = LOW;
      digitalWrite(USER_LED_PIN, g_cur_led_state);
      delay(1000);
    }
  }


 


}
      
