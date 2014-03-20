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

#include <cln/io.h>
#include <cln/integer.h>
#include <cln/integer_io.h>
#include <cln/rational.h>
#include <cln/rational_io.h>
#include <cln/dfloat.h>

#include <boost/mpl/vector.hpp>
#include <boost/numeric/conversion/converter.hpp>
#include <boost/math/common_factor.hpp>

namespace cln {
extern cl_read_flags cl_I_read_flags;
extern cl_read_flags cl_RA_read_flags;
}

namespace cutsat {

/** The Boolean type */
typedef bool Boolean;

/** The integer type */
typedef cln::cl_I Integer;

/** Native integer type */
typedef boost::int64_t Integer64;

/** The rational type */
typedef cln::cl_RA Rational;

template <typename Number> struct NumberUtils;

template <>
struct NumberUtils<cln::cl_I> {
    inline static Integer read(const char* string, const char** error) {
        return cln::read_integer(cln::cl_I_read_flags, string, NULL, error);
    }
    inline static cln::cl_I pow(const cln::cl_I& a, unsigned pow) {
    	return cln::expt_pos(a, pow);
    }
    inline static cln::cl_I ceil(double value) {
        return boost::numeric::Ceil<int>::nearbyint(value);
    }
    inline static cln::cl_I floor(double value) {
        return boost::numeric::Floor<int>::nearbyint(value);
    }
    inline static cln::cl_I lcm(const cln::cl_I& a, const cln::cl_I& b) {
        return cln::lcm(a, b);
    }
    inline static cln::cl_I gcd(const cln::cl_I& a, const cln::cl_I& b) {
        return cln::gcd(a, b);
    }
    inline static cln::cl_I abs(const cln::cl_I& a) {
        return cln::abs(a);
    }
    inline static unsigned toUnsigned(const cln::cl_I& x) {
        return cln::cl_I_to_uint(x);
    }
    inline static int toInt(const cln::cl_I& x) {
        return cln::cl_I_to_int(x);
    }
    inline static cln::cl_I divideDown(const cln::cl_I& a, const cln::cl_I& b) {
        return cln::floor1(a, b);
    }
    inline static cln::cl_I divideUp(const cln::cl_I& a, const cln::cl_I& b) {
        return cln::ceiling1(a, b);
    }
    // Does a divide b
    inline static bool divides(const cln::cl_I& a, const cln::cl_I& b) {
    	return cln::mod(b, a) == 0;
    }
    // Return number of digits of a
    inline static unsigned digits(cln::cl_I a) {
    	unsigned digits = 0;
    	do {
    		digits ++;
    		a = cln::floor1(a, 10);
    	} while (a > 0);
    	return digits;
    }
};

template <>
struct NumberUtils<cln::cl_RA> {
    inline static cln::cl_RA read(const char* string, const char** error) {
        return cln::read_rational(cln::cl_RA_read_flags, string, NULL, error);
    }
    inline static cln::cl_RA ceil(double value) {
        return cln::rational(cln::cl_DF(value));
    }
    inline static cln::cl_RA floor(double value) {
        return cln::rational(cln::cl_DF(value));
    }
    inline static cln::cl_I getDenominator(const cln::cl_RA& value) {
        return cln::denominator(value);
    }
    inline static cln::cl_I getNumerator(const cln::cl_RA& value) {
        return cln::numerator(value);
    }
    inline static cln::cl_I ceil(const cln::cl_RA& value) {
        return NumberUtils<Integer>::divideUp(getNumerator(value), getDenominator(value));
    }
    inline static cln::cl_I floor(const cln::cl_RA& value) {
        return NumberUtils<Integer>::divideDown(getNumerator(value), getDenominator(value));
    }
};

template <>
struct NumberUtils<int> {
    inline static int read(const char* string, const char** error) {
        return strtol(string, const_cast<char**>(error), 10);
    }
    inline static int ceil(double value) {
        return boost::numeric::Ceil<int>::nearbyint(value);
    }
    inline static int floor(double value) {
        return boost::numeric::Floor<int>::nearbyint(value);
    }
    inline static int gcd(int a, int b) {
        return boost::math::gcd<int>(a, b);
    }
    inline static int lcm(int a, int b) {
        return boost::math::lcm<int>(a, b);
    }
};

}
