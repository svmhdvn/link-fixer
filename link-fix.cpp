#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>

using namespace std;

enum State {
    ST_CHAR,
    ST_OPEN1,
    ST_CLOSE1,
    ST_OPEN2,
    ST_REF,
    ST_LINK
};


int main() {
    enum State cur = ST_CHAR;
    vector<string> refs;
    unordered_map<string, string> lookup;
    ifstream file("test.md");
    char c;

    string ref;
    string link;
    while (file.get(c)) {
        switch (cur) {
            case ST_CHAR:
                if ('[' == c) {
                    cur = ST_OPEN1;
                    ref.clear();
                }
                break;
            case ST_OPEN1:
                if (']' == c) {
                    cur = ST_CLOSE1;
                } else {
                    ref.push_back(c);
                }
                break;
            case ST_CLOSE1:
                if ('[' == c) {
                    cur = ST_OPEN2;
                } else if (':' == c) {
                    cur = ST_LINK;
                    link.clear();
                } else {
                    cur = ST_CHAR;
                }
                break;
            case ST_OPEN2:
                if (']' == c) {
                    cur = ST_CHAR;
                    refs.push_back(ref);
                } else {
                    cur = ST_REF;
                    ref.clear();
                    ref.push_back(c);
                }
                break;
            case ST_REF:
                if (']' == c) {
                    cur = ST_CHAR;
                    refs.push_back(ref);
                }
                break;
            case ST_LINK:
                if ('\n' == c) {
                    cur = ST_CHAR;
                    lookup[ref] = link;
                } else if (' ' != c) {
                    link.push_back(c);
                }
                break;
        }
    }

    file.close();

    if (ST_CHAR != cur) {
        cerr << "Fatal error: ended up on non default state: " << cur << endl;
        return 1;
    }

    cout << "List of refs (" << refs.size() << "):" << endl;
    for (vector<string>::iterator it = refs.begin(); it != refs.end(); ++it) {
        cout << "\t[" << *it << ']' << endl;
    }

    cout << endl << "Lookup table (" << lookup.size() << "):" << endl;
    for (unordered_map<string, string>::iterator it = lookup.begin(); it != lookup.end(); ++it) {
        cout << "\t[" << it->first << "]: " << it->second << endl;
    }
}
