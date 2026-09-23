#include <iostream>
#include <fstream>
#include <array>
#include <cstdint>
#include <iomanip>
#include <cmath>

using namespace std;

int main()
{
	cout << "Press Enter to start the program" << endl;
	cin.get();

	const char* inputFile = "wolf.bmp";

	ifstream file(inputFile, ios::in | ios::binary);

	if (!file)
	{
		cerr << "Error opening file: " << inputFile << endl;
		return 1;
	}

	array<uint64_t, 256> byteCounts{};

	char buffer[4096];

	while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
	{
		streamsize bytesRead = file.gcount();
		for (streamsize i = 0; i < bytesRead; i++) {
			unsigned char byteValue = static_cast<unsigned char>(buffer[i]);

			byteCounts[byteValue]++;
		}
	}

	file.close();

	uint64_t totalBytes = 0;

	for (size_t i = 0; i < 256; ++i) {
		totalBytes += byteCounts[i];

		cout << "Byte value: " << i << ": " << byteCounts[i] << '\n';
	}
	cout << "Total bytes read: " << totalBytes << endl;


	double entropy = 0.0;

	for (size_t i = 0; i < 256; ++i)
	{

		if (byteCounts[i] == 0)
		{
			continue;
		}

		double probability = static_cast<double>(byteCounts[i]) / static_cast<double>(totalBytes);


		entropy -= probability * log2(probability);
	}


	cout << fixed << setprecision(6);

	cout << "\nByte Entropy: " << entropy << " bits/byte" << endl;

	return 0;
}