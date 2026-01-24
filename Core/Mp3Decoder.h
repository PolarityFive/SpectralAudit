#pragma once
#include <string>
#include <vector>

class Mp3Decoder {
public:
	static bool decodeMp3Mono(const std::string& path, std::vector<double>& samples, int& sampleRate);
};
