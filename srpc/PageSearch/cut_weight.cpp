#include "cut_weight.h"

vector<string> CutWeight::cut(const string &line)
{
	vector<string> words;
	jieba.Cut(line, words);
	// printf("Cut words:");
	// for (const auto &word : words)
	// {
	// 	printf(" %s", word.c_str());
	// }
	// printf("\n");
	return words;
}

vector<pair<string, double>> CutWeight::cut_weight(const string &str, int N, ReadPage &readPage)
{
	vector<pair<string, double>> words_weight;
	unordered_map<string, int> wordCount;
	vector<string> words = cut(str);

	for (const auto &word : words)
		wordCount[word]++;

	double norm = 0.0;
	vector<double> weights;

	for (const auto &kv : wordCount)
	{
		const string &word = kv.first;
		int tf = kv.second;
		int df = 0;

		auto it = readPage.invertedIndex.find(word);
		if (it != readPage.invertedIndex.end())
		{
			df = it->second.size();
		}
		else
		{
			cout << "词 [" << word << "] 不在倒排索引中\n";
		}

		double idf = log2((double)(N + 1) / (df + 1));
		double weight = tf * idf;
		weights.push_back(weight);
		norm += weight * weight;

		cout << "Word: " << word
			 << ", TF: " << tf
			 << ", DF: " << df
			 << ", IDF: " << idf
			 << ", Weight: " << weight << endl;
	}

	norm = sqrt(norm);
	if (norm == 0)
		norm = 1; // 避免除 0

	size_t i = 0;
	for (const auto &kv : wordCount)
	{
		const string &word = kv.first;
		double weight = weights[i++] / norm;
		words_weight.emplace_back(word, weight);
	}

	return words_weight;
}
