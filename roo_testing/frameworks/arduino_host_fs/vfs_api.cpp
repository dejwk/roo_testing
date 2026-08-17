// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "vfs_api.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace fs;

// Implemented by the ESP32 board model and backed by FakeEsp32().fs_root().
const char *GetVfsRoot();

namespace {

constexpr size_t kDefaultFileBufferSize = 4096;

void AppendPath(std::string &result, const char *part) {
  if (part == nullptr || part[0] == '\0') {
    return;
  }
  if (result.empty()) {
    result = part;
    return;
  }
  const bool result_has_slash = result.back() == '/';
  const bool part_has_slash = part[0] == '/';
  if (result_has_slash && part_has_slash) {
    result.append(part + 1);
  } else {
    if (!result_has_slash && !part_has_slash) {
      result.push_back('/');
    }
    result.append(part);
  }
}

std::string HostPath(const char *mountpoint, const char *path) {
  const char *root = GetVfsRoot();
  if (root == nullptr || root[0] == '\0' || mountpoint == nullptr ||
      path == nullptr) {
    return {};
  }
  std::string result(root);
  AppendPath(result, mountpoint);
  AppendPath(result, path);
  return result;
}

} // namespace

FileImplPtr VFSImpl::open(const char *fpath, const char *mode,
                          const bool create) {
  if (_mountpoint == nullptr) {
    log_e("File system is not mounted");
    return FileImplPtr();
  }
  if (fpath == nullptr || fpath[0] != '/') {
    log_e("%s does not start with /", fpath == nullptr ? "(null)" : fpath);
    return FileImplPtr();
  }

  if (create && mode != nullptr && mode[0] != 'r') {
    const char *slash = strchr(fpath + 1, '/');
    while (slash != nullptr) {
      std::string folder(fpath, slash - fpath);
      if (!mkdir(folder.c_str())) {
        log_e("Creating folder: %s failed", folder.c_str());
        return FileImplPtr();
      }
      slash = strchr(slash + 1, '/');
    }
  }

  return std::make_shared<VFSFileImpl>(this, fpath,
                                       mode == nullptr ? FILE_READ : mode);
}

bool VFSImpl::exists(const char *fpath) {
  if (_mountpoint == nullptr) {
    log_e("File system is not mounted");
    return false;
  }
  VFSFileImpl file(this, fpath, FILE_READ);
  const bool result = static_cast<bool>(file);
  file.close();
  return result;
}

bool VFSImpl::rename(const char *path_from, const char *path_to) {
  if (_mountpoint == nullptr) {
    log_e("File system is not mounted");
    return false;
  }
  if (path_from == nullptr || path_from[0] != '/' || path_to == nullptr ||
      path_to[0] != '/') {
    log_e("bad arguments");
    return false;
  }
  if (!exists(path_from)) {
    log_e("%s does not exist", path_from);
    return false;
  }

  const std::string from = HostPath(_mountpoint, path_from);
  const std::string to = HostPath(_mountpoint, path_to);
  return !from.empty() && !to.empty() &&
         ::rename(from.c_str(), to.c_str()) == 0;
}

bool VFSImpl::remove(const char *fpath) {
  if (_mountpoint == nullptr) {
    log_e("File system is not mounted");
    return false;
  }
  if (fpath == nullptr || fpath[0] != '/') {
    log_e("bad arguments");
    return false;
  }

  VFSFileImpl file(this, fpath, FILE_READ);
  if (!file || file.isDirectory()) {
    file.close();
    log_e("%s does not exist or is a directory", fpath);
    return false;
  }
  file.close();

  const std::string path = HostPath(_mountpoint, fpath);
  return !path.empty() && ::unlink(path.c_str()) == 0;
}

bool VFSImpl::mkdir(const char *fpath) {
  if (_mountpoint == nullptr) {
    log_e("File system is not mounted");
    return false;
  }
  if (fpath == nullptr || fpath[0] != '/') {
    log_e("bad arguments");
    return false;
  }

  VFSFileImpl file(this, fpath, FILE_READ);
  if (file && file.isDirectory()) {
    file.close();
    return true;
  }
  if (file) {
    file.close();
    log_e("%s is a file", fpath);
    return false;
  }

  const std::string path = HostPath(_mountpoint, fpath);
  return !path.empty() && ::mkdir(path.c_str(), ACCESSPERMS) == 0;
}

bool VFSImpl::rmdir(const char *fpath) {
  if (_mountpoint == nullptr) {
    log_e("File system is not mounted");
    return false;
  }
  if (fpath == nullptr || fpath[0] != '/') {
    log_e("bad arguments");
    return false;
  }
  // SPIFFS has a flat namespace on the device.  Keep the previous emulator's
  // behavior even though the backing host filesystem supports directories.
  if (strcmp(_mountpoint, "/spiffs") == 0) {
    log_e("rmdir is unnecessary in SPIFFS");
    return false;
  }

  VFSFileImpl file(this, fpath, FILE_READ);
  if (!file || !file.isDirectory()) {
    file.close();
    log_e("%s does not exist or is a file", fpath);
    return false;
  }
  file.close();

  const std::string path = HostPath(_mountpoint, fpath);
  return !path.empty() && ::rmdir(path.c_str()) == 0;
}

VFSFileImpl::VFSFileImpl(VFSImpl *fs, const char *fpath, const char *mode)
    : _fs(fs), _f(nullptr), _d(nullptr), _path(nullptr), _isDirectory(false),
      _stat{}, _written(false) {
  if (fs == nullptr || fpath == nullptr) {
    return;
  }
  const std::string host_path = HostPath(_fs->_mountpoint, fpath);
  if (host_path.empty()) {
    return;
  }

  _path = strdup(fpath);
  if (_path == nullptr) {
    log_e("strdup(%s) failed", fpath);
    return;
  }

  const char *open_mode = mode == nullptr ? FILE_READ : mode;
  if (::stat(host_path.c_str(), &_stat) == 0) {
    if (S_ISREG(_stat.st_mode)) {
      _f = fopen(host_path.c_str(), open_mode);
      if (_f == nullptr) {
        log_e("fopen(%s) failed: %s", host_path.c_str(), strerror(errno));
      } else if (_stat.st_blksize == 0) {
        setvbuf(_f, nullptr, _IOFBF, kDefaultFileBufferSize);
      }
    } else if (S_ISDIR(_stat.st_mode)) {
      _isDirectory = true;
      _d = opendir(host_path.c_str());
      if (_d == nullptr) {
        _isDirectory = false;
        log_e("opendir(%s) failed: %s", host_path.c_str(), strerror(errno));
      }
    } else {
      log_e("Unsupported host file type for %s", host_path.c_str());
    }
    return;
  }

  if (open_mode[0] != 'r') {
    _f = fopen(host_path.c_str(), open_mode);
    if (_f == nullptr) {
      log_e("fopen(%s) failed: %s", host_path.c_str(), strerror(errno));
      return;
    }
    if (::stat(host_path.c_str(), &_stat) == 0 && _stat.st_blksize == 0) {
      setvbuf(_f, nullptr, _IOFBF, kDefaultFileBufferSize);
    }
  }
}

VFSFileImpl::~VFSFileImpl() { close(); }

void VFSFileImpl::close() {
  if (_path != nullptr) {
    free(_path);
    _path = nullptr;
  }
  if (_d != nullptr) {
    closedir(_d);
    _d = nullptr;
  }
  if (_f != nullptr) {
    fclose(_f);
    _f = nullptr;
  }
  _isDirectory = false;
}

VFSFileImpl::operator bool() {
  return (_isDirectory && _d != nullptr) || _f != nullptr;
}

time_t VFSFileImpl::getLastWrite() {
  _getStat();
  return _stat.st_mtime;
}

void VFSFileImpl::_getStat() const {
  if (_path == nullptr) {
    return;
  }
  const std::string path = HostPath(_fs->_mountpoint, _path);
  if (!path.empty() && ::stat(path.c_str(), &_stat) == 0) {
    _written = false;
  }
}

size_t VFSFileImpl::write(const uint8_t *buffer, size_t size) {
  if (_isDirectory || _f == nullptr || buffer == nullptr || size == 0) {
    return 0;
  }
  _written = true;
  return fwrite(buffer, 1, size, _f);
}

size_t VFSFileImpl::read(uint8_t *buffer, size_t size) {
  if (_isDirectory || _f == nullptr || buffer == nullptr || size == 0) {
    return 0;
  }
  return fread(buffer, 1, size, _f);
}

void VFSFileImpl::flush() {
  if (_isDirectory || _f == nullptr) {
    return;
  }
  fflush(_f);
  fsync(fileno(_f));
}

bool VFSFileImpl::seek(uint32_t position, SeekMode mode) {
  return !_isDirectory && _f != nullptr && fseek(_f, position, mode) == 0;
}

size_t VFSFileImpl::position() const {
  if (_isDirectory || _f == nullptr) {
    return 0;
  }
  const long result = ftell(_f);
  return result < 0 ? 0 : static_cast<size_t>(result);
}

size_t VFSFileImpl::size() const {
  if (_isDirectory || _f == nullptr) {
    return 0;
  }
  if (_written) {
    _getStat();
  }
  return static_cast<size_t>(_stat.st_size);
}

bool VFSFileImpl::setBufferSize(size_t size) {
  return !_isDirectory && _f != nullptr &&
         setvbuf(_f, nullptr, _IOFBF, size) == 0;
}

const char *VFSFileImpl::path() const { return _path; }

const char *VFSFileImpl::name() const {
  return _path == nullptr ? nullptr : pathToFileName(_path);
}

boolean VFSFileImpl::isDirectory() { return _isDirectory; }

FileImplPtr VFSFileImpl::openNextFile(const char *mode) {
  if (!_isDirectory || _d == nullptr) {
    return FileImplPtr();
  }
  while (dirent *entry = readdir(_d)) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    std::string name(_path == nullptr ? "/" : _path);
    AppendPath(name, entry->d_name);
    FileImplPtr result = std::make_shared<VFSFileImpl>(_fs, name.c_str(), mode);
    if (result && static_cast<bool>(*result)) {
      return result;
    }
  }
  return FileImplPtr();
}

boolean VFSFileImpl::seekDir(long position) {
  if (_d == nullptr) {
    return false;
  }
  seekdir(_d, position);
  return true;
}

String VFSFileImpl::getNextFileName() { return getNextFileName(nullptr); }

String VFSFileImpl::getNextFileName(bool *is_directory) {
  if (!_isDirectory || _d == nullptr) {
    return "";
  }
  while (dirent *entry = readdir(_d)) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    std::string name(_path == nullptr ? "/" : _path);
    AppendPath(name, entry->d_name);
    if (is_directory != nullptr) {
      if (entry->d_type == DT_DIR) {
        *is_directory = true;
      } else if (entry->d_type == DT_REG) {
        *is_directory = false;
      } else {
        struct stat info{};
        const std::string host_path = HostPath(_fs->_mountpoint, name.c_str());
        *is_directory = !host_path.empty() &&
                        ::stat(host_path.c_str(), &info) == 0 &&
                        S_ISDIR(info.st_mode);
      }
    }
    return String(name.c_str());
  }
  return "";
}

void VFSFileImpl::rewindDirectory() {
  if (_isDirectory && _d != nullptr) {
    rewinddir(_d);
  }
}
