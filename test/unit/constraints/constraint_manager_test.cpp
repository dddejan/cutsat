#include <boost/test/unit_test.hpp>

#include "constraints/constraint_manager.h"

using namespace cutsat;

class ConstraintManagerTestFixture
{

public:

	ConstraintManagerTestFixture() {
    }

    ~ConstraintManagerTestFixture() {
    }
};

BOOST_FIXTURE_TEST_SUITE(ConstraintManagerTest, ConstraintManagerTestFixture);

BOOST_AUTO_TEST_CASE(Variables)
{
	ConstraintManager d_cm;

	Variable x_int = d_cm.newVariable(TypeInteger);
	BOOST_CHECK(x_int.getType() == TypeInteger);

	Variable x_rational = d_cm.newVariable(TypeRational);
	BOOST_CHECK(x_rational.getType() == TypeRational);
	BOOST_CHECK(x_int != x_rational);
}

BOOST_AUTO_TEST_SUITE_END();
