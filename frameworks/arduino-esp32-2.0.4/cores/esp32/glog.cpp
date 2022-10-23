#include "gflags.h"
#include "glog/logging.h"

void initLogging(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  FLAGS_alsologtostderr=true;
  // google::InitGoogleLogging(argv[0]);
}
