#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <iomanip>

using namespace std;

const int LZ77_SEARCH_BUFFER = 4095;
const int LZ77_LOOKAHEAD_BUFFER = 255;
const int LZW_MAX_CODE = 4095;

struct Token
{
	uint16_t offset;
	uint8_t length;
	uint8_t nextByte;
};

bool readFile(const char* fileName, vector<unsigned char>& data)
{
	ifstream file(fileName, ios::in | ios::binary);

	if (!file) {
		return false;
	}

	char buffer[4096];

	while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
		streamsize bytesRead = file.gcount();

		for (streamsize i = 0; i < bytesRead; ++i) {
			unsigned char byteValue = static_cast<unsigned char>(buffer[i]);

			data.push_back(byteValue);
		}
	}

	file.close();

	return true;
}

bool writeFile(const char* fileName, const vector<unsigned char>& data) {
	ofstream file(fileName, ios::binary);

	if (!file) {
		return false;
	}

	if (!data.empty()) {
		file.write(reinterpret_cast<const char*>(data.data()), static_cast<streamsize>(data.size()));
	}

	return static_cast<bool>(file);
}

uint64_t getFileSize(const char* fileName) {
	ifstream file(fileName, ios::binary | ios::ate);

	if (!file) {
		return 0;
	}

	streamoff size = file.tellg();

	if (size < 0) {
		return 0;
	}

	return static_cast<uint64_t>(size);
}

void writeBits(ofstream& file, uint32_t value, int count, unsigned char& currentByte, int& bitsInByte) {
	for (int i = 0; i < count; ++i) {
		currentByte |= static_cast<unsigned char>(((value >> i) & 1u) << bitsInByte);

		++bitsInByte;

		if (bitsInByte == 8) {
			file.put(static_cast<char>(currentByte));
			currentByte = 0;
			bitsInByte = 0;
		}
	}
}

bool readBits(ifstream& file, uint32_t& value, int count, unsigned char& currentByte, int& bitsInByte) {
	value = 0;

	for (int i = 0; i < count; ++i) {
		if (bitsInByte == 0) {
			int character = file.get();

			if (character == EOF) {
				return false;
			}

			currentByte = static_cast<unsigned char>(character);
		}

		value |=(static_cast<uint32_t>((currentByte >> bitsInByte) & 1u) << i);

		++bitsInByte;

		if (bitsInByte == 8) {
			bitsInByte = 0;
		}
	}

	return true;
}

vector<Token> compression_lz77(const vector<unsigned char>& input) {
	vector<Token> data;

	int inputLength = static_cast<int>(input.size());

	int position = 0;

	while (position < inputLength) {
		Token token{};

		token.offset = 0;
		token.length = 0;
		token.nextByte = input[position];

		int maxOffset =(position < LZ77_SEARCH_BUFFER) ? position : LZ77_SEARCH_BUFFER;

		int maxSearchLength =(position + LZ77_LOOKAHEAD_BUFFER > inputLength) ? inputLength - position : LZ77_LOOKAHEAD_BUFFER;

		for (int offset = 1; offset <= maxOffset; ++offset) {
			int len = 0;

			while (len < maxSearchLength && input[position - offset + len] == input[position + len]) {
				++len;
			}

			if (len > token.length) {
				token.offset = static_cast<uint16_t>(offset);

				token.length = static_cast<uint8_t>(len);

				if (position + len < inputLength) {
					token.nextByte = input[position + len];
				} else {
					token.nextByte = 0;
				}
			}
		}

		data.push_back(token);

		position += static_cast<int>(token.length) + 1;
	}

	return data;
}

vector<unsigned char> decompression_lz77(const vector<Token>& compressedData, size_t originalSize) {
	vector<unsigned char> decompressed;

	decompressed.reserve(originalSize);

	for (const Token& token : compressedData) {
		if (token.offset == 0) {
			if (decompressed.size() < originalSize) {
				decompressed.push_back(token.nextByte);
			}
		} else {
			size_t startPos = decompressed.size() - token.offset;

			for (int i = 0; i < token.length; ++i) {
				decompressed.push_back(decompressed[startPos + i]);
			}

			if (decompressed.size() < originalSize) {
				decompressed.push_back(token.nextByte);
			}
		}
	}

	return decompressed;
}

bool saveLZ77(const char* fileName, const vector<Token>& data, uint64_t originalSize) {
	ofstream file(fileName, ios::binary);

	if (!file) {
		return false;
	}

	uint64_t tokenCount = static_cast<uint64_t>(data.size());

	file.write(reinterpret_cast<const char*>(&originalSize), sizeof(originalSize));

	file.write(reinterpret_cast<const char*>(&tokenCount), sizeof(tokenCount));

	unsigned char currentByte = 0;
	int bitsInByte = 0;

	for (const Token& token : data) {
		uint32_t value = 
			(static_cast<uint32_t>(token.offset) << 16) 
			|(static_cast<uint32_t>(token.length) << 8) 
			| static_cast<uint32_t>(token.nextByte);

		writeBits(file, value, 28, currentByte, bitsInByte);
	}

	if (bitsInByte != 0) {
		file.put(static_cast<char>(currentByte));
	}

	return static_cast<bool>(file);
}

bool loadLZ77(const char* fileName, vector<Token>& data, uint64_t& originalSize) {
	ifstream file(fileName, ios::binary);

	if (!file) {
		return false;
	}

	uint64_t tokenCount = 0;

	file.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));

	file.read(reinterpret_cast<char*>(&tokenCount), sizeof(tokenCount));

	if (!file) {
		return false;
	}

	data.clear();

	unsigned char currentByte = 0;
	int bitsInByte = 0;

	for (uint64_t i = 0; i < tokenCount; ++i) {
		uint32_t value = 0;

		if (!readBits(file, value, 28, currentByte, bitsInByte)) {
			return false;
		}

		Token token{};

		token.offset = static_cast<uint16_t>((value >> 16) & 0x0FFFu);
		token.length = static_cast<uint8_t>((value >> 8) & 0xFFu);
		token.nextByte = static_cast<uint8_t>(value & 0xFFu);

		data.push_back(token);
	}

	return true;
}

vector<uint16_t> encoding(const vector<unsigned char>& input) {
	vector<uint16_t> outputCode;

	if (input.empty()) {
		return outputCode;
	}

	unordered_map<string, uint16_t> table;

	for (int i = 0; i <= 255; ++i) {
		string ch = "";

		ch += static_cast<char>(i);

		table[ch] = static_cast<uint16_t>(i);
	}

	string p = "";
	string c = "";

	p += static_cast<char>(input[0]);

	uint16_t code = 256;

	for (size_t i = 0; i < input.size(); ++i) {
		c = "";

		if (i != input.size() - 1) {
			c += static_cast<char>(input[i + 1]);
		}

		if (table.find(p + c) != table.end()) {
			p = p + c;
		} else {
			outputCode.push_back(table[p]);

			if (code <= LZW_MAX_CODE) {
				table[p + c] = code;
				++code;
			}

			p = c;
		}
	}
	outputCode.push_back(table[p]);
	return outputCode;
}

vector<unsigned char> decoding(const vector<uint16_t>& input, uint64_t originalSize) {
	vector<unsigned char> output;

	if (input.empty()) {
		return output;
	}

	vector<string> table(LZW_MAX_CODE + 1);

	for (int i = 0; i <= 255; ++i) {
		table[i] =string(1, static_cast<char>(i));
	}

	uint16_t code = 256;

	uint16_t old = input[0];

	string s = table[old];

	output.insert(output.end(), s.begin(), s.end());

	for (size_t i = 1; i < input.size(); ++i)
	{
		uint16_t n =
			input[i];

		if (n <= LZW_MAX_CODE && !table[n].empty()) {
			s = table[n];
		}
		else if (n == code) {
			s = table[old] + table[old][0];
		} else {
			return {};
		}

		output.insert(output.end(), s.begin(), s.end());

		if (code <= LZW_MAX_CODE) {
			table[code] = table[old] + s[0];

			++code;
		}

		old = n;
	}

	if (output.size() != originalSize) {
		return {};
	}

	return output;
}

bool saveLZW(const char* fileName, const vector<uint16_t>& codes, uint64_t originalSize) {
	ofstream file(fileName, ios::binary);

	if (!file) {
		return false;
	}

	uint64_t codeCount = static_cast<uint64_t>(codes.size());

	file.write(reinterpret_cast<const char*>(&originalSize), sizeof(originalSize));

	file.write(reinterpret_cast<const char*>(&codeCount), sizeof(codeCount));

	unsigned char currentByte = 0;
	int bitsInByte = 0;

	for (uint16_t code : codes) {
		writeBits(file, code, 12, currentByte, bitsInByte);
	}

	if (bitsInByte != 0) {
		file.put(static_cast<char>(currentByte));
	}

	return static_cast<bool>(file);
}

bool loadLZW(const char* fileName, vector<uint16_t>& codes, uint64_t& originalSize) {
	ifstream file(fileName, ios::binary);

	if (!file) {
		return false;
	}

	uint64_t codeCount = 0;

	file.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));

	file.read(reinterpret_cast<char*>(&codeCount), sizeof(codeCount));

	if (!file) {
		return false;
	}

	codes.clear();

	unsigned char currentByte = 0;
	int bitsInByte = 0;

	for (uint64_t i = 0; i < codeCount; ++i) {
		uint32_t value = 0;

		if (!readBits(file, value, 12, currentByte, bitsInByte)) {
			return false;
		}

		codes.push_back(static_cast<uint16_t>(value));
	}

	return true;
}

void printCompression(const char* name, uint64_t originalSize, uint64_t compressedSize) {
	double ratio = static_cast<double>(originalSize) / static_cast<double>(compressedSize);

	double saving =(1.0 - static_cast<double>(compressedSize) / static_cast<double>(originalSize)) * 100.0;

	cout << "\n" << name << ":" << endl;

	cout << "Compressed size: " << compressedSize << " bytes" << endl;

	cout << fixed << setprecision(6);

	cout << "Compression ratio: " << ratio << ":1" << endl;

	cout << "Space saving: " << saving << "%" << endl;
}

int main() {
	cout << "Press Enter to start the program" << endl;

	cin.get();

	const char* inputFile = "apache_loghub.log";

	vector<unsigned char> input;

	if (!readFile(inputFile, input)) {
		cerr << "Error opening file: " << inputFile << endl;

		return 1;
	}

	uint64_t originalSize = static_cast<uint64_t>(input.size());

	cout << "Input file: " << inputFile << endl;

	cout << "Original size: " << originalSize << " bytes" << endl;

	vector<Token> lz77 = compression_lz77(input);

	if (!saveLZ77("apache_loghub.lz77", lz77, originalSize)) {
		cerr << "Error writing LZ77 file." << endl;

		return 1;
	}

	vector<Token> loadedLZ77;

	uint64_t lz77OriginalSize = 0;

	if (!loadLZ77("apache_loghub.lz77", loadedLZ77, lz77OriginalSize)) {
		cerr << "Error loading LZ77 file." << endl;

		return 1;
	}

	vector<unsigned char> lz77Decoded = decompression_lz77(loadedLZ77, static_cast<size_t>(lz77OriginalSize));

	if (!writeFile("apache_loghub_lz77_decoded.log", lz77Decoded)) {
		cerr << "Error writing LZ77 decoded file." << endl;

		return 1;
	}

	cout << "\nLZ77 decoded correctly: " << (input == lz77Decoded ? "YES" : "NO") << endl;

	printCompression("LZ77", originalSize, getFileSize("apache_loghub.lz77"));

	vector<uint16_t> lzw = encoding(input);

	if (!saveLZW("apache_loghub.lzw", lzw, originalSize)) {
		cerr << "Error writing LZW file." << endl;

		return 1;
	}

	vector<uint16_t> loadedLZW;

	uint64_t lzwOriginalSize = 0;

	if (!loadLZW("apache_loghub.lzw", loadedLZW, lzwOriginalSize)) {
		cerr << "Error loading LZW file." << endl;

		return 1;
	}

	vector<unsigned char> lzwDecoded = decoding(loadedLZW, lzwOriginalSize);

	if (!writeFile("apache_loghub_lzw_decoded.log",lzwDecoded)) {
		cerr << "Error writing LZW decoded file." << endl;

		return 1;
	}

	cout << "\nLZW decoded correctly: " << (input == lzwDecoded ? "YES" : "NO") << endl;

	printCompression("LZW", originalSize, getFileSize("apache_loghub.lzw"));

	return 0;
}