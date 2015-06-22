/*
 * Copyright (C) 2003-2015 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * The .mpdignore backend code.
 *
 */

#ifndef MPD_EXCLUDE_H
#define MPD_EXCLUDE_H

#include "check.h"
#include "Compiler.h"
#include "fs/Glob.hxx"

#ifdef HAVE_CLASS_GLOB
#include <forward_list>
#endif

class Path;

class ExcludeList {
#ifdef HAVE_CLASS_GLOB
	std::forward_list<Glob> patterns;
#endif

public:
	gcc_pure
	bool IsEmpty() const {
#ifdef HAVE_CLASS_GLOB
		return patterns.empty();
#else
		/* not implemented */
		return true;
#endif
	}

	/**
	 * Loads and parses a .mpdignore file.
	 */
	bool LoadFile(Path path_fs);

	/**
	 * Checks whether one of the patterns in the .mpdignore file matches
	 * the specified file name.
	 */
	bool Check(Path name_fs) const;
};


#endif
