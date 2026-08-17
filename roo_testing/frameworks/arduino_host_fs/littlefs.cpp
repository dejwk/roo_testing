// Host adapter for the Arduino-ESP32 LittleFS facade.

#include "LittleFS.h"

#ifdef CONFIG_LITTLEFS_PAGE_SIZE

#include <cstdlib>
#include <cstring>

#include "vfs_api.h"

using namespace fs;

namespace {

class HostLittleFSImpl : public VFSImpl {};

} // namespace

LittleFSFS::LittleFSFS()
    : FS(FSImplPtr(new HostLittleFSImpl())), partitionLabel_(nullptr) {}

LittleFSFS::~LittleFSFS() {
  free(partitionLabel_);
  partitionLabel_ = nullptr;
}

bool LittleFSFS::begin(bool format_on_fail, const char *base_path,
                       uint8_t max_open_files, const char *partition_label) {
  (void)format_on_fail;
  (void)max_open_files;
  free(partitionLabel_);
  partitionLabel_ =
      partition_label == nullptr ? nullptr : strdup(partition_label);
  _impl->mountpoint(base_path);
  return true;
}

void LittleFSFS::end() { _impl->mountpoint(nullptr); }

bool LittleFSFS::format() { return true; }

size_t LittleFSFS::totalBytes() { return 0; }

size_t LittleFSFS::usedBytes() { return 0; }

LittleFSFS LittleFS;

#endif // CONFIG_LITTLEFS_PAGE_SIZE
