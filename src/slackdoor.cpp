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
static PinState g_user_led_state = LOW;
volatile bool g_button_pressed = false;


void set_user_led_state(PinState mode) {
    g_user_led_state = mode;
    digitalWrite(USER_LED_PIN, g_user_led_state);
}

void togger_user_led_state() {
    PinState next_state = (HIGH == g_user_led_state) ? LOW : HIGH;
    set_user_led_state(next_state);
}

// Blink the LED quickly to indicate the bell has been rung
void indicate_doorbell_working() {
  for (int i = 0; i < 30; i++) {
    Particle.process();
    delay(500);
    togger_user_led_state();
  }
}

// Blink the LED slowly to indicate we're waiting for a doorbell press
void indicate_doorbell_idle() {
  for (int i = 0; i < 2; i++) {
    togger_user_led_state();
    delay(1000);
  }
}

// Regardless of the network technology, it takes some time to connect to the Particle cloud:
// blink the LED while connecting
void wait_for_connection() {
  while (!Particle.connected()) {
    togger_user_led_state();
    Particle.process();
    delay(100);
  }
}

// Someone had pressed the doorbell: publish an event to the Particle Cloud
void publish_doorbell_event() {
  set_user_led_state(HIGH);

  if (!Particle.connected()) {
    Particle.connect();
    wait_for_connection();
  }

  // Trigger the webhook integration
  Particle.publish("evt_door", EMPTY_EVT_DATA, PRIVATE);
  delay(50);

  indicate_doorbell_working();
}

// Interrupt service routine called when button is pressed:
// We don't do much processing here, just note that the button was pressed
void isr_button_pressed() {
  g_button_pressed = true;
}

// setup() runs once, when the device is first turned on.
void setup() {
  pinMode(USER_LED_PIN, OUTPUT);
  // Configure button input pin as pullup-- when button is pressed it will fall to ground
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

// Runs forever after setup()
void loop() {
  g_user_led_state = LOW;
  digitalWrite(USER_LED_PIN, g_user_led_state);
  
  int32_t powerSource = DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_POWER_SOURCE);
  if (POWER_SOURCE_BATTERY == powerSource) {
    // cleanup interrupt in case it's previosly attached
    detachInterrupt(BUTTON_PIN);

    // sleep until the doorbell is pressed
    SystemSleepConfiguration sleep_cfg;
    sleep_cfg.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .network(9)
          .gpio(BUTTON_PIN, FALLING);
    SystemSleepResult result = System.sleep(sleep_cfg);

    // check whether we awoke specifically due to button click
    if (SystemSleepWakeupReason::BY_GPIO == result.wakeupReason()) {
      publish_doorbell_event();
    }
  }
  else {
    if (g_button_pressed) {
      g_button_pressed = false;
      publish_doorbell_event();
    }
    else {
      // detect when button is pressed
      attachInterrupt(BUTTON_PIN, isr_button_pressed, FALLING);
      indicate_doorbell_idle();
    }
  }


}
      
