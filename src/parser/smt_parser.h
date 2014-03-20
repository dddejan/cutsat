/**
 * Copyright 2010 Dejan Jovanovic.
 *
 * This file is part of cutsat.
 *
 * Cutsat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cutsat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cutsat.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <string>
#include <cstdio>

#include "parser/parser.h"

namespace cutsat {

class SmtParser : public Parser {

    /** The variables we are using */
    std::map<std::string, Variable> d_variables;

    /** Constraint coefficients (in order) */
    std::map<Variable, Integer>  d_constraintC;

    /** Constraint right-hand-side */
    Integer d_constraintRHS;

    /** Parse the benchmark */
    void benchmark();

    /** The input file */
    std::FILE* d_input;

    /** Line number */
    unsigned d_lineNumber;

    /** Buffer */
    char* d_buffer;

    /** Current token */
    char d_token[1000];

    /** Size of the buffer */
    size_t d_bufferSize;

    /** Current pointer into the buffer */
    const char* d_bufferPtr;

    inline void nextLine() {
       ssize_t read = getline(&d_buffer, &d_bufferSize, d_input);
       if (read == -1) throw ParserException(d_lineNumber, "can read from " + d_filename);
       d_bufferPtr = d_buffer;
       d_lineNumber ++;
    }

    inline void ensureBuffer() {
        // If we are at the end read more
        if (!*d_bufferPtr) {
            nextLine();
        }
    }

    /** Gets the next character from the stream */
    inline char get() {
        ensureBuffer();
        return *(d_bufferPtr++);
    }

    inline char peek() {
        return *(d_bufferPtr);
    }

    /** Skip whitespace */
    inline void skipSpace() {
        for(;;) {
            ensureBuffer();
            if (!isspace(*d_bufferPtr)) return;
            d_bufferPtr ++;
        }
    }

    inline void token() {
        skipSpace();
        char* tokenPtr = d_token;
        while(*d_bufferPtr && (isalnum(*d_bufferPtr) || *d_bufferPtr == '_')) {
            *(tokenPtr++) = get();
        }
        *tokenPtr = 0;
    }

    inline void match(char start, char end) {
        skipSpace();
        if (get() != start) throw ParserException(d_lineNumber, d_filename);
        while (get() != end);
    }

    inline void match(const char* token) {
        skipSpace();
        for(;*token;) {
            if (*(token++) != get()) throw ParserException(d_lineNumber, d_filename);
        }
    }

    /** Read a function declarations (int variables) */
    void functions();

    /** Read a predicate declarations (bool variables) */
    void predicates();

    /** Read an assumption (paren = have opening '(') */
    void assumption(bool paren = true);

    /** Read a smt arithmetic sum and add it to the constraint with coefficient m */
    void sum(Integer m);

    /** Read the formula */
    void formula();

public:

    SmtParser(Solver& solver)
    : Parser(solver), d_input(NULL), d_lineNumber(0), d_buffer(NULL), d_bufferPtr(NULL) {}

    void parse() throw(ParserException);
};

}

