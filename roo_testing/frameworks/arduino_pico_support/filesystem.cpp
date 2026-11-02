#include "roo_testing/frameworks/arduino_pico_support/filesystem.h"

#include <FSImpl.h>
#include <LittleFS.h>
#include <SD.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace roo_testing {
struct PicoFilesystemState {
  explicit PicoFilesystemState(bool prune) : prune_empty_parents(prune) {
    const char* temporary = std::getenv("TEST_TMPDIR");
    std::string pattern =
        std::string(temporary == nullptr ? "/var/tmp" : temporary) +
        "/roo-pico-fs-XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back(0);
    char* created = mkdtemp(buffer.data());
    if (created != nullptr) root = created;
  }
  ~PicoFilesystemState() {
    std::error_code error;
    if (!root.empty()) std::filesystem::remove_all(root, error);
  }
  bool ready() const { return mounted && media; }
  // Confine paths to the owned root and reject symlinks, including dangling
  // ones. Explicit parent segments are unsupported rather than escaping
  // storage.
  std::string resolve(const char* path) const {
    if (root.empty() || path == nullptr || path[0] != '/') return {};
    std::string result = root;
    std::string relative(path);
    size_t start = 1;
    while (start < relative.size()) {
      size_t end = relative.find('/', start);
      if (end == std::string::npos) end = relative.size();
      std::string part = relative.substr(start, end - start);
      if (part == "..") return {};
      if (!part.empty() && part != ".") {
        result += "/" + part;
        struct stat st;
        if (lstat(result.c_str(), &st) == 0 && S_ISLNK(st.st_mode)) return {};
      }
      start = end + 1;
    }
    return result;
  }
  std::string root;
  bool mounted = false;
  bool media = true;
  bool fail_mount = false;
  bool prune_empty_parents;
};

namespace {
class HostFile : public fs::FileImpl {
 public:
  HostFile(std::shared_ptr<PicoFilesystemState> state, std::string path, int fd,
           bool directory, bool writable)
      : state_(std::move(state)),
        path_(std::move(path)),
        fd_(fd),
        directory_(directory),
        writable_(writable) {}
  ~HostFile() override { close(); }
  size_t write(const uint8_t* data, size_t size) override {
    if (!valid() || !writable_ || directory_) return 0;
    ssize_t count = ::write(fd_, data, size);
    return count > 0 ? count : 0;
  }
  int read(uint8_t* data, size_t size) override {
    if (!valid() || directory_) return -1;
    return ::read(fd_, data, std::min(size, static_cast<size_t>(INT_MAX)));
  }
  void flush() override {
    if (valid() && writable_) fsync(fd_);
  }
  bool seek(uint32_t pos, fs::SeekMode mode) override {
    if (!valid() || directory_) return false;
    const int origin = mode == fs::SeekSet   ? SEEK_SET
                       : mode == fs::SeekCur ? SEEK_CUR
                                             : SEEK_END;
    return lseek(fd_, pos, origin) >= 0;
  }
  size_t position() const override {
    if (!valid() || directory_) return 0;
    off_t pos = lseek(fd_, 0, SEEK_CUR);
    return pos >= 0 ? pos : 0;
  }
  size_t size() const override {
    struct stat st;
    return valid() && fstat(fd_, &st) == 0 && !directory_ ? st.st_size : 0;
  }
  int availableForWrite() override {
    return valid() && writable_ ? INT_MAX : 0;
  }
  bool truncate(uint32_t size) override {
    return valid() && writable_ && ftruncate(fd_, size) == 0;
  }
  void close() override {
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
  }
  const char* name() const override {
    return path_.c_str() + path_.find_last_of('/') + 1;
  }
  const char* fullName() const override { return path_.c_str(); }
  bool isFile() const override { return valid() && !directory_; }
  bool isDirectory() const override { return valid() && directory_; }

 private:
  bool valid() const { return fd_ >= 0 && state_->ready(); }
  std::shared_ptr<PicoFilesystemState> state_;
  std::string path_;
  int fd_;
  bool directory_;
  bool writable_;
};

// Opens a checked native file without creating missing parent directories.
fs::FileImplPtr Open(std::shared_ptr<PicoFilesystemState> state,
                     const char* path, fs::OpenMode mode,
                     fs::AccessMode access) {
  if (!state->ready()) return {};
  std::string native = state->resolve(path);
  if (native.empty()) return {};
  const bool writable = (access & fs::AM_WRITE) != 0;
  int flags =
      writable ? ((access & fs::AM_READ) != 0 ? O_RDWR : O_WRONLY) : O_RDONLY;
  if ((mode & fs::OM_CREATE) != 0) flags |= O_CREAT;
  if ((mode & fs::OM_APPEND) != 0) flags |= O_APPEND;
  if ((mode & fs::OM_TRUNCATE) != 0) flags |= O_TRUNC;
  int fd = ::open(native.c_str(), flags | O_NOFOLLOW, 0600);
  if (fd < 0) return {};
  struct stat st;
  if (fstat(fd, &st) != 0) {
    ::close(fd);
    return {};
  }
  std::string normalized = native.substr(state->root.size());
  if (normalized.empty()) normalized = "/";
  return std::make_shared<HostFile>(state, normalized, fd, S_ISDIR(st.st_mode),
                                    writable);
}

class HostDir : public fs::DirImpl {
 public:
  HostDir(std::shared_ptr<PicoFilesystemState> state, std::string path)
      : state_(std::move(state)) {
    std::string native = state_->resolve(path.c_str());
    DIR* dir = native.empty() ? nullptr : ::opendir(native.c_str());
    if (dir == nullptr) return;
    while (dirent* entry = readdir(dir)) {
      if (std::string(entry->d_name) == "." ||
          std::string(entry->d_name) == "..")
        continue;
      entries_.push_back(path + (path.back() == '/' ? "" : "/") +
                         entry->d_name);
    }
    closedir(dir);
    std::sort(entries_.begin(), entries_.end());
  }
  fs::FileImplPtr openFile(fs::OpenMode mode, fs::AccessMode access) override {
    return current() ? Open(state_, entries_[index_ - 1].c_str(), mode, access)
                     : nullptr;
  }
  const char* fileName() override {
    return current() ? entries_[index_ - 1].c_str() : "";
  }
  size_t fileSize() override {
    struct stat st;
    return statCurrent(st) ? st.st_size : 0;
  }
  bool isFile() const override {
    struct stat st;
    return statCurrent(st) && S_ISREG(st.st_mode);
  }
  bool isDirectory() const override {
    struct stat st;
    return statCurrent(st) && S_ISDIR(st.st_mode);
  }
  bool next() override {
    if (!state_->ready() || index_ >= entries_.size()) {
      index_ = entries_.size() + 1;
      return false;
    }
    ++index_;
    return true;
  }
  bool rewind() override {
    index_ = 0;
    return state_->ready();
  }

 private:
  bool current() const {
    return state_->ready() && index_ > 0 && index_ <= entries_.size();
  }
  bool statCurrent(struct stat& st) const {
    if (!current()) return false;
    std::string native = state_->resolve(entries_[index_ - 1].c_str());
    return !native.empty() && ::stat(native.c_str(), &st) == 0;
  }
  std::shared_ptr<PicoFilesystemState> state_;
  std::vector<std::string> entries_;
  size_t index_ = 0;
};

class HostFs : public fs::FSImpl {
 public:
  explicit HostFs(std::shared_ptr<PicoFilesystemState> state)
      : state_(std::move(state)) {}
  bool setConfig(const fs::FSConfig&) override { return !state_->mounted; }
  bool begin() override {
    state_->mounted =
        state_->media && !state_->fail_mount && !state_->root.empty();
    return state_->mounted;
  }
  void end() override { state_->mounted = false; }
  bool format() override {
    if (!state_->media || state_->root.empty() || state_->mounted) return false;
    std::error_code error;
    std::filesystem::remove_all(state_->root, error);
    return !error && std::filesystem::create_directory(state_->root, error) &&
           !error;
  }
  bool info(fs::FSInfo& info) override {
    if (!state_->ready()) return false;
    uint64_t used = 0;
    std::error_code error;
    std::filesystem::recursive_directory_iterator entry(state_->root, error);
    const std::filesystem::recursive_directory_iterator end;
    while (!error && entry != end) {
      if (entry->is_regular_file(error)) used += entry->file_size(error);
      entry.increment(error);
    }
    if (error) return false;
    // Nominal capacity is informational; disk-full and flash wear are not
    // modeled.
    info = {std::max(uint64_t{1024 * 1024}, used), used, 4096, 256, 256, 255};
    return true;
  }
  fs::FileImplPtr open(const char* path, fs::OpenMode mode,
                       fs::AccessMode access) override {
    return Open(state_, path, mode, access);
  }
  bool exists(const char* path) override {
    struct stat st;
    return inspect(path, st);
  }
  fs::DirImplPtr openDir(const char* path) override {
    struct stat st;
    if (!inspect(path, st) || !S_ISDIR(st.st_mode)) return {};
    return std::make_shared<HostDir>(state_, path);
  }
  bool rename(const char* from, const char* to) override {
    std::string a = state_->resolve(from);
    std::string b = state_->resolve(to);
    return state_->ready() && !a.empty() && !b.empty() && a != state_->root &&
           b != state_->root && ::rename(a.c_str(), b.c_str()) == 0;
  }
  bool remove(const char* path) override {
    std::string native = state_->resolve(path);
    if (!state_->ready() || native.empty() || native == state_->root)
      return false;
    const bool removed = state_->prune_empty_parents
                             ? ::remove(native.c_str()) == 0
                             : ::unlink(native.c_str()) == 0;
    if (removed) pruneParents(native);
    return removed;
  }
  bool mkdir(const char* path) override {
    std::string native = state_->resolve(path);
    return state_->ready() && !native.empty() &&
           ::mkdir(native.c_str(), 0700) == 0;
  }
  bool rmdir(const char* path) override {
    if (state_->prune_empty_parents) return remove(path);
    std::string native = state_->resolve(path);
    if (!state_->ready() || native.empty() || native == state_->root ||
        ::rmdir(native.c_str()) != 0)
      return false;
    pruneParents(native);
    return true;
  }
  bool stat(const char* path, fs::FSStat* result) override {
    struct stat st;
    if (result == nullptr || !inspect(path, st)) return false;
    *result = {static_cast<size_t>(st.st_size),
               static_cast<size_t>(st.st_blksize), st.st_ctime, st.st_atime,
               S_ISDIR(st.st_mode)};
    return true;
  }

 private:
  // The core's LittleFS remove() also attempts to remove all empty ancestors.
  void pruneParents(const std::string& removed) {
    if (!state_->prune_empty_parents) return;
    std::filesystem::path parent = std::filesystem::path(removed).parent_path();
    while (parent != state_->root && !parent.empty()) {
      if (::rmdir(parent.c_str()) != 0) break;
      parent = parent.parent_path();
    }
  }

  bool inspect(const char* path, struct stat& st) const {
    std::string native = state_->resolve(path);
    return state_->ready() && !native.empty() &&
           ::stat(native.c_str(), &st) == 0;
  }
  std::shared_ptr<PicoFilesystemState> state_;
};
}  // namespace

PicoFilesystem::PicoFilesystem(bool prune_empty_parents)
    : PicoFilesystem(
          std::make_shared<PicoFilesystemState>(prune_empty_parents)) {}
PicoFilesystem::PicoFilesystem(std::shared_ptr<PicoFilesystemState> state)
    : fs::FS(std::make_shared<HostFs>(state)), state_(std::move(state)) {}
PicoFilesystem::~PicoFilesystem() { end(); }
void PicoFilesystem::setMediaPresent(bool present) { state_->media = present; }
void PicoFilesystem::setMountFailure(bool fail) { state_->fail_mount = fail; }
bool PicoFilesystem::mounted() const { return state_->ready(); }
const std::string& PicoFilesystem::hostRoot() const { return state_->root; }
}  // namespace roo_testing

roo_testing::PicoFilesystem SDFS;
roo_testing::PicoFilesystem LittleFS(true);
SDClass SD;
