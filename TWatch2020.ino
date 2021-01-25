// An Arduino based framework for the Lilygo T-Watch 2020
// Much of the code is based on the sample apps for the
// T-watch that were written and copyrighted by Lewis He.
//(Copyright (c) 2019 lewis he)

#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include <soc/rtc.h>
#include "esp_wifi.h"
#include "esp_sleep.h"
#include <WiFi.h>


#define G_EVENT_VBUS_PLUGIN         _BV(0)
#define G_EVENT_VBUS_REMOVE         _BV(1)
#define G_EVENT_CHARGE_DONE         _BV(2)

#define G_EVENT_WIFI_SCAN_START     _BV(3)
#define G_EVENT_WIFI_SCAN_DONE      _BV(4)
#define G_EVENT_WIFI_CONNECTED      _BV(5)
#define G_EVENT_WIFI_BEGIN          _BV(6)
#define G_EVENT_WIFI_OFF            _BV(7)

enum {
    Q_EVENT_WIFI_SCAN_DONE,
    Q_EVENT_WIFI_CONNECT,
    Q_EVENT_BMA_INT,
    Q_EVENT_AXP_INT,
} ;

#define DEFAULT_SCREEN_TIMEOUT  30*1000

#define WATCH_FLAG_SLEEP_MODE   _BV(1)
#define WATCH_FLAG_SLEEP_EXIT   _BV(2)
#define WATCH_FLAG_BMA_IRQ      _BV(3)
#define WATCH_FLAG_AXP_IRQ      _BV(4)

QueueHandle_t g_event_queue_handle = NULL;
EventGroupHandle_t g_event_group = NULL;
EventGroupHandle_t isr_group = NULL;
bool lenergy = false;
TTGOClass *ttgo;

uint32_t targetTime = 0;       // for next 1 second display update
uint32_t loopCounter = 0;
// uint32_t clockUpTime = 0;      // track the time the clock is displayed

uint8_t hh, mm, ss, mmonth, dday; // H, M, S variables
uint16_t yyear; // Year is 16 bit int

void setup() {
  Serial.begin(115200);

  //Create a program that allows the required message objects and group flags
  g_event_queue_handle = xQueueCreate(20, sizeof(uint8_t));
  g_event_group = xEventGroupCreate();
  isr_group = xEventGroupCreate();
  
  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  
  ttgo->tft->setTextFont(1);
  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK); // Note: the new fonts do not draw the background colour
  
  // Turn on the IRQ used
  ttgo->power->adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);
  ttgo->power->enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);
  ttgo->power->clearIRQ();

  // Turn off unused power
  ttgo->power->setPowerOutPut(AXP202_EXTEN, AXP202_OFF);
  ttgo->power->setPowerOutPut(AXP202_DCDC2, AXP202_OFF);
  ttgo->power->setPowerOutPut(AXP202_LDO3, AXP202_OFF);
  ttgo->power->setPowerOutPut(AXP202_LDO4, AXP202_OFF);
  
  //Initialize lvgl
  ttgo->lvgl_begin();

  // Enable BMA423 interrupt ，
  // The default interrupt configuration,
  // you need to set the acceleration parameters, please refer to the BMA423_Accel example
  ttgo->bma->attachInterrupt();

  //Connection interrupted to the specified pin
  pinMode(BMA423_INT1, INPUT);
  attachInterrupt(BMA423_INT1, [] {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventBits_t  bits = xEventGroupGetBitsFromISR(isr_group);
    if (bits & WATCH_FLAG_SLEEP_MODE)
    {
        //! For quick wake up, use the group flag
        xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_BMA_IRQ, &xHigherPriorityTaskWoken);
    } else
    {
        uint8_t data = Q_EVENT_BMA_INT;
        xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
    }

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR ();
    }
  }, RISING);

  // Connection interrupted to the specified pin
  pinMode(AXP202_INT, INPUT);
  attachInterrupt(AXP202_INT, [] {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventBits_t  bits = xEventGroupGetBitsFromISR(isr_group);
    if (bits & WATCH_FLAG_SLEEP_MODE)
    {
        //! For quick wake up, use the group flag
        xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_AXP_IRQ, &xHigherPriorityTaskWoken);
    } else
    {
        uint8_t data = Q_EVENT_AXP_INT;
        xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
    }
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR ();
    }
  }, FALLING);

  //Check if the RTC clock matches, if not, use compile time
  ttgo->rtc->check();

  //Synchronize time to system time
  ttgo->rtc->syncToSystem();
  
  displayTime(true); // Our GUI to show the time
  ttgo->openBL(); // Turn on the backlight

  //vibration motor
  pinMode(4, OUTPUT);

}

void quickBuzz() {
  digitalWrite(4, HIGH);
  delay(200);
  digitalWrite(4, LOW);
}

void loop() {
  loopCounter++;
  if (targetTime < millis()) {
    targetTime = millis() + 1000;
    displayTime(ss == 0); // Call every second but only update time every minute
  }

  int16_t x, y;
  if (ttgo->getTouch(x, y)) {
    while (ttgo->getTouch(x, y)) {} // wait for user to release

    // This is where the app selected from the menu is launched
    // If you add an app, follow the variable update instructions
    // at the beginning of the menu code and then add a case
    // statement on to this switch to call your paticular
    // app routine.

    switch (modeMenu()) { // Call modeMenu. The return is the desired app number
      case 0: // Zero is the clock, just exit the switch
        break;
      case 1:
        jSats();
        break;
      case 2:
        appAccel();
        break;
      case 3:
        appBattery();
        break;
      case 4:
        appTouch();
        break;
      case 5:
        appSetTime();
        break;
    }
    loopCounter++;
    displayTime(true);
  }
}
