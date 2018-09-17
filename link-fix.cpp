#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <ctype.h>

#define TRY_OR_RETURN(cmd) do { \
    err = cmd;                  \
    if (err) {                  \
        return err;             \
    }                           \
} while (0);

using namespace std;

enum State {
    ST_CHAR,
    ST_OPEN1,
    ST_CLOSE1,
    ST_CLOSE1SPACE,
    ST_OPEN2,
    ST_REF,
    ST_PAREN,
};

// NOTE: moves file marker
// expects to be right after the opening bracket, paren, or quote
// return 0 if no errors
int handleTitle(ifstream &in, char closing, string &title) {
    char c;
    while (EOF != (c = in.get())) {
        if ('\n' == c) {
            cerr << "Error: unfinished title tag in link definition" << endl;
           return 1;
        } else if (c == closing) {
            break;
        } else {
            title.push_back(c);
        }
    }

    if (EOF == c) {
        cerr << "Error: reached EOF in the middle of a title tag" << endl;
        return 1;
    }
    return 0;
}

// NOTE: moves file marker
int handleLinkDef(ifstream &in, pair<string, string> &link) {
    char c;
    int err = 0;
    // ignore leading spaces before HREF
    do {
        c = in.get();
    } while (isblank(c));

    if ('\n' == c) {
        cerr << "Error: empty reference link definition" << endl;
        return 1;
    }

    // record HREF
    while (!isspace(c)) {
        link.first.push_back(c);
        c = in.get();
    }

    // skip over all spaces and newlines after HREF until next input (making sure to skip)
    char p;
    while (isspace(p = in.peek())) {
        c = in.get();
    }

    // handle title if there exists one
    if ('"' == p || '\'' == p) {
        c = in.get();
        TRY_OR_RETURN(handleTitle(in, p, link.second));
    } else if ('(' == p) {
        c = in.get();
        TRY_OR_RETURN(handleTitle(in, ')', link.second));
    }

    // skip over ending spaces too so that we don't add extra newlines
    while (isspace(in.peek()) && (EOF != in.get()));
    return 0;
}

int handleParen(ifstream &in, pair<string, string> &link) {
    char c;
    int err = 0;
    while ((EOF != (c = in.get())) && !isblank(c) && (')' != c)) {
        link.first.push_back(c);
    }

    if (in.eof()) {
        cerr << "Error: reached EOF in the middle of an inline link" << endl;
        return 1;
    }

    if (isblank(c)) {
        if ((EOF != (c = in.get())) && ('"' != c)) {
            cerr << "Error: extra spaces or wrong formatting in inline link definition" << endl;
            return 1;
        }

        TRY_OR_RETURN(handleTitle(in, '"', link.second));

        if (')' != in.get()) {
            cerr << "Error: missing closing parenthesis in inline link definition" << endl;
            return 1;
        }
    }

    return err;
}

// expects to be right after the opening paren
void handleRef(vector<string> &refs, string &ref) {
    vector<string>::iterator it = find(refs.begin(), refs.end(), ref);
    if (refs.end() == it) {
        refs.push_back(ref);
        cout << '[' << refs.size() << ']';
    } else {
        cout << '[' << distance(refs.begin(), it) + 1 << ']';
    }
}

void clear(string &ref, pair<string, string> &link) {
    ref.clear();
    link.first.clear();
    link.second.clear();
}

int main(int argc, char *argv[]) {
    enum State cur = ST_CHAR;
    vector<string> refs;
    unordered_map< string, pair<string, string> > lookup;
    char c;
    int err;
    string ref;
    pair<string, string> link;

    ifstream in(argv[1]);
    if (!in.good()) {
        cerr << "Error: can't read file " << argv[1] << endl;
        return 1;
    }

    while (EOF != (c = in.get())) {
        switch (cur) {
            case ST_CHAR:
                if ('[' == c) {
                    cur = ST_OPEN1;
                } else {
                    cout.put(c);
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
                    cout << '[' << ref << ']';
                    cur = ST_OPEN2;
                } else if ('(' == c) {
                    cout << '[' << ref << ']';
                    cur = ST_PAREN;
                } else if (':' == c) {
                    TRY_OR_RETURN(handleLinkDef(in, link));
                    lookup[ref] = link;
                    clear(ref, link);
                    cur = ST_CHAR;
                } else if (isblank(c)) {
                    cur = ST_CLOSE1SPACE;
                } else {
                    cout << '[' << ref << ']';
                    clear(ref, link);
                    cur = ST_CHAR;
                }
                break;
            case ST_CLOSE1SPACE:
                if ('[' == c) {
                    cur = ST_OPEN2;
                } else {
                    cur = ST_CHAR;
                }
                cout << '[' << ref << ']';
                break;
            case ST_PAREN:
                if ('#' == c) {
                    cout << "(#";
                } else {
                    handleRef(refs, ref);
                    link.first.push_back(c);
                    TRY_OR_RETURN(handleParen(in, link));
                    lookup[ref] = link;
                }
                clear(ref, link);
                cur = ST_CHAR;
                break;
            case ST_OPEN2:
                if (']' == c) {
                    handleRef(refs, ref);
                    clear(ref, link);
                    cur = ST_CHAR;
                } else {
                    clear(ref, link);
                    ref.push_back(c);
                    cur = ST_REF;
                }
                break;
            case ST_REF:
                if (']' == c) {
                    handleRef(refs, ref);
                    clear(ref, link);
                    cur = ST_CHAR;
                } else {
                    ref.push_back(c);
                }
                break;
        }
    }

    in.close();

    if (ST_CHAR != cur) {
        cerr << "Error: finished reading file, but ended up on non-default state" << endl;
        return 1;
    }

    for (int i = 0; i < refs.size(); ++i) {
        pair<string, string> link = lookup[refs[i]];
        cout << '[' << i + 1 << "]: " << link.first;
        if (!link.second.empty()) {
            cout << " \"" << link.second << '"' << endl;
        } else {
            cout << endl;
        }
    }
}
