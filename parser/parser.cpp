#include <iostream>
#include <fstream>

using namespace std;

int main() {
    fstream inFile, outFile;
    string word;

    inFile.open("DATALOG.TXT");
    if (inFile.fail()) {
        cout << "Could not find file." << endl;
        return 1;
    }

    while (inFile >> word) {
        cout << word << endl;
    }
    inFile.close();

    return 0;
}