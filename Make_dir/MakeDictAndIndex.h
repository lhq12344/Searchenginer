#ifndef MAIN_H
#define MAIN_H

#include "../include/cppjieba/Jieba.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
using namespace std;

static string ENfilepath[] = {
	{"yuliao/english.txt"},
	{"yuliao/The_Holy_Bible.txt"},
};

static string CNfilepath[] = {
	{"yuliao/art/C3-Art0002.txt"},
	{"yuliao/art/C3-Art0003.txt"},
	{"yuliao/art/C3-Art0005.txt"},
	{"yuliao/art/C3-Art0007.txt"},
	{"yuliao/art/C3-Art0009.txt"},
	{"yuliao/art/C3-Art00011.txt"},
	{"yuliao/art/C3-Art00013.txt"},
	{"yuliao/art/C3-Art00015.txt"},
	{"yuliao/art/C3-Art00017.txt"},
	{"yuliao/art/C3-Art00019.txt"},
	{"yuliao/art/C3-Art00021.txt"},
	{"yuliao/art/C3-Art00023.txt"},
	{"yuliao/art/C3-Art00025.txt"},
};

static string ENStopWordsfilepath[] = {
	{"yuliao/stop_words_eng.txt"}};

static string CNStopWordsfilepath[] = {
	{"yuliao/stop_words_zh.txt"}};

class MakeDictAndIndex
{
public:
	MakeDictAndIndex() = default;
	~MakeDictAndIndex() = default;

	// 切割函数
	vector<string> ENcut(const string &line);
	vector<string> CNcut(const string &line);

	void makeENdict();
	void makeCNdict();
	void makeENindex();
	void makeCNindex();

private:
	// jieba构造参数
	cppjieba::Jieba jieba{"../include/dict/jieba.dict.utf8",
						  "../include/dict/hmm_model.utf8",
						  "../include/dict/user.dict.utf8",
						  "../include/dict/idf.utf8",
						  "../include/dict/stop_words.utf8"};

	vector<string> ENstopword;
	vector<string> CNstopword;
	map<string, int> ENdict;						 // 英文词典：词 -> 频次
	map<string, int> CNdict;						 // 中文词典：词 -> 频次
	vector<pair<string, int>> ENdictread;			 // 英文词典
	vector<pair<string, int>> CNdictread;			 // 中文词典
	unordered_map<string, set<int>> ENinvertedIndex; // 倒排索引
	unordered_map<string, set<int>> CNinvertedIndex; // 倒排索引
};
#endif // MAIN_HCN