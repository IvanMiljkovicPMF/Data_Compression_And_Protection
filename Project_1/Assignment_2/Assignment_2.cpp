#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <cstdint>
#include <iomanip>
#include <iterator>

using namespace std;

struct Symbol {
    int value;
    uint64_t frequency;
    string code;
};

struct Node {
    int value;
    uint64_t frequency;
    Node* left;
    Node* right;

    Node(int v, uint64_t f) : value(v), frequency(f), left(nullptr), right(nullptr) {}
    Node(int v, uint64_t f, Node* l, Node* r) : value(v), frequency(f), left(l), right(r) {}
};

struct Compare {
    bool operator()(Node* a, Node* b) const {
        if (a->frequency != b->frequency){ 
            return a->frequency > b->frequency;
        }
        return a->value > b->value;
    }
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

uint64_t fileSize(const string& filename) {
    ifstream file(filename, ios::binary | ios::ate);
    if (!file) {
		return 0;
    }
    return static_cast<uint64_t>(file.tellg());
}

void deleteTree(Node* root) {
    if (root == nullptr) {
		return;
    }

    deleteTree(root->left);
    deleteTree(root->right);
    delete root;
}

void shannonFano(vector<Symbol>& symbols, int start, int end) {
    if (start >= end) {
        return;
    }
       
    uint64_t total = 0;
    for (int i = start; i <= end; ++i) {
        total += symbols[i].frequency;
    }
        
    uint64_t sum = 0;
    uint64_t bestDifference = UINT64_MAX;
    int split = start;

    for (int i = start; i < end; ++i) {
        sum += symbols[i].frequency;
        uint64_t other = total - sum;
        uint64_t difference = (sum > other) ? sum - other : other - sum;

        if (difference < bestDifference) {
            bestDifference = difference;
            split = i;
        }
    }

    for (int i = start; i <= split; ++i) {
		symbols[i].code += '0';
    }

    for (int i = split + 1; i <= end; ++i) {
		symbols[i].code += '1';
    }

    shannonFano(symbols, start, split);
    shannonFano(symbols, split + 1, end);
}

Node* buildHuffmanTree(const array<uint64_t, 256>& frequency) {
    priority_queue<Node*, vector<Node*>, Compare> minHeap;

    for (int i = 0; i < 256; ++i) {
        if (frequency[i] > 0) {
            minHeap.push(new Node(i, frequency[i]));
        }
    }

    if (minHeap.empty()) {
		return nullptr;
    }

    while (minHeap.size() > 1) {
        Node* left = minHeap.top();
        minHeap.pop();

        Node* right = minHeap.top();
        minHeap.pop();

        Node* parent = new Node(
            min(left->value, right->value),
            left->frequency + right->frequency,
            left,
            right
        );

        minHeap.push(parent);
    }

    return minHeap.top();
}

void generateHuffmanCodes(Node* root, const string& code, vector<Symbol>& codes) {
    if (root == nullptr) {
		return;
    }

    if (root->left == nullptr && root->right == nullptr) {
        codes.push_back({ root->value, root->frequency, code.empty() ? "0" : code });
        return;
    }

    generateHuffmanCodes(root->left, code + '0', codes);
    generateHuffmanCodes(root->right, code + '1', codes);
}

void writeBit(vector<unsigned char>& bytes, uint64_t& bits, int value) {
    if (bits % 8 == 0) {
        bytes.push_back(0);
    }
    if (value != 0) {
        bytes.back() |= static_cast<unsigned char>(1u << (7 - bits % 8));
    }
    ++bits;
}

int readBit(const vector<unsigned char>& bytes, uint64_t& position) {
    int value = (bytes[position / 8] >> (7 - position % 8)) & 1;
    ++position;
    return value;
}

Node* makeTreeFromCodes(const vector<Symbol>& codes) {
    if (codes.empty()) {
		return nullptr;
    }

    if (codes.size() == 1) {
		return new Node(codes[0].value, codes[0].frequency);
    }

    Node* root = new Node(-1, 0);

    for (const Symbol& symbol : codes) {
        Node* current = root;

        for (char bit : symbol.code) {
            if (bit == '0') {
                if (current->left == nullptr) {
					current->left = new Node(-1, 0);
                }

                current = current->left;
            }
            else {
                if (current->right == nullptr) {
                    current->right = new Node(-1, 0);
                }
                    
                current = current->right;
            }
        }

        current->value = symbol.value;
        current->frequency = symbol.frequency;
    }

    return root;
}

void writeTree(Node* root, vector<unsigned char>& data, uint64_t& bits) {
    if (root == nullptr) {
		return;
    }

    if (root->left == nullptr && root->right == nullptr) {
        writeBit(data, bits, 1);

        for (int i = 7; i >= 0; --i) {
			writeBit(data, bits, (root->value >> i) & 1);
        }

        return;
    }

    writeBit(data, bits, 0);
    writeTree(root->left, data, bits);
    writeTree(root->right, data, bits);
}

Node* readTree(const vector<unsigned char>& data, uint64_t& position) {
    if (position >= data.size() * 8) {
		return nullptr;
    }

    if (readBit(data, position) == 1) {
        int value = 0;

        for (int i = 7; i >= 0; --i) {
            value |= readBit(data, position) << i;
        } 

        return new Node(value, 0);
    }

    Node* root = new Node(-1, 0);
    root->left = readTree(data, position);
    root->right = readTree(data, position);
    return root;
}

void encodeData(const vector<unsigned char>& input, const vector<Symbol>& codes, vector<unsigned char>& output, uint64_t& bitCount) {
    array<string, 256> codeTable{};

    for (const Symbol& symbol : codes) {
		codeTable[symbol.value] = symbol.code;
    }

    for (unsigned char value : input) {
        for (char bit : codeTable[value]) {
            writeBit(output, bitCount, bit == '1');
        }
            
    }
}

bool saveCompressed(const string& filename, const vector<unsigned char>& input, const vector<Symbol>& codes) {
    ofstream file(filename, ios::binary);
    
    if (!file) {
		return false;
    }

    Node* root = makeTreeFromCodes(codes);

    vector<unsigned char> treeData;
    uint64_t treeBits = 0;
    writeTree(root, treeData, treeBits);

    vector<unsigned char> encodedData;
    uint64_t dataBits = 0;
    encodeData(input, codes, encodedData, dataBits);

    uint64_t originalSize = input.size();
    uint64_t storedTreeBits = treeBits;
    uint64_t storedDataBits = dataBits;

    file.write(reinterpret_cast<const char*>(&originalSize), sizeof(originalSize));
    file.write(reinterpret_cast<const char*>(&storedTreeBits), sizeof(storedTreeBits));
    file.write(reinterpret_cast<const char*>(&storedDataBits), sizeof(storedDataBits));

    if (!treeData.empty()) {
        file.write(reinterpret_cast<const char*>(treeData.data()), treeData.size());
    }

    if (!encodedData.empty()) {
        file.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    }  

    bool result = static_cast<bool>(file);
    deleteTree(root);
    return result;
}

bool decodeCompressed(const string& filename, const string& outputFilename) {
    ifstream file(filename, ios::binary);
    if (!file) {
		return false;
    }

    uint64_t originalSize = 0;
    uint64_t treeBits = 0;
    uint64_t dataBits = 0;

    file.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));
    file.read(reinterpret_cast<char*>(&treeBits), sizeof(treeBits));
    file.read(reinterpret_cast<char*>(&dataBits), sizeof(dataBits));

    if (!file) {
		return false;
    }

    size_t treeBytes = static_cast<size_t>((treeBits + 7) / 8);
    vector<unsigned char> treeData(treeBytes);

    if (treeBytes > 0) {
        file.read(reinterpret_cast<char*>(treeData.data()), treeData.size());
    }

    if (!file) {
		return false;
    }

    vector<unsigned char> encodedData{istreambuf_iterator<char>(file), istreambuf_iterator<char>()};

    Node* root = nullptr;
    uint64_t treePosition = 0;

    if (treeBits > 0) {
        root = readTree(treeData, treePosition);
    }

    ofstream output(outputFilename, ios::binary);
    if (!output) {
        deleteTree(root);
        return false;
    }

    if (originalSize == 0) {
        deleteTree(root);
        return true;
    }

    if (root == nullptr) {
        deleteTree(root);
        return false;
    }

    if (root->left == nullptr && root->right == nullptr) {
        unsigned char value = static_cast<unsigned char>(root->value);

        for (uint64_t i = 0; i < originalSize; ++i) {
			output.put(static_cast<char>(value));
        }

        deleteTree(root);
        return static_cast<bool>(output);
    }

    Node* current = root;
    uint64_t position = 0;
    uint64_t decoded = 0;

    while (position < dataBits && decoded < originalSize) {
        current = readBit(encodedData, position) == 0 ? current->left : current->right;

        if (current == nullptr) {
            break;
        }

        if (current->left == nullptr && current->right == nullptr) {
            unsigned char value = static_cast<unsigned char>(current->value);
            output.put(static_cast<char>(value));
            ++decoded;
            current = root;
        }
    }

    bool result = decoded == originalSize && static_cast<bool>(output);
    deleteTree(root);
    return result;
}

bool compareFiles(const string& firstName, const string& secondName) {
    ifstream first(firstName, ios::binary);
    ifstream second(secondName, ios::binary);

    if (!first || !second) {
		return false;
    }

    const size_t bufferSize = 4096;
    char firstBuffer[bufferSize];
    char secondBuffer[bufferSize];

    while (true) {
        first.read(firstBuffer, bufferSize);
        second.read(secondBuffer, bufferSize);

        streamsize firstCount = first.gcount();
        streamsize secondCount = second.gcount();

        if (firstCount != secondCount) {
			return false;
        }

        if (firstCount == 0) {
            return true;
        }

        for (streamsize i = 0; i < firstCount; ++i) {
            if (firstBuffer[i] != secondBuffer[i]) {
                return false;
            }
        }
    }
}

int main() {
    cout << "Press Enter to start the program" << endl;
    cin.get();

    const char* inputFile = "war-peace.txt";
    cout << "Input file: " << inputFile << endl;

    vector<unsigned char> data;
    if (!readFile(inputFile, data)) {
        cout << "Input file is empty or could not be loaded." << endl;
        return 1;
    }

    array<uint64_t, 256> frequency{};

    for (unsigned char value : data) {
        ++frequency[value];
    }
        
    vector<Symbol> shannonCodes;

    for (int i = 0; i < 256; ++i) {
        if (frequency[i] > 0) {
            shannonCodes.push_back({ i, frequency[i], "" });
        }
    }

    sort(shannonCodes.begin(), shannonCodes.end(), [](const Symbol& a, const Symbol& b) {
        if (a.frequency != b.frequency) {
            return a.frequency > b.frequency;
        }

        return a.value < b.value;
    });

    shannonFano(shannonCodes,0,static_cast<int>(shannonCodes.size()) - 1);

    Node* huffmanRoot = buildHuffmanTree(frequency);
    vector<Symbol> huffmanCodes;
    generateHuffmanCodes(huffmanRoot, "", huffmanCodes);

    saveCompressed("shannon_fano.bin", data, shannonCodes);
    saveCompressed("huffman.bin", data, huffmanCodes);

    bool shannonResult = decodeCompressed("shannon_fano.bin", "shannon_fano_decoded.txt");

    bool huffmanResult = decodeCompressed("huffman.bin", "huffman_decoded.txt");

    uint64_t originalSize = fileSize(inputFile);
    uint64_t shannonSize = fileSize("shannon_fano.bin");
    uint64_t huffmanSize = fileSize("huffman.bin");

    cout << fixed << setprecision(2);
    cout << "Original file size: " << originalSize << " bytes" << endl;

    cout << endl;
    cout << "Shannon-Fano:" << endl;
    cout << "Compressed file size: " << shannonSize << " bytes" << endl;

    if (originalSize > 0) {
        cout << "Compression ratio: " << (static_cast<double>(shannonSize) / originalSize) * 100 << "%" << endl;
        cout << "Space saving: " << (1.0 - static_cast<double>(shannonSize) / originalSize) * 100 << "%" << endl;
    }

    cout << "Decoded file is " << (shannonResult ? "correct." : "not correct.") << endl;
    cout << "Files are " << (compareFiles(inputFile, "shannon_fano_decoded.txt") ? "identical." : "different.") << endl;

    cout << endl;
    cout << "Huffman:" << endl;
    cout << "Compressed file size: " << huffmanSize << " bytes" << endl;

    if (originalSize > 0) {
        cout << "Compression ratio: " << (static_cast<double>(huffmanSize) / originalSize) * 100 << "%" << endl;
        cout << "Space saving: " << (1.0 - static_cast<double>(huffmanSize) / originalSize) * 100 << "%" << endl;
    }

    cout << "Decoded file is "<< (huffmanResult ? "correct." : "not correct.") << endl;
    cout << "Files are " << (compareFiles(inputFile, "huffman_decoded.txt") ? "identical." : "different.") << endl;

    deleteTree(huffmanRoot);
    return 0;
}
