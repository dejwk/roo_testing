// Host adapter for the Arduino-ESP32 SPIFFS facade.

#include "SPIFFS.h"

#include <cstdlib>
#include <cstring>

#include "vfs_api.h"

using namespace fs;

namespace {

class HostSpiffsImpl : public VFSImpl {
public:
  bool exists(const char *path) override {
    File file = open(path, FILE_READ, false);
    return static_cast<bool>(file) && !file.isDirectory();
  }
};

} // namespace

SPIFFSFS::SPIFFSFS()
    : FS(FSImplPtr(new HostSpiffsImpl())), partitionLabel_(nullptr) {}

SPIFFSFS::~SPIFFSFS() {
  free(partitionLabel_);
  partitionLabel_ = nullptr;
}

bool SPIFFSFS::begin(bool format_on_fail, const char *base_path,
                     uint8_t max_open_files, const char *partition_label) {
  (void)format_on_fail;
  (void)max_open_files;
  free(partitionLabel_);
  partitionLabel_ =
      partition_label == nullptr ? nullptr : strdup(partition_label);
  _impl->mountpoint(base_path);
  return true;
}

void SPIFFSFS::end() { _impl->mountpoint(nullptr); }

bool SPIFFSFS::format() { return true; }

size_t SPIFFSFS::totalBytes() { return 0; }

size_t SPIFFSFS::usedBytes() { return 0; }

SPIFFSFS SPIFFS;
