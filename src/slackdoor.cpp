/*
 * Project slackdoor
 * Description: Publishes to a Particle webhook when a doorbell is pressed.
 * Author: Todd Stellanova
 */

#include <Particle.h>

// We select this system mode in order to control _when_ the potentially
// lengthy network connection process runs.
SYSTEM_MODE(SEMI_AUTOMATIC);

const uint16_t g_user_led = D7; 
const uint16_t g_button_input = D2;

// setup() runs once, when the device is first turned on.
void setup() {
  // Configure button input pin as pullup-- when switch is clicked it will fall to ground
  pinMode(g_user_led, OUTPUT);
  pinMode(g_button_input, INPUT_PULLUP);
}

// runs forever after setup
void loop() {
  PinState cur_led_state = LOW;
  digitalWrite(g_user_led, cur_led_state);

  // sleep until the doorbell is pressed
  SystemSleepConfiguration sleep_cfg;
  sleep_cfg.mode(SystemSleepMode::ULTRA_LOW_POWER)
        .gpio(g_button_input, FALLING);
  SystemSleepResult result = System.sleep(sleep_cfg);

  // TODO check whether we awoke specifically due to button click
  // (sleep may end for other reasons)
  cur_led_state = HIGH;
  digitalWrite(g_user_led, HIGH);
  // provide a little time to catch a DFU over USB
  delay(100);


  // Regardless of the network technology, it takes some time to connect to the Particle cloud:
  // blink the LED while connecting
  if (!Particle.connected()) {
    Particle.connect();
    while (!Particle.connected()) {
      cur_led_state = (HIGH == cur_led_state) ? LOW : HIGH;
      digitalWrite(g_user_led, cur_led_state);
      Particle.process();
      delay(100);
    }
  }

  // Placeholder data (unused)
  String data = String(2);
  // Trigger the webhook integration
  Particle.publish("evt_door", data, PRIVATE);
  delay(50);

  // blink the LED to indicate the bell has been rung
  for (int i = 0; i < 30; i++) {
    Particle.process();
    delay(500);
    cur_led_state = (HIGH == cur_led_state) ? LOW : HIGH;
    digitalWrite(g_user_led, cur_led_state);
  }

  // gracefully shutdown the cloud connection
  Particle.disconnect();
}
      
