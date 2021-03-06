// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <functional>
#include <vector>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include "../src/output.h"

using test = std::function<int()>;

size_t f = 0; // fails
size_t n = 0; // current test no.

ostream_t* tout;

int fail(std::string msg, int r=1) {
	return *tout<<'#'<<n<<": "<<msg<<"\n", r; }
int ok() { return 0; }

int run(const std::vector<test>& tests, std::string name, ostream_t* os=&COUT){
	//try {
		tout = os;
		for (auto t : tests) ++n, f += t();
		*tout << name << ": " << n-f << '/' << n << " ok.\n";
		if (f) *tout << f << '/' << n << " failed.\n";
		*tout << std::flush;
	//} catch (std::exception& e) { *tout << e.what() << std::endl;}
	return f > 0 ? 1 : 0;
}

// read file content
int file_read(const char* path, char* data, size_t size) {
#ifdef _WIN32
	HANDLE fd = ::CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (fd == INVALID_HANDLE_VALUE) return fail("file cannot be opened", -1);
	DWORD r;
	if (::ReadFile(fd, data, size, &r, 0) == FALSE) return fail("file cannot be read", -3);
	if (r != (DWORD)size) return fail("read failed", -2);
        ::CloseHandle(fd);
#else
	int fd = ::open(path, O_RDONLY | O_NONBLOCK);
	if (fd == -1) return fail("file cannot be opened", -1);
	int r = ::read(fd, data, size);
	if (r != (int)size) return fail("read failed", -2);
	::close(fd);
#endif
	return ok();
}

// compare file content and mem
int file_and_mem_cmp(const char* path, const char* expected, size_t size) {
	char* data = (char*) malloc(size);
	if (file_read(path, data, size) == -1) return -1;
	auto flags = COUT.flags();
	for (size_t pos = 0; pos != size; ++pos) {
		if (data[pos] != expected[pos]) {
			COUT << "differs at pos: " << pos << " 0x"
				<< std::hex << std::setw(2)
				<< std::setfill(syschar_t('0')) << pos <<std::endl;
			COUT.flags(flags);
			COUT << "\t got: " << (int_t)data[pos]
				<< " exp: " << (int_t)expected[pos] << std::endl;
			return -2;
		}
	}
	free(data);
	return ok();
}
