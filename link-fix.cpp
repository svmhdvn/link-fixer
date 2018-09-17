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
    ST_CLOSE1SPACE,
    ST_OPEN2,
    ST_REF,
    ST_PAREN,
};

// NOTE: moves file marker
// expects to be right after the opening bracket, paren, or quote
string handleTitle(ifstream &in, char closing) {
    string title;
    char c;
    while (in.get(c)) {
        if ('\n' == c) {
            // TODO: error here due to malformed title
        } else if (c == closing) {
            break;
        } else {
            title.push_back(c);
        }
    }

    return title;
}

// NOTE: moves file marker
void handleLinkDef(ifstream &in, pair<string, string> &link) {
    char c;
    // ignore leading spaces before HREF
    do {
        in.get(c);
    } while (' ' == c || '\t' == c);

    // record HREF
    while (!isspace(c)) {
        link.first.push_back(c);
        in.get(c);
    }

    // skip over all spaces and newlines after HREF until next input (making sure to skip)
    char p;
    while (isspace(p = in.peek())) {
        in.get(c);
    }

    // handle title if there exists one
    if ('"' == p || '\'' == p) {
        in.get(c);
        link.second = handleTitle(in, c);
    } else if ('(' == p) {
        in.get(c);
        link.second = handleTitle(in, ')');
    }

    // skip over ending spaces too so that we don't add extra newlines
    while (isspace(p = in.peek())) {
        in.get(c);
    }

    return;
}

// NOTE: moves file marker
// expects to be right after the opening paren
void handleRef(ofstream &out, vector<string> &refs, string &ref) {
    vector<string>::iterator it = find(refs.begin(), refs.end(), ref);
    if (refs.end() == it) {
        refs.push_back(ref);
        out << '[' << refs.size() << ']';
    } else {
        out << '[' << distance(refs.begin(), it) + 1 << ']';
    }
}

void handleParen(ifstream &in, pair<string, string> &link) {
    char c;
    while (in.get(c) && !isblank(c) && ')' != c) {
        link.first.push_back(c);
    }

    if (isblank(c)) {
        if (in.get(c) && '"' != c) {
            // TODO: error here, doesn't match implicit link title format
            return;
        }
        link.second = handleTitle(in, '"');
    }

    if (in.get(c) && ')' != c) {
        // TODO: error, no matching closing paren
        return;
    }
}

void clear(string &ref, pair<string, string> &link) {
    ref.clear();
    link.first.clear();
    link.second.clear();
}

int main() {
    enum State cur = ST_CHAR;
    vector<string> refs;
    unordered_map< string, pair<string, string> > lookup;
    ifstream in("test.md");
    ofstream out("test-out.md");
    char c;

    string ref;
    pair<string, string> link;
    while (in.get(c)) {
        switch (cur) {
            case ST_CHAR:
                if ('[' == c) {
                    cur = ST_OPEN1;
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
                    out << '[' << ref << ']';
                    cur = ST_OPEN2;
                } else if ('(' == c) {
                    out << '[' << ref << ']';
                    cur = ST_PAREN;
                } else if (':' == c) {
                    handleLinkDef(in, link);
                    lookup[ref] = link;
                    clear(ref, link);
                    cur = ST_CHAR;
                } else if (isspace(c)) {
                    cur = ST_CLOSE1SPACE;
                } else {
                    out << '[' << ref << ']';
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
                out << '[' << ref << ']';
                break;
            case ST_PAREN:
                if ('#' == c) {
                    out << "(#";
                } else {
                    handleRef(out, refs, ref);
                    link.first.push_back(c);
                    handleParen(in, link);
                    lookup[ref] = link;
                }
                clear(ref, link);
                cur = ST_CHAR;
                break;
            case ST_OPEN2:
                if (']' == c) {
                    handleRef(out, refs, ref);
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
                    handleRef(out, refs, ref);
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
        cerr << "Fatal error: ended up on non default state: " << cur << endl;
        return 1;
    }

    for (int i = 0; i < refs.size(); ++i) {
        pair<string, string> link = lookup[refs[i]];
        out << '[' << i + 1 << "]: " << link.first;
        if (!link.second.empty()) {
            out << " \"" << link.second << '"' << endl;
        } else {
            out << endl;
        }
    }

    out.close();
}
