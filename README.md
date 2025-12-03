# Raw C++ Image Converter

A high-performance PNG to JPEG converter written in raw C++17 without any external libraries (no `libpng`, `libjpeg`, `zlib`, etc.).

## Features

- **Zero Dependencies**: Implements file formats from scratch.
- **PNG Decoder**:
  - Custom DEFLATE implementation (RFC 1951) with Dynamic and Fixed Huffman codes.
  - Supports Truecolor (RGB) and Truecolor+Alpha (RGBA).
  - Implements all PNG filter types (None, Sub, Up, Average, Paeth).
- **JPEG Encoder**:
  - RGB to YCbCr color conversion.
  - Forward Discrete Cosine Transform (FDCT).
  - Quantization and ZigZag reordering.
  - Huffman Entropy Encoding (RFC 10918).

## Build

Requirements: `clang++` or `g++` with C++17 support.

```bash
make
```

## Usage

```bash
./converter <input.png> <output.jpg>
```

Example:
```bash
./converter image.png result.jpg
```

## Testing

To run the end-to-end verification test:

```bash
# Generate a test PNG and convert it
make && ./tests/test_gen && ./converter test.png output.jpg
```
