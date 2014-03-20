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

#include <vector>
#include <iostream>
#include <boost/program_options.hpp>

#include "util/trace.h"
#include "util/enums.h"
#include "parser/parser.h"
#include "solver/solver.h"

using namespace std;
using namespace cutsat;
using namespace boost::program_options;

TraceTag main_tag("main");

/**
 * Parses the program arguments.
 */
void getOptions(int argc, char* argv[], variables_map& variables);

/**
 * Prints the model of the solver.
 */
void printModel(const Solver& solver);

/**
 * Setup the options.
 */
void setOptions(Solver& solver, variables_map& variables);

/** The main program */
int main(int argc, char* argv[]) {

    // Get the options from command line and config files
    variables_map options;
    getOptions(argc, argv, options);

    // Get the files to sovle
    vector<string> files;
    if (options.count("input") > 0) {
        // We have filenames
        files = options.at("input").as< vector<string> >();
    } else {
        // Otherwise read from standard input
        files.push_back("-");
    }

    // Go through the input files and try to solve
    for(unsigned i = 0; i < files.size(); ++ i) {
        try {
            CUTSAT_TRACE("main") << "Solving " << files[i] << endl;
            // Create the constraint manager
            ConstraintManager cm;
            // Create the solver
            Solver solver(cm);
            // Set the options
            setOptions(solver, options);
            // Create the parser
            ParserRef parser = Parser::newParser(files[i].c_str(), solver);
            // Parser the problem
            parser->parse();
            // Add the variables to trace
            if (options.count("trace-var") > 0) {
            	vector<string> vars = options.at("trace-var").as< vector<string> >();
            	for (unsigned i = 0; i < vars.size(); ++ i) {
            		solver.addVaribleToTrace(vars[i].c_str());
            	}
            }
            // If asked, print the SMT
            if (options.count("output-smt") > 0) {
                solver.printProblem(cout, OutputFormatSmt);
            }
            // If asked, print the SMT
            if (options.count("output-mps") > 0) {
                solver.printProblem(cout, OutputFormatMps);
            }
            // If asked, print the SMT
            if (options.count("output-opb") > 0) {
            	solver.printProblem(cout, OutputFormatOpb);
            }
            // If not parse only, solve the problem
            if (options.count("parse-only") == 0) {
            	// Solve the problem
                SolverStatus result = solver.solve();
                // Output the result
                cout << result << endl;
                // If the model is requested print it
                if (result == Satisfiable && options.count("model") > 0) {
                    printModel(solver);
                }
                // If the stats are required, print them
                if (options.count("stats") > 0) {
                    cout << solver.getStatistics() << endl;
                }
                // Check for expected result
                if (options.count("expect") > 0) {
                  string expectedResult = options.at("expect").as<string>();
                  if (expectedResult == "sat" && result != Satisfiable) return -1;
                  if (expectedResult == "unsat" && result != Unsatisfiable) return -1;
                }
            }
        } catch (CutSatException& e) {
            cerr << e << endl;
            return -1;
        }
    }

    return 0;
}

void getOptions(int argc, char* argv[], variables_map& variables) {

#ifdef CUTSAT_TRACING_ENABLED
    string tags = "Enable a trace tag: " + TraceTag::getAvailableTagsAsString() + ".";
#endif
    // Define the options
    options_description description("Options");
    description.add_options()
        ("help,h",
                "Prints this help message")
        ("verbosity,v",
                value<unsigned>()->default_value(0),
                "Set the verbosity of the output.")
#ifdef CUTSAT_TRACING_ENABLED
        ("debug,d",
                value< vector<string> >(),
                tags.c_str())
        ("Debug,D",
                value< vector<string> >(),
                "Regular expression trace tags")
#endif
        ("input,i",
                value< vector<string> >(),
                "A problem to solve")
        ("expect,e",
                value<string>(),
                "Expected answer (sat, unsat)")
        ("model,m",
        		"Print the model")
        ("stats,s",
                "Print the statistics")
        ("linear-order",
        		"Use the order in which the variables were introduced")
        ("parse-only",
        		"Only parse the problem")
        ("output-cuts",
                "Output the SMT queries to prove the cuts for each conflict")
        ("output-smt",
                "Output the problem in SMT format")
        ("output-mps",
                "Output the problem in MPS format")
        ("output-opb",
                "Output the problem in OPB format, all variables are assumed binary")
        ("trace-var", value< vector<string> >(),
        		"Variable to trace")
        ("bound-estimate", value<unsigned>()->default_value(0),
        		"Supply an estimate for the bound of the solution")
        ("replace-vars-with-slacks",
        		"Replace all the variables with x = x+ - x-, with x+ >= 0 and x- >= 0, in order to have everything bounded")
        ("try-fourier-motzkin",
                "Try Fourier-Motzkin elimination before dynamic cuts")
        ("check-model",
        		"Validate the model if the problem is satisfiable")
        ("default-bound", value<int>()->default_value(-1),
        		"Default value for unbounded variables")
    ;

    // The input files can be positional
    positional_options_description positional;
    positional.add("input", -1);

    // Parse the options
    bool parseError = false;
    try {
        store(command_line_parser(argc, argv).options(description).positional(positional).run(), variables);
    } catch(...) {
        parseError = true;
    }

    // Set the trace tags
    if (variables.count("debug") > 0) {
        vector<string> tags = variables.at("debug").as< vector<string> >();
        for(unsigned i = 0; i < tags.size(); ++ i) {
            Trace::enable(tags[i]);
        }
    }

    // Set the regular expression trace tags
    if (variables.count("Debug") > 0) {
        vector<string> tags = variables.at("Debug").as< vector<string> >();
        for(unsigned i = 0; i < tags.size(); ++ i) {
            Trace::enableRegex(tags[i]);
        }
    }

    // If help needed, print it out
    if (parseError || variables.count("help") > 0) {
        if (parseError) cout << "Error parsing command line!" << endl;
        cout << description << endl;
        if (parseError) exit(1);
        else exit(0);
    }
}

void printModel(const Solver& solver)
{
    const map<string, Variable>& variables = solver.getVariables();

    map<string, Variable>::const_iterator it = variables.begin();
    map<string, Variable>::const_iterator it_end = variables.end();
    for (; it != it_end; ++ it) {
        cout << it->first << ": ";
        Variable var = it->second;
        switch (var.getType()) {
            case TypeInteger:
                cout << solver.getValue<TypeInteger>(var) << endl;
                break;
            default:
                assert(false);
        }
    }
}

void setOptions(Solver& solver, variables_map& options) {

	Verbosity verbosity = (Verbosity)options.at("verbosity").as<unsigned>();
	solver.setVerbosity(verbosity);

	solver.setPropagation(options.count("disable-propagation") == 0);
	solver.setCheckModel(options.count("check-model") > 0);
	solver.setDynamicOrder(options.count("linear-order") == 0);
	solver.setOutputCuts(options.count("output-cuts") > 0);
    solver.setBoundEstimate(options.at("bound-estimate").as<unsigned>());
    solver.setDefaultBound(options.at("default-bound").as<int>());
    solver.setReplaceVarsWithSlacks(options.count("replace-vars-with-slacks") > 0);
    solver.setTryFourierMotzkin(options.count("try-fourier-motzkin") > 0);
}
