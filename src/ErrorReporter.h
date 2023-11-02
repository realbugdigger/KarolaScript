#include <iostream>

#include "lexer/lexer.h"

//    In a full-featured language implementation, you will likely have multiple ways errors get displayed: on stderr, in an IDE’s error window, logged to a file, etc.
//    You don’t want that code smeared all over your scanner and parser.

//    Ideally, we would have an actual abstraction, some kind of “ErrorReporter” interface that gets passed to the scanner and parser so that we can swap out different reporting strategies.




// It will be good to implement error message like this

//    Error: Unexpected "," in argument list.
//
//    15 | function(first, second,);
//                               ^-- Here.



class ErrorReporter {
private:
    ErrorReporter(){}
public:
    static void error(int line, const char* message) {
        report(line, "", message);
    }

    static void error(Token token, const char* message) {
        if (token.type == TOKEN_EOF) {
            report(token.line, " at end", message);
        } else {
            report(token.line, "", message);
        }
    }
private:
    static void report(int line, const char* where, const char* message) {
        fprintf(stderr, "[%d] Error %s: %s\n", line, where, message);
    }
};