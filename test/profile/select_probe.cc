extern "C" int roo_testing_arduino_optional_dependency();

extern "C" int roo_testing_arduino_select_probe() {
  return roo_testing_arduino_optional_dependency();
}
