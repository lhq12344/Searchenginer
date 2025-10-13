#include "json_utils.h"
#include <string>

// 在一段字符串中提取出第一个完整的 JSON 对象或数组（{...} 或 [...]）。
std::string extract_first_json(const std::string &s)
{
	size_t p = s.find_first_of("{[");
	if (p == std::string::npos)
		return {};

	char open = s[p];
	char close = (open == '{') ? '}' : ']';
	int depth = 0;
	bool in_str = false;

	for (size_t i = p; i < s.size(); ++i)
	{
		char c = s[i];
		if (c == '\\')
		{
			// skip escaped char inside string
			++i;
			continue;
		}
		if (c == '"')
		{
			in_str = !in_str;
			continue;
		}
		if (in_str)
			continue;
		if (c == open)
			++depth;
		else if (c == close)
		{
			--depth;
			if (depth == 0)
			{
				return s.substr(p, i - p + 1);
			}
		}
	}

	// 未找到闭合，返回从首个 JSON 起点到末尾的内容（尽量尝试）
	return s.substr(p);
}