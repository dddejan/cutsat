#include <boost/test/unit_test.hpp>

#include "parser/parser.h"

using namespace cutsat;

struct ParserTestFixture
{

	ConstraintManager d_cm;
	Solver d_solver;

public:

	ParserTestFixture()
	: d_solver(d_cm) {
    }

    ~ParserTestFixture() {
    }
};

BOOST_FIXTURE_TEST_SUITE(ParserTest, ParserTestFixture);

BOOST_AUTO_TEST_CASE(newParser)
{
	ParserRef parser = Parser::newParser("-", d_solver);
}

BOOST_AUTO_TEST_SUITE_END();
