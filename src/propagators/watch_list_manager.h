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

#include <vector>

#include "constraints/constraint_manager.h"
#include "propagators/events.h"

namespace cutsat {

class WatchList {

    typedef std::vector<ConstraintRef> container_class;

    /** Marks if the list of watched constraints needs cleanup */
    bool d_needsCleanup;

    /** List of watched constraints */
    container_class d_watchedConstraints;

 public:

    /** Iterator to go through the watches */
    typedef container_class::iterator iterator;
    /** Const iterator to go through the watches */
    typedef container_class::const_iterator const_iterator;

    /** Iterator to go through the watches */
    typedef container_class::reverse_iterator reverse_iterator;
    /** Const iterator to go through the watches */
    typedef container_class::const_reverse_iterator const_reverse_iterator;

    WatchList(): d_needsCleanup(false) {}

    ConstraintRef operator [] (size_t i) const {
        return d_watchedConstraints[i];
    }

    size_t getSize() const {
        return d_watchedConstraints.size();
    }

    bool needsCleanup() const {
        return d_needsCleanup;
    }

    void needsCleanup(bool needsCleanup) {
        d_needsCleanup = needsCleanup;
    }

    void clean(const ConstraintManager& cm) {
        assert(d_needsCleanup);
        unsigned i, i_end, j;
        for (i = j = 0, i_end = d_watchedConstraints.size(); i < i_end; ++ i) {
            ConstraintRef constraint = d_watchedConstraints[i];
            // This is safe as all the constraint types share the same initial header
            if (!cm.get<ConstraintTypeInteger>(constraint).isDeleted()) {
                d_watchedConstraints[j++] = constraint;
            } else {
            	assert(!cm.get<ConstraintTypeInteger>(constraint).inUse());
            }
        }
        d_watchedConstraints.resize(j);
        d_needsCleanup = false;
    }

    template<bool positive>
    void push_back(ConstraintRef constraintRef) {
        if (positive) {
        	d_watchedConstraints.push_back(ConstraintManager::unsetFlag(constraintRef));
        } else {
        	d_watchedConstraints.push_back(ConstraintManager::setFlag(constraintRef));
        }
    }

    void resize(iterator newEnd) {
        d_watchedConstraints.resize(newEnd - begin());
    }

    inline iterator begin() {
        return d_watchedConstraints.begin();
    }

    inline iterator end() {
        return d_watchedConstraints.end();
    }

    inline const_iterator begin() const {
        return d_watchedConstraints.begin();
    }

    inline const_iterator end() const {
        return d_watchedConstraints.end();
    }

    inline reverse_iterator rbegin() {
        return d_watchedConstraints.rbegin();
    }

    inline reverse_iterator rend() {
        return d_watchedConstraints.rend();
    }

    inline const_reverse_iterator rbegin() const {
        return d_watchedConstraints.rbegin();
    }

    inline const_reverse_iterator rend() const {
        return d_watchedConstraints.rend();
    }
};


class WatchListManager {

    /** Watchlist indexed by variables (one per type of event) */
    std::vector<WatchList> d_watchLists;

    /** The constraint manager (needed for cleanup) */
    const ConstraintManager& d_cm;

public:

    WatchListManager(const ConstraintManager& cm)
    : d_cm(cm) {}

    void addVariable(Variable var) {
        size_t neededSize = (var.getId() + 1) * MODIFICATION_COUNT;
        if (neededSize > d_watchLists.size()) {
            d_watchLists.resize(neededSize);
        }
    }

    WatchList& getWatchList(Variable var, VariableModificationType eventType) {
        WatchList& list = d_watchLists[var.getId()*MODIFICATION_COUNT + eventType];
        if (list.needsCleanup()) {
            list.clean(d_cm);
        }
        return list;
    }

    template<VariableModificationType eventType>
    void needsCleanup(Variable var) {
        unsigned index = var.getId()*MODIFICATION_COUNT + eventType;
        d_watchLists[index].needsCleanup(true);
    }

    void cleanAll() {
    	for(unsigned list = 0, list_end = d_watchLists.size(); list < list_end; ++ list) {
    		if (d_watchLists[list].needsCleanup()) {
    			d_watchLists[list].clean(d_cm);
    		}
    	}
    }

    void gcUpdate(const std::map<ConstraintRef, ConstraintRef>& reallocMap) {
    	for(unsigned list = 0, list_end = d_watchLists.size(); list < list_end; ++ list) {
    		WatchList& watchList = d_watchLists[list];
    		assert(!watchList.needsCleanup());
    		// Now update the new constraints
    		WatchList::iterator it = watchList.begin();
    		WatchList::iterator it_end = watchList.end();
    		for(; it != it_end; ++ it) {
    			if (ConstraintManager::getFlag(*it)) {
    				assert(reallocMap.find(ConstraintManager::unsetFlag(*it)) != reallocMap.end());
    				*it = ConstraintManager::setFlag(reallocMap.find(ConstraintManager::unsetFlag(*it))->second);
    			} else {
    				assert(reallocMap.find(*it) != reallocMap.end());
    				*it = reallocMap.find(*it)->second;
    			}
    		}
    	}
    }
};

}
