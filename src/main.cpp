#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include "GB.h"

// Just default to the first file it finds with gb extension
std::string findFirstFileWithExtension(std::string& directory) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.path().extension() == ".gb") {
                return entry.path().string();
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    return "";  // No file found
}

// Perhaps move to GB or CPU class
std::vector<uint8_t> loadByteData(const std::string& filename) {
    std::string fileToLoad = filename;

    if (!std::filesystem::exists(fileToLoad)) {
        //std::cout << "File: " << fileToLoad << " does not exist looking for alternative..." << std::endl;
        std::string directory = std::filesystem::path(filename).parent_path().string();

        if (directory.empty()) directory = "../roms";

        fileToLoad = findFirstFileWithExtension(directory);

        if (fileToLoad == "") {
            throw std::runtime_error("No GB files found");
        }

		//std::cout << "Found file: " << fileToLoad << std::endl;
    }

    std::ifstream file(fileToLoad, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open '" + filename + "'.");
    }
    
    //std::cout << "Using the file: " << fileToLoad << std::endl;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<unsigned int>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read '" + filename + "'.");
    }

    return buffer;
}


int main(int argc, char* argv[]) {
    std::string filename = "../roms/doesnt_exist.gb"; // doesn't exist (Also all code assumes you're running from the build directory)
    if (argc > 1) {
        filename = argv[1]; // Won't force the file to be .gb
    }
	std::vector<uint8_t> romData;
    try {
        romData = loadByteData(filename); // Load the ROM data verified correctness
    }
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

    //std::cout << "Starting GB Emulator" << std::endl;
    GB *gameboy = new GB(romData); // wanted on stack but IDE complains
    while (true) { // Change to a running flag
        gameboy->run();
		//break; // Remove this eventually
    }
    delete gameboy;
    return 0;
}