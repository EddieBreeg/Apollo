#include <core/ULID.hpp>
#include <iostream>

int main(int, const char**)
{
	const apollo::ULID id = apollo::ULID::Generate();
	char buf[37] = {};
	id.ToChars(buf);
	std::cout << buf << '\n';
}