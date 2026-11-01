cmake_minimum_required(VERSION 3.13)
# Supply pristine pinned checkouts, including ArduinoCore-API and lwIP.
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
project(roo_pico_target_qualification C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
if(NOT PICO_SDK_VERSION_STRING STREQUAL "2.3.1")
  message(FATAL_ERROR "Target qualification requires the pinned Pico SDK 2.3.1")
endif()
foreach(required ROO_ARDUINO_PICO_PATH ROO_LWIP_PATH ROO_CLIENT_ROOT)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "Set ${required}; see test/pico_target/README.md")
  endif()
endforeach()

add_executable(sdk_consumer sdk_consumer.cpp)
target_link_libraries(sdk_consumer pico_stdlib hardware_adc hardware_pwm
  hardware_spi hardware_i2c)

# Compile actual upstream Arduino headers, without native patches or ROO_TESTING.
get_filename_component(ROO_TESTING_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/peripherals.cpp"
  "#include \"${ROO_TESTING_ROOT}/examples/arduino_pico/Peripherals.ino\"\n")
add_library(arduino_consumer OBJECT "${CMAKE_CURRENT_BINARY_DIR}/peripherals.cpp")
target_compile_options(arduino_consumer PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
target_compile_definitions(arduino_consumer PRIVATE ARDUINO=10819
  ARDUINO_ARCH_RP2040=1 ARDUINO_RASPBERRY_PI_PICO=1 F_CPU=125000000L)
target_include_directories(arduino_consumer PUBLIC
  "${ROO_ARDUINO_PICO_PATH}/cores/rp2040"
  "${ROO_ARDUINO_PICO_PATH}/variants/rpipico"
  "${ROO_ARDUINO_PICO_PATH}/libraries/SPI/src"
  "${ROO_ARDUINO_PICO_PATH}/libraries/Wire/src"
  "${ROO_ARDUINO_PICO_PATH}/include"
  "${ROO_LWIP_PATH}/src/include"
  "${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include")
target_link_libraries(arduino_consumer PUBLIC pico_stdlib hardware_adc hardware_pwm
  hardware_spi hardware_i2c hardware_pio hardware_dma pico_multicore pico_rand
  pico_unique_id pico_util hardware_exception hardware_watchdog)

# Existing physical single-threaded mode is explicit; no fake runtime identity.
add_library(roo_consumers OBJECT roo_consumers.cpp)
target_compile_options(roo_consumers PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
target_compile_definitions(roo_consumers PRIVATE ARDUINO=10819
  ARDUINO_ARCH_RP2040=1 ARDUINO_RASPBERRY_PI_PICO=1 F_CPU=125000000L
  ROO_THREADS_SINGLETHREADED=1)
foreach(client roo_time roo_threads roo_scheduler roo_display roo_backport roo_io
    roo_collections roo_logging)
  target_include_directories(roo_consumers PRIVATE "${ROO_CLIENT_ROOT}/${client}/src")
endforeach()
target_link_libraries(roo_consumers PRIVATE arduino_consumer)
