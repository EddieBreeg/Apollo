#include <core/Log.hpp>
#include <catch2/catch_session.hpp>
#include <spdlog/spdlog.h>

int main(int argc, const char** argv)
{
	spdlog::set_level(spdlog::level::trace);
	return Catch::Session().run(argc, argv);
}