#include "fake_esp32_nvs.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <vector>

// #include "esp32-hal-spi.h"
// #include "soc/gpio_sig_map.h"
// #include "soc/gpio_struct.h"
// #include "soc/spi_struct.h"

// #include "nvs_flash.h"

#define ESP_OK 0

#define ESP_ERR_NVS_BASE 0x1100 /*!< Starting number of error codes */
#define ESP_ERR_NVS_NOT_INITIALIZED \
  (ESP_ERR_NVS_BASE + 0x01) /*!< The storage driver is not initialized */
#define ESP_ERR_NVS_NOT_FOUND \
  (ESP_ERR_NVS_BASE +         \
   0x02) /*!< Id namespace doesn’t exist yet and mode is NVS_READONLY */
#define ESP_ERR_NVS_TYPE_MISMATCH                                         \
  (ESP_ERR_NVS_BASE + 0x03) /*!< The type of set or get operation doesn't \
                               match the type of value stored in NVS */
#define ESP_ERR_NVS_READ_ONLY \
  (ESP_ERR_NVS_BASE + 0x04) /*!< Storage handle was opened as read only */
#define ESP_ERR_NVS_NOT_ENOUGH_SPACE                                         \
  (ESP_ERR_NVS_BASE + 0x05) /*!< There is not enough space in the underlying \
                               storage to save the value */
#define ESP_ERR_NVS_INVALID_NAME \
  (ESP_ERR_NVS_BASE + 0x06) /*!< Namespace name doesn’t satisfy constraints */
#define ESP_ERR_NVS_INVALID_HANDLE \
  (ESP_ERR_NVS_BASE + 0x07) /*!< Handle has been closed or is NULL */
#define ESP_ERR_NVS_REMOVE_FAILED                                              \
  (ESP_ERR_NVS_BASE +                                                          \
   0x08) /*!< The value wasn’t updated because flash write operation has     \
            failed. The value was written however, and update will be finished \
            after re-initialization of nvs, provided that flash operation      \
            doesn’t fail again. */
#define ESP_ERR_NVS_KEY_TOO_LONG \
  (ESP_ERR_NVS_BASE + 0x09) /*!< Key name is too long */
#define ESP_ERR_NVS_PAGE_FULL \
  (ESP_ERR_NVS_BASE +         \
   0x0a) /*!< Internal error; never returned by nvs API functions */
#define ESP_ERR_NVS_INVALID_STATE                                           \
  (ESP_ERR_NVS_BASE +                                                       \
   0x0b) /*!< NVS is in an inconsistent state due to a previous error. Call \
            nvs_flash_init and nvs_open again, then retry. */
#define ESP_ERR_NVS_INVALID_LENGTH \
  (ESP_ERR_NVS_BASE +              \
   0x0c) /*!< String or blob length is not sufficient to store data */
#define ESP_ERR_NVS_NO_FREE_PAGES                                              \
  (ESP_ERR_NVS_BASE +                                                          \
   0x0d) /*!< NVS partition doesn't contain any empty pages. This may happen   \
            if NVS partition was truncated. Erase the whole partition and call \
            nvs_flash_init again. */
#define ESP_ERR_NVS_VALUE_TOO_LONG                                    \
  (ESP_ERR_NVS_BASE + 0x0e) /*!< String or blob length is longer than \
                               supported by the implementation */
#define ESP_ERR_NVS_PART_NOT_FOUND                                             \
  (ESP_ERR_NVS_BASE + 0x0f) /*!< Partition with specified name is not found in \
                               the partition table */

#define ESP_ERR_NVS_NEW_VERSION_FOUND                                          \
  (ESP_ERR_NVS_BASE + 0x10) /*!< NVS partition contains data in new format and \
                               cannot be recognized by this version of code */
#define ESP_ERR_NVS_XTS_ENCR_FAILED \
  (ESP_ERR_NVS_BASE +               \
   0x11) /*!< XTS encryption failed while writing NVS entry */
#define ESP_ERR_NVS_XTS_DECR_FAILED \
  (ESP_ERR_NVS_BASE +               \
   0x12) /*!< XTS decryption failed while reading NVS entry */
#define ESP_ERR_NVS_XTS_CFG_FAILED \
  (ESP_ERR_NVS_BASE + 0x13) /*!< XTS configuration setting failed */
#define ESP_ERR_NVS_XTS_CFG_NOT_FOUND \
  (ESP_ERR_NVS_BASE + 0x14) /*!< XTS configuration not found */
#define ESP_ERR_NVS_ENCR_NOT_SUPPORTED \
  (ESP_ERR_NVS_BASE +                  \
   0x15) /*!< NVS encryption is not supported in this version */
#define ESP_ERR_NVS_KEYS_NOT_INITIALIZED \
  (ESP_ERR_NVS_BASE + 0x16) /*!< NVS key partition is uninitialized */
#define ESP_ERR_NVS_CORRUPT_KEY_PART \
  (ESP_ERR_NVS_BASE + 0x17) /*!< NVS key partition is corrupt */
#define ESP_ERR_NVS_WRONG_ENCRYPTION                                       \
  (ESP_ERR_NVS_BASE + 0x19) /*!< NVS partition is marked as encrypted with \
                               generic flash encryption. This is forbidden \
                               since the NVS encryption works differently. */

#define ESP_ERR_NVS_CONTENT_DIFFERS                                            \
  (ESP_ERR_NVS_BASE +                                                          \
   0x18) /*!< Internal error; never returned by nvs API functions.  NVS key is \
            different in comparison */

#define NVS_DEFAULT_PART_NAME                                             \
  "nvs" /*!< Default partition name of the NVS partition in the partition \
           table */

namespace {

enum class ValueType {
  I8,
  U8,
  I16,
  U16,
  I32,
  U32,
  I64,
  U64,
  STR,
  BLOB,
};

struct EntryValue {
  ValueType type = ValueType::I32;
  int64_t sint_value = 0;
  uint64_t uint_value = 0;
  std::string str_value;
  std::string blob_value;
};

struct NamespaceStorage {
  std::map<std::string, EntryValue> entries;
};

struct PartitionStorage {
  std::map<std::string, NamespaceStorage> name_spaces;
};

struct NvsStorage {
  std::map<std::string, PartitionStorage> partitions;
};

struct JsonValue {
  enum class Type {
    kNull,
    kBool,
    kNumber,
    kString,
    kObject,
    kArray,
  };

  Type type = Type::kNull;
  bool bool_value = false;
  std::string string_value;
  std::string number_value;
  std::map<std::string, JsonValue> object_value;
  std::vector<JsonValue> array_value;
};

static void SkipWhitespace(const std::string& input, size_t* pos) {
  while (*pos < input.size() &&
         std::isspace(static_cast<unsigned char>(input[*pos]))) {
    ++(*pos);
  }
}

static bool ConsumeChar(const std::string& input, size_t* pos, char ch) {
  SkipWhitespace(input, pos);
  if (*pos >= input.size() || input[*pos] != ch) return false;
  ++(*pos);
  return true;
}

static bool ParseString(const std::string& input, size_t* pos,
                        std::string* out) {
  SkipWhitespace(input, pos);
  if (*pos >= input.size() || input[*pos] != '"') return false;
  ++(*pos);
  std::string result;
  while (*pos < input.size()) {
    char c = input[*pos];
    if (c == '"') {
      ++(*pos);
      *out = std::move(result);
      return true;
    }
    if (c == '\\') {
      ++(*pos);
      if (*pos >= input.size()) return false;
      char esc = input[*pos];
      switch (esc) {
        case '"':
          result.push_back('"');
          break;
        case '\\':
          result.push_back('\\');
          break;
        case '/':
          result.push_back('/');
          break;
        case 'b':
          result.push_back('\b');
          break;
        case 'f':
          result.push_back('\f');
          break;
        case 'n':
          result.push_back('\n');
          break;
        case 'r':
          result.push_back('\r');
          break;
        case 't':
          result.push_back('\t');
          break;
        case 'u': {
          if (*pos + 4 >= input.size()) return false;
          int code = 0;
          for (int i = 0; i < 4; ++i) {
            char h = input[*pos + 1 + i];
            code <<= 4;
            if (h >= '0' && h <= '9')
              code += h - '0';
            else if (h >= 'a' && h <= 'f')
              code += 10 + (h - 'a');
            else if (h >= 'A' && h <= 'F')
              code += 10 + (h - 'A');
            else
              return false;
          }
          if (code <= 0x7F)
            result.push_back(static_cast<char>(code));
          else
            result.push_back('?');
          *pos += 4;
          break;
        }
        default:
          return false;
      }
      ++(*pos);
      continue;
    }
    result.push_back(c);
    ++(*pos);
  }
  return false;
}

static bool ParseNumber(const std::string& input, size_t* pos,
                        std::string* out) {
  SkipWhitespace(input, pos);
  size_t start = *pos;
  if (*pos < input.size() && input[*pos] == '-') ++(*pos);
  bool has_digits = false;
  while (*pos < input.size() &&
         std::isdigit(static_cast<unsigned char>(input[*pos]))) {
    has_digits = true;
    ++(*pos);
  }
  if (!has_digits) return false;
  *out = input.substr(start, *pos - start);
  return true;
}

static bool ParseValue(const std::string& input, size_t* pos, JsonValue* out);

static bool ParseArray(const std::string& input, size_t* pos, JsonValue* out) {
  if (!ConsumeChar(input, pos, '[')) return false;
  JsonValue value;
  value.type = JsonValue::Type::kArray;
  SkipWhitespace(input, pos);
  if (*pos < input.size() && input[*pos] == ']') {
    ++(*pos);
    *out = std::move(value);
    return true;
  }
  while (true) {
    JsonValue element;
    if (!ParseValue(input, pos, &element)) return false;
    value.array_value.push_back(std::move(element));
    SkipWhitespace(input, pos);
    if (*pos < input.size() && input[*pos] == ']') {
      ++(*pos);
      *out = std::move(value);
      return true;
    }
    if (!ConsumeChar(input, pos, ',')) return false;
  }
}

static bool ParseObject(const std::string& input, size_t* pos, JsonValue* out) {
  if (!ConsumeChar(input, pos, '{')) return false;
  JsonValue value;
  value.type = JsonValue::Type::kObject;
  SkipWhitespace(input, pos);
  if (*pos < input.size() && input[*pos] == '}') {
    ++(*pos);
    *out = std::move(value);
    return true;
  }
  while (true) {
    std::string key;
    if (!ParseString(input, pos, &key)) return false;
    if (!ConsumeChar(input, pos, ':')) return false;
    JsonValue element;
    if (!ParseValue(input, pos, &element)) return false;
    value.object_value.emplace(std::move(key), std::move(element));
    SkipWhitespace(input, pos);
    if (*pos < input.size() && input[*pos] == '}') {
      ++(*pos);
      *out = std::move(value);
      return true;
    }
    if (!ConsumeChar(input, pos, ',')) return false;
  }
}

static bool ParseValue(const std::string& input, size_t* pos, JsonValue* out) {
  SkipWhitespace(input, pos);
  if (*pos >= input.size()) return false;
  char c = input[*pos];
  if (c == '"') {
    std::string s;
    if (!ParseString(input, pos, &s)) return false;
    out->type = JsonValue::Type::kString;
    out->string_value = std::move(s);
    return true;
  }
  if (c == '{') return ParseObject(input, pos, out);
  if (c == '[') return ParseArray(input, pos, out);
  if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
    std::string number;
    if (!ParseNumber(input, pos, &number)) return false;
    out->type = JsonValue::Type::kNumber;
    out->number_value = std::move(number);
    return true;
  }
  if (input.compare(*pos, 4, "true") == 0) {
    *pos += 4;
    out->type = JsonValue::Type::kBool;
    out->bool_value = true;
    return true;
  }
  if (input.compare(*pos, 5, "false") == 0) {
    *pos += 5;
    out->type = JsonValue::Type::kBool;
    out->bool_value = false;
    return true;
  }
  if (input.compare(*pos, 4, "null") == 0) {
    *pos += 4;
    out->type = JsonValue::Type::kNull;
    return true;
  }
  return false;
}

static std::string HexEncode(const std::string& input) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (unsigned char c : input) {
    out << std::setw(2) << static_cast<int>(c);
  }
  return out.str();
}

static bool HexDecode(const std::string& input, std::string* out) {
  if (input.size() % 2 != 0) return false;
  std::string result;
  result.reserve(input.size() / 2);
  for (size_t i = 0; i < input.size(); i += 2) {
    char hi = input[i];
    char lo = input[i + 1];
    auto to_val = [](char ch) -> int {
      if (ch >= '0' && ch <= '9') return ch - '0';
      if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
      if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
      return -1;
    };
    int h = to_val(hi);
    int l = to_val(lo);
    if (h < 0 || l < 0) return false;
    result.push_back(static_cast<char>((h << 4) | l));
  }
  *out = std::move(result);
  return true;
}

static const char* TypeToString(ValueType type) {
  switch (type) {
    case ValueType::I8:
      return "I8";
    case ValueType::U8:
      return "U8";
    case ValueType::I16:
      return "I16";
    case ValueType::U16:
      return "U16";
    case ValueType::I32:
      return "I32";
    case ValueType::U32:
      return "U32";
    case ValueType::I64:
      return "I64";
    case ValueType::U64:
      return "U64";
    case ValueType::STR:
      return "STR";
    case ValueType::BLOB:
      return "BLOB";
  }
  return "I32";
}

static bool TypeFromString(const std::string& value, ValueType* out) {
  if (value == "I8") {
    *out = ValueType::I8;
    return true;
  }
  if (value == "U8") {
    *out = ValueType::U8;
    return true;
  }
  if (value == "I16") {
    *out = ValueType::I16;
    return true;
  }
  if (value == "U16") {
    *out = ValueType::U16;
    return true;
  }
  if (value == "I32") {
    *out = ValueType::I32;
    return true;
  }
  if (value == "U32") {
    *out = ValueType::U32;
    return true;
  }
  if (value == "I64") {
    *out = ValueType::I64;
    return true;
  }
  if (value == "U64") {
    *out = ValueType::U64;
    return true;
  }
  if (value == "STR") {
    *out = ValueType::STR;
    return true;
  }
  if (value == "BLOB") {
    *out = ValueType::BLOB;
    return true;
  }
  return false;
}

static void WriteIndent(std::ostringstream& out, int indent) {
  for (int i = 0; i < indent; ++i) out << ' ';
}

static void WriteJsonString(std::ostringstream& out, const std::string& value) {
  out << '"';
  for (unsigned char c : value) {
    switch (c) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(c) << std::dec;
        } else {
          out << c;
        }
    }
  }
  out << '"';
}

static void WriteEntryValue(std::ostringstream& out, const EntryValue& value,
                            int indent) {
  out << "{\n";
  WriteIndent(out, indent + 2);
  out << "\"type\": ";
  WriteJsonString(out, TypeToString(value.type));
  out << ",\n";
  WriteIndent(out, indent + 2);
  if (value.type == ValueType::STR) {
    out << "\"str\": ";
    WriteJsonString(out, value.str_value);
  } else if (value.type == ValueType::BLOB) {
    out << "\"blob_hex\": ";
    WriteJsonString(out, HexEncode(value.blob_value));
  } else if (value.type == ValueType::U8 || value.type == ValueType::U16 ||
             value.type == ValueType::U32 || value.type == ValueType::U64) {
    out << "\"uint\": " << value.uint_value;
  } else {
    out << "\"sint\": " << value.sint_value;
  }
  out << "\n";
  WriteIndent(out, indent);
  out << "}";
}

static void WriteStorage(std::ostringstream& out, const NvsStorage& storage,
                         int indent) {
  out << "{\n";
  WriteIndent(out, indent + 2);
  out << "\"partitions\": {";
  if (!storage.partitions.empty()) out << "\n";
  bool first_part = true;
  for (const auto& part_it : storage.partitions) {
    if (!first_part) out << ",\n";
    first_part = false;
    WriteIndent(out, indent + 4);
    WriteJsonString(out, part_it.first);
    out << ": {\n";
    WriteIndent(out, indent + 6);
    out << "\"name_spaces\": {";
    if (!part_it.second.name_spaces.empty()) out << "\n";
    bool first_ns = true;
    for (const auto& ns_it : part_it.second.name_spaces) {
      if (!first_ns) out << ",\n";
      first_ns = false;
      WriteIndent(out, indent + 8);
      WriteJsonString(out, ns_it.first);
      out << ": {\n";
      WriteIndent(out, indent + 10);
      out << "\"entries\": {";
      if (!ns_it.second.entries.empty()) out << "\n";
      bool first_entry = true;
      for (const auto& entry_it : ns_it.second.entries) {
        if (!first_entry) out << ",\n";
        first_entry = false;
        WriteIndent(out, indent + 12);
        WriteJsonString(out, entry_it.first);
        out << ": ";
        WriteEntryValue(out, entry_it.second, indent + 12);
      }
      if (!ns_it.second.entries.empty()) out << "\n";
      WriteIndent(out, indent + 10);
      out << "}";
      out << "\n";
      WriteIndent(out, indent + 8);
      out << "}";
    }
    if (!part_it.second.name_spaces.empty()) out << "\n";
    WriteIndent(out, indent + 6);
    out << "}";
    out << "\n";
    WriteIndent(out, indent + 4);
    out << "}";
  }
  if (!storage.partitions.empty()) out << "\n";
  WriteIndent(out, indent + 2);
  out << "}\n";
  WriteIndent(out, indent);
  out << "}";
}

static bool JsonToStorage(const JsonValue& root, NvsStorage* storage,
                          std::string* error) {
  if (root.type != JsonValue::Type::kObject) {
    if (error) *error = "Root must be an object";
    return false;
  }
  auto parts_it = root.object_value.find("partitions");
  if (parts_it == root.object_value.end()) return true;
  if (parts_it->second.type != JsonValue::Type::kObject) {
    if (error) *error = "partitions must be an object";
    return false;
  }
  for (const auto& part_pair : parts_it->second.object_value) {
    if (part_pair.second.type != JsonValue::Type::kObject) continue;
    PartitionStorage partition;
    auto ns_it = part_pair.second.object_value.find("name_spaces");
    if (ns_it != part_pair.second.object_value.end() &&
        ns_it->second.type == JsonValue::Type::kObject) {
      for (const auto& ns_pair : ns_it->second.object_value) {
        if (ns_pair.second.type != JsonValue::Type::kObject) continue;
        NamespaceStorage ns_storage;
        auto entries_it = ns_pair.second.object_value.find("entries");
        if (entries_it != ns_pair.second.object_value.end() &&
            entries_it->second.type == JsonValue::Type::kObject) {
          for (const auto& entry_pair : entries_it->second.object_value) {
            if (entry_pair.second.type != JsonValue::Type::kObject) continue;
            const auto& entry_obj = entry_pair.second.object_value;
            auto type_it = entry_obj.find("type");
            if (type_it == entry_obj.end() ||
                type_it->second.type != JsonValue::Type::kString) {
              if (error) *error = "entry missing type";
              return false;
            }
            EntryValue entry;
            if (!TypeFromString(type_it->second.string_value, &entry.type)) {
              if (error) *error = "unknown entry type";
              return false;
            }
            if (entry.type == ValueType::STR) {
              auto str_it = entry_obj.find("str");
              if (str_it == entry_obj.end() ||
                  str_it->second.type != JsonValue::Type::kString) {
                if (error) *error = "string entry missing str";
                return false;
              }
              entry.str_value = str_it->second.string_value;
            } else if (entry.type == ValueType::BLOB) {
              auto blob_it = entry_obj.find("blob_hex");
              if (blob_it == entry_obj.end() ||
                  blob_it->second.type != JsonValue::Type::kString) {
                if (error) *error = "blob entry missing blob_hex";
                return false;
              }
              if (!HexDecode(blob_it->second.string_value, &entry.blob_value)) {
                if (error) *error = "invalid blob_hex";
                return false;
              }
            } else if (entry.type == ValueType::U8 ||
                       entry.type == ValueType::U16 ||
                       entry.type == ValueType::U32 ||
                       entry.type == ValueType::U64) {
              auto uint_it = entry_obj.find("uint");
              if (uint_it == entry_obj.end() ||
                  uint_it->second.type != JsonValue::Type::kNumber) {
                if (error) *error = "unsigned entry missing uint";
                return false;
              }
              entry.uint_value = std::stoull(uint_it->second.number_value);
            } else {
              auto sint_it = entry_obj.find("sint");
              if (sint_it == entry_obj.end() ||
                  sint_it->second.type != JsonValue::Type::kNumber) {
                if (error) *error = "signed entry missing sint";
                return false;
              }
              entry.sint_value = std::stoll(sint_it->second.number_value);
            }
            ns_storage.entries.emplace(entry_pair.first, std::move(entry));
          }
        }
        partition.name_spaces.emplace(ns_pair.first, std::move(ns_storage));
      }
    }
    storage->partitions.emplace(part_pair.first, std::move(partition));
  }
  return true;
}

}  // namespace

class NvsImpl {
 public:
  struct Handle {
    std::string partition_name;
    std::string ns_name;
    bool readonly;
  };

  NvsImpl(const std::string& path) : path_(path), next_id_(0) {
    if (!path_.empty()) {
      std::ifstream input_file(path);
      if (!input_file.is_open()) {
        std::cerr << "WARNING: could not open the NVS storage file - '" << path
                  << "'. Creating new one." << std::endl;
        // Try creating a new file.
        save();
        return;
      }
      std::cout << "NVS storage file opened: " << path << std::endl;
      std::string val((std::istreambuf_iterator<char>(input_file)),
                      std::istreambuf_iterator<char>());
      JsonValue root;
      size_t pos = 0;
      if (!ParseValue(val, &pos, &root)) {
        std::cerr << "Could not parse JSON - '" << path << "'" << std::endl;
        exit(EXIT_FAILURE);
      }
      SkipWhitespace(val, &pos);
      if (pos != val.size()) {
        std::cerr << "Trailing data after JSON in '" << path << "'"
                  << std::endl;
        exit(EXIT_FAILURE);
      }
      std::string error;
      if (!JsonToStorage(root, &storage_, &error)) {
        std::cerr << "Could not parse NVS JSON - '" << path << "': " << error
                  << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  esp_err_t init(const char* partition_name) {
    storage_.partitions[partition_name];
    save();
    return ESP_OK;
  }

  void save() {
    std::ostringstream out;
    WriteStorage(out, storage_, 0);
    std::string val = out.str();
    std::ofstream output_file(path_);
    output_file << val;
    output_file.close();
  }

  esp_err_t open(const char* part_name, const char* name, bool readonly,
                 nvs_handle_t* out_handle) {
    if (storage_.partitions.find(part_name) == storage_.partitions.end()) {
      return ESP_ERR_NVS_PART_NOT_FOUND;
    }
    PartitionStorage& part = storage_.partitions[part_name];
    if (part.name_spaces.find(name) == part.name_spaces.end()) {
      if (readonly) {
        return ESP_ERR_NVS_NOT_FOUND;
      }
      part.name_spaces[name];
    }
    int id = next_id_++;
    Handle handle = {
        .partition_name = part_name,
        .ns_name = name,
        .readonly = readonly,
    };
    open_partitions_[id] = handle;
    *out_handle = id;
    return ESP_OK;
  }

  esp_err_t set(nvs_handle_t handle, const char* key, EntryValue val) {
    if (open_partitions_.find(handle) == open_partitions_.end()) {
      return ESP_ERR_NVS_INVALID_HANDLE;
    }
    Handle& h = open_partitions_[handle];
    if (h.readonly) {
      return ESP_ERR_NVS_READ_ONLY;
    }
    if (strlen(key) > 15) {
      return ESP_ERR_NVS_INVALID_NAME;
    }
    storage_.partitions[h.partition_name].name_spaces[h.ns_name].entries[key] =
        std::move(val);
    return ESP_OK;
  }

  esp_err_t get(nvs_handle_t handle, const char* key, ValueType expected_type,
                EntryValue& val) {
    if (open_partitions_.find(handle) == open_partitions_.end()) {
      return ESP_ERR_NVS_INVALID_HANDLE;
    }
    Handle& h = open_partitions_[handle];
    if (strlen(key) > 15) {
      return ESP_ERR_NVS_INVALID_NAME;
    }
    auto& entries =
        storage_.partitions[h.partition_name].name_spaces[h.ns_name].entries;
    if (entries.find(key) == entries.end()) {
      return ESP_ERR_NVS_NOT_FOUND;
    }
    const auto& entry = entries[key];
    if (entry.type != expected_type) {
      return ESP_ERR_NVS_TYPE_MISMATCH;
    }
    val = entry;
    return ESP_OK;
  }

  void close(nvs_handle_t handle) { open_partitions_.erase(handle); }

  esp_err_t erase_key(nvs_handle_t handle, const char* key) {
    if (open_partitions_.find(handle) == open_partitions_.end()) {
      return ESP_ERR_NVS_INVALID_HANDLE;
    }
    Handle& h = open_partitions_[handle];
    if (h.readonly) {
      return ESP_ERR_NVS_READ_ONLY;
    }
    if (strlen(key) > 15) {
      return ESP_ERR_NVS_INVALID_NAME;
    }
    storage_.partitions[h.partition_name].name_spaces[h.ns_name].entries.erase(
        key);
    return ESP_OK;
  }

  esp_err_t erase_all(nvs_handle_t handle) {
    if (open_partitions_.find(handle) == open_partitions_.end()) {
      return ESP_ERR_NVS_INVALID_HANDLE;
    }
    Handle& h = open_partitions_[handle];
    if (h.readonly) {
      return ESP_ERR_NVS_READ_ONLY;
    }
    storage_.partitions[h.partition_name]
        .name_spaces[h.ns_name]
        .entries.clear();
    return ESP_OK;
  }

 private:
  NvsStorage storage_;
  std::string path_;
  std::map<int, Handle> open_partitions_;
  int next_id_;
};

Nvs::Nvs(const std::string& path) : impl_(new NvsImpl(path)) {}

Nvs::~Nvs() { delete impl_; }

void Nvs::save() { impl_->save(); }

esp_err_t Nvs::init(const char* partition_name) {
  return impl_->init(partition_name);
}

esp_err_t Nvs::open(const char* part_name, const char* name, bool readonly,
                    nvs_handle_t* out_handle) {
  return impl_->open(part_name, name, readonly, out_handle);
}

esp_err_t Nvs::set_i8(nvs_handle_t handle, const char* key, int8_t value) {
  EntryValue val;
  val.type = ValueType::I8;
  val.sint_value = value;
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u8(nvs_handle_t handle, const char* key, uint8_t value) {
  EntryValue val;
  val.type = ValueType::U8;
  val.uint_value = value;
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_i16(nvs_handle_t handle, const char* key, int16_t value) {
  EntryValue val;
  val.type = ValueType::I16;
  val.sint_value = value;
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u16(nvs_handle_t handle, const char* key, uint16_t value) {
  EntryValue val;
  val.type = ValueType::U16;
  val.uint_value = value;
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_i32(nvs_handle_t handle, const char* key, int32_t value) {
  EntryValue val;
  val.type = ValueType::I32;
  val.sint_value = value;
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u32(nvs_handle_t handle, const char* key, uint32_t value) {
  EntryValue val;
  val.type = ValueType::U32;
  val.uint_value = value;
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_i64(nvs_handle_t handle, const char* key, int64_t value) {
  EntryValue val;
  val.type = ValueType::I64;
  val.sint_value = value;
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u64(nvs_handle_t handle, const char* key, uint64_t value) {
  EntryValue val;
  val.type = ValueType::U64;
  val.uint_value = value;
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_str(nvs_handle_t handle, const char* key,
                       const char* value) {
  EntryValue val;
  val.type = ValueType::STR;
  val.str_value = value;
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_blob(nvs_handle_t handle, const char* key, const void* value,
                        size_t length) {
  EntryValue val;
  val.type = ValueType::BLOB;
  val.blob_value = std::string((const char*)value, length);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::get_i8(nvs_handle_t handle, const char* key, int8_t* value) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::I8, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value;
  return ESP_OK;
}

esp_err_t Nvs::get_u8(nvs_handle_t handle, const char* key, uint8_t* value) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::U8, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value;
  return ESP_OK;
}

esp_err_t Nvs::get_i16(nvs_handle_t handle, const char* key, int16_t* value) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::I16, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value;
  return ESP_OK;
}

esp_err_t Nvs::get_u16(nvs_handle_t handle, const char* key, uint16_t* value) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::U16, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value;
  return ESP_OK;
}

esp_err_t Nvs::get_i32(nvs_handle_t handle, const char* key, int32_t* value) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::I32, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value;
  return ESP_OK;
}

esp_err_t Nvs::get_u32(nvs_handle_t handle, const char* key, uint32_t* value) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::U32, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value;
  return ESP_OK;
}

esp_err_t Nvs::get_i64(nvs_handle_t handle, const char* key, int64_t* value) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::I64, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value;
  return ESP_OK;
}

esp_err_t Nvs::get_u64(nvs_handle_t handle, const char* key, uint64_t* value) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::U64, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value;
  return ESP_OK;
}

esp_err_t Nvs::get_str(nvs_handle_t handle, const char* key, char* value,
                       size_t* length) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::STR, val);
  if (err != ESP_OK) return err;
  size_t actual = std::max(*length, val.str_value.size() + 1);
  if (value != nullptr) {
    memcpy(value, val.str_value.c_str(), actual);
  }
  *length = actual;
  return ESP_OK;
}

esp_err_t Nvs::get_blob(nvs_handle_t handle, const char* key, char* value,
                        size_t* length) {
  EntryValue val;
  esp_err_t err = impl_->get(handle, key, ValueType::BLOB, val);
  if (err != ESP_OK) return err;
  size_t actual = std::max(*length, val.blob_value.size());
  if (value != nullptr) {
    memcpy(value, val.blob_value.c_str(), actual);
  }
  *length = actual;
  return ESP_OK;
}

esp_err_t Nvs::commit() {
  impl_->save();
  return ESP_OK;
}

void Nvs::close(nvs_handle_t handle) { impl_->close(handle); }

esp_err_t Nvs::erase_key(nvs_handle_t handle, const char* key) {
  return impl_->erase_key(handle, key);
}

esp_err_t Nvs::erase_all(nvs_handle_t handle) {
  return impl_->erase_all(handle);
}
