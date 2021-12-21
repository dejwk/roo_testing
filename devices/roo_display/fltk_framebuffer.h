#pragma once

#include <memory>

class EventQueue;

class Framebuffer {
 public:
  enum Rotation {
    kRotationNone,
    kRotationRight,
    kRotationUpsideDown,
    kRotationLeft
  };

  Framebuffer(int16_t width, int16_t height, int magnification,
              Rotation rotation = kRotationNone);
              
  Framebuffer(Framebuffer&& other);
  Framebuffer(const Framebuffer& other);  // Left unimplemented.

  ~Framebuffer();

  void init();

  void setRotation(Rotation rotation);

  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                uint32_t color_rgb);
  void flush();
  bool isMouseClicked(int16_t* x, int16_t* y);

  int16_t width() const { return width_; }
  int16_t height() const { return height_; }

 private:
  int16_t width_;
  int16_t height_;
  int magnification_;
  bool xy_swapped_;
  bool bottom_to_top_;
  bool right_to_left_;
  EventQueue* queue_;
};
