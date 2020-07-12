/*
* Copyright (C) 2020 Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef e5bd51be_BASE64_HPP__
#define e5bd51be_BASE64_HPP__

#include "string_view.hpp"
#include <string>
#include <cstdint>

// Non-standard base64-encoder meant for URL-safe outputs. Doesn't pad and uses -_ instead of +/
void base64_url(std::string&, std::string_view input);

inline std::string base64_url(std::string_view input) {
	std::string rv;
	base64_url(rv, input);
	return rv;
}

inline void base64_url(std::string& str, uint32_t input) {
	const char* r = reinterpret_cast<const char*>(&input);
	return base64_url(str, std::string_view(r, sizeof(input)));
}

inline std::string base64_url(uint32_t input) {
	std::string rv;
	base64_url(rv, input);
	return rv;
}

inline void base64_url(std::string& str, uint64_t input) {
	const char* r = reinterpret_cast<const char*>(&input);
	return base64_url(str, std::string_view(r, sizeof(input)));
}

inline std::string base64_url(uint64_t input) {
	std::string rv;
	base64_url(rv, input);
	return rv;
}

#endif
