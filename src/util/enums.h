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

enum OutputFormat {
    OutputFormatSmt,
    OutputFormatMps,
    OutputFormatOpb,
    OutputFormatCnf,
    OutputFormatIlp
};

enum Verbosity {
	VERBOSITY_NO_OUTPUT,
    VERBOSITY_BASIC_INFO,
    VERBOSITY_DETAILED,
    VERBOSITY_EXTREME
};


}
