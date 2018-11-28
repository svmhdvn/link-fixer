// TODO case insensitive references (all lowercase)
// TODO markdown has a rule that 2 trailing spaces = <br> tag
// TODO generate a list of all closable shortcodes inside datadog/documentation/layouts/shortcodes/*.html
// TODO pass in list of closable shortcodes to look for
// TODO implement this as a pre commit hook in git (probably the best way to do it)

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <ctype.h>

#define TRY_OR_ERR(cmd) do { \
    err = cmd;               \
    if (err) {               \
        return err;          \
    }                        \
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
    ST_CODEBLOCK,
    ST_BRACE
};

struct shortcode {
    char open_delim;
    char close_delim;
    bool is_closing;
    string name;
    string args;
};

struct linkdef {
    string link;
    string title;
};

struct context {
    vector<string> refs;
    unordered_map<string, struct linkdef> lookup;
};

const int num_shortcodes = 5;
const string shortcodes[] = {"nextlink", "tab", "table", "tabs", "whatsnext"};

vector<struct context> context_stack;
struct linkdef ld;
enum State cur = ST_CHAR;
string ref;

bool closable_shortcode(const string name) {
    for (int i = 0; i < num_shortcodes; ++i) {
        if (shortcodes[i] == name) {
            return true;
        }
    }
    return false;
}

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
// Should be past all spaces after colon
int handleLinkDef(ifstream &in, struct linkdef &ld) {
    char c = in.get();
    int err = 0;

    // record HREF
    while (!isspace(c) && EOF != c) {
        ld.link.push_back(c);
        c = in.get();
    }

    // skip over all spaces and newlines after HREF until next input (making sure to skip)
    char p;
    while (EOF != (p = in.peek()) && isspace(p)) {
        c = in.get();
    }

    // handle title if there exists one
    if ('"' == p || '\'' == p) {
        c = in.get();
        TRY_OR_ERR(handleTitle(in, p, ld.title));
    } else if ('(' == p) {
        c = in.get();
        TRY_OR_ERR(handleTitle(in, ')', ld.title));
    }

    // skip over ending spaces too so that we don't add extra newlines
    while (isspace(in.peek()) && (EOF != in.get()));
    return 0;
}

int handleParen(ifstream &in, struct linkdef &ld) {
    char c;
    int err = 0;
    while ((EOF != (c = in.get())) && !isblank(c) && (')' != c)) {
        ld.link.push_back(c);
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

        TRY_OR_ERR(handleTitle(in, '"', ld.title));

        if (')' != in.get()) {
            cerr << "Error: missing closing parenthesis in inline link definition" << endl;
            return 1;
        }
    }

    return err;
}

// right after the second opening brace
int tryMatchShortcode(ifstream &in, struct shortcode *psc) {
    char open_delim = in.peek();
    if ('<' != open_delim && '%' != open_delim) {
        return 0;
    }
    in.get();
    char close_delim = ('<' == open_delim) ? '>' : '%';

    char c;
    while ((EOF != (c = in.get())) && isblank(c));
    if (EOF == c) {
        cerr << "Error: unfinished shortcode, reached EOF" << endl;
        return 1;
    }

    string name;
    bool is_closing = false;
    if ('/' == c) {
        is_closing = true;
        c = in.get();
    }

    do {
        name.push_back(c);
    } while ((EOF != (c = in.get())) && !isblank(c));

    string args = " ";
    while ((EOF != (c = in.get())) && close_delim != c) {
        args.push_back(c);
    }

    char c1 = in.get();
    char c2 = in.get();
    if ('}' != c1 && '}' != c2) {
        cerr << "Error: unfinished shortcode, no closing }} found" << endl;
        return 1;
    }

    psc->open_delim = open_delim;
    psc->close_delim = close_delim;
    psc->is_closing = is_closing;
    psc->name = name;
    psc->args = args;
    return 0;
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

void clear(string &ref, struct linkdef &ld) {
    ref.clear();
    ld.link.clear();
    ld.title.clear();
}

void outputLinkdefs(struct context &ctx) {
    for (unsigned i = 0; i < ctx.refs.size(); ++i) {
        struct linkdef ld = ctx.lookup[ctx.refs[i]];
        cout << '[' << i + 1 << "]: " << ld.link;
        if (!ld.title.empty()) {
            cout << " \"" << ld.title << '"' << endl;
        } else {
            cout << endl;
        }
    }
}

int state_machine(char c, ifstream &in) {
    int err = 0;
    switch (cur) {
        case ST_CHAR:
            if ('{' == c) {
                cout.put(c);
                cur = ST_BRACE;
            } else if ('[' == c) {
                cur = ST_OPEN1;
            } else if ('`' == c) {
                cout.put(c);
                cur = ST_CODEBLOCK;
            } else {
                cout.put(c);
            }
            break;
        case ST_BRACE: {
            cur = ST_CHAR;
            if ('{' != c) {
                return state_machine(c, in);
            }
            cout.put(c);

            struct shortcode sc = {0};
            TRY_OR_ERR(tryMatchShortcode(in, &sc));
            if (!sc.open_delim) {
                break;
            }

            // matching the shortcode worked
            if (sc.is_closing) {
                struct context ctx = context_stack.back();
                context_stack.pop_back();
                outputLinkdefs(ctx);
            } else if (closable_shortcode(sc.name)) {
                context_stack.emplace_back();
            }

            // // TEST
            // if (sc.is_closing) {
            //     cerr << "CLOSE: " << sc.name << endl;
            // } else {
            //     cerr << "OPEN: " << sc.name << endl;
            // }

            // output the rest of the shortcode
            cout << sc.open_delim << ' ';
            cout << (sc.is_closing ? "/" : "") << sc.name;
            cout << sc.args << sc.close_delim << "}}";
            break;
        }
        case ST_CODEBLOCK:
            cout.put(c);
            if ('`' == c) {
                cur = ST_CHAR;
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
                // ignore leading spaces before HREF
                while (isblank(in.peek())) {
                    c = in.get();
                }

                // This could either be an empty link reference or a harmless case.
                // Either way, we ignore if we end the line on the colon.
                if ('\n' == in.peek()) {
                    cout << '[' << ref << "]:";
                    clear(ref, ld);
                } else {
                    TRY_OR_ERR(handleLinkDef(in, ld));
                    context_stack.back().lookup[ref] = ld;
                    clear(ref, ld);
                }
                cur = ST_CHAR;
            } else if (isblank(c)) {
                cur = ST_CLOSE1SPACE;
            } else {
                cout << '[' << ref << ']';
                clear(ref, ld);
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
                handleRef(context_stack.back().refs, ref);
                ld.link.push_back(c);
                TRY_OR_ERR(handleParen(in, ld));
                context_stack.back().lookup[ref] = ld;
            }
            clear(ref, ld);
            cur = ST_CHAR;
            break;
        case ST_OPEN2:
            if (']' == c) {
                handleRef(context_stack.back().refs, ref);
                clear(ref, ld);
                cur = ST_CHAR;
            } else {
                clear(ref, ld);
                ref.push_back(c);
                cur = ST_REF;
            }
            break;
        case ST_REF:
            if (']' == c) {
                handleRef(context_stack.back().refs, ref);
                clear(ref, ld);
                cur = ST_CHAR;
            } else {
                ref.push_back(c);
            }
            break;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char c;
    int err = 0;

    ifstream in(argv[1]);
    if (!in.good()) {
        cerr << "Error: can't read file " << argv[1] << endl;
        return 1;
    }

    // start off with the top level context
    context_stack.emplace_back();

    while (EOF != (c = in.get())) {
        TRY_OR_ERR(state_machine(c, in));
    }
    in.close();

    if (ST_CHAR != cur) {
        cerr << "Error: finished reading file, but ended up on non-default state: " << cur << endl;
        return 1;
    }

    // merge all remaining trailing contexts into the global context
    struct context global;
    for (vector<struct context>::iterator it = context_stack.begin();
            it != context_stack.end();
            ++it) {
        struct context ctx = *it;
        global.refs.insert(global.refs.end(), ctx.refs.begin(), ctx.refs.end());
        global.lookup.merge(ctx.lookup);
    }

    // output the global list of linkdefs
    outputLinkdefs(global);
}
