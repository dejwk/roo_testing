#include <pthread.h>
#include <stdlib.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

// #define FLTK_DEVICE_NOISE_BITS 6
// #define FLTK_MAX_PIXELS_PER_MS 100

/* Fltk headers */
#include <FL/Enumerations.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

#include "roo_testing/transducers/ui/fltk_viewport.h"

class DeviceManager;

DeviceManager *manager();

void setup();
void loop();

struct Message {
  Message() {}
  enum Type { INIT, FILLRECT } type;
  union Content {
    Content() {}
    struct FillRectMessage {
      FillRectMessage() {}
      FillRectMessage(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      uint32_t color_argb)
          : x0(x0), y0(y0), x1(x1), y1(y1), color_argb(color_argb) {}
      int x0;
      int y0;
      int x1;
      int y1;
      uint32_t color_argb;
    } fillrect;

    struct InitMessage {
      InitMessage() {}
      int width;
      int height;
    } init;
  } content;
  ~Message() {}
};

Message createInitDisplayMsg(int width, int height) {
  Message message;
  message.type = Message::INIT;
  message.content.init.width = width;
  message.content.init.height = height;
  return message;
}

Message createFillRectMsg(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                          uint32_t color_argb) {
  Message message;
  message.type = Message::FILLRECT;
  message.content.fillrect =
      Message::Content::FillRectMessage(x0, y0, x1, y1, color_argb);
  return message;
}

class MutexLock {
 public:
  MutexLock(pthread_mutex_t *mutex) : mutex_(mutex) {
    pthread_mutex_lock(mutex_);
  }
  ~MutexLock() { pthread_mutex_unlock(mutex_); }

 private:
  pthread_mutex_t *mutex_;
};

class EventQueue {
 public:
  EventQueue(int capacity)
      : queue_(),
        capacity_(capacity),
        mutex_(PTHREAD_MUTEX_INITIALIZER),
        nonempty_(PTHREAD_COND_INITIALIZER),
        nonfull_(PTHREAD_COND_INITIALIZER),
        mouse_x_(0),
        mouse_y_(0),
        mouse_pressed_(false) {}

  void push(Message message) {
    MutexLock lock(&mutex_);
    while (queue_.size() >= capacity_) {
      pthread_cond_wait(&nonfull_, &mutex_);
    }
    if (queue_.empty()) {
      pthread_cond_signal(&nonempty_);
      Fl::awake();
    }
    queue_.push(message);
  }

  bool empty() const {
    MutexLock lock(&mutex_);
    return queue_.empty();
  }

  bool full() const {
    MutexLock lock(&mutex_);
    return queue_.size() == capacity_;
  }

  int size() const {
    MutexLock lock(&mutex_);
    return queue_.size();
  }

  bool popInitMessage(Message::Content::InitMessage *message) {
    MutexLock lock(&mutex_);
    while (!queue_.empty()) {
      if (queue_.size() == capacity_) {
        pthread_cond_signal(&nonfull_);
      }
      if (queue_.front().type == Message::INIT) {
        *message = queue_.front().content.init;
        queue_.pop();
        return true;
      } else if (queue_.front().type == Message::FILLRECT) {
      }
      queue_.pop();
    }
    return false;
  }

  bool popFillRectMessage(Message::Content::FillRectMessage *message) {
    MutexLock lock(&mutex_);
    while (!queue_.empty()) {
      if (queue_.size() == capacity_) {
        pthread_cond_signal(&nonfull_);
      }
      if (queue_.front().type == Message::FILLRECT) {
        *message = queue_.front().content.fillrect;
        queue_.pop();
        return true;
      }
      queue_.pop();
    }
    return false;
  }

  void get_mouse_status(int16_t *x, int16_t *y, bool *pressed) const {
    MutexLock lock(&mutex_);
    *x = mouse_x_;
    *y = mouse_y_;
    *pressed = mouse_pressed_;
  }

  void set_mouse_status(int16_t x, int16_t y, bool pressed) {
    MutexLock lock(&mutex_);
    mouse_x_ = x;
    mouse_y_ = y;
    mouse_pressed_ = pressed;
    //    Serial.println(String("X ") + x + " y " + y + " pressed :" + pressed);
  }

 private:
  std::queue<Message> queue_;
  size_t capacity_;
  mutable pthread_mutex_t mutex_;
  pthread_cond_t nonempty_;
  pthread_cond_t nonfull_;

  int16_t mouse_x_, mouse_y_;
  bool mouse_pressed_;
};

typedef pthread_t Fl_Thread;
extern "C" {
typedef void *(Fl_Thread_Func)(void *);
}

static int fl_create_thread(Fl_Thread &t, Fl_Thread_Func *f, void *p) {
  return pthread_create((pthread_t *)&t, 0, f, p);
}

static Fl_Thread device_thread;

class OffscreenBox : public Fl_Box {
 public:
  OffscreenBox(int w, int h, EventQueue *queue)
      : Fl_Box(0, 0, w, h), w_(w), h_(h), queue_(queue) {}

  ~OffscreenBox() {}

  int16_t raw_width() const { return w_; }
  int16_t raw_height() const { return h_; }

  int handle(int event) override {
    switch (event) {
      case FL_ENTER: {
        queue_->set_mouse_status(Fl::event_x(), Fl::event_y(), false);
        return 1;
      }
      case FL_LEAVE: {
        queue_->set_mouse_status(Fl::event_x(), Fl::event_y(), false);
        return 0;
      }
      case FL_DRAG: {
        queue_->set_mouse_status(Fl::event_x(), Fl::event_y(), true);
        return 0;
      }
      case FL_MOVE: {
        queue_->set_mouse_status(Fl::event_x(), Fl::event_y(), false);
        return 0;
      }
      case FL_PUSH:
        queue_->set_mouse_status(Fl::event_x(), Fl::event_y(), true);
        return 1;
      case FL_RELEASE: {
        queue_->set_mouse_status(Fl::event_x(), Fl::event_y(), false);
        return 1;
      }
      default: {
        return Fl_Box::handle(event);
      }
    }
  }

 private:
  void draw() {
    int i = 0;
    // long time = millis();
    Message::Content::FillRectMessage msg;
    while (queue_->popFillRectMessage(&msg)) {
#ifdef FLTK_DEVICE_NOISE_BITS
      color ^= (rand() % (1 << FLTK_DEVICE_NOISE_BITS)) * 0x01010100;
#endif
      fl_rectf(msg.x0, msg.y0, msg.x1 - msg.x0 + 1, msg.y1 - msg.y0 + 1,
               msg.color_argb << 8);
      ++i;
    }
    // long elapsed = millis() - time;
    //    Serial.println(String("Drawn ") + i + " pixels in " + elapsed + "
    //    ms");
  }

  int w_, h_;
  EventQueue *queue_;
};

/*****************************************************************************/
// static OffscreenBox *os_box = 0; // a widget to view the offscreen with

/*****************************************************************************/

int pixelCounter = 0;

void FltkViewport::fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            uint32_t color_argb) {
  queue_->push(createFillRectMsg(x0, y0, x1, y1, color_argb));
}

FltkViewport::FltkViewport() : queue_(new EventQueue(100000)) {}

FltkViewport::~FltkViewport() { delete queue_; }

void FltkViewport::flush() { Fl::awake(); }

bool FltkViewport::isMouseClicked(int16_t *x, int16_t *y) {
  bool result;
  queue_->get_mouse_status(x, y, &result);
  if (!result) return false;
  if (*x < 0 || *y < 0 || *x >= width() || *y >= height()) return false;
  return true;
}

class Device {
 public:
  Device(EventQueue *queue) : initialized_(false), queue_(queue) {}

  void show(int width, int height) {
    window_.reset(new Fl_Window(width, height, "roo_display emulator"));
    window_->begin();
    box_.reset(new OffscreenBox(width, height, queue_));
    window_->end();
    window_->show();
    window_->make_current();
  }

  void refresh() {
    if (queue_->empty()) return;
    // Serial.println("Queue size");
    // Serial.println(queue_->size());
    if (!initialized_) {
      Message::Content::InitMessage msg;
      if (!queue_->popInitMessage(&msg)) return;
      show(msg.width, msg.height);
      initialized_ = true;
    }
    box_->damage(FL_DAMAGE_ALL);
  }

 private:
  bool initialized_;
  std::unique_ptr<Fl_Window> window_;
  std::unique_ptr<OffscreenBox> box_;
  EventQueue *queue_;
};

extern "C" void *device_func(void *p);

class DeviceManager {
 public:
  DeviceManager()
      : device_(nullptr),
        exiting_(false),
        exited_(false),
        device_mutex_(PTHREAD_MUTEX_INITIALIZER),
        nonempty_(PTHREAD_COND_INITIALIZER) {
    fl_create_thread(device_thread, device_func, this);
  }

  ~DeviceManager() {
    exit();
    while (!exited()) {
    }
  }

  void run() {
    Fl::lock();
    while (Fl::wait() > 0 || getDevice()) {
      if (exiting()) break;
      Device *device = getDevice();
      device->refresh();
    }
    MutexLock lock(&device_mutex_);
    exited_ = true;
  }

  void addDevice(EventQueue *queue) {
    MutexLock lock(&device_mutex_);
    device_.reset(new Device(queue));
    pthread_cond_signal(&nonempty_);
  }

  void exit() {
    MutexLock lock(&device_mutex_);
    exiting_ = true;
  }

  bool exiting() {
    MutexLock lock(&device_mutex_);
    return exiting_;
  }

  bool exited() {
    MutexLock lock(&device_mutex_);
    return exited_;
  }

 private:
  Device *getDevice() {
    MutexLock lock(&device_mutex_);
    while (device_.get() == nullptr) {
      pthread_cond_wait(&nonempty_, &device_mutex_);
    }
    return device_.get();
  }

  std::unique_ptr<Device> device_;
  bool exiting_;
  bool exited_;

  mutable pthread_mutex_t device_mutex_;
  pthread_cond_t nonempty_;
};

extern "C" void *device_func(void *p) {
  DeviceManager *manager = (DeviceManager *)p;
  manager->run();
  return nullptr;
}

DeviceManager *manager() {
  static DeviceManager manager;
  return &manager;
}

int main(int argc, char **argv) {
  setup();
  while (true) {
    loop();
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }
}

void FltkViewport::init(int16_t width, int16_t height) {
  Viewport::init(width, height);
  manager()->addDevice(queue_);
  queue_->push(createInitDisplayMsg(width, height));
  int16_t x1 = width - 1;
  int16_t y1 = height - 1;
  uint32_t gray = 0xFF808080;
  // for (int iy = 0; iy <= y1; ++iy) {
  for (int ix = 0; ix <= x1; ++ix) {
    queue_->push(
        createFillRectMsg(ix, 0, ix, y1, gray + 0x010101 * (rand() % 64)));
  }
  // }
}
