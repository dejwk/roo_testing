// TFT emulator for the esp-display library.
// It uses the FLTK library to render data on the screen.

/* Standard headers */
#include <pthread.h>
#include <stdlib.h>

#include <iostream>
#include <mutex>
#include <queue>

/* Fltk headers */
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

#include <Arduino.h>
#include "fltk_device.h"

//#include "esp_display.h"
//#include "offscreen.h"

using namespace espdisplay;

class DeviceManager;

DeviceManager* manager();

void setup();
void loop();

struct Message {
  Message() {}
  enum Type { INIT, DRAWPIXEL } type;
  union Content {
    Content() {}
    struct PixelMessage {
      PixelMessage() {}
      PixelMessage(int x, int y, Color color) : x(x), y(y), color(color) {}
      int x;
      int y;
      Color color;
    } drawpixel;

    struct InitMessage {
      InitMessage() {}
      int width;
      int height;
      int magnification;
    } init;
  } content;
  ~Message() {}
};

Message initDisplay(int width, int height, int magnification) {
  Message message;
  message.type = Message::INIT;
  message.content.init.width = width;
  message.content.init.height = height;
  message.content.init.magnification = magnification;
  return message;
}

Message drawPixel(int x, int y, Color color) {
  Message message;
  message.type = Message::DRAWPIXEL;
  message.content.drawpixel.x = x;
  message.content.drawpixel.y = y;
  message.content.drawpixel.color = color;
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
        nonfull_(PTHREAD_COND_INITIALIZER) {}
  // bool hasInitMessage() {
  //   MutexLock lock(&mutex_);
  //   return (!queue_.empty() && queue_.front().type == Message::INIT);
  // }

  // bool hasPixelMessage() {
  //   MutexLock lock(&mutex_);
  //   return (!queue_.empty() && queue_.front().type == Message::DRAWPIXEL);
  // }

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
    if (message.type == Message::INIT) {
      std::cout << "Initializing queue: " << queue_.size();
    }
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
      } else if (queue_.front().type == Message::DRAWPIXEL) {
      }
      queue_.pop();
    }
    return false;
  }

  bool popPixelMessage(Message::Content::PixelMessage *message) {
    MutexLock lock(&mutex_);
    while (!queue_.empty()) {
      if (queue_.size() == capacity_) {
        pthread_cond_signal(&nonfull_);
      }
      if (queue_.front().type == Message::DRAWPIXEL) {
        *message = queue_.front().content.drawpixel;
        queue_.pop();
        return true;
      }
      queue_.pop();
    }
    return false;
  }

 private:
  std::queue<Message> queue_;
  size_t capacity_;
  mutable pthread_mutex_t mutex_;
  pthread_cond_t nonempty_;
  pthread_cond_t nonfull_;
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
  OffscreenBox(int w, int h, int magnification, EventQueue* queue)
      : Fl_Box(0, 0, w * magnification, h * magnification),
        w_(w),
        h_(h),
        magnification_(magnification),
        queue_(queue) {}

  ~OffscreenBox() {}

  int16_t width() const { return w_; }
  int16_t height() const { return h_; }

  int magnification() const { return magnification_; }

 private:
  void draw() {
    int i = 0;
    //long time = millis();
    Message::Content::PixelMessage msg;
    while (queue_->popPixelMessage(&msg)) {
      fl_rectf(msg.x * magnification_, msg.y * magnification_, magnification_,
               magnification_, msg.color.asRgbi());
      ++i;
    }
    //long elapsed = millis() - time;
    //    Serial.println(String("Drawn ") + i + " pixels in " + elapsed + "
    //    ms");
  }

  int w_, h_;
  int magnification_;
  EventQueue* queue_;
};

/*****************************************************************************/
// static OffscreenBox *os_box = 0; // a widget to view the offscreen with

/*****************************************************************************/

void fillRect(EventQueue* queue, int x, int y, int w, int h, Color color) {
  for (int iy = y; iy < y + h; ++iy) {
    for (int ix = x; ix < x + w; ++ix) {
      queue->push(drawPixel(ix, iy, color));
    }
  }
}

EmulatorDevice::EmulatorDevice(int w, int h, int magnification)
    : queue_(new EventQueue(100000)), bgcolor_(0xFF7F7F7F), width_(w), height_(h) {
  queue_->push(initDisplay(w, h, magnification));
}

EmulatorDevice::~EmulatorDevice() { delete queue_; }

void EmulatorDevice::begin() {}
void EmulatorDevice::end() { Fl::awake(); }

void EmulatorDevice::fillColor(Color color, uint32_t pixel_count) {
  while (pixel_count-- > 0) {
    fillRect(queue_, cursor_x_, cursor_y_, 1, 1, color);
    advance();
  }
}

void EmulatorDevice::writeColors(Color *colors, uint32_t pixel_count) {
  for (uint32_t i = 0; i < pixel_count; ++i) {
    fillRect(queue_, cursor_x_, cursor_y_, 1, 1, colors[i]);
    advance();
  }
}

void EmulatorDevice::drawPixels(int16_t *xs, int16_t *ys, Color *colors,
                                uint16_t pixel_count) {
  for (int i = 0; i < pixel_count; ++i) {
    fillRect(queue_, xs[i], ys[i], 1, 1, colors[i]);
  }
}

void EmulatorDevice::paintPixels(int16_t *xs, int16_t *ys, Color *colors,
                                 uint16_t pixel_count) {
  for (int i = 0; i < pixel_count; ++i) {
    fillRect(queue_, xs[i], ys[i], 1, 1, alphaBlend(bgcolor_, colors[i].asArgb()));
  }
}

int16_t EmulatorDevice::width() const { return width_; }
int16_t EmulatorDevice::height() const { return height_; }

void EmulatorDevice::advance() {
  ++cursor_x_;
  if (cursor_x_ > addr_x1_) {
    cursor_x_ = addr_x0_;
    ++cursor_y_;
  }
}

class DisplayDeviceHandle {
 public:
  DisplayDeviceHandle(EventQueue* queue) : initialized_(false), queue_(queue) {}

  void show(int width, int height, int magnification) {
    window_.reset(
        new Fl_Window(width * magnification,
                      height * magnification, "esp-display emulator"));
    window_->begin();
    box_.reset(new OffscreenBox(width, height, magnification, queue_));
    window_->end();
    window_->show();
    window_->make_current();
    Serial.println("Window shown");
  }

  void refresh() {
    if (queue_->empty()) return;
    Serial.println("Queue size");
    Serial.println(queue_->size());
    if (!initialized_) {
      Message::Content::InitMessage msg;
      Serial.println("Seeking the INIT message");
      if (!queue_->popInitMessage(&msg)) return;
      Serial.println("Showing");
      show(msg.width, msg.height, msg.magnification);
      initialized_ = true;
    }
    box_->damage(FL_DAMAGE_ALL);
  }

 private:
  bool initialized_;
  std::unique_ptr<Fl_Window> window_;
  std::unique_ptr<OffscreenBox> box_;
  EventQueue* queue_;
};

extern "C" void *device_func(void *p);

class DeviceManager {
 public:
  DeviceManager()
      : device_(nullptr),
        device_mutex_(PTHREAD_MUTEX_INITIALIZER),
        nonempty_(PTHREAD_COND_INITIALIZER) {
    fl_create_thread(device_thread, device_func, this);
  }

  void run() {
    Fl::lock();
    while (Fl::wait() > 0 || getDevice()) {
      DisplayDeviceHandle* device = getDevice();
      device->refresh();
    }
    Serial.println("Leaving the loop");
  }

  void addDevice(EventQueue* queue) {
    MutexLock lock(&device_mutex_);
    device_.reset(new DisplayDeviceHandle(queue));
    pthread_cond_signal(&nonempty_);
  }

 private:
  std::unique_ptr<DisplayDeviceHandle> device_;

  DisplayDeviceHandle* getDevice() {
    MutexLock lock(&device_mutex_);
    while (device_.get() == nullptr) {
      pthread_cond_wait(&nonempty_, &device_mutex_);
    }
    return device_.get();
  }

  mutable pthread_mutex_t device_mutex_;
  pthread_cond_t nonempty_;
};

extern "C" void *device_func(void *p) {
  DeviceManager* manager = (DeviceManager*)p;
  manager->run();
}


DeviceManager* manager() {
  static DeviceManager manager;
  return &manager;
}

int main(int argc, char **argv) {
  Serial.println("Main");

  setup();
  while (true) {
    loop();
    delay(1);
  }
}

void EmulatorDevice::init() {
  manager()->addDevice(queue_);
  fillRect(queue_, 0, 0, width_, height_, Color(128, 128, 128));
}

