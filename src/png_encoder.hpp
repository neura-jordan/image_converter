#ifndef PNG_ENCODER_HPP
#define PNG_ENCODER_HPP

#include "image.hpp"
#include <cstdint>
#include <string>
#include <vector>

class PngEncoder {
public:
  // Encodes the image to a PNG file (uncompressed)
  static void encode(const Image &img, const std::string &filepath);

private:
  static void writeChunk(std::ofstream &file, const char *type,
                         const std::vector<uint8_t> &data);
  static void writeIHDR(std::ofstream &file, int width, int height,
                        int channels);
  static void writeIDAT(std::ofstream &file, const Image &img);
  static void writeIEND(std::ofstream &file);
};

#endif // PNG_ENCODER_HPP
