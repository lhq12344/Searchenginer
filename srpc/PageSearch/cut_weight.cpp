#include "cut_weight.h"

vector<string> CutWeight::cut(const string &line)
{
	vector<string> words;
	jieba.Cut(line, words);
	return words;
}

vector<pair<string, double>> CutWeight::cut_weight(const string &str)
{
	vector<pair<string, double>> words_weight;
	// 记录句子中每个词出现的次数
	unordered_map<string, int> wordCount;
	vector<string> words = cut(str);
	for (const auto &word : words)
	{
		wordCount[word]++;
	}
	// 计算权重

	vector<double> weights; // 存储每个词的权重，顺序与 wordCount[i] 中的词顺序一致

	// 归一化权重
	for (const auto &kv : wordCount)
	{
		const string &word = kv.first;
		int tf = kv.second;
		int df = 1;
		double idf = log2(1 / (df + 1)); // 避免除零错误
		double weight = tf * idf;
		weights.push_back(weight);
		norm += weight * weight;
	}

	norm = sqrt(norm);
	// 更新权重
	for (const auto &kv : wordCount)
	{
		const string &word = kv.first;
		double weight = weights.front();
		weights.erase(weights.begin());
		weight /= norm; // 归一化
		words_weight.push_back({word, weight});
	}
	words_weight.clear();
	return words_weight;
}