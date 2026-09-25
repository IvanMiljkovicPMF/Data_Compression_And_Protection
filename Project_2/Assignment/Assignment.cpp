#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <string>

using namespace std;

const int n = 15;
const int nk = 9;
const int wr = 5;
const int wc = 3;

int weight(int value) {
    int result = 0;

    for (int i = 0; i < n; i++) {
        if (value & (1 << i)) {
            result++;
        }
    }
    return result;
}

int calculateSyndrome(const vector<vector<int>>& H, int error) {
    int syndrome = 0;

    for (int i = 0; i < nk; i++) {
        int value = 0;

        for (int j = 0; j < n; j++) {
            if (error & (1 << (n - 1 - j))) {
                value ^= H[i][j];
            }
        }
        if (value == 1) {
            syndrome |= (1 << (nk - 1 - i));
        }
    }
    return syndrome;
}

void printSyndrome(int syndrome) {
    for (int i = nk - 1; i >= 0; i--) {
        cout << ((syndrome >> i) & 1);
    }
}

void printError(int error) {
    for (int i = n - 1; i >= 0; i--) {
        cout << ((error >> i) & 1);
    }
}

bool isCodeword(const vector<vector<int>>& H, const vector<int>& x) {
    for (int i = 0; i < nk; i++) {
        int value = 0;
        for (int j = 0; j < n; j++) {
            if (H[i][j] == 1) {
                value ^= x[j];
            }
        }
        if (value != 0) {
            return false;
        }
    }
    return true;
}

vector<int> gallagerB(const vector<vector<int>>& H, const vector<int>& y, double th0, double th1, int maxIterations) {
    vector<int> x = y;
    vector<int> nextX(n);

    for (int iteration = 0; iteration < maxIterations; iteration++) {
        for (int j = 0; j < n; j++) {
            int zeros = 0;
            int ones = 0;
            int neighbors = 0;

            for (int i = 0; i < nk; i++) {
                if (H[i][j] == 1) {
                    neighbors++;

                    int sum = 0;

                    for (int l = 0; l < n; l++) {
                        if (l != j && H[i][l] == 1) {
                            sum += x[l];
                        }
                    }

                    int message = sum % 2;

                    if (message == 0) {
                        zeros++;
                    }
                    else {
                        ones++;
                    }
                }
            }
            if (zeros >= th0 * neighbors) {
                nextX[j] = 0;
            }
            else if (ones >= th1 * neighbors) {
                nextX[j] = 1;
            }
            else {
                nextX[j] = y[j];
            }
        }
        x = nextX;

        if (isCodeword(H, x)) {
            return x;
        }
    }
    return x;
}

bool isZeroVector(const vector<int>& x) {
    for (int bit : x) {
        if (bit != 0) {
            return false;
        }
    }
    return true;
}

void findMinimumError(const vector<vector<int>>& H, int codeDistance) {
    const double th0 = 0.5;
    const double th1 = 0.5;
    const int maxIterations = 100;

    const int numberOfErrors = 1 << n;

    for (int currentWeight = 1; currentWeight <= n; currentWeight++) {

        for (int error = 1; error < numberOfErrors; error++) {

            if (weight(error) != currentWeight) {
                continue;
            }

            vector<int> e(n);

            for (int i = 0; i < n; i++) {
                e[i] = (error >> (n - 1 - i)) & 1;
            }

            vector<int> decoded = gallagerB(H, e, th0, th1, maxIterations);

            if (!isZeroVector(decoded)) {
                cout << "Minimum error weight: " << currentWeight << endl;

                cout << "Error vector e: ";
                for (int bit : e) {
                    cout << bit;
                }
                cout << endl;

                cout << "Gallager B result: ";
                for (int bit : decoded) {
                    cout << bit;
                }
                cout << endl;

                cout << "th0 = " << th0 << endl;
                cout << "th1 = " << th1 << endl;


                cout << "Comparison: ";

                if (currentWeight < codeDistance) {
                    cout << "Error weight = " << currentWeight << " < " << " Code distance = " << codeDistance << endl;
                }
                else if (currentWeight == codeDistance) {
                    cout << "Error weight = " << currentWeight << " = " << " Code distance = " << codeDistance << endl;
                }
                else {
                    cout << "Error weight = " << currentWeight << " > " << "Code distance = " << codeDistance << endl;
                }
                return;
            }
        }
    }
}

int main() {
    const int numberOfSyndromes = 1 << nk;
    const int numberOfErrors = 1 << n;

    unsigned seed;

    cout << "Enter your index number: ";
    cin >> seed;

    vector<vector<int>> H(nk, vector<int>(n, 0));

    int groupRows = nk / wc;

    for (int i = 0; i < groupRows; i++) {
        for (int j = 0; j < wr; j++) {
            H[i][i * wr + j] = 1;
        }
    }

    mt19937 generator(seed);

    for (int group = 1; group < wc; group++) {
        vector<int> permutation(n);

        for (int i = 0; i < n; i++) {
            permutation[i] = i;
        }

        shuffle(permutation.begin(), permutation.end(), generator);

        for (int i = 0; i < groupRows; i++) {
            for (int j = 0; j < wr; j++) {
                H[group * groupRows + i][permutation[i * wr + j]] = 1;
            }
        }
    }

    cout << "\nH matrix:\n\n";

    for (int i = 0; i < nk; i++) {
        for (int j = 0; j < n; j++) {
            cout << H[i][j] << " ";
        }
        cout << endl;
    }

    vector<int> corrector(numberOfSyndromes, 0);
    vector<bool> found(numberOfSyndromes, false);

    int foundSyndromes = 0;

    for (int currentWeight = 0; currentWeight <= n; currentWeight++) {

        for (int error = 0; error < numberOfErrors; error++) {

            if (weight(error) != currentWeight) {
                continue;
            }

            int syndrome = calculateSyndrome(H, error);

            if (!found[syndrome]) {
                corrector[syndrome] = error;
                found[syndrome] = true;
                foundSyndromes++;
            }
        }

        if (foundSyndromes == numberOfSyndromes) {
            break;
        }
    }

    cout << "\nSyndrome table:\n\n";
    cout << "Syndrome  Corrector\n";

    for (int syndrome = 0; syndrome < numberOfSyndromes; syndrome++) {
        printSyndrome(syndrome);
        cout << "       ";

        if (found[syndrome]) {
            printError(corrector[syndrome]);
        }
        else {
            cout << "Not available";
        }

        cout << endl;
    }

    int codeDistance = n + 1;

    for (int currentWeight = 1; currentWeight <= n; currentWeight++) {

        for (int error = 1; error < numberOfErrors; error++) {

            if (weight(error) != currentWeight) {
                continue;
            }

            int syndrome = calculateSyndrome(H, error);

            if (syndrome == 0) {
                codeDistance = currentWeight;
                break;
            }
        }

        if (codeDistance != n + 1) {
            break;
        }
    }

    cout << "\nCode distance: " << codeDistance << endl;

    string yInput;

    cout << "\nEnter received word y (15 bits): ";
    cin >> yInput;

    if (yInput.length() != n) {
        cout << "Received word must contain exactly 15 bits." << endl;
        return 1;
    }

    vector<int> y(n);

    for (int i = 0; i < n; i++) {
        if (yInput[i] != '0' && yInput[i] != '1') {
            cout << "Received word must contain only 0 and 1." << endl;
            return 1;
        }

        y[i] = yInput[i] - '0';
    }

    double th0;
    double th1;
    int maxIterations;

    cout << "Enter th0: ";
    cin >> th0;

    cout << "Enter th1: ";
    cin >> th1;

    cout << "Enter maximum number of iterations: ";
    cin >> maxIterations;

    vector<int> decoded = gallagerB(H, y, th0, th1, maxIterations);

    cout << "\nGallager B result:\n";

    for (int bit : decoded) {
        cout << bit;
    }

    cout << endl;

    if (isCodeword(H, decoded)) {
        cout << "Result is a codeword." << endl;
    }
    else {
        cout << "Decoding did not reach a codeword." << endl;
    }

    findMinimumError(H, codeDistance);

    return 0;
}