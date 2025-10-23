#ifndef CUT_WEIGHT_H
#define CUT_WEIGHT_H

#include "../../include/cppjieba/Jieba.hpp"
#include "ReadPage.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <filesystem>

using namespace std;

class CutWeight
{
public:
	CutWeight() = default;
	~CutWeight() = default;
	vector<pair<string, double>> cut_weight(const string &str, int N, ReadPage &readPage);
	double get_norm() const { return norm; }

private:
	// jieba构造参数
	cppjieba::Jieba jieba{"../../include/dict/jieba.dict.utf8",
						  "../../include/dict/hmm_model.utf8",
						  "../../include/dict/user.dict.utf8",
						  "../../include/dict/idf.utf8",
						  "../../include/dict/stop_words.utf8"};
	double norm = 0.0; // 用于归一化
	vector<string> cut(const string &line);
};
#endif // !CUT_WEIGHT_H
