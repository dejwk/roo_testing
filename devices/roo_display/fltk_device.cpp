// TFT emulator for the esp-display library.
// It uses the FLTK library to render data on the screen.

/* Standard headers */
#include <pthread.h>
#include <stdlib.h>

#include <iostream>
#include <mutex>
#include <queue>

// #define FLTK_DEVICE_NOISE_BITS 6
// #define FLTK_MAX_PIXELS_PER_MS 100

/* Fltk headers */
#include <FL/Enumerations.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

#include <Arduino.h>
#include "fltk_device.h"

using namespace roo_display;

class DeviceManager;

DeviceManager *manager();

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
  OffscreenBox(int w, int h, int magnification, EventQueue *queue)
      : Fl_Box(0, 0, w * magnification, h * magnification),
        w_(w),
        h_(h),
        magnification_(magnification),
        queue_(queue) {}

  ~OffscreenBox() {}

  int16_t raw_width() const { return w_; }
  int16_t raw_height() const { return h_; }

  int magnification() const { return magnification_; }

  int handle(int event) override {
    switch (event) {
      case FL_ENTER: {
        queue_->set_mouse_status(Fl::event_x() / magnification_,
                                 Fl::event_y() / magnification_, false);
        return 1;
      }
      case FL_LEAVE: {
        queue_->set_mouse_status(Fl::event_x() / magnification_,
                                 Fl::event_y() / magnification_, false);
        return 0;
      }
      case FL_DRAG: {
        queue_->set_mouse_status(Fl::event_x() / magnification_,
                                 Fl::event_y() / magnification_, true);
        return 0;
      }
      case FL_MOVE: {
        queue_->set_mouse_status(Fl::event_x() / magnification_,
                                 Fl::event_y() / magnification_, false);
        return 0;
      }
      case FL_PUSH:
        queue_->set_mouse_status(Fl::event_x() / magnification_,
                                 Fl::event_y() / magnification_, true);
        return 1;
      case FL_RELEASE: {
        queue_->set_mouse_status(Fl::event_x() / magnification_,
                                 Fl::event_y() / magnification_, false);
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
    Message::Content::PixelMessage msg;
    while (queue_->popPixelMessage(&msg)) {
      uint32_t color = msg.color.asArgb() << 8;
#ifdef FLTK_DEVICE_NOISE_BITS
  color ^= (rand() % (1 << FLTK_DEVICE_NOISE_BITS)) * 0x01010100;
#endif
      fl_rectf(msg.x * magnification_, msg.y * magnification_, magnification_,
               magnification_, color);
      ++i;
    }
    // long elapsed = millis() - time;
    //    Serial.println(String("Drawn ") + i + " pixels in " + elapsed + "
    //    ms");
  }

  int w_, h_;
  int magnification_;
  EventQueue *queue_;
};

/*****************************************************************************/
// static OffscreenBox *os_box = 0; // a widget to view the offscreen with

/*****************************************************************************/

int pixelCounter = 0;

void Framebuffer::fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color) {
  if (orientation_.isXYswapped()) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (orientation_.isBottomToTop()) {
    std::swap(y0, y1);
    y0 = height_ - y0;
    y1 = height_ - y1;
  }
  if (orientation_.isRightToLeft()) {
    std::swap(x0, x1);
    x0 = width_ - x0;
    x1 = width_ - x1;
  }

  for (int iy = y0; iy <= y1; ++iy) {
    for (int ix = x0; ix <= x1; ++ix) {
      queue_->push(drawPixel(ix, iy, color));
#ifdef FLTK_MAX_PIXELS_PER_MS
      if (++pixelCounter >= FLTK_MAX_PIXELS_PER_MS) {
        pixelCounter = 0;
        delay(1);
      }
#endif
    }
  }
}

void EmulatorDevice::orientationUpdated() {
  framebuffer_.setOrientation(orientation());
}


// void EmulatorDevice::rectFill(int x, int y, int w, int h, Color color) {
//   Orientation my_orientation = orientation();
//   if (my_orientation.isXYswapped()) {
//     std::swap(x, y);
//     std::swap(w, h);
//   }
//   if (my_orientation.isBottomToTop()) {
//     y = raw_height() - y - h;
//   }
//   if (my_orientation.isRightToLeft()) {
//     x = raw_width() - x - w;
//   }

//   for (int iy = y; iy < y + h; ++iy) {
//     for (int ix = x; ix < x + w; ++ix) {
//       queue_->push(drawPixel(ix, iy, color));
//     }
//   }
// }

Framebuffer::Framebuffer(int16_t width, int16_t height, int magnification)
    : width_(width), height_(height), magnification_(magnification),
      orientation_(),
      queue_(new EventQueue(100000)) {
  queue_->push(initDisplay(width, height, magnification));
}

Framebuffer::Framebuffer(Framebuffer&& other) {
  width_ = other.width_;
  height_ = other.height_;
  magnification_ = other.magnification_;
  orientation_ = other.orientation_;
  queue_ = other.queue_;
  other.queue_ = nullptr;
}

Framebuffer::~Framebuffer() { delete queue_; }

void Framebuffer::flush() { Fl::awake(); }

EmulatorDevice::EmulatorDevice(int w, int h, int magnification)
    : DisplayDevice(w, h),
      framebuffer_(w, h, magnification),
      bgcolor_(0xFF7F7F7F) {}

void EmulatorDevice::begin() {}
void EmulatorDevice::end() { framebuffer_.flush(); }

Color EmulatorDevice::effective_color(roo_display::PaintMode mode, Color color) {
  switch (mode) {
    case PAINT_MODE_REPLACE: {
      return color;
      break;
    }
    case PAINT_MODE_BLEND: {
      return alphaBlend(bgcolor_, color);
    }
    default: {
      return color;
    }
  }
}

void EmulatorDevice::write(roo_display::Color *colors, uint32_t pixel_count) {
  for (uint32_t i = 0; i < pixel_count; ++i) {
    framebuffer_.fillRect(cursor_x_, cursor_y_, cursor_x_, cursor_y_, effective_color(paint_mode_, colors[i]));
    advance();
  }
}

// void EmulatorDevice::fill(PaintMode mode, roo_display::Color color,
//                           uint32_t pixel_count) {
//   for (uint32_t i = 0; i < pixel_count; ++i) {
//     rectFill(cursor_x_, cursor_y_, 1, 1, effective_color(mode, color));
//     advance();
//   }
// }

void EmulatorDevice::writeRects(PaintMode mode, Color* color, int16_t *x0, int16_t *y0,
                                int16_t *x1, int16_t* y1, uint16_t count) {
  while (count-- > 0) {
    framebuffer_.fillRect(*x0++, *y0++, *x1++, *y1++, effective_color(mode, *color++));
  }
}

void EmulatorDevice::fillRects(PaintMode mode, Color color, int16_t *x0, int16_t *y0,
                               int16_t *x1, int16_t* y1, uint16_t count) {
  while (count-- > 0) {
    framebuffer_.fillRect(*x0++, *y0++, *x1++, *y1++, effective_color(mode, color));
  }
}

void EmulatorDevice::writePixels(PaintMode mode, roo_display::Color *colors,
                                 int16_t *xs, int16_t *ys,
                                 uint16_t pixel_count) {
  for (int i = 0; i < pixel_count; ++i) {
    framebuffer_.fillRect(xs[i], ys[i], xs[i], ys[i], effective_color(mode, colors[i]));
  }
}

void EmulatorDevice::fillPixels(PaintMode mode, roo_display::Color color,
                                int16_t *xs, int16_t *ys,
                                uint16_t pixel_count) {
  for (int i = 0; i < pixel_count; ++i) {
    framebuffer_.fillRect(xs[i], ys[i], xs[i], ys[i], effective_color(mode, color));
  }
}

void EmulatorDevice::advance() {
  ++cursor_x_;
  if (cursor_x_ > addr_x1_) {
    cursor_x_ = addr_x0_;
    ++cursor_y_;
  }
}

bool Framebuffer::isMouseClicked(int16_t* x, int16_t* y) {
  bool result;
  queue_->get_mouse_status(x, y, &result);
  return result;
}

bool EmulatorDevice::getTouch(int16_t *x, int16_t *y, int16_t* z) {
  int16_t x_display, y_display;
  bool result = framebuffer_.isMouseClicked(&x_display, &y_display);
  if (result) {
    *x = (int32_t)4096 * x_display / effective_width();
    *y = (int32_t)4096 * y_display / effective_height();
    *z = 100;
  }
  return result;
}

class Device {
 public:
  Device(EventQueue *queue) : initialized_(false), queue_(queue) {}

  void show(int width, int height, int magnification) {
    window_.reset(new Fl_Window(width * magnification, height * magnification,
                                "roo_display emulator"));
    window_->begin();
    box_.reset(new OffscreenBox(width, height, magnification, queue_));
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
      show(msg.width, msg.height, msg.magnification);
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
        device_mutex_(PTHREAD_MUTEX_INITIALIZER),
        nonempty_(PTHREAD_COND_INITIALIZER),
        exiting_(false),
        exited_(false) {
    fl_create_thread(device_thread, device_func, this);
  }

  ~DeviceManager() {
    exit();
    while (!exited()) {}
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
  mutable pthread_mutex_t device_mutex_;
  pthread_cond_t nonempty_;
  bool exiting_;
  bool exited_;
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
    delay(1);
  }
}

void Framebuffer::init() {
  manager()->addDevice(queue_);

  int16_t x1 = width_ - 1;
  int16_t y1 = height_ - 1;
  if (orientation_.isXYswapped()) {
    std::swap(x1, y1);
  }
  uint32_t gray = 0xFF808080;
  for (int iy = 0; iy <= y1; ++iy) {
    for (int ix = 0; ix <= x1; ++ix) {
      //queue_->push(drawPixel(ix, iy, gray));
      queue_->push(drawPixel(ix, iy, gray + 0x010101 * (rand() % 64)));
    }
  }
}

void EmulatorDevice::init() {
  framebuffer_.init();
}
