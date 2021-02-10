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
// this volatile state variable is shared between an interrupt handler and loop()
static volatile bool button_pressed = false;
static system_tick_t last_trigger_time = 0;


// Force the user LED to a particular state
void set_user_led_state(PinState mode) {
    user_led_state = mode;
    digitalWrite(USER_LED_PIN, user_led_state);
}

// Invert the user LED state
void toggle_user_led_state() {
    PinState next_state = (HIGH == user_led_state) ? LOW : HIGH;
    set_user_led_state(next_state);
}

// Blink the LED slowly to indicate we're waiting for a doorbell press
void indicate_doorbell_idle() {
    toggle_user_led_state();
    delay(500);
}




// Someone has pressed the doorbell button: publish an event to the Particle Cloud
void publish_doorbell_event() {
  // Indicate that we're contacting the cloud
  set_user_led_state(HIGH);

  // Trigger the webhook integration
  Particle.publish("household/frontdoor/bell01", EMPTY_EVT_DATA, PRIVATE);
  // Optional: send vital statistics to the cloud every time we get a doorbell button press
  Particle.publishVitals();
}

// Simulate a doorbell button press-- useful for development testing
// Cloud functions must return int and take one String
int fake_button_press(String extra) {
  last_trigger_time = millis();
  button_pressed = true;
  return 0;
}

// Interrupt service routine called when doorbell button is pressed:
// We don't do much processing here, just note that the button was pressed
void isr_button_pressed() {

  // Ideally we would just set button_pressed = true
  // and be done; however, real-world buttons may require 
  // "debouncing" to ensure we only recognize one button press
  // of a certain duration, within a certain time window.

  //verify that the button press duration is at least some bare minimum
  int32_t live_button = pinReadFast(DOORBELL_BUTTON_PIN);
  if (live_button == HIGH) {
    system_tick_t cur_time = millis();
    if ((cur_time - last_trigger_time) > BUTTON_DEBOUNCE_TIME) {
      last_trigger_time = cur_time;
      button_pressed = true;
      // Next, button_pressed should be detected in loop()
    }
  }
}

// Runs once, when the device is first turned on or reset. 
void setup() {
  // Configure the user LED pin as an output so we can light it up
  pinMode(USER_LED_PIN, OUTPUT);
  // Configure button input pin as pulldown:
  // It will be weakly pulled to ground (zero volts) by the Particle device,
  // and when the button is pressed it will connect to V+ and rise.
  // (This assumes the button connects between V+ and the input pin.)
  pinMode(DOORBELL_BUTTON_PIN, INPUT_PULLDOWN);

  // initial LED state
  set_user_led_state(LOW);

  // listen on button pin for button press events, 
  // where the voltage on the input pin rises 
  attachInterrupt(DOORBELL_BUTTON_PIN, isr_button_pressed, RISING);

  // for convenience, device a remotely-triggerable function that will fake a button press
  Particle.function("fakeButtonPress", fake_button_press);


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
      
