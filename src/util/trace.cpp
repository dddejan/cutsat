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

#include "util/trace.h"
#include <sstream>

#ifdef CUTSAT_TRACING_ENABLED
#include <boost/regex.hpp>
using namespace boost;
#endif

using namespace std;

namespace cutsat {

unsigned TraceTag::s_availableTagsCount = 0;

const char* TraceTag::s_availableTags[100];

set<string> Trace::s_enabledTags;

TraceTag::TraceTag(const char* tag)
{
#ifdef CUTSAT_TRACING_ENABLED
    s_availableTags[s_availableTagsCount++] = tag;
#endif
}

bool Trace::isEnabled()
{
#ifdef CUTSAT_TRACING_ENABLED
    return true;
#else
    return false;
#endif
}

void Trace::enableRegex(string regexTag)
{
#ifdef CUTSAT_TRACING_ENABLED
    regex re(regexTag);
    const char** availableTags = TraceTag::getAvailableTags();
    for (unsigned i = 0, i_end = TraceTag::getAvailableTagsCount(); i < i_end; ++ i) {
        if (regex_match(availableTags[i], re)) {
            enable(availableTags[i]);
        }
    }
#endif
}

bool Trace::isEnabled(std::string tag) {
    // Check if we are enabled
    if (s_enabledTags.find(tag) != s_enabledTags.end()) return true;
    return false;
}

void Trace::enableAll() {
    for (unsigned i = 0; i < TraceTag::s_availableTagsCount; ++ i) {
        enable(TraceTag::s_availableTags[i]);
    }
}

string TraceTag::getAvailableTagsAsString() {
    set<string> availableTags;

    for (unsigned i = 0; i < s_availableTagsCount; ++ i) {
        availableTags.insert(s_availableTags[i]);
    }

    set<string>::iterator tag = availableTags.begin();
    set<string>::iterator tag_end = availableTags.end();
    stringstream ss;
    bool first = true;
    while (tag != tag_end) {
        if (!first) {
            ss << ",";
        }
        first = false;
        ss << *tag;
        ++ tag;
    }
    if (first) {
        ss << "none";
    }
    return ss.str();
}

}
