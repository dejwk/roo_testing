#include <memory>

#include "FL/Fl_Image.H"

int main() {
  unsigned char pixels[] = {0x12, 0x34, 0x56};
  Fl_RGB_Image source(pixels, 1, 1, 3);
  std::unique_ptr<Fl_Image> scaled(source.copy(2, 2));
  return scaled == nullptr;
}
