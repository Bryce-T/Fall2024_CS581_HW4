/*
Bryce Taylor
bktaylor2@crimson.ua.edu
CS 581
Homework #3

This is a helper file to compare whether 2 or 3 outputs are equal.
If outputs are equal, then there should be no unintended flaws in the
game's logic caused by mulithreading.
*/

#include <fstream>
#include <iostream>

using namespace std;

bool compare2(string f1, string f2) {
    ifstream File1(f1);
    ifstream File2(f2);
    string s1;
    string s2;

    while (getline(File1, s1)) {
        getline(File2, s2);
        if (s1 != s2) {
            return false;
        }
    }

    return true;
}

bool compare3(string f1, string f2, string f3) {
    ifstream File1(f1);
    ifstream File2(f2);
    ifstream File3(f3);
    string s1;
    string s2;
    string s3;

    while (getline(File1, s1)) {
        getline(File2, s2);
        getline(File3, s3);
        if (s1 != s2 || s1 != s3) {
            return false;
        }
    }
    
    return true;    
}

int main(int argc, char* argv[]) {
    bool equals;

    if (argc < 3 || argc > 4) {
        cout << "Usage: (file 1) (file 2) (optional: file 3)" << endl;
        return 1;
    }

    if (argc == 3) {
        equals = compare2(argv[1], argv[2]);
    }
    else {
        equals = compare3(argv[1], argv[2], argv[3]);
    }

    if (equals == true) {
        cout << "The files are equal." << endl;
    }
    else {
        cout << "The files are not equal." << endl;
    }

    return 0;
}