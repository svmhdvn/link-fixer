#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <ctype.h>

using namespace std;

enum State {
    ST_CHAR,
    ST_OPEN1,
    ST_CLOSE1,
    ST_OPEN2,
    ST_REF,
    ST_LINK,
    ST_PAREN
};

int main() {
    enum State cur = ST_CHAR;
    vector<string> refs;
    unordered_map<string, string> lookup;
    ifstream in("test.md");
    ofstream out("test-out.md");
    char c;

    string ref;
    string link;
    while (in.get(c)) {
        switch (cur) {
            case ST_CHAR:
                if ('[' == c) {
                    cur = ST_OPEN1;
                    ref.clear();
                } else {
                    out.put(c);
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
                    out << '[' << ref << ']';
                } else if ('(' == c) {
                    cur = ST_PAREN;
                    out << '[' << ref << ']';
                } else if (':' == c) {
                    cur = ST_LINK;
                    link.clear();
                } else {
                    cur = ST_CHAR;
                    out << '[' << ref << ']';
                }
                break;
            case ST_PAREN:
                if (')' == c || isspace(c)) {
                    cur = ST_CHAR;
                    vector<string>::iterator it = find(refs.begin(), refs.end(), ref);
                    if (refs.end() == it) {
                        refs.push_back(ref);
                        out << '[' << refs.size() << ']';
                    } else {
                        out << '[' << distance(refs.begin(), it) + 1 << ']';
                    }
                    lookup[ref] = link;
                } else if ('#' == c) {
                    cur = ST_CHAR;
                    out << "(#";
                } else {
                    link.push_back(c);
                }
                break;
            case ST_OPEN2:
                if (']' == c) {
                    cur = ST_CHAR;
                    vector<string>::iterator it = find(refs.begin(), refs.end(), ref);
                    if (refs.end() == it) {
                        refs.push_back(ref);
                        out << '[' << refs.size() << ']';
                    } else {
                        out << '[' << distance(refs.begin(), it) + 1 << ']';
                    }
                } else {
                    cur = ST_REF;
                    ref.clear();
                    ref.push_back(c);
                }
                break;
            case ST_REF:
                if (']' == c) {
                    cur = ST_CHAR;
                    vector<string>::iterator it = find(refs.begin(), refs.end(), ref);
                    if (refs.end() == it) {
                        refs.push_back(ref);
                        out << '[' << refs.size() << ']';
                    } else {
                        out << '[' << distance(refs.begin(), it) + 1 << ']';
                    }
                } else {
                    ref.push_back(c);
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

    in.close();

    if (ST_CHAR != cur) {
        cerr << "Fatal error: ended up on non default state: " << cur << endl;
        return 1;
    }

    for (int i = 0; i < refs.size(); ++i) {
        out << '[' << i + 1 << "]: " << lookup[refs[i]] << endl;
    }

    out.close();
}
