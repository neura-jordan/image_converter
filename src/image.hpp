#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

struct Image {
  int width;
  int height;
  int channels;              // 3 for RGB, 4 for RGBA
  std::vector<uint8_t> data; // Row-major order: r,g,b, r,g,b...

  Image(int w, int h, int c) : width(w), height(h), channels(c) {
    data.resize(static_cast<size_t>(w) * h * c);
  }

  Image() : width(0), height(0), channels(0) {}
};

#endif // IMAGE_HPP
