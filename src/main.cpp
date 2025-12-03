#include "image.hpp"
#include "jpeg_encoder.hpp"
#include "png_decoder.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

bool fileExists(const std::string &filename) {
  std::ifstream f(filename.c_str());
  return f.good();
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " <input.png> <output.jpg> [-q/--quality <1-100>]"
              << std::endl;
    return 1;
  }

  std::string inputPath = argv[1];
  std::string outputPath = argv[2];
  int quality = 50;

  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-q" || arg == "--quality") {
      if (i + 1 < argc) {
        try {
          quality = std::stoi(argv[++i]);
          if (quality < 1 || quality > 100) {
            std::cerr << "Error: Quality must be between 1 and 100."
                      << std::endl;
            return 1;
          }
        } catch (...) {
          std::cerr << "Error: Invalid quality value." << std::endl;
          return 1;
        }
      } else {
        std::cerr << "Error: Missing value for quality flag." << std::endl;
        return 1;
      }
    } else {
      std::cerr << "Warning: Unknown argument '" << arg << "'" << std::endl;
    }
  }

  if (!fileExists(inputPath)) {
    std::cerr << "Error: Input file '" << inputPath << "' does not exist."
              << std::endl;
    return 1;
  }

  // Optional: Check extension
  if (inputPath.size() < 4 ||
      inputPath.substr(inputPath.size() - 4) != ".png") {
    std::cerr << "Warning: Input file does not have .png extension."
              << std::endl;
  }

  try {
    std::cout << "Processing..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    // 1. Decode PNG
    std::cout << "Decoding " << inputPath << "..." << std::endl;
    Image img = PngDecoder::decode(inputPath);
    std::cout << "  Dimensions: " << img.width << "x" << img.height
              << std::endl;
    std::cout << "  Channels: " << img.channels << std::endl;

    // 2. Encode JPEG
    std::cout << "Encoding to " << outputPath << " with quality " << quality
              << "..." << std::endl;
    JpegEncoder::encode(img, outputPath, quality);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Success! Conversion took " << elapsed.count() << " seconds."
              << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
