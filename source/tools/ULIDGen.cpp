#include <core/ULID.hpp>
#include <iostream>

int main(int , const char**)
{
	const brk::ULID id = brk::ULID::Generate();
	char buf[37] = {};
	id.ToChars(buf);
	std::cout << buf << '\n';
}