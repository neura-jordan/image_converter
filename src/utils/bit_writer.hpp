#ifndef BIT_WRITER_HPP
#define BIT_WRITER_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

class BitWriter {
public:
  BitWriter() : current_byte_(0), bit_count_(0), byte_stuffing_(false) {}

  void enableByteStuffing(bool enable) { byte_stuffing_ = enable; }

  // Write n bits (MSB first for JPEG)
  void writeBits(uint32_t value, int n) {
    for (int i = n - 1; i >= 0; --i) {
      if ((value >> i) & 1) {
        current_byte_ |= (1 << (7 - bit_count_));
      }
      bit_count_++;
      if (bit_count_ == 8) {
        pushByte(current_byte_);
        current_byte_ = 0;
        bit_count_ = 0;
      }
    }
  }

  void writeMarker(uint8_t marker) {
    alignToByte();
    buffer_.push_back(0xFF);
    buffer_.push_back(marker);
  }

  void alignToByte() {
    if (bit_count_ > 0) {
      // Pad with 1s for JPEG
      current_byte_ |= (0xFF >> bit_count_);
      pushByte(current_byte_);
      current_byte_ = 0;
      bit_count_ = 0;
    }
  }

  std::vector<uint8_t> getData() {
    // Make sure we are aligned (though usually caller should have called
    // writeMarker(EOI) which aligns) If not aligned, align now.
    if (bit_count_ > 0) {
      alignToByte();
    }
    return buffer_;
  }

  void clear() {
    buffer_.clear();
    current_byte_ = 0;
    bit_count_ = 0;
    byte_stuffing_ = false;
  }

private:
  void pushByte(uint8_t b) {
    buffer_.push_back(b);
    if (byte_stuffing_ && b == 0xFF) {
      buffer_.push_back(0x00);
    }
  }

  std::vector<uint8_t> buffer_;
  uint8_t current_byte_;
  int bit_count_;
  bool byte_stuffing_;
};

#endif // BIT_WRITER_HPP
