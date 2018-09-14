#include <iostream>
#include <fstream>

enum State {
    ST_CHAR,
    ST_OPEN1,
    ST_CLOSE1,
    ST_OPEN2,
    ST_REF,
    ST_LINK,
    ST_DEF
};

enum State cur = ST_CHAR;

int main() {
    ifstream file("ruby.md");
    char c;

    while (file.get(c)) {
        switch (cur) {
            case ST_CHAR:
                if ('[' == c) {
                    cur = ST_OPEN1;
                }
                break;
            case ST_OPEN1:
                if (']' == c) {
                    cur = ST_CLOSE1;
                } else {

                }
                break;
            case ST_CLOSE1:
                if ('[' == c) {
                    cur = ST_OPEN2;
                }
        }
    }

    file.close();
}
