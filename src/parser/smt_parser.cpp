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

#include <cstdio>
#include <cstring>
#include <ctype.h>

#include "parser/smt_parser.h"
#include "constraints/number.h"

using namespace std;
using namespace cutsat;

void SmtParser::parse() throw(ParserException) {

    // Open the file
    d_input = fopen(d_filename.c_str(), "r");
    if (d_input == NULL) throw ParserException(d_lineNumber, "can't open " + d_filename);

    // Ensure the basic buffer
    d_bufferPtr = d_buffer = (char*)malloc(1000);
    d_buffer[0] = 0;
    d_bufferSize = 1000;

    // Parse the benchmark
    benchmark();

    // Free the buffer memory
    if (d_buffer != NULL) {
        free(d_buffer);
        d_buffer = NULL;
    }
}

void SmtParser::benchmark()
{
    match("(");
    match("benchmark");
    token();

    for(;;) {

    	skipSpace();
    	if (peek() == ')') {
    		break;
    	}

    	match(":");
    	token();

    	if (strcmp(d_token, "status") == 0) { nextLine(); }
        else if (strcmp(d_token, "category") == 0) { match('{', '}'); }
        else if (strcmp(d_token, "logic") == 0) { match("QF_LIA"); }
        else if (strcmp(d_token, "extrafuns") == 0) { functions(); }
        else if (strcmp(d_token, "extrapreds") == 0) { predicates(); }
        else if (strcmp(d_token, "assumption") == 0) { assumption(); }
        else if (strcmp(d_token, "formula") == 0) { formula(); }
        else throw ParserException(d_lineNumber, d_filename);
    }

    match(")");
}

void SmtParser::functions()
{
    match("(");

    for(;;) {

    	skipSpace();
    	if (peek() == ')') {
    		break;
    	}

        match("(");
        token();
        match("Int");

        assert(d_variables.find(d_token) == d_variables.end());
        d_variables[d_token] = d_solver.newVariable(TypeInteger, d_token);

        match(")");
    }

    match(")");
}

void SmtParser::predicates()
{
    match("(");

    for(;;) {
        match("(");
        token();
        assert(d_variables.find(d_token) == d_variables.end());
        Variable var = d_solver.newVariable(TypeInteger, d_token);
        d_variables[d_token] = var;
        d_solver.setUpperBound<TypeInteger>(var, 1);
        d_solver.setLowerBound<TypeInteger>(var, 0);
        match(")");
    }

    match(")");
}

void SmtParser::assumption(bool paren)
{
    d_constraintC.clear();

    enum Relation { SMT_LT, SMT_GT, SMT_LE, SMT_GE, SMT_EQ };

    if (paren) {
    	match("(");
    }

    // Get the relation
    skipSpace();
    char c = get();
    Relation op;
    Integer m;
    if (c == '>') {
    	if (peek() == '=') { get(); op = SMT_GE; d_constraintRHS = 0; m = 1; }
    	else { op = SMT_GT; d_constraintRHS = 1; m =  1; }
    }
    else if (c == '<') {
    	if (peek() == '=') { get(); op = SMT_LE; d_constraintRHS = 0; m = -1; }
    	else { op = SMT_LT; d_constraintRHS = 1; m = -1; }
    } else if (c == '=') {
    	op = SMT_EQ; d_constraintRHS = 0; m = 1;
    } else {
    	throw ParserException(d_lineNumber, d_filename);
    }

    // Get the left hand side
    sum(m);
    // Get the right hand side
    sum(-m);

    if (paren) {
    	match(")");
    }

    // Add the constraint
    addIntegerConstraint(d_constraintC, d_constraintRHS);
    if (op == SMT_EQ) {
    	map<Variable,Integer>::iterator it = d_constraintC.begin();
    	map<Variable,Integer>::iterator it_end = d_constraintC.end();
    	for (; it != it_end; ++ it) {
    		it->second = -(it->second);
    	}
    	d_constraintRHS = -d_constraintRHS;
        addIntegerConstraint(d_constraintC, d_constraintRHS);
    }
}

void SmtParser::sum(Integer m)
{
    const char* error = NULL;
    skipSpace();
    char c = peek();
    if (c == '(') {
        get();
        skipSpace();
        c = peek();
        switch(c) {
            case '+':
                get();
                for(;;) {
                    sum(m);
                    skipSpace();
                    if (peek() == ')') {
                        get();
                        return;
                    }
                }
                break;
            case '*': {
                get();
                skipSpace();
                Integer number;
                if (peek() == '(') {
                    get();
                    match("~");
                    skipSpace();
                    number = -NumberUtils<Integer>::read(d_bufferPtr, &error);
                    d_bufferPtr = error;
                    match(")");
                } else {
                    number = NumberUtils<Integer>::read(d_bufferPtr, &error);
                    d_bufferPtr = error;
                }
                sum(m*number);
                break;
            }
            case '-':
                get();
                sum(m);
                sum(-m);
                break;
            case '~':
                get();
                sum(-m);
                break;
            default:
                throw ParserException(d_lineNumber, d_filename);
        }
        match(")");
    } else {
        if (isalpha(c)) {
            token();
            Variable var = d_variables[d_token];
            CUTSAT_TRACE("parser") << m << "*" << var << endl;
            d_constraintC[var] += m;
        } else if (isdigit(c)) {
            Integer number = NumberUtils<Integer>::read(d_bufferPtr, &error);
            d_bufferPtr = error;
            d_constraintRHS -= m*number;
        } else {
            throw ParserException(d_lineNumber, d_filename);
        }
    }
}

void SmtParser::formula() {
	skipSpace();

	if (peek() == '(') {
		match("(");

		skipSpace();

		if (peek() == 'a') {
			// and
			match("and");

			do {
				assumption();
				skipSpace();
			} while (peek() != ')');
		} else {
			// just a single assumption
			assumption(false);
		}

		match(")");
	} else {
		match("true");
	}
}
