#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
using namespace std;

// UTF-8 aware helpers ------------------------------------------------------
// Split a UTF-8 encoded std::string into a vector of UTF-8 "characters" (each element
// is a substring containing one UTF-8 codepoint). This keeps multi-byte CJK characters
// intact so edit distance works on characters rather than raw bytes.
static inline std::vector<std::string> utf8_split(const std::string &s)
{
	std::vector<std::string> out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size();)
	{
		unsigned char c = static_cast<unsigned char>(s[i]);
		size_t len = 1;
		if ((c & 0x80) == 0)
			len = 1;
		else if ((c & 0xE0) == 0xC0)
			len = 2;
		else if ((c & 0xF0) == 0xE0)
			len = 3;
		else if ((c & 0xF8) == 0xF0)
			len = 4;
		else
			len = 1; // fallback for malformed

		if (i + len > s.size())
			len = s.size() - i; // clamp malformed tail

		out.emplace_back(s.substr(i, len));
		i += len;
	}
	return out;
}

static inline int editDistanceChars(const std::vector<std::string> &a, const std::vector<std::string> &b)
{
	int m = (int)a.size();
	int n = (int)b.size();
	vector<vector<int>> dp(m + 1, vector<int>(n + 1));
	for (int i = 0; i <= m; ++i)
		dp[i][0] = i;
	for (int j = 0; j <= n; ++j)
		dp[0][j] = j;
	for (int i = 1; i <= m; ++i)
	{
		for (int j = 1; j <= n; ++j)
		{
			if (a[i - 1] == b[j - 1])
				dp[i][j] = dp[i - 1][j - 1];
			else
				dp[i][j] = std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]}) + 1;
		}
	}
	return dp[m][n];
}

static inline double calculateSimilarity(const string &word1, const string &word2)
{
	auto v1 = utf8_split(word1);
	auto v2 = utf8_split(word2);
	int distance = editDistanceChars(v1, v2);
	int maxLen = std::max((int)v1.size(), (int)v2.size());
	if (maxLen == 0)
		return 1.0;
	return 1.0 - (double)distance / (double)maxLen;
}

class SimilarityComparator
{
private:
	string targetWord;

public:
	SimilarityComparator(const string &target) : targetWord(target) {}

	bool operator()(const string &a, const string &b) const
	{
		// 先计算相似度
		double simA = calculateSimilarity(a, targetWord);
		double simB = calculateSimilarity(b, targetWord);

		// 使用字典序作为最终裁决，确保严格弱序
		if (std::abs(simA - simB) < 1e-9)
		{
			// 相似度非常接近，视为相等，使用字典序
			return a < b;
		}

		// 相似度明显不同
		return simA > simB; // 降序：相似度高的排在前面
	}
};