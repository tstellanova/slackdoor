/*
 * Project slackdoor
 * Description: Publishes to a Particle Cloud webhook when a doorbell button is pressed.
 * Author: Todd Stellanova
 */

#include "Particle.h"


const pin_t USER_LED_PIN = D7; 
const pin_t DOORBELL_BUTTON_PIN = D2;
const String EMPTY_EVT_DATA = String(0);
const system_tick_t BUTTON_DEBOUNCE_TIME = 3000;

static PinState user_led_state = LOW;
static volatile bool button_pressed = false;
static system_tick_t last_trigger_time = 0;

SerialLogHandler g_log_handler(LOG_LEVEL_WARN, { // Logging level for non-application messages
    { "app", LOG_LEVEL_INFO } // Logging level for application messages
});

// force the user LED to a particular state
void set_user_led_state(PinState mode) {
    user_led_state = mode;
    digitalWrite(USER_LED_PIN, user_led_state);
}

// invert the user LED state
void togger_user_led_state() {
    PinState next_state = (HIGH == user_led_state) ? LOW : HIGH;
    set_user_led_state(next_state);
}

// Blink the LED slowly to indicate we're waiting for a doorbell press
void indicate_doorbell_idle() {
    togger_user_led_state();
    delay(500);
}

// Someone has pressed the doorbell button: publish an event to the Particle Cloud
void publish_doorbell_event() {
  set_user_led_state(HIGH);

  // Trigger the webhook integration
  Particle.publish("household/frontdoor/bell01", EMPTY_EVT_DATA, PRIVATE);
  // send vital statistics to the cloud every time we get a doorbell button press
  Particle.publishVitals();
}

// Interrupt service routine called when doorbell button is pressed:
// We don't do much processing here, just note that the button was pressed
void isr_button_pressed() {
  //verify that the button press duration is at least some bare minimum
  int32_t live_button = pinReadFast(DOORBELL_BUTTON_PIN);
  if (live_button == HIGH) {
    system_tick_t cur_time = millis();
    if ((cur_time - last_trigger_time) > BUTTON_DEBOUNCE_TIME) {
      last_trigger_time = cur_time;
      button_pressed = true;
    }
  }
}

// Runs once, when the device is first turned on.
void setup() {
  pinMode(USER_LED_PIN, OUTPUT);
  // Configure button input pin as pulldown-- when button is pressed it will connect to +V and rise
  pinMode(DOORBELL_BUTTON_PIN, INPUT_PULLDOWN);

  // initial LED state
  set_user_led_state(LOW);

  // listen on button pin for button press events
  attachInterrupt(DOORBELL_BUTTON_PIN, isr_button_pressed, RISING);

}

// Runs forever after setup()
void loop() {

  if (button_pressed) {
    button_pressed = false;
    publish_doorbell_event();
  }
  else {
    // indicate that we're just idling, waiting for a button press
    indicate_doorbell_idle();
  }
  
}
      
