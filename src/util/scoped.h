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

namespace cutsat {

/**
 * Restores the value of an object to the original one when we get out of scope. For example
 * for resetting the value on returing from a method.
 */
template <typename T>
class Scoped {
	T& t;
	T t_original;
public:
	Scoped(T& t)
	: t(t), t_original(t) {}
	~Scoped() {
		t = t_original;
	}
};

}

