# Task: Phase 4 - Integration & Delivery

## Role
You are a Senior C++ Systems Engineer.

## Context
We have a working PNG decoder and JPEG encoder. The `main.cpp` currently runs a hardcoded test.

## Objective
Finalize the project by creating a proper CLI tool.

## Requirements
- **CLI Usage**: `./converter <input.png> <output.jpg>`
- **Validation**:
    - Check if input file exists.
    - Check if input is a valid PNG.
    - Handle errors gracefully (print error message and exit with non-zero code).
- **Performance**:
    - Ensure the build is optimized (`-O3`).
- **Cleanup**: Remove the hardcoded test generation code from `main.cpp` (or move it to a separate test file if you want to keep it, but the main entry point should be clean).

## Detailed Instructions

### 1. Update `src/main.cpp`
- Parse `argc` and `argv`.
- If arguments are missing, print usage: `Usage: ./converter <input.png> <output.jpg>`
- Load the PNG using `PngDecoder::decode`.
- Save the JPG using `JpegEncoder::encode`.
- Measure and print execution time (optional but nice).

### 2. Verify End-to-End
- Find a real PNG image (or generate one using the previous test code, save it, and then run the converter on it).
- Verify the output JPG opens.

## Deliverables
- Clean `src/main.cpp`.
- `Makefile` (ensure it's still correct).
- A working binary `converter`.
