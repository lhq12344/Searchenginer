#ifndef MAKEPAGE_H
#define MAKEPAGE_H

#include "../include/cppjieba/Jieba.hpp"
#include "../include/tinyxml2.h"
#include "rcc.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <filesystem>
#if __has_include("fasttext.h")
#include "fasttext.h"
#elif __has_include(<fasttext/fasttext.h>)
#include <fasttext/fasttext.h>
#else
#error "fastText headers not found. Please set FASTTEXT_INC or FASTTEXT_ROOT include paths."
#endif

using namespace std;

struct Item
{
	int docid;
	string title;
	string link;
	string description;
	// 显式构造函数确保初始化
	Item() : docid(0), title(""), link(""), description("") {}
};

class MakePage
{
public:
	MakePage() : totalpage(0)
	{
		// 确保 vector 是空的
		pageLib.clear();
	}
	~MakePage()
	{
		// 按正确顺序清理
		pageLib.clear();
		totalpage = 0;
	}

	void makepage();		  // 创建网页库和偏移库
	void makeinverseindex();  // 创建倒排索引
	void makefasttextmodel(); // 创建 fasttext 模型
	void makefasttextfile();  // 创建 fasttext 训练文本文件
private:
	// rcc构造参数
	RSS rss{4000};
	// jieba构造参数
	cppjieba::Jieba jieba{"../include/dict/jieba.dict.utf8",
						  "../include/dict/hmm_model.utf8",
						  "../include/dict/user.dict.utf8",
						  "../include/dict/idf.utf8",
						  "../include/dict/stop_words.utf8"};
	int totalpage = 0;
	// 结构体存储网页内容
	unordered_map<string, set<pair<int, double>>> inverseinde; // 词 -> （文件index，权重）
	std::vector<Item> pageLib;
	vector<string> cut(const string &line);
};

#endif // !MAKEPAGE_H
