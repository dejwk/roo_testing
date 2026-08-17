extern "C" int roo_testing_idf_capability_dependency();
extern "C" int roo_testing_idf_frontend_dependency();

extern "C" int roo_testing_idf_select_probe() {
  return roo_testing_idf_capability_dependency() +
         roo_testing_idf_frontend_dependency();
}
