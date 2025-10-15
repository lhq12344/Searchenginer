#include "../include/Utils.h"
#include <sstream>
#include <cctype>

std::string urlDecode(const std::string &encoded)
{
	std::string decoded;
	for (size_t i = 0; i < encoded.size(); ++i)
	{
		if (encoded[i] == '%' && i + 2 < encoded.size())
		{
			char hex1 = encoded[i + 1];
			char hex2 = encoded[i + 2];
			int val = 0;
			if (isxdigit(static_cast<unsigned char>(hex1)) && isxdigit(static_cast<unsigned char>(hex2)))
			{
				std::stringstream ss;
				ss << std::hex << std::string(1, hex1) << std::string(1, hex2);
				ss >> val;
				decoded += static_cast<char>(val);
				i += 2;
				continue;
			}
		}
		decoded += encoded[i];
	}
	return decoded;
}
