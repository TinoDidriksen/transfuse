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

#include "options.hpp"
#include "base64.hpp"
#include "filesystem.hpp"
#include "shared.hpp"
#include <unicode/uclean.h>
#include <xxhash.h>
#include <iostream>
#include <fstream>
#include <random>
#include <memory>
#include <stdexcept>
#include <cstdlib>

namespace Transfuse {

fs::path extract(fs::path tmpdir, fs::path infile, std::string_view format, std::string_view stream, bool wipe);

std::ostream* write_or_stdout(const char* arg, std::unique_ptr<std::ostream>& out) {
	if (arg[0] == '-' && arg[1] == 0) {
		return &std::cout;
	}
	out.reset(new std::ofstream(arg, std::ios::binary));
	if (!out->good()) {
		std::string msg{"Could not write file "};
		msg += arg;
		throw std::runtime_error(msg);
	}
	return out.get();
}

}

int main(int argc, char* argv[]) {
	using namespace icu;
	using namespace Transfuse;
	using namespace Options;

	auto opts = make_options(
		O('h', "help", "shows this help"),
		O('?', "", "shows this help"),
		spacer(),
		O('f', "format", ARG_REQ, "input file format: txt, html, odt, odp, docx, pptx; defaults to auto"),
		O('s', "stream", ARG_REQ, "output stream format: apertium, visl; defaults to apertium"),
		O('m', "mode", ARG_REQ, "operating mode: extract, inject, clean; default depends on executable used"),
		O('d', "dir", ARG_REQ, "folder to store state in (implies -k); defaults to creating temporary"),
		O('k', "keep", ARG_NO, "don't delete temporary folder after injection"),
		O('K', "no-keep", ARG_NO, "always delete folder"),
		O('i', "input", ARG_REQ, "input file, if not passed as arg; default and - is stdin"),
		O('o', "output", ARG_REQ, "output file, if not passed as arg; default and - is stdout"),
		spacer(),
		O(0, "url64", ARG_REQ, "base64-url encodes the passed value"),
		O(0, "hash32", ARG_REQ, "xxhash32 + base64-url encodes the passed value"),
		O(0, "hash64", ARG_REQ, "xxhash64 + base64-url encodes the passed value"),
		final()
	);
	argc = opts.parse(argc, argv);

	std::string exe = fs::path(argv[0]).stem().string();
	if (opts['h'] || opts['?']) {
		std::cout << exe << " [options] [input-file] [output-file]\n";
		std::cout << "\n";
		std::cout << "Options:\n";
		std::cout << opts.explain();
		return 0;
	}

	if (auto o = opts["url64"]) {
		std::cout << base64_url(o->value) << std::endl;
		return 0;
	}
	if (auto o = opts["hash32"]) {
		auto xxh = XXH32(o->value.data(), o->value.size(), 0);
		std::cout << base64_url(xxh) << std::endl;
		return 0;
	}
	if (auto o = opts["hash64"]) {
		auto xxh = XXH64(o->value.data(), o->value.size(), 0);
		std::cout << base64_url(xxh) << std::endl;
		return 0;
	}

	std::string_view mode{ "clean" };
	if (exe == "tf-extract") {
		mode = "extract";
	}
	else if (exe == "tf-inject") {
		mode = "inject";
	}
	else if (exe == "tf-clean") {
		mode = "clean";
	}

	std::string_view format{"auto"};
	std::string_view stream{"apertium"};
	fs::path tmpdir;

	fs::path infile;
	std::ostream* out = nullptr;
	std::unique_ptr<std::ostream> _out;

	// Handle cmdline arguments
	while (auto o = opts.get()) {
		switch (o->opt) {
		case 'f':
			format = o->value;
			break;
		case 's':
			stream = o->value;
			break;
		case 'm':
			mode = o->value;
			break;
		case 'd':
			tmpdir = path(o->value);
			opts.set("keep");
			break;
		case 'K':
			opts.unset("keep");
			break;
		case 'i':
			infile = path(o->value);
			break;
		case 'o':
			out = write_or_stdout(o->value.data(), _out);
			break;
		}
	}

	// Funnel remaining unparsed arguments into input and/or output files
	if (argc > 2) {
		if (infile.empty() && !out) {
			infile = argv[1];
			out = write_or_stdout(argv[2], _out);
		}
		else if (infile.empty()) {
			infile = argv[1];
		}
		else if (!out) {
			out = write_or_stdout(argv[1], _out);
		}
	}
	else if (argc > 1) {
		if (infile.empty()) {
			infile = argv[1];
		}
		else if (!out) {
			out = write_or_stdout(argv[2], _out);
		}
	}
	if (infile.empty()) {
		infile = "-";
	}
	if (!out) {
		out = &std::cout;
	}

	UErrorCode status = U_ZERO_ERROR;
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		throw std::runtime_error(concat("Could not initialize ICU: ", u_errorName(status)));
	}

	auto curdir = fs::current_path();

	if (mode == "clean") {
		tmpdir = extract(tmpdir, infile, format, stream, opts["no-keep"] != nullptr);
	}
	else if (mode == "extract") {
		tmpdir = extract(tmpdir, infile, format, stream, opts["no-keep"] != nullptr);
		std::ifstream data("extracted", std::ios::binary);
		(*out) << data.rdbuf();
	}
	else if (mode == "inject") {
	}

	// If neither --dir nor --keep, wipe the temporary folder
	if (!opts["keep"] && (mode == "clean" || mode == "inject")) {
		fs::current_path(curdir);
		fs::remove_all(tmpdir);
	}
}
