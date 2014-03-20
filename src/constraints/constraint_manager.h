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
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>

#include <boost/cstdint.hpp>
#include <boost/integer_traits.hpp>

#include "util/config.h"
#include "constraints/constraint.h"

namespace cutsat {

/**
 * Constraint manager allocates the memory on demand expanding the memory. The amount of wasted
 * memory is kept so that the user decide if she wants to do a garbage-collection sweep.
 */
class ConstraintManager {

public:

    /** The reference to a constraint */
    typedef boost::uint64_t ConstraintRef;

    /** The null constraint */
    static const ConstraintRef NullConstraint = boost::integer_traits<ConstraintRef>::const_max;

private:

    /** Pointer to the memory */
    char* d_memory;

    /** Pointer to the new memory when doing garbage collection */
    char* d_gcMemory;

    /** Current capacity of each memory */
    size_t d_capacity;

    /** Current size of each memory */
    size_t d_size;

    /** Size of the new memory when doing garbage collection */
    size_t d_gcSize;

    /** The current total wasted memory */
    size_t d_wasted;

    /** How many variables we have */
    size_t d_variablesCount;

    /** Count for each pair (variable, polarity) in how many constraint it appears in */
    std::vector<unsigned> d_variableOccursCount;

    /** Default initial size for each of the memories */
    static const size_t s_initialSize = 100000;

    /** We have one additional bit for any application specific data */
    static const size_t s_type_bits = 3;

    /** We have one additional bit for any application specific data */
    static const size_t s_data_bits = s_type_bits + 1;

    /** Mask to extract the type bits */
    static const boost::uint32_t s_type_mask = (1 << s_type_bits) - 1;

    /** Mask to extract the flag bits */
    static const boost::uint32_t s_flag_mask = (1 << s_type_bits);

    inline unsigned align(unsigned size) {
    	return (size + 7) & ~((size_t)7);
    }

     /** Return the memory index of the constraint reference */
    inline static size_t getIndex(ConstraintRef ref) {
        return ref >> s_data_bits;
    }

    /**
     * Returns the reference, given a constraint pointer and a type. This is used when allocating
     * new memory
     */
    inline ConstraintRef getConstraintRef(ConstraintType type, const char* cPtr) const {
        return  ((ConstraintRef)(cPtr - d_memory) << s_data_bits) | type;
    }

    /**
     * Returns the reference, given the index into memory a type. This is used when allocating
     * new memory
     */
    inline ConstraintRef getConstraintRef(ConstraintType type, size_t index) const {
        return ((ConstraintRef) index << s_data_bits) | type;
    }

public:

    /**
     * Create a new constraint given the variables (constant = 0).
     */
    template <ConstraintType type>
    ConstraintRef newConstraint(const std::vector< Literal<type> >& literals, const typename ConstraintTraits<type>::constant_type& constant = 0, bool learnt = false);

    /**
     * Erase the given constraint.
     */
    template<ConstraintType type>
    inline void eraseConstraint(ConstraintRef constraint);

    /**
     * Creates a new variable.
     * @param type the type of the variable
     * @param name the name of the variable
     */
    Variable newVariable(VariableType type);

    unsigned getVariablesCount() const {
        return d_variablesCount;
    }

    /**
     * Creates a constraint manager.
     */
    ConstraintManager(size_t initialSize = s_initialSize);

    size_t getSize() const {
        return d_size;
    }

    size_t getCapacity() const {
        return d_capacity;
    }

    size_t getWasted() const {
        return d_wasted;
    }

    /**
     * Get the number of occurances of a variable.
     */
    size_t getOccuranceCount(Variable decisionVar, bool negated) {
    	return negated ? d_variableOccursCount[2*decisionVar.getId() + 1] : d_variableOccursCount[2*decisionVar.getId()];
    }

    /**
     * Returns the constraint pointed to by the reference.
     */
    template <ConstraintType type>
    TypedConstraint<type>& get(ConstraintRef ref) const {
        return *((TypedConstraint<type>*) (d_memory + getIndex(ref)));
    }

    /** Returns the type of the constraint given the reference */
    inline static ConstraintType getType(ConstraintRef ref) {
        return static_cast<ConstraintType>(ref & s_type_mask);
    }

    inline static ConstraintRef setFlag(ConstraintRef constraintRef) {
    	return constraintRef | s_flag_mask;
    }

    inline static ConstraintRef unsetFlag(ConstraintRef constraintRef) {
    	return ((constraintRef >> s_data_bits) << s_data_bits) | (constraintRef & s_type_mask);
    }

    inline static bool getFlag(ConstraintRef constraintRef) {
    	return constraintRef & s_flag_mask;
    }

    void gcBegin();
    void gcMove(std::vector<ConstraintRef>& constraints, std::map<ConstraintRef, ConstraintRef>& reallocMap);
    void gcEnd();

private:

    /**
     * Allocate a new bloc of a given size in the given memory.
     */
    template <ConstraintType type>
    inline char* allocate(size_t size);
};

/**
 * Constraint reference encapsulates the index of the constraint in the memory, the type of the constraint, and an
 * additional Boolean flag.
 */
typedef ConstraintManager::ConstraintRef ConstraintRef;

template <ConstraintType type>
ConstraintManager::ConstraintRef ConstraintManager::newConstraint(
        const std::vector< Literal<type> >& lits, const typename ConstraintTraits<type>::constant_type& constant, bool learnt) {

    CUTSAT_TRACE("constraints") << "newConstraint(" << lits << "," << constant << ")" << std::endl;

    // Compute the size (this should be safe as variant puts the template data at the end)
    size_t size = sizeof(TypedConstraint<type>) + sizeof(typename TypedConstraint<type>::literal_type)*lits.size();

    // Allocate the memory
    char* memory = allocate<type>(size);

    // Initialize the constraint
    TypedConstraint<type>* constraint = new (memory) TypedConstraint<type>(lits, constant, learnt);

    // Count the variables
    for (unsigned i = 0; i < lits.size(); ++ i) {
    	if (lits[i].isNegated()) {
    		d_variableOccursCount[2*lits[i].getVariable().getId()] ++;
    	} else {
    		d_variableOccursCount[2*lits[i].getVariable().getId() + 1] ++;
    	}
    }

    CUTSAT_TRACE("constraints") << "newConstraint() => " << *constraint << std::endl;

    return getConstraintRef(type, memory);
}

template <ConstraintType type>
inline char* ConstraintManager::allocate(size_t size) {

    CUTSAT_TRACE("constraints") << "allocate(" << size << ")" << std::endl;

    // The memory we are dealing with
    // Align the size
    size = align(size);
    // Ensure enough capacity
    size_t requested = d_size + size;
    if (requested > d_capacity) {
        while (requested > d_capacity) {
            d_capacity += d_capacity >> 1;
        }
        // Reallocate the memory
        d_memory = (char*)std::realloc(d_memory, d_capacity);
        if (d_memory == NULL) {
        	throw CutSatException("out of memory!");
        }
    }
    // The pointer to the new memory
    char* pointer = d_memory + d_size;
    // Increase the used size
    d_size += size;
    // And that's it
    return pointer;
}

template<ConstraintType type>
inline void ConstraintManager::eraseConstraint(ConstraintRef constraintRef) {
    TypedConstraint<type>& constraint = get<type>(constraintRef);
    assert(!constraint.inUse());
    size_t size = sizeof(TypedConstraint<type>) + sizeof(typename TypedConstraint<type>::literal_type)*constraint.getSize();
    for (unsigned i = 0; i < constraint.getSize(); ++ i) {
    	if (constraint.getLiteral(i).isNegated()) {
    		d_variableOccursCount[2*constraint.getLiteral(i).getVariable().getId()] --;
    	} else {
    		d_variableOccursCount[2*constraint.getLiteral(i).getVariable().getId() + 1] --;
    	}
    }
    constraint.setDeleted(true);
    // Deallocation is just increasing the wasted
    d_wasted += align(size);
}

}

