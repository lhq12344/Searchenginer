#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
using namespace std;

int editDistance(const string &word1, const string &word2)
{
	int m = word1.length();
	int n = word2.length();
	vector<vector<int>> dp(m + 1, vector<int>(n + 1));

	for (int i = 0; i <= m; i++)
		dp[i][0] = i;
	for (int j = 0; j <= n; j++)
		dp[0][j] = j;

	for (int i = 1; i <= m; i++)
	{
		for (int j = 1; j <= n; j++)
		{
			if (word1[i - 1] == word2[j - 1])
			{
				dp[i][j] = dp[i - 1][j - 1];
			}
			else
			{
				dp[i][j] = min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]}) + 1;
			}
		}
	}

	return dp[m][n];
}

double calculateSimilarity(const string &word1, const string &word2)
{
	int distance = editDistance(word1, word2);
	int maxLen = max(word1.length(), word2.length());

	if (maxLen == 0)
		return 1.0;

	return 1.0 - (double)distance / maxLen;
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