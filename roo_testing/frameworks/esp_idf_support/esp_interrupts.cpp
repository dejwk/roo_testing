#include "roo_testing/frameworks/esp_idf_support/esp_interrupts.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/interrupts/interrupt_controller.h"

namespace {

constexpr size_t kVectorCapacity = 64;
constexpr size_t kRegistrationCapacity = 64;

constexpr uint64_t kVectorPublished = uint64_t{1} << 0;
constexpr unsigned kVectorEpochShift = 1;

constexpr uint64_t kRecordPublished = uint64_t{1} << 0;
constexpr uint64_t kRecordEnabled = uint64_t{1} << 1;
constexpr uint64_t kRecordMatched = uint64_t{1} << 2;
constexpr unsigned kRecordEpochShift = 3;

struct SourceVector {
  std::atomic<uint64_t> publication{0};
  std::atomic<int> source{0};
  std::atomic<uint32_t> controller_slot{static_cast<uint32_t>(-1)};
  std::atomic<uint32_t> controller_generation{0};

  // Accessed only from a FreeRTOS task with port interrupts masked.
  int flags = 0;
  int level = 0;
};

struct VectorIdentity {
  uint32_t slot;
  uint64_t publication;
};

}  // namespace

struct intr_handle_data_t {
  std::atomic<uint64_t> state{0};
  std::atomic<uint32_t> vector_slot{static_cast<uint32_t>(-1)};
  std::atomic<uint64_t> vector_publication{0};
  std::atomic<intr_handler_t> handler{nullptr};
  std::atomic<void*> argument{nullptr};
  std::atomic<uint32_t> status_register{0};
  std::atomic<uint32_t> status_mask{0};
};

namespace {

std::array<SourceVector, kVectorCapacity> source_vectors;
std::array<intr_handle_data_t, kRegistrationCapacity> registrations;

static_assert(std::atomic<uint64_t>::is_always_lock_free,
              "ESP interrupt state must be signal-safe");
static_assert(std::atomic<int>::is_always_lock_free,
              "ESP interrupt sources must be signal-safe");
static_assert(std::atomic<uint32_t>::is_always_lock_free,
              "ESP interrupt identities must be signal-safe");
static_assert(std::atomic<intr_handler_t>::is_always_lock_free,
              "ESP interrupt handlers must be signal-safe");
static_assert(std::atomic<void*>::is_always_lock_free,
              "ESP interrupt arguments must be signal-safe");

uint64_t VectorEpoch(uint64_t publication) {
  return publication >> kVectorEpochShift;
}

uint64_t PublishedVectorEpoch(uint64_t epoch) {
  return (epoch << kVectorEpochShift) | kVectorPublished;
}

uint64_t NextVectorEpoch(uint64_t publication) {
  uint64_t epoch = VectorEpoch(publication) + 1;
  if (epoch == 0) epoch = 1;
  return epoch;
}

uint64_t RecordEpoch(uint64_t state) { return state >> kRecordEpochShift; }

uint64_t RecordState(uint64_t epoch, uint64_t flags) {
  return (epoch << kRecordEpochShift) | flags;
}

uint64_t NextRecordEpoch(uint64_t state) {
  uint64_t epoch = RecordEpoch(state) + 1;
  if (epoch == 0) epoch = 1;
  return epoch;
}

bool IsTaskOperationContext() {
  return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
         xPortIsFreeRtosTask() == pdTRUE && xPortInIsrContext() == pdFALSE;
}

uint32_t VectorSlot(const SourceVector& vector) {
  return static_cast<uint32_t>(&vector - source_vectors.data());
}

bool IsVectorPublished(uint64_t publication) {
  return (publication & kVectorPublished) != 0;
}

bool IsRecordPublished(uint64_t state) {
  return (state & kRecordPublished) != 0;
}

SourceVector* FindSourceVector(int source) {
  for (SourceVector& vector : source_vectors) {
    const uint64_t publication =
        vector.publication.load(std::memory_order_relaxed);
    if (IsVectorPublished(publication) &&
        vector.source.load(std::memory_order_relaxed) == source) {
      return &vector;
    }
  }
  return nullptr;
}

SourceVector* FindFreeVector() {
  for (SourceVector& vector : source_vectors) {
    if (!IsVectorPublished(
            vector.publication.load(std::memory_order_relaxed))) {
      return &vector;
    }
  }
  return nullptr;
}

intr_handle_data_t* FindFreeRecord() {
  for (intr_handle_data_t& record : registrations) {
    if (!IsRecordPublished(record.state.load(std::memory_order_relaxed))) {
      return &record;
    }
  }
  return nullptr;
}

intr_handle_data_t* FindRecord(intr_handle_t handle) {
  if (handle == nullptr) return nullptr;
  for (intr_handle_data_t& record : registrations) {
    if (&record == handle) return &record;
  }
  return nullptr;
}

bool LoadVectorControllerHandle(VectorIdentity identity,
                                roo_testing::InterruptHandle* out_handle) {
  if (identity.slot >= source_vectors.size()) return false;
  SourceVector& vector = source_vectors[identity.slot];
  const uint64_t before = vector.publication.load(std::memory_order_acquire);
  if (before != identity.publication || !IsVectorPublished(before)) {
    return false;
  }
  roo_testing::InterruptHandle handle{
      vector.controller_slot.load(std::memory_order_relaxed),
      vector.controller_generation.load(std::memory_order_relaxed)};
  if (vector.publication.load(std::memory_order_acquire) != before) {
    return false;
  }
  *out_handle = handle;
  return handle.isValid();
}

bool LoadRecordIdentity(intr_handle_data_t& record, uint64_t expected_state,
                        VectorIdentity* out_identity) {
  if (!IsRecordPublished(expected_state)) return false;
  VectorIdentity identity{
      record.vector_slot.load(std::memory_order_relaxed),
      record.vector_publication.load(std::memory_order_relaxed)};
  if (record.state.load(std::memory_order_acquire) != expected_state) {
    return false;
  }
  *out_identity = identity;
  return true;
}

bool RecordBelongsTo(intr_handle_data_t& record, uint64_t state,
                     VectorIdentity identity) {
  return IsRecordPublished(state) &&
         record.vector_slot.load(std::memory_order_relaxed) == identity.slot &&
         record.vector_publication.load(std::memory_order_relaxed) ==
             identity.publication;
}

bool AnyLiveRecord(VectorIdentity identity) {
  for (intr_handle_data_t& record : registrations) {
    const uint64_t state = record.state.load(std::memory_order_acquire);
    if (RecordBelongsTo(record, state, identity)) return true;
  }
  return false;
}

bool AnyEnabledRecord(VectorIdentity identity) {
  for (intr_handle_data_t& record : registrations) {
    const uint64_t state = record.state.load(std::memory_order_acquire);
    if ((state & kRecordEnabled) != 0 &&
        RecordBelongsTo(record, state, identity)) {
      return true;
    }
  }
  return false;
}

bool UpdateVectorEnablement(VectorIdentity identity) {
  roo_testing::InterruptHandle controller_handle;
  if (!LoadVectorControllerHandle(identity, &controller_handle)) return false;
  if (AnyEnabledRecord(identity)) {
    return roo_testing::enableInterrupt(controller_handle);
  }
  return roo_testing::disableInterrupt(controller_handle);
}

bool StatusMatches(
    uint32_t status_register, uint32_t status_mask,
    const roo_testing::esp_idf::InterruptStatusSnapshot* snapshots,
    size_t snapshot_count) {
  if (status_register == 0) return true;
  for (size_t i = 0; i < snapshot_count; ++i) {
    if (snapshots[i].target_address == status_register &&
        (snapshots[i].value & status_mask) != 0) {
      return true;
    }
  }
  return false;
}

bool MarkRecordMatched(
    intr_handle_data_t& record, VectorIdentity identity,
    const roo_testing::esp_idf::InterruptStatusSnapshot* snapshots,
    size_t snapshot_count) {
  uint64_t state = record.state.load(std::memory_order_acquire);
  while (IsRecordPublished(state)) {
    if (!RecordBelongsTo(record, state, identity)) return false;
    const uint32_t status_register =
        record.status_register.load(std::memory_order_relaxed);
    const uint32_t status_mask =
        record.status_mask.load(std::memory_order_relaxed);
    if (!StatusMatches(status_register, status_mask, snapshots,
                       snapshot_count)) {
      return false;
    }
    if (record.state.compare_exchange_weak(state, state | kRecordMatched,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
      return true;
    }
  }
  return false;
}

void DispatchSourceVector(void* argument) {
  SourceVector& vector = *static_cast<SourceVector*>(argument);
  const uint64_t publication =
      vector.publication.load(std::memory_order_acquire);
  if (!IsVectorPublished(publication)) return;
  const VectorIdentity identity{VectorSlot(vector), publication};

  // Clear every status match in this shared-vector pass. A disabled shared
  // handler is skipped rather than retaining a private pending callback.
  for (intr_handle_data_t& record : registrations) {
    uint64_t state = record.state.load(std::memory_order_acquire);
    while ((state & (kRecordPublished | kRecordMatched)) ==
           (kRecordPublished | kRecordMatched)) {
      if (!RecordBelongsTo(record, state, identity)) break;
      if (!record.state.compare_exchange_weak(state, state & ~kRecordMatched,
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire)) {
        continue;
      }
      if ((state & kRecordEnabled) != 0) {
        intr_handler_t handler = record.handler.load(std::memory_order_acquire);
        void* handler_argument =
            record.argument.load(std::memory_order_acquire);
        if (handler != nullptr) handler(handler_argument);
      }
      break;
    }
  }
}

int NormalizeFlags(int flags) {
  if ((flags & ESP_INTR_FLAG_LEVELMASK) == 0) {
    flags |= (flags & ESP_INTR_FLAG_SHARED) != 0 ? ESP_INTR_FLAG_LEVEL1
                                                 : ESP_INTR_FLAG_LOWMED;
  }
  return flags;
}

int SelectLevel(int flags) {
  for (int level = 1; level <= 7; ++level) {
    if ((flags & (1 << level)) != 0) return level;
  }
  return 0;
}

bool IsCompatibleSharedVector(const SourceVector& vector, int flags) {
  if ((vector.flags & ESP_INTR_FLAG_SHARED) == 0 ||
      (flags & ESP_INTR_FLAG_SHARED) == 0) {
    return false;
  }
  if ((vector.flags & ESP_INTR_FLAG_IRAM) != (flags & ESP_INTR_FLAG_IRAM)) {
    return false;
  }
  return (flags & (1 << vector.level)) != 0;
}

esp_err_t RegistrationError(roo_testing::InterruptRegistrationResult result) {
  switch (result) {
    case roo_testing::InterruptRegistrationResult::kRegistered:
      return ESP_OK;
    case roo_testing::InterruptRegistrationResult::kInvalidArgument:
      return ESP_ERR_INVALID_ARG;
    case roo_testing::InterruptRegistrationResult::kNoCapacity:
      return ESP_ERR_NOT_FOUND;
    case roo_testing::InterruptRegistrationResult::kWrongContext:
    case roo_testing::InterruptRegistrationResult::kBackendUnavailable:
      return ESP_ERR_INVALID_STATE;
  }
  return ESP_FAIL;
}

void NoopInterruptHandler(void*) {}

esp_err_t AllocateInterrupt(int source, int flags, uint32_t status_register,
                            uint32_t status_mask, intr_handler_t handler,
                            void* argument, intr_handle_t* return_handle) {
  if ((flags & ESP_INTR_FLAG_SHARED) != 0 &&
      ((flags & ESP_INTR_FLAG_EDGE) != 0 || handler == nullptr || source < 0)) {
    return ESP_ERR_INVALID_ARG;
  }
  if ((flags & ESP_INTR_FLAG_HIGH) != 0 && handler != nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  if (status_register != 0 && status_mask == 0) {
    return ESP_ERR_INVALID_ARG;
  }
  if (!IsTaskOperationContext()) return ESP_ERR_INVALID_STATE;

  flags = NormalizeFlags(flags);
  portENTER_CRITICAL(nullptr);

  intr_handle_data_t* free_record = FindFreeRecord();
  if (free_record == nullptr) {
    portEXIT_CRITICAL(nullptr);
    return ESP_ERR_NOT_FOUND;
  }

  SourceVector* vector = FindSourceVector(source);
  bool new_vector = false;
  roo_testing::InterruptHandle controller_handle;
  uint64_t vector_publication = 0;
  if (vector != nullptr) {
    if (!IsCompatibleSharedVector(*vector, flags)) {
      portEXIT_CRITICAL(nullptr);
      return ESP_ERR_NOT_FOUND;
    }
    vector_publication = vector->publication.load(std::memory_order_relaxed);
  } else {
    vector = FindFreeVector();
    if (vector == nullptr) {
      portEXIT_CRITICAL(nullptr);
      return ESP_ERR_NOT_FOUND;
    }
    const roo_testing::InterruptRegistrationResult result =
        roo_testing::registerInterrupt(DispatchSourceVector, vector, false,
                                       &controller_handle);
    if (result != roo_testing::InterruptRegistrationResult::kRegistered) {
      portEXIT_CRITICAL(nullptr);
      return RegistrationError(result);
    }
    vector_publication = PublishedVectorEpoch(
        NextVectorEpoch(vector->publication.load(std::memory_order_relaxed)));
    vector->source.store(source, std::memory_order_relaxed);
    vector->controller_slot.store(controller_handle.slot,
                                  std::memory_order_relaxed);
    vector->controller_generation.store(controller_handle.generation,
                                        std::memory_order_relaxed);
    vector->flags = flags;
    vector->level = SelectLevel(flags);
    new_vector = true;
  }

  const uint64_t old_record_state =
      free_record->state.load(std::memory_order_relaxed);
  const uint64_t record_flags =
      kRecordPublished |
      ((flags & ESP_INTR_FLAG_INTRDISABLED) == 0 ? kRecordEnabled : 0);
  free_record->vector_slot.store(VectorSlot(*vector),
                                 std::memory_order_relaxed);
  free_record->vector_publication.store(vector_publication,
                                        std::memory_order_relaxed);
  free_record->handler.store(
      handler == nullptr ? NoopInterruptHandler : handler,
      std::memory_order_relaxed);
  free_record->argument.store(argument, std::memory_order_relaxed);
  // Real non-shared dispatch ignores intrstatus metadata.
  free_record->status_register.store(
      (flags & ESP_INTR_FLAG_SHARED) != 0 ? status_register : 0,
      std::memory_order_relaxed);
  free_record->status_mask.store(
      (flags & ESP_INTR_FLAG_SHARED) != 0 ? status_mask : 0,
      std::memory_order_relaxed);
  free_record->state.store(
      RecordState(NextRecordEpoch(old_record_state), record_flags),
      std::memory_order_release);

  if (new_vector) {
    vector->publication.store(vector_publication, std::memory_order_release);
  }
  const VectorIdentity identity{VectorSlot(*vector), vector_publication};
  if (!UpdateVectorEnablement(identity)) {
    free_record->state.store(
        RecordState(
            RecordEpoch(free_record->state.load(std::memory_order_relaxed)), 0),
        std::memory_order_release);
    if (new_vector) {
      vector->publication.store(
          PublishedVectorEpoch(VectorEpoch(vector_publication)) &
              ~kVectorPublished,
          std::memory_order_release);
      roo_testing::unregisterInterrupt(controller_handle);
    }
    portEXIT_CRITICAL(nullptr);
    return ESP_ERR_INVALID_STATE;
  }

  if (return_handle != nullptr) *return_handle = free_record;
  portEXIT_CRITICAL(nullptr);
  return ESP_OK;
}

esp_err_t SetRecordEnabled(intr_handle_data_t& record, bool enabled) {
  portENTER_CRITICAL(nullptr);
  uint64_t state = record.state.load(std::memory_order_acquire);
  while (IsRecordPublished(state)) {
    VectorIdentity identity;
    if (!LoadRecordIdentity(record, state, &identity)) {
      state = record.state.load(std::memory_order_acquire);
      continue;
    }
    const uint64_t desired =
        enabled ? state | kRecordEnabled : state & ~kRecordEnabled;
    if (!record.state.compare_exchange_weak(state, desired,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
      continue;
    }
    const bool updated = UpdateVectorEnablement(identity);
    portEXIT_CRITICAL(nullptr);
    return updated ? ESP_OK : ESP_ERR_INVALID_ARG;
  }
  portEXIT_CRITICAL(nullptr);
  return ESP_ERR_INVALID_ARG;
}

}  // namespace

namespace roo_testing::esp_idf {

void raiseInterruptSource(int source) noexcept {
  raiseInterruptSource(source, nullptr, 0);
}

void raiseInterruptSource(int source, const InterruptStatusSnapshot* snapshots,
                          size_t snapshot_count) noexcept {
  if (snapshots == nullptr) snapshot_count = 0;
  for (SourceVector& vector : source_vectors) {
    const uint64_t publication =
        vector.publication.load(std::memory_order_acquire);
    if (!IsVectorPublished(publication) ||
        vector.source.load(std::memory_order_relaxed) != source ||
        vector.publication.load(std::memory_order_acquire) != publication) {
      continue;
    }

    const VectorIdentity identity{VectorSlot(vector), publication};
    bool matched = false;
    for (intr_handle_data_t& record : registrations) {
      matched =
          MarkRecordMatched(record, identity, snapshots, snapshot_count) ||
          matched;
    }
    if (matched) {
      InterruptHandle controller_handle;
      if (LoadVectorControllerHandle(identity, &controller_handle)) {
        setInterruptPending(controller_handle);
      }
    }
    // Exactly one vector may own a source. Once this assertion has observed a
    // stable owner, it must not cross free/reuse and discover a replacement in
    // a later slot during the same call.
    return;
  }
}

}  // namespace roo_testing::esp_idf

extern "C" {

esp_err_t esp_intr_alloc(int source, int flags, intr_handler_t handler,
                         void* argument, intr_handle_t* return_handle) {
  return AllocateInterrupt(source, flags, 0, 0, handler, argument,
                           return_handle);
}

esp_err_t esp_intr_alloc_intrstatus(int source, int flags,
                                    uint32_t status_register,
                                    uint32_t status_mask,
                                    intr_handler_t handler, void* argument,
                                    intr_handle_t* return_handle) {
  return AllocateInterrupt(source, flags, status_register, status_mask, handler,
                           argument, return_handle);
}

esp_err_t esp_intr_free(intr_handle_t handle) {
  if (!IsTaskOperationContext()) return ESP_ERR_INVALID_STATE;
  intr_handle_data_t* record = FindRecord(handle);
  if (record == nullptr) return ESP_ERR_INVALID_ARG;

  portENTER_CRITICAL(nullptr);
  uint64_t state = record->state.load(std::memory_order_acquire);
  VectorIdentity identity;
  bool unpublished = false;
  while (IsRecordPublished(state)) {
    if (!LoadRecordIdentity(*record, state, &identity)) {
      state = record->state.load(std::memory_order_acquire);
      continue;
    }
    if (record->state.compare_exchange_weak(
            state, RecordState(RecordEpoch(state), 0),
            std::memory_order_acq_rel, std::memory_order_acquire)) {
      unpublished = true;
      break;
    }
  }
  if (!unpublished) {
    portEXIT_CRITICAL(nullptr);
    return ESP_ERR_INVALID_ARG;
  }

  record->handler.store(nullptr, std::memory_order_relaxed);
  record->argument.store(nullptr, std::memory_order_relaxed);
  record->status_register.store(0, std::memory_order_relaxed);
  record->status_mask.store(0, std::memory_order_relaxed);
  record->vector_slot.store(static_cast<uint32_t>(-1),
                            std::memory_order_relaxed);
  record->vector_publication.store(0, std::memory_order_relaxed);

  bool removed = true;
  if (!AnyLiveRecord(identity)) {
    roo_testing::InterruptHandle controller_handle;
    removed = LoadVectorControllerHandle(identity, &controller_handle);
    SourceVector& vector = source_vectors[identity.slot];
    vector.publication.store(
        PublishedVectorEpoch(VectorEpoch(identity.publication)) &
            ~kVectorPublished,
        std::memory_order_release);
    if (removed) {
      removed = roo_testing::unregisterInterrupt(controller_handle);
    }
    vector.controller_slot.store(static_cast<uint32_t>(-1),
                                 std::memory_order_relaxed);
    vector.controller_generation.store(0, std::memory_order_relaxed);
    vector.source.store(0, std::memory_order_relaxed);
    vector.flags = 0;
    vector.level = 0;
  } else {
    removed = UpdateVectorEnablement(identity);
  }
  portEXIT_CRITICAL(nullptr);
  return removed ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_intr_enable(intr_handle_t handle) {
  intr_handle_data_t* record = FindRecord(handle);
  return record == nullptr ? ESP_ERR_INVALID_ARG
                           : SetRecordEnabled(*record, true);
}

esp_err_t esp_intr_disable(intr_handle_t handle) {
  intr_handle_data_t* record = FindRecord(handle);
  return record == nullptr ? ESP_ERR_INVALID_ARG
                           : SetRecordEnabled(*record, false);
}

}  // extern "C"
