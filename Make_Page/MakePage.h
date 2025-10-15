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
	int totalpage = 0;
	// 结构体存储网页内容
	unordered_map<string, set<pair<int, double>>> inverseinde; // 词 -> （文件index，权重）
	std::vector<Item> pageLib;
	vector<string> cut(const string &line);
};

#endif // !MAKEPAGE_H
