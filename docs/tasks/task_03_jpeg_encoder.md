# Task: Phase 3 - JPEG Encoder Implementation

## Role
You are a Senior C++ Systems Engineer. You are an expert in image compression and the JPEG standard (ISO/IEC 10918-1).

## Context
We have a working PNG decoder. Now we need to implement the **JPEG Encoder** to convert the raw `Image` data into a `.jpg` file.

## Objective
Implement a baseline JPEG encoder from scratch.

## Requirements
- **No external libraries**.
- **Input**: `Image` struct (RGB/RGBA).
- **Output**: `.jpg` file.
- **Standards**: Baseline DCT, Huffman coding.
- **Quality**: Use standard quantization tables (approx 50-75% quality equivalent).
- **Subsampling**: 4:4:4 (No subsampling) is easiest to start with. 4:2:0 is optional but better. Let's stick to **4:4:4** for simplicity unless you feel confident.

## Detailed Instructions

### 1. JPEG Structure (`src/jpeg_encoder.hpp`, `src/jpeg_encoder.cpp`)
Create a `JpegEncoder` class.
- `static void encode(const Image& img, const std::string& filepath);`

### 2. Color Conversion
- Convert RGB pixels to YCbCr.
- Formula (standard JPEG):
    - Y  =  0.299R + 0.587G + 0.114B
    - Cb = -0.1687R - 0.3313G + 0.5B + 128
    - Cr =  0.5R - 0.4187G - 0.0813B + 128
- Shift values to range [-128, 127] for DCT (or keep 0-255 and subtract 128 during DCT).

### 3. Block Processing (8x8)
- Pad the image width/height to be multiples of 8.
- Split into 8x8 blocks.
- Process each channel (Y, Cb, Cr) separately (or interleaved MCU).
- **Forward DCT (FDCT)**: Implement the 8x8 DCT formula. A naive O(N^4) implementation is fine for now, or use AAN algorithm for O(N^2).
- **Quantization**: Divide DCT coefficients by the Quantization Table and round to integer. Use standard Luminance table for Y and Chrominance table for Cb/Cr.

### 4. Entropy Encoding
- **ZigZag Reordering**: Reorder the 64 coefficients.
- **DC Coefficient**: Store difference from previous block's DC. Encode using DC Huffman table.
- **AC Coefficients**: Run-Length Encode (RLE) zeros. Encode using AC Huffman table.
- Use `BitWriter` to write the bitstream.
- **Byte Stuffing**: If a byte `0xFF` is generated, write `0x00` after it (unless it's a marker).

### 5. File Structure (JFIF)
Write the JPEG markers:
- `SOI` (0xFFD8)
- `APP0` (JFIF header)
- `DQT` (Define Quantization Tables)
- `SOF0` (Start of Frame - Baseline DCT)
- `DHT` (Define Huffman Tables - Standard Luma/Chroma DC/AC tables)
- `SOS` (Start of Scan)
- [Compressed Data]
- `EOI` (0xFFD9)

## Deliverables
- `src/jpeg_encoder.hpp` and `src/jpeg_encoder.cpp`.
- Update `src/main.cpp` to convert the loaded PNG to JPG.
- **Verification**: The output JPG should open in a standard image viewer.

## Tips
- Use standard JPEG Huffman tables (Annex K of the standard).
- Be careful with `0xFF` byte stuffing in the entropy stream.
- Start with 4:4:4 (1 MCU = 1 Y block, 1 Cb block, 1 Cr block).
