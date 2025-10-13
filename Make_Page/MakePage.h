#ifndef MAKEPAGE_H
#define MAKEPAGE_H

#include "../include/cppjieba/Jieba.hpp"
#include "rcc.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <regex>
#include <filesystem>
#include <sstream>
#include <cctype>
using namespace std;

class MakePage
{
public:
	MakePage(/* args */) = default;
	~MakePage() = default;

	void makepage();		 // 创建网页库和偏移库
	void makeinverseindex(); // 创建倒排索引

private:
	// rcc构造参数
	RSS rss{4000};
	// jieba构造参数
	cppjieba::Jieba jieba{"../include/dict/jieba.dict.utf8",
						  "../include/dict/hmm_model.utf8",
						  "../include/dict/user.dict.utf8",
						  "../include/dict/idf.utf8",
						  "../include/dict/stop_words.utf8"};

	// 结构体存储网页内容
	unordered_map<string, set<pair<int, double>>> inverseinde; // 词 -> （文件index，权重）

	vector<string> cut(const string &line);
};

#endif // !MAKEPAGE_H
