#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <iomanip>
#include <chrono>
#include "jpeg_custom_coder/jpeg.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

std::vector<unsigned long> TimeQOI;
std::vector<unsigned long> TimeJPEG;
std::vector<unsigned long> TimeCustomJPEG;
std::vector<unsigned long> TimePNG;

std::vector<unsigned long> CompressionQOI;
std::vector<unsigned long> CompressionJPEG;
std::vector<unsigned long> CompressionCustomJPEG;
std::vector<unsigned long> CompressionPNG;
std::vector<unsigned long> Uncompressed;

void qoi_test(const char * filename, const void * data, unsigned width, unsigned height, uint8_t channels){
    //init vals
	int size{};
	void * encoded;
    qoi_desc desc{ width, height, channels, 0 };

    //clock start
    auto start = std::chrono::high_resolution_clock::now();

    //init file
    std::ofstream file(filename, std::ios_base::binary);

    //encode
	encoded = qoi_encode(data, &desc, &size);
    assert(encoded);

    //write file
    file.write(static_cast<char *>(encoded), size);
    file.close();

    //clock end
    auto end = std::chrono::high_resolution_clock::now() - start;

    //accumulate info
    TimeQOI.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(end).count());
    CompressionQOI.push_back(size);

    free(encoded);
}

void jpeg_test(const char * filename, const void * data, unsigned width, unsigned height, uint8_t channels){
    auto start = std::chrono::high_resolution_clock::now();

    stbi_write_jpg(filename, static_cast<int>(width), static_cast<int>(height), channels, data, 0);

    auto end = std::chrono::high_resolution_clock::now() - start;

    TimeJPEG.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(end).count());
    CompressionJPEG.push_back(std::filesystem::file_size(filename));
}

void png_test(const char * filename, const void *data, unsigned width, unsigned height, uint8_t channels){
    auto start = std::chrono::high_resolution_clock::now();

    stbi_write_png(filename, static_cast<int>(width), static_cast<int>(height), channels, data, width * channels);

    auto end = std::chrono::high_resolution_clock::now() - start;

    TimePNG.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(end).count());
    CompressionPNG.push_back(std::filesystem::file_size(filename));
}

void custom_jpeg_test(const char * filename, const void * data, unsigned width, unsigned height, uint8_t channels){
    auto start = std::chrono::high_resolution_clock::now();

    jpeg_encoder_t enc = jpeg_alloc();
    enc->compression_lvl = 3;

    jpeg_encode_data(enc, width, height, static_cast<const uint8_t * >(data));

    jpeg_write_to_file(enc, filename);

    auto end = std::chrono::high_resolution_clock::now() - start;

    jpeg_free(enc);

    TimeCustomJPEG.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(end).count());
    CompressionCustomJPEG.push_back(std::filesystem::file_size(filename));
}

int main(int argc, char** argv){   
    TimeQOI.reserve(20000);
    TimeJPEG.reserve(20000);
    TimeCustomJPEG.reserve(20000);
    TimePNG.reserve(20000);

    CompressionQOI.reserve(20000);
    CompressionJPEG.reserve(20000);
    CompressionCustomJPEG.reserve(20000);
    CompressionPNG.reserve(20000);
    Uncompressed.reserve(20000);

    if (argc != 2){
        std::cerr << "USAGE\n\n";
        std::cerr << argv[0] << " [input_folder]" << std::endl;
        return -1;
    }
    
    std::filesystem::path input_path(argv[1]);
    
    std::filesystem::path qoi_out_path(input_path / "qoi");
    std::filesystem::path jpeg_out_path(input_path / "jpeg");
    std::filesystem::path png_out_path(input_path / "png");
    std::filesystem::path custom_jpeg_out_path(input_path / "custom_jpeg");

    std::filesystem::create_directory(qoi_out_path);
    std::filesystem::create_directory(jpeg_out_path);
    std::filesystem::create_directory(png_out_path);
    std::filesystem::create_directory(custom_jpeg_out_path);

    for (auto const& dir_entry : std::filesystem::directory_iterator(input_path))
    {
        if (!dir_entry.path().has_extension()) continue;
        int width, height, channels;
        uint8_t * img = stbi_load(dir_entry.path().string().c_str(), &width, &height, &channels, 0);

        if (!img){
            std::cerr << "Failed to load image: " << dir_entry.path() << std::endl;
            continue;
        }

        //write QOI
        auto ofname = qoi_out_path / dir_entry.path().filename();
        ofname.replace_extension("qoi");
        qoi_test(ofname.string().c_str(), img, width, height, channels);

        //write JPEG
        ofname = jpeg_out_path / dir_entry.path().filename();
        ofname.replace_extension("jpeg");
        jpeg_test(ofname.string().c_str(), img, width, height, channels);

        //write PNG
        ofname = png_out_path / dir_entry.path().filename();
        ofname.replace_extension("png");
        png_test(ofname.string().c_str(), img, width, height, channels);

        //write custom JPEG
        ofname = custom_jpeg_out_path / dir_entry.path().filename();
        ofname.replace_extension("jpg");
        custom_jpeg_test(ofname.string().c_str(), img, width, height, channels);

        Uncompressed.push_back(width * height * channels);

        stbi_image_free(img);
    }

    auto totalQOI = std::accumulate(TimeQOI.begin(), TimeQOI.end(), 0);
    auto totalJPEG = std::accumulate(TimeJPEG.begin(), TimeJPEG.end(), 0);
    auto totalPNG = std::accumulate(TimePNG.begin(), TimePNG.end(), 0);
    auto totalCustomJPEG = std::accumulate(TimeCustomJPEG.begin(), TimeCustomJPEG.end(), 0);

    auto totalSize = std::accumulate(Uncompressed.begin(), Uncompressed.end(), 0);
    auto totalQOISize = std::accumulate(CompressionQOI.begin(), CompressionQOI.end(), 0);
    auto totalJPEGSize = std::accumulate(CompressionJPEG.begin(), CompressionJPEG.end(), 0);
    auto totalPNGSize = std::accumulate(CompressionPNG.begin(), CompressionPNG.end(), 0);
    auto totalCustomJPEGSize = std::accumulate(CompressionCustomJPEG.begin(), CompressionCustomJPEG.end(), 0);    

    std::cout << "Images     : " << Uncompressed.size() << '\n';
    std::cout << "Time-------------------------------------\n";
    std::cout << "TotalQOI   : " << totalQOI << "ms" << '\n';
    std::cout << "TotalJPEG  : " << totalJPEG << "ms" << '\n';
    std::cout << "TotalPNG   : " << totalPNG << "ms" << '\n';
    std::cout << "TotalCus   : " << totalCustomJPEG << "ms" << '\n';
    std::cout << "Compression-------------------------------\n";
    std::cout << "QOI %      : " << static_cast<double>(totalQOISize) / static_cast<double>(totalSize) * 100.0 << '\n';
    std::cout << "JPEG %     : " << static_cast<double>(totalJPEGSize) / static_cast<double>(totalSize) * 100.0 << '\n';
    std::cout << "PNG %      : " << static_cast<double>(totalPNGSize) / static_cast<double>(totalSize) * 100.0 << '\n';
    std::cout << "Custom %   : " << static_cast<double>(totalCustomJPEGSize) / static_cast<double>(totalSize) * 100.0 << '\n';
    std::cout << "Compression-------------------------------\n";
    std::cout << "Total size : " << totalSize << " bytes\n";
    std::cout << "QOI        : " << std::setw(10) << totalQOISize << " d: " << std::setw(10) << totalSize - totalQOISize << '\n';
    std::cout << "JPEG       : " << std::setw(10) << totalJPEGSize << " d: " << std::setw(10) << totalSize - totalJPEGSize << '\n';
    std::cout << "PNG        : " << std::setw(10) << totalPNGSize << " d: " << std::setw(10) << totalSize - totalPNGSize << '\n';
    std::cout << "Custom     : " << std::setw(10) << totalCustomJPEGSize << " d: " << std::setw(10) << totalSize - totalCustomJPEGSize << '\n';

    return 0;
}