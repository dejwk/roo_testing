#pragma once

#include <memory>

class EventQueue;

class Viewport {
 public:
  Viewport() : width_(-1), height_(-1) {}

  virtual ~Viewport() {}

  virtual void init(int16_t width, int16_t height) {
    width_ = width;
    height_ = height;
  }

  virtual void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                        uint32_t color_argb) = 0;

  virtual void flush() {}

  virtual bool isMouseClicked(int16_t* x, int16_t* y) { return false; }

  int16_t width() const { return width_; }
  int16_t height() const { return height_; }

 private:
  int16_t width_;
  int16_t height_;
};

class FlexViewport : public Viewport {
 public:
  enum Rotation {
    kRotationNone,
    kRotationRight,
    kRotationUpsideDown,
    kRotationLeft
  };

  FlexViewport(Viewport& delegate, int magnification,
               Rotation rotation = kRotationNone);

  virtual ~FlexViewport() {}

  void init(int16_t width, int16_t height) override {
    Viewport::init(width, height);
    if (xy_swapped_) {
      std::swap(width, height);
    }
    delegate_.init(width * magnification(), height * magnification());
  }

  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                uint32_t color_argb) override;

  void flush() override { delegate_.flush(); }

  bool isMouseClicked(int16_t* x, int16_t* y) override;

  int magnification() const { return magnification_; }

 private:
  Viewport& delegate_;
  int magnification_;
  bool xy_swapped_;
  bool bottom_to_top_;
  bool right_to_left_;
};

class FltkViewport : public Viewport {
 public:
  FltkViewport();

  ~FltkViewport();

  void init(int16_t width, int16_t height) override;

  void flush() override;

  bool isMouseClicked(int16_t* x, int16_t* y) override;

  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                uint32_t color_argb) override;

 private:
  EventQueue* queue_;
};
