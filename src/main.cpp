#include "image.hpp"
#include "jpeg_decoder.hpp"
#include "jpeg_encoder.hpp"
#include "png_decoder.hpp"
#include "png_encoder.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

bool fileExists(const std::string &filename) {
  std::ifstream f(filename.c_str());
  return f.good();
}

bool hasExtension(const std::string &filename, const std::string &ext) {
  if (filename.length() < ext.length())
    return false;
  std::string fileExt = filename.substr(filename.length() - ext.length());
  std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);
  return fileExt == ext;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " <input> <output> [-q/--quality <1-100>]" << std::endl;
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

  enum Mode { PNG_TO_JPG, JPG_TO_PNG, UNKNOWN };
  Mode mode = UNKNOWN;

  if (hasExtension(inputPath, ".png") &&
      (hasExtension(outputPath, ".jpg") || hasExtension(outputPath, ".jpeg"))) {
    mode = PNG_TO_JPG;
  } else if ((hasExtension(inputPath, ".jpg") ||
              hasExtension(inputPath, ".jpeg")) &&
             hasExtension(outputPath, ".png")) {
    mode = JPG_TO_PNG;
  } else {
    std::cerr << "Error: Could not determine conversion mode from extensions."
              << std::endl;
    std::cerr << "Supported conversions: .png -> .jpg, .jpg -> .png"
              << std::endl;
    return 1;
  }

  try {
    std::cout << "Processing..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    if (mode == PNG_TO_JPG) {
      // 1. Decode PNG
      std::cout << "Decoding PNG " << inputPath << "..." << std::endl;
      Image img = PngDecoder::decode(inputPath);
      std::cout << "  Dimensions: " << img.width << "x" << img.height
                << std::endl;
      std::cout << "  Channels: " << img.channels << std::endl;

      // 2. Encode JPEG
      std::cout << "Encoding to JPEG " << outputPath << " with quality "
                << quality << "..." << std::endl;
      JpegEncoder::encode(img, outputPath, quality);
    } else {
      // 1. Decode JPEG
      std::cout << "Decoding JPEG " << inputPath << "..." << std::endl;
      Image img = JpegDecoder::decode(inputPath);
      std::cout << "  Dimensions: " << img.width << "x" << img.height
                << std::endl;
      std::cout << "  Channels: " << img.channels << std::endl;

      // 2. Encode PNG
      std::cout << "Encoding to PNG " << outputPath << "..." << std::endl;
      PngEncoder::encode(img, outputPath);
    }

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
