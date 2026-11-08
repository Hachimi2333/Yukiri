#include <Yukiri/Styling/Theme.hpp>

namespace Yukiri
{
Theme Theme::Light() noexcept
{
	return {Color::FromHex(0xF5F6F8), Color::FromHex(0xFFFFFF), Color::FromHex(0xEDF1F6),
		Color::FromHex(0xDFE6F0), Color::FromHex(0x202632), Color::FromHex(0x6B7484),
		Color::FromHex(0xDCE1E9), Color::FromHex(0x3674EA), Color::FromHex(0x2664D7),
		Color::FromHex(0xFFFFFF), Color::FromHex(0xCADCFF)};
}

Theme Theme::Dark() noexcept
{
	return {Color::FromHex(0x151820), Color::FromHex(0x1E232D), Color::FromHex(0x2B3341),
		Color::FromHex(0x364153), Color::FromHex(0xE9EDF5), Color::FromHex(0x9AA6BA),
		Color::FromHex(0x394352), Color::FromHex(0x719FFF), Color::FromHex(0x8AB0FF),
		Color::FromHex(0x101A2D), Color::FromHex(0x344F7B)};
}
}
