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

    // Fields: timestamp (ms), energy used, cumulative energy, mpge, lat, long, trip, distance, date, time, course, speed
    // timestamp (ms), current, voltage
    // The first number is the timestamp; every time you hit a timestamp, everything before is saved

    return 0;
}