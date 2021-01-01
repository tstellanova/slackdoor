/*
 * Project slackdoor
 * Description: Publishes to a Particle webhook when a doorbell is pressed.
 * Author: Todd Stellanova
 */

#include "Particle.h"

// We select this system mode in order to control _when_ the potentially
// lengthy network connection process runs.
SYSTEM_MODE(SEMI_AUTOMATIC);

const uint16_t USER_LED_PIN = D7; 
const uint16_t BUTTON_PIN = D2;
const String EMPTY_EVT_DATA = String(0);
static PinState g_user_led_state = LOW;
volatile bool g_button_pressed = false;

SerialLogHandler g_log_handler(LOG_LEVEL_WARN, { // Logging level for non-application messages
    { "app", LOG_LEVEL_INFO } // Logging level for application messages
});

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
  if (!Particle.connected()) {
    Particle.connect();
    while (!Particle.connected()) {
      togger_user_led_state();
      Particle.process();
      delay(100);
    }
  }

}

// Someone had pressed the doorbell: publish an event to the Particle Cloud
void publish_doorbell_event() {
  set_user_led_state(HIGH);

  wait_for_connection();

  // Trigger the webhook integration
  Particle.publish("household/frontdoor/bell02", EMPTY_EVT_DATA, PRIVATE);
  delay(50);

  indicate_doorbell_working();
}

// detect whether we're running on battery power
bool battery_powered() {
  #if(PLATFORM_ID == PLATFORM_ARGON)
    // check whether we have external (non-battery) power
    int usb_pwr_val = digitalRead(PWR);
    Log.info("usb_pwr_val: %d", usb_pwr_val);
    return (LOW == usb_pwr_val);
  #elif(PLATFORM_ID == PLATFORM_BORON)
    // check whether we are discharging the battery 
    int batt_state = System.batteryState();
    Log.info("batt_state: %d", batt_state);
    float batt_soc = System.batteryCharge();
    Log.info("soc: %.1f", batt_soc);
    return (BATTERY_STATE_DISCHARGING == batt_state);
  #else
  #error "Unsupported platform"
  #endif
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

  #if(PLATFORM_ID == PLATFORM_ARGON)
  pinMode(PWR, INPUT);
  #endif

  Log.info(" -- BEGIN --");
}

// Runs forever after setup()
void loop() {
  set_user_led_state(LOW);
  
  if (battery_powered())  {
    // cleanup interrupt in case it's previosly attached
    detachInterrupt(BUTTON_PIN);

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
      g_button_pressed = false;
      publish_doorbell_event();
    }
    else {
      // detect when button is pressed
      attachInterrupt(BUTTON_PIN, isr_button_pressed, FALLING);
      // ensure that we're connected to the network, awaiting updates
      wait_for_connection();
      indicate_doorbell_idle();
    }
  }


}
      
