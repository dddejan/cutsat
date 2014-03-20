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

#include <set>
#include <vector>
#include <cassert>
#include <iostream>

#include "util/exception.h"

namespace cutsat {

class Trace {

    /** Tags we are tracing */
    static std::set<std::string> s_enabledTags;

public:

    /**
     * Returns true if tracing is enabled.
     * @return true if tracing is enabled
     */
    static bool isEnabled();

    /**
     * Registeres the tag to the list of available tags.
     * @param tag
     */
    static void registerTag(const char* tag);

    /**
     * Set whether to trace a tag or not.
     * @param tag the tag
     * @param trace true if we are tracing it, false if not
     */
    static void enable(std::string tag) throw (CutSatException) {
        s_enabledTags.insert(tag);
    }

    /**
     * Enable all registered tags.
     */
    static void enableAll();

    /**
     * Enables a regular expression tag for tracing.
     * @param tag
     */
    static void enableRegex(std::string tag);

    /**
     * Returns true if we are tracing this tag.
     * @param tag the tag
     * @return true if tracing
     */
    static bool isEnabled(std::string tag);

};

class TraceTag {

    /** Available tags */
    static const char* s_availableTags[100];

    /** Number of registered tags */
    static unsigned s_availableTagsCount;

    friend class Trace;

 public:

    /**
     * Add another tag to all the registered tags.
     * @param tag the tag
     */
    TraceTag(const char* tag);

    /**
     * Returns the string representing all available tags separated by a comma.
     * @return
     */
    static std::string getAvailableTagsAsString();

    static unsigned getAvailableTagsCount() {
        return s_availableTagsCount;
    }

    static const char** getAvailableTags() {
        return s_availableTags;
    }
};

template<typename T>
inline std::ostream& operator << (std::ostream& out, std::vector<T> someVector) {
    out << "[";
    bool first = true;
    for(unsigned i = 0; i < someVector.size(); ++ i) {
        if (!first) out << ",";
        else { first = false; }
        out << someVector[i];
    }
    out << "]";
    return out;
}

}

#ifndef CUTSAT_TRACING_ENABLED
    #define CUTSAT_TRACE(tag) if (false) std::cerr
	#define CUTSAT_TRACE_FN(tag) if (false) std::cerr
#else
    #include <boost/current_function.hpp>
    #define CUTSAT_TRACE(tag) if (cutsat::Trace::isEnabled(tag)) std::cerr
    #define CUTSAT_TRACE_FN(tag) if (cutsat::Trace::isEnabled(tag)) std::cerr << "[" << BOOST_CURRENT_FUNCTION<< "] " << std::endl
#endif


