#include <boost/test/unit_test.hpp>

#include "solver/solver_state.h"
#include <vector>

using namespace std;
using namespace cutsat;

struct SolverStateTestFixture
{

	ConstraintManager d_cm;
	SolverState d_solverState;

public:

	SolverStateTestFixture()
	: d_solverState(d_cm) {
    }

    ~SolverStateTestFixture() {
    }
};

BOOST_FIXTURE_TEST_SUITE(SolverStateTest, SolverStateTestFixture);

BOOST_AUTO_TEST_CASE(pushPopLower)
{
	// The variables
	Variable x = d_cm.newVariable(TypeInteger);
	Variable y = d_cm.newVariable(TypeInteger);
	Variable z = d_cm.newVariable(TypeInteger);
	d_solverState.newVariable(x, "x");
	d_solverState.newVariable(y, "y");
	d_solverState.newVariable(z, "z");

	// Literals we'll be using
	vector<IntegerConstraintLiteral> literals;

	// Some reasons
	literals.clear();
	literals.push_back(IntegerConstraintLiteral(1, x));
	literals.push_back(IntegerConstraintLiteral(1, y));
	literals.push_back(IntegerConstraintLiteral(1, z));
	ConstraintRef c1 = d_cm.newConstraint(literals, 0);

	// Some reasons
	literals.clear();
	literals.push_back(IntegerConstraintLiteral(1, x));
	literals.push_back(IntegerConstraintLiteral(2, y));
	literals.push_back(IntegerConstraintLiteral(2, z));
	ConstraintRef c2 = d_cm.newConstraint(literals, 0);

	// Some reasons
	literals.clear();
	literals.push_back(IntegerConstraintLiteral(1, x));
	literals.push_back(IntegerConstraintLiteral(3, y));
	literals.push_back(IntegerConstraintLiteral(3, z));
	ConstraintRef c3 = d_cm.newConstraint(literals, 0);

	unsigned t0 = d_solverState.getTrailSize();
	d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(x, 0, ConstraintManager::NullConstraint);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x) == 0);
	BOOST_CHECK(d_solverState.getLowerBoundConstraint(x) == ConstraintManager::NullConstraint);

	unsigned t1 = d_solverState.getTrailSize();
	d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(x, 1, c1);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x) == 1);
	BOOST_CHECK(d_solverState.getLowerBoundConstraint(x) == c1);

	unsigned t2 = d_solverState.getTrailSize();
	d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(x, 2, c2);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x) == 2);
	BOOST_CHECK(d_solverState.getLowerBoundConstraint(x) == c2);

	unsigned t3 = d_solverState.getTrailSize();
	d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(x, 3, c3);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x) == 3);
	BOOST_CHECK(d_solverState.getLowerBoundConstraint(x) == c3);

	BOOST_CHECK(t0 == 0);
	BOOST_CHECK(t1 == 1);
	BOOST_CHECK(t2 == 2);
	BOOST_CHECK(t3 == 3);

	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t0) == 0);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t1) == 1);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t2) == 2);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t3) == 3);

	// Now pop the trail
	d_solverState.cancelUntil(t2);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x) == 2);
	BOOST_CHECK(d_solverState.getLowerBoundConstraint(x) == c2);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t0) == 0);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t1) == 1);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t2) == 2);

	d_solverState.cancelUntil(t1);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x) == 1);
	BOOST_CHECK(d_solverState.getLowerBoundConstraint(x) == c1);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t0) == 0);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t1) == 1);

	d_solverState.cancelUntil(t0);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x) == 0);
	BOOST_CHECK(d_solverState.getLowerBoundConstraint(x) == ConstraintManager::NullConstraint);
	BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, t0) == 0);
}

BOOST_AUTO_TEST_CASE(pushPopUpper)
{
    // The variables
    Variable x = d_cm.newVariable(TypeInteger);
    Variable y = d_cm.newVariable(TypeInteger);
    Variable z = d_cm.newVariable(TypeInteger);
    d_solverState.newVariable(x, "x");
    d_solverState.newVariable(y, "y");
    d_solverState.newVariable(z, "z");

    // Literals we'll be using
    vector<IntegerConstraintLiteral> literals;

    // Some reasons
    literals.clear();
    literals.push_back(IntegerConstraintLiteral(1, x));
    literals.push_back(IntegerConstraintLiteral(1, y));
    literals.push_back(IntegerConstraintLiteral(1, z));
    ConstraintRef c1 = d_cm.newConstraint(literals, 0);

    // Some reasons
    literals.clear();
    literals.push_back(IntegerConstraintLiteral(1, x));
    literals.push_back(IntegerConstraintLiteral(2, y));
    literals.push_back(IntegerConstraintLiteral(2, z));
    ConstraintRef c2 = d_cm.newConstraint(literals, 0);

    // Some reasons
    literals.clear();
    literals.push_back(IntegerConstraintLiteral(1, x));
    literals.push_back(IntegerConstraintLiteral(3, y));
    literals.push_back(IntegerConstraintLiteral(3, z));
    ConstraintRef c3 = d_cm.newConstraint(literals, 0);

    unsigned t0 = d_solverState.getTrailSize();
    d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(x, 3, ConstraintManager::NullConstraint);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x) == 3);
    BOOST_CHECK(d_solverState.getUpperBoundConstraint(x) == ConstraintManager::NullConstraint);

    unsigned t1 = d_solverState.getTrailSize();
    d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(x, 2, c1);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x) == 2);
    BOOST_CHECK(d_solverState.getUpperBoundConstraint(x) == c1);

    unsigned t2 = d_solverState.getTrailSize();
    d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(x, 1, c2);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x) == 1);
    BOOST_CHECK(d_solverState.getUpperBoundConstraint(x) == c2);

    unsigned t3 = d_solverState.getTrailSize();
    d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(x, 0, c3);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x) == 0);
    BOOST_CHECK(d_solverState.getUpperBoundConstraint(x) == c3);

    BOOST_CHECK(t0 == 0);
    BOOST_CHECK(t1 == 1);
    BOOST_CHECK(t2 == 2);
    BOOST_CHECK(t3 == 3);

    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t0) == 3);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t1) == 2);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t2) == 1);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t3) == 0);

    // Now pop the trail
    d_solverState.cancelUntil(t2);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x) == 1);
    BOOST_CHECK(d_solverState.getUpperBoundConstraint(x) == c2);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t0) == 3);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t1) == 2);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t2) == 1);

    d_solverState.cancelUntil(t1);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x) == 2);
    BOOST_CHECK(d_solverState.getUpperBoundConstraint(x) == c1);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t0) == 3);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t1) == 2);

    d_solverState.cancelUntil(t0);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x) == 3);
    BOOST_CHECK(d_solverState.getUpperBoundConstraint(x) == ConstraintManager::NullConstraint);
    BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, t0) == 3);
}

BOOST_AUTO_TEST_CASE(pushPopLowerMany)
{
	// The variables
	Variable x = d_cm.newVariable(TypeInteger);
	Variable y = d_cm.newVariable(TypeInteger);
	Variable z = d_cm.newVariable(TypeInteger);
	d_solverState.newVariable(x, "x");
	d_solverState.newVariable(y, "y");
	d_solverState.newVariable(z, "z");

	vector<ConstraintRef> reasons;

	// Literals we'll be using
	vector<IntegerConstraintLiteral> literals;

	// Some reasons
	literals.clear();
	literals.push_back(IntegerConstraintLiteral(1, x));
	literals.push_back(IntegerConstraintLiteral(1, y));
	literals.push_back(IntegerConstraintLiteral(1, z));

	for (unsigned i = 0; i < 100; ++ i) {
		reasons.push_back(d_cm.newConstraint(literals, i));
		BOOST_CHECK(d_solverState.getTrailSize() == i);
		d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(x, i, reasons[i]);
		for (unsigned j = 0; j <= i; ++ j) {
			BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x, j) == j);
			BOOST_CHECK(d_solverState.getLowerBoundConstraint(x, j) == reasons[j]);
		}
	}

	for (int i = 99; i >= 0; -- i) {
		BOOST_CHECK((int) d_solverState.getTrailSize() == i + 1);
		BOOST_CHECK(d_solverState.getLowerBound<TypeInteger>(x) == i);
		BOOST_CHECK(d_solverState.getLowerBoundConstraint(x) == reasons[i]);
		d_solverState.cancelUntil(i - 1);
	}
}

BOOST_AUTO_TEST_CASE(pushPopUpperMany)
{
	// The variables
	Variable x = d_cm.newVariable(TypeInteger);
	Variable y = d_cm.newVariable(TypeInteger);
	Variable z = d_cm.newVariable(TypeInteger);
	d_solverState.newVariable(x, "x");
	d_solverState.newVariable(y, "y");
	d_solverState.newVariable(z, "z");

	vector<ConstraintRef> reasons;

	// Literals we'll be using
	vector<IntegerConstraintLiteral> literals;

	// Some reasons
	literals.clear();
	literals.push_back(IntegerConstraintLiteral(1, x));
	literals.push_back(IntegerConstraintLiteral(1, y));
	literals.push_back(IntegerConstraintLiteral(1, z));

	for (unsigned i = 0; i < 100; ++ i) {
		reasons.push_back(d_cm.newConstraint(literals, i));
		BOOST_CHECK(d_solverState.getTrailSize() == i);
		d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(x, 100 - i, reasons[i]);
		for (unsigned j = 0; j <= i; ++ j) {
			BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x, j) == 100 - j);
			BOOST_CHECK(d_solverState.getUpperBoundConstraint(x, j) == reasons[j]);
		}
	}

	for (int i = 99; i >= 0; -- i) {
		BOOST_CHECK((int) d_solverState.getTrailSize() == i + 1);
		BOOST_CHECK(d_solverState.getUpperBound<TypeInteger>(x) == 100 - i);
		BOOST_CHECK(d_solverState.getUpperBoundConstraint(x) == reasons[i]);
		d_solverState.cancelUntil(i - 1);
	}
}

BOOST_AUTO_TEST_SUITE_END();

