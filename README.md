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

### Basic Conversion
Automatically detects direction based on file extensions.

```bash
# Convert PNG to JPG
./converter input.png output.jpg

# Convert JPG to PNG
./converter input.jpg output.png
```

### Quality Control (PNG to JPG)
Specify the quality of the output JPEG (1-100). Default is 50.

```bash
# High quality (larger file)
./converter input.png output.jpg -q 90

# Low quality (smaller file)
./converter input.png output.jpg --quality 10
```

## Testing

To run the end-to-end verification test:

```bash
# Generate a test PNG and perform round-trip conversion
make && ./tests/test_gen
./converter test.png test.jpg
./converter test.jpg test_restored.png
```
