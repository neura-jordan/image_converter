#ifndef JPEG_ENCODER_HPP
#define JPEG_ENCODER_HPP

#include "image.hpp"
#include "utils/bit_writer.hpp"
#include <cstdint>
#include <string>
#include <vector>

class JpegEncoder {
public:
  static void encode(const Image &img, const std::string &filepath,
                     int quality = 50);

private:
  struct HuffmanTable {
    std::vector<uint8_t> bits;    // Count of codes of each length (1-16)
    std::vector<uint8_t> huffval; // Symbols sorted by code length
    // Derived lookup tables for encoding could be added here,
    // but for now we might just use the raw tables or build a map on the
    // fly/static. Actually, for encoding we need Symbol -> Code/Length.
    std::vector<uint32_t> codes;
    std::vector<uint8_t> codeLengths;
  };

  static void initTables();
  static void writeHeaders(BitWriter &writer, int width, int height,
                           const uint8_t *lumaTable,
                           const uint8_t *chromaTable);
  static void writeFooter(BitWriter &writer);
  static void processBlock(BitWriter &writer, const float *blockData,
                           const uint8_t *quantTable, int &prevDC,
                           const HuffmanTable &dcTable,
                           const HuffmanTable &acTable);

  // Core math
  static void rgbToYcbcr(const uint8_t *rgb, float *y, float *cb, float *cr);
  static void fdct(float *block);
  static void quantize(float *block, const uint8_t *quantTable);
  static void zigzag(const float *input, float *output);

  // Entropy coding helpers
  static void encodeBlock(BitWriter &writer, const float *quantizedBlock,
                          int &prevDC, const HuffmanTable &dcTable,
                          const HuffmanTable &acTable);

  // Standard Tables
  static const uint8_t ZIGZAG[64];
  static const uint8_t QUANT_LUMA[64];
  static const uint8_t QUANT_CHROMA[64];

  // We will populate these in initTables or just have them as static members
  static HuffmanTable DC_LUMA;
  static HuffmanTable AC_LUMA;
  static HuffmanTable DC_CHROMA;
  static HuffmanTable AC_CHROMA;
  static bool tablesInitialized;
};

#endif // JPEG_ENCODER_HPP
