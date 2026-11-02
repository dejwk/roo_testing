#include <atomic>
#include <cstdio>

#include "FreeRTOS.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "pico/mutex.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/buses/uart/fake_uart.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/system/timer.h"
#include "runtime.h"
#include "semphr.h"
#include "task.h"

namespace {
class Echo : public SimpleFakeSpiDevice {
 public:
  Echo() : SimpleFakeSpiDevice("pico_freertos_echo") {}
  void transfer(const FakeSpiInterface&, uint8_t* bytes,
                uint16_t bits) override {
    configASSERT(isSelected());
    for (uint16_t i = 0; i < bits / 8; ++i) bytes[i] ^= 0xff;
    ++calls;
  }
  int calls = 0;
};
Echo echoes[2];
FakeUartCable cable;
std::atomic<int> result{1};
SemaphoreHandle_t done;
SemaphoreHandle_t start;
mutex_t mutex;
bool mutex_acquired = false;
int64_t finished[2];

void MutexWorker(void*) {
  mutex_enter_blocking(&mutex);
  mutex_acquired = true;
  mutex_exit(&mutex);
  vTaskSuspend(nullptr);
}

void Writer(void*) {
  vTaskDelay(2);
  const uint8_t byte = 0x5a;
  uart_write_blocking(uart1, &byte, 1);
  vTaskSuspend(nullptr);
}

void Reader(void*) {
  uint8_t byte;
  uart_read_blocking(uart0, &byte, 1);
  configASSERT(false);
}

void SpiWorker(void* arg) {
  const intptr_t index = reinterpret_cast<intptr_t>(arg);
  xSemaphoreTake(start, portMAX_DELAY);
  const uint cs = index == 0 ? 17 : 12;
  gpio_put(cs, false);
  uint8_t tx[32] = {};
  uint8_t rx[32];
  configASSERT(spi_write_read_blocking(index == 0 ? spi0 : spi1, tx, rx, 32) ==
               32);
  gpio_put(cs, true);
  configASSERT(rx[0] == 0xff && rx[31] == 0xff);
  finished[index] = system_time_get_micros();
  xSemaphoreGive(done);
  vTaskSuspend(nullptr);
}

// Verifies actual SDK mutex contention, a task-fed UART read, cancellation of
// a blocked reader, and independent SPI controllers progressing during a wait.
void Test(void*) {
  mutex_init(&mutex);
  mutex_enter_blocking(&mutex);
  configASSERT(!mutex_try_enter(&mutex, nullptr));
  TaskHandle_t worker;
  configASSERT(xTaskCreate(MutexWorker, "mutex", 8192, nullptr, 3, &worker) ==
               pdPASS);
  configASSERT(!mutex_acquired);
  system_time_delay_micros(100);
  mutex_exit(&mutex);
  configASSERT(mutex_acquired);
  vTaskDelete(worker);

  configASSERT(xTaskCreate(Writer, "uart_writer", 8192, nullptr, 3, &worker) ==
               pdPASS);
  uint8_t byte = 0;
  uart_read_blocking(uart0, &byte, 1);
  configASSERT(byte == 0x5a);
  vTaskDelete(worker);
  configASSERT(xTaskCreate(Reader, "uart_reader", 8192, nullptr, 3, &worker) ==
               pdPASS);
  vTaskDelete(worker);
  system_time_delay_micros(10);

  done = xSemaphoreCreateCounting(2, 0);
  start = xSemaphoreCreateCounting(2, 0);
  TaskHandle_t spi_workers[2];
  for (intptr_t i = 0; i < 2; ++i) {
    configASSERT(xTaskCreate(SpiWorker, "spi", 16384,
                             reinterpret_cast<void*>(i), 3,
                             &spi_workers[i]) == pdPASS);
  }
  vTaskSuspendAll();
  xSemaphoreGive(start);
  xSemaphoreGive(start);
  xTaskResumeAll();
  xSemaphoreTake(done, portMAX_DELAY);
  xSemaphoreTake(done, portMAX_DELAY);
  configASSERT(finished[1] < finished[0]);
  configASSERT(echoes[0].calls == 1 && echoes[1].calls == 1);
  vTaskDelete(spi_workers[0]);
  vTaskDelete(spi_workers[1]);
  vSemaphoreDelete(start);
  vSemaphoreDelete(done);
  result.store(0);
  RooFreeRtosEndScheduler();
}
}  // namespace

int main() {
  FakePico().attachUartDevice(cable.end_a(), 0, 1);
  FakePico().attachUartDevice(cable.end_b(), 4, 5);
  uart_init(uart0, 115200);
  uart_init(uart1, 115200);
  for (uint pin : {0, 1, 4, 5}) gpio_set_function(pin, GPIO_FUNC_UART);
  FakePico().attachSpiDevice(echoes[0], 18, 16, 19);
  FakePico().attachSpiDevice(echoes[1], 10, 8, 11);
  FakePico().attachOutput(17, echoes[0].cs());
  FakePico().attachOutput(12, echoes[1].cs());
  spi_init(spi0, 10000);
  spi_init(spi1, 1000000);
  for (uint pin : {18, 16, 19, 10, 8, 11})
    gpio_set_function(pin, GPIO_FUNC_SPI);
  for (uint pin : {17, 12}) {
    gpio_init(pin);
    gpio_put(pin, true);
    gpio_set_dir(pin, GPIO_OUT);
  }
  TaskHandle_t task;
  configASSERT(xTaskCreate(Test, "peripheral", 16384, nullptr, 1, &task) ==
               pdPASS);
  vTaskStartScheduler();
  RooFreeRtosDeleteFinalTask();
  while (!TryBeginSystemTimeServiceShutdownForHost()) {
  }
  FinishSystemTimeServiceShutdownForHost();
  std::puts("Pico FreeRTOS peripheral tests passed");
  return result.load();
}
