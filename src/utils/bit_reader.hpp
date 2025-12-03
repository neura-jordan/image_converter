#ifndef BIT_READER_HPP
#define BIT_READER_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

class BitReader {
public:
  BitReader(const uint8_t *data, size_t size)
      : data_(data), size_(size), byte_offset_(0), bit_offset_(0) {}

  // Read n bits (LSB first for DEFLATE)
  uint32_t readBits(int n) {
    if (n > 32)
      throw std::invalid_argument("Cannot read more than 32 bits");
    uint32_t result = 0;
    for (int i = 0; i < n; ++i) {
      if (byte_offset_ >= size_) {
        throw std::out_of_range("End of stream");
      }
      uint8_t byte = data_[byte_offset_];
      if ((byte >> bit_offset_) & 1) {
        result |= (1 << i);
      }
      bit_offset_++;
      if (bit_offset_ == 8) {
        bit_offset_ = 0;
        byte_offset_++;
      }
    }
    return result;
  }

  void advanceBits(int n) {
    for (int i = 0; i < n; ++i) {
      if (byte_offset_ >= size_) {
        // Depending on spec, might allow advancing past end if not reading?
        // For safety, let's throw or clamp. Throwing is safer for now.
        throw std::out_of_range("End of stream");
      }
      bit_offset_++;
      if (bit_offset_ == 8) {
        bit_offset_ = 0;
        byte_offset_++;
      }
    }
  }

  // Align to the next byte boundary
  void alignToByte() {
    if (bit_offset_ != 0) {
      bit_offset_ = 0;
      byte_offset_++;
    }
  }

  bool hasMore() const { return byte_offset_ < size_; }

  size_t getByteOffset() const { return byte_offset_; }

private:
  const uint8_t *data_;
  size_t size_;
  size_t byte_offset_;
  int bit_offset_;
};

#endif // BIT_READER_HPP
