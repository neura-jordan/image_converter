#ifndef JPEG_DECODER_HPP
#define JPEG_DECODER_HPP

#include "image.hpp"
#include <cstdint>
#include <string>
#include <vector>

class JpegDecoder {
public:
  static Image decode(const std::string &filepath);

private:
  struct HuffmanTable {
    std::vector<uint8_t> bits;
    std::vector<uint8_t> huffval;
    // Lookup tables for faster decoding
    std::vector<int> minCode;
    std::vector<int> maxCode;
    std::vector<int> valPtr;
  };

  struct QuantTable {
    uint8_t values[64];
  };

  struct Component {
    int id;
    int hSampFactor;
    int vSampFactor;
    int quantTableId;
    int dcTableId;
    int acTableId;
    int prevDC;
    std::vector<uint8_t> rawData; // For now, maybe just keep track of blocks
  };

  // JPEG Markers
  static const uint16_t SOI = 0xFFD8;
  static const uint16_t SOF0 = 0xFFC0;
  static const uint16_t DHT = 0xFFC4;
  static const uint16_t DQT = 0xFFDB;
  static const uint16_t SOS = 0xFFDA;
  static const uint16_t EOI = 0xFFD9;
  static const uint16_t COM = 0xFFFE;
  static const uint16_t APP0 = 0xFFE0;

  // ZigZag order
  static const uint8_t ZIGZAG[64];

  static void parseSegments(const std::vector<uint8_t> &data,
                            std::vector<QuantTable> &quantTables,
                            std::vector<HuffmanTable> &dcTables,
                            std::vector<HuffmanTable> &acTables,
                            std::vector<Component> &components, int &width,
                            int &height, const uint8_t *&scanData,
                            size_t &scanDataLen);

  static void buildHuffmanTable(HuffmanTable &table);
  static int decodeHuffman(const uint8_t *data, size_t &bitOffset,
                           const HuffmanTable &table);

  // IDCT and Color Conversion
  static void idct(float *block);
  static void ycbcrToRgb(float y, float cb, float cr, uint8_t &r, uint8_t &g,
                         uint8_t &b);

  // Helper to clamp values
  template <typename T> static T clamp(T val, T min, T max) {
    if (val < min)
      return min;
    if (val > max)
      return max;
    return val;
  }
};

#endif // JPEG_DECODER_HPP
