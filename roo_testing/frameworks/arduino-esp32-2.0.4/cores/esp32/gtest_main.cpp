#include <atomic>
#include <iostream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "Arduino.h"

#if (ARDUINO_USB_CDC_ON_BOOT|ARDUINO_USB_MSC_ON_BOOT|ARDUINO_USB_DFU_ON_BOOT) && !ARDUINO_USB_MODE
#include "USB.h"
#if ARDUINO_USB_MSC_ON_BOOT
#include "FirmwareMSC.h"
#endif
#endif

TaskHandle_t loopTaskHandle = NULL;

bool loopTaskWDTEnabled;

void loopTask(void *pvParameters) {
  exit(RUN_ALL_TESTS());
}

extern "C" void app_main()
{
#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
    Serial.begin();
#endif
#if ARDUINO_USB_MSC_ON_BOOT && !ARDUINO_USB_MODE
    MSC_Update.begin();
#endif
#if ARDUINO_USB_DFU_ON_BOOT && !ARDUINO_USB_MODE
    USB.enableDFU();
#endif
#if ARDUINO_USB_ON_BOOT && !ARDUINO_USB_MODE
    USB.begin();
#endif
    loopTaskWDTEnabled = false;
    initArduino();
    xTaskCreateUniversal(loopTask, "loopTask", 65536, NULL, 1, &loopTaskHandle, ARDUINO_RUNNING_CORE);
}

// #endif

extern "C" void call_start_cpu0();

int main(int argc, char* argv[]) {
    call_start_cpu0();
    testing::InitGoogleMock(&argc, argv);
    app_main();
    vTaskStartScheduler();
    return 0;
}
