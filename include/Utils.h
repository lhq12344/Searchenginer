#pragma once
#include <string>

// Decode percent-encoded URL string, e.g. "a%20b" -> "a b"
std::string urlDecode(const std::string &encoded);
