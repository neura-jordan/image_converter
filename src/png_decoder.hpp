#ifndef PNG_DECODER_HPP
#define PNG_DECODER_HPP

#include "image.hpp"
#include <cstdint>
#include <string>
#include <vector>

class PngDecoder {
public:
  static Image decode(const std::string &filepath);

private:
  struct Chunk {
    uint32_t length;
    std::string type;
    std::vector<uint8_t> data;
    uint32_t crc;
  };

  static uint32_t readBigEndian(const uint8_t *buffer);
  static void parseIHDR(const std::vector<uint8_t> &data, int &width,
                        int &height, uint8_t &bitDepth, uint8_t &colorType,
                        uint8_t &compressionMethod, uint8_t &filterMethod,
                        uint8_t &interlaceMethod);

  // DEFLATE / Zlib helpers
  static std::vector<uint8_t>
  inflate(const std::vector<uint8_t> &compressedData);

  struct HuffmanCode {
    int symbol;
    int length;
  };

  struct HuffmanTree {
    std::vector<int> counts;  // Count of codes of each length
    std::vector<int> symbols; // Symbols sorted by length

    void build(const std::vector<int> &codeLengths);
    int decode(class BitReader &reader) const;
  };

  static void decodeHuffmanBlock(class BitReader &reader,
                                 std::vector<uint8_t> &out);
  static void decodeFixedHuffmanBlock(class BitReader &reader,
                                      std::vector<uint8_t> &out);
  static void decodeDynamicHuffmanBlock(class BitReader &reader,
                                        std::vector<uint8_t> &out);
  static void decodeUncompressedBlock(class BitReader &reader,
                                      std::vector<uint8_t> &out);

  // Filtering helpers
  static std::vector<uint8_t>
  unfilterScanlines(const std::vector<uint8_t> &data, int width, int height,
                    int bytesPerPixel);
  static uint8_t paethPredictor(uint8_t a, uint8_t b, uint8_t c);
};

#endif // PNG_DECODER_HPP
