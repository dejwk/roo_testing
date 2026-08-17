extern "C" int roo_testing_arduino_select_probe();

int main() { return roo_testing_arduino_select_probe() == 10819 ? 0 : 1; }
