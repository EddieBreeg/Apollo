#include <core/Errno.hpp>
#include <core/Json.hpp>
#include <core/ThreadPool.hpp>
#include <core/ULID.hpp>
#include <filesystem>
#include <freetype/freetype.h>
#include <fstream>
#include <iostream>
#include <msdfgen.h>
#include <rendering/text/FontAtlas.hpp>
#include <span>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace {
	bool Compare(auto&& val, auto&&... candidates)
	{
		return ((val == candidates) || ...);
	}

	template <class T, class... U>
	static constexpr bool IsOneOf = (std::is_same_v<T, U> || ...);

	template <class T>
	bool ParseValue(std::span<const char*>& inout_args, T& out_val)
	{
		if (inout_args.empty())
			return false;

		if constexpr (IsOneOf<T, std::string_view, std::filesystem::path, const char*>)
		{
			out_val = inout_args[0];
		}
		else if constexpr (std::is_arithmetic_v<T>)
		{
			const int len = strlen(inout_args[0]);
			const auto result = std::from_chars(inout_args[0], inout_args[0] + len, out_val);
			if ((bool)result.ec)
				return false;
		}
		else if constexpr (std::is_same_v<T, apollo::rdr::txt::GlyphRange>)
		{
			if (inout_args.size() < 2)
				return false;

			int len = strlen(inout_args[0]);
			uint32 first, last;
			auto result = std::from_chars(inout_args[0], inout_args[0] + len, first);
			if ((bool)result.ec)
				return false;
			inout_args = inout_args.subspan(1);

			len = strlen(inout_args[0]);
			result = std::from_chars(inout_args[0], inout_args[0] + len, last);
			if ((bool)result.ec)
				return false;

			out_val = { first, last };
		}

		inout_args = inout_args.subspan(1);
		return true;
	}

	struct Settings
	{
		const char* m_InputPath;
		std::filesystem::path m_BaseOutputPath = std::filesystem::current_path() / "out";
		uint32 m_Resolution = 64;
		apollo::rdr::txt::GlyphRange m_Range;
		double m_Padding = 1.0 / 64;
		bool m_HelpMessage = false;
		bool m_PaddingSpecified = false;
		double m_PixelRange = 10.0;
	};

	bool ParseSettings(std::span<const char*> args, Settings& out_settings)
	{
		if (args.empty())
			return false;

		std::string_view name = args[0];
		if (Compare(name, "--help", "-h"))
		{
			out_settings.m_HelpMessage = true;
			return true;
		}

		out_settings.m_InputPath = name.data();
		args = args.subspan(1);

		while (args.size())
		{
			name = args[0];
			args = args.subspan(1);

			if (Compare(name, "--help", "-h"))
			{
				out_settings.m_HelpMessage = true;
				return true;
			}
			else if (Compare(name, "-o", "--output"))
			{
				if (!ParseValue(args, out_settings.m_BaseOutputPath))
				{
					std::cerr << "Invalid value for output parameter\n";
					return false;
				}
			}
			else if (Compare(name, "-s", "--size"))
			{
				if (!ParseValue(args, out_settings.m_Resolution))
				{
					std::cerr << "Invalid value for size parameter\n";
					return false;
				}
			}
			else if (Compare(name, "-p", "--padding"))
			{
				if (!ParseValue(args, out_settings.m_Padding))
				{
					std::cerr << "Invalid value for padding parameter\n";
					return false;
				}
				out_settings.m_PaddingSpecified = true;
			}
			else if (Compare(name, "-r", "--range"))
			{
				if (!ParseValue(args, out_settings.m_Range))
				{
					std::cerr << "Invalid values for range parameter\n";
					return false;
				}
			}
			else if (Compare(name, "-d", "--distanceRange"))
			{
				if (!ParseValue(args, out_settings.m_PixelRange))
				{
					std::cerr << "Invalid values for distance range parameter\n";
					return false;
				}
			}
		}
		return true;
	}

	std::ostream& PrintHelp(std::ostream& out)
	{
		static constexpr const char* helpMsg =
			R"(Usage: atlasGen fontFile] [-o|--output outputName] [-r|--range first last] [-s|--size pixelSize] [-p|--padding glyphPadding] [-h|--help]

-fontFile: Path to the font truetype/opentype font file.
-o|--ouput: The base path where to put the result files. Extensions will be appended to this. Defaults to ./out
-r|--range: The first and last unicode codepoints to include in the atlas (as integers).
The default is Latin-1 Unicode range U+0020 U+00FF, that is: the 'space' character up to the 'small Y with Diaeresis' character
-s|--size: The vertical resolution of the MSDF bitmaps, in pixels. This is a scale factor, the actual
sizes will depend on the glyphs themselves. The default is 64.
-p|--padding: The padding to put around glyphs in the atlas, expressed in normalized font units (em). The default is 1/pixelSize.
-d|--distanceRange: The spread of the distance fields, in pixel. The default is 10.0.)";
		return out << helpMsg;
	}

	struct FreetypeContext
	{
		operator FT_Library() noexcept { return m_Handle; }
		~FreetypeContext()
		{
			if (m_Handle)
				FT_Done_FreeType(m_Handle);
		}
		FT_Library m_Handle = nullptr;
	};
} // namespace

namespace apollo::rdr::txt {
	void to_json(nlohmann::json& out_json, const apollo::rdr::txt::Glyph& glyph)
	{
		out_json = {
			{ "codepoint", uint32(glyph.m_Char) },
			{ "index", glyph.m_Index },
			{ "advance", glyph.m_Advance },
			{ "offset", glyph.m_Offset },
			{ "uv", glyph.m_Uv },
		};
	}
} // namespace apollo::rdr::txt

int main(int argc, const char** argv)
{
	Settings settings;
	const bool parseRes = ParseSettings(std::span{ argv + 1, size_t(argc) - 1 }, settings);
	if (!parseRes || settings.m_HelpMessage)
	{
		PrintHelp(std::cout) << '\n';
		return parseRes ? 0 : 1;
	}
	if (!settings.m_Resolution)
	{
		std::cerr << "Size can't be 0\n";
		return 1;
	}
	if (!settings.m_PaddingSpecified)
	{
		settings.m_Padding = 1.0 / settings.m_Resolution;
	}
	if (settings.m_Range.m_First > settings.m_Range.m_Last)
	{
		std::swap(settings.m_Range.m_First, settings.m_Range.m_Last);
	}

	const std::string atlasPath = settings.m_BaseOutputPath.string() + ".atlas";

	std::ofstream atlasFile{ atlasPath, std::ios::binary };
	if (!atlasFile.is_open())
	{
		std::cout << "Failed to open " << atlasPath
				  << "for writing: " << apollo::GetErrnoMessage(errno) << '\n';
		return 1;
	}

	FreetypeContext freetype;
	FT_Error err = FT_Init_FreeType(&freetype.m_Handle);

	if (err) [[unlikely]]
	{
		std::cerr << "Failed to init freetype: " << FT_Error_String(err) << '\n';
		return 1;
	}

	FT_Face face = nullptr;
	err = FT_New_Face(freetype, settings.m_InputPath, 0, &face);
	if (err)
	{
		std::cerr << "Failed to load font file " << settings.m_InputPath << ": "
				  << FT_Error_String(err) << '\n';
		return 1;
	}

	apollo::mt::ThreadPool threadPool;
	apollo::rdr::txt::AtlasGenerator generator{
		face,
		settings.m_Resolution * 16,
		settings.m_Resolution,
		settings.m_Padding,
	};
	std::vector<apollo::rdr::txt::Glyph> glyphs;
	std::vector<msdfgen::Shape> shapes;
	std::vector<uint32> indices;
	const glm::uvec2 atlasSize = generator.LoadGlyphRange(settings.m_Range, glyphs, shapes, indices);
	using Pixel = apollo::rdr::RGBAPixel<uint8>;
	auto buffer = std::make_unique<Pixel[]>(atlasSize.x * atlasSize.y);
	apollo::rdr::BitmapView bitmapView{ buffer.get(), atlasSize.x, atlasSize.y };
	generator.Rasterize(settings.m_PixelRange, glyphs, shapes, bitmapView, threadPool);

	const std::string pngPath = settings.m_BaseOutputPath.string() + ".png";

	const int ret = stbi_write_png(
		pngPath.c_str(),
		(int)atlasSize.x,
		(int)atlasSize.y,
		4,
		buffer.get(),
		bitmapView.GetByteStride());
	if (!ret)
		return 1;

	const nlohmann::json json{
		{ "fontFile", settings.m_InputPath },
		{ "textureFile", pngPath },
		{
			"range",
			{
				{ "first", settings.m_Range.m_First },
				{ "last", settings.m_Range.m_Last },
			},
		},
		{ "padding", settings.m_Padding },
		{ "pixelSize", settings.m_Resolution },
		{ "pixelRange", settings.m_PixelRange },
		{ "glyphs", glyphs },
	};
	atlasFile << json.dump(1, '\t');
	std::cout << "Packed " << glyphs.size() << " glyphs\n";
}