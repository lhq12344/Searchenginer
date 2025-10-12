#ifndef READINDIST_H
#define READINDIST_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
using namespace std;
using std::ifstream;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

static const std::string EN_DICT_FILE = "../../Make_dir/dict/en_dict.txt";

static const std::string CN_DICT_FILE = "../../Make_dir/dict/cn_dict.txt";

static const std::string EN_INDEX_FILE = "../../Make_dir/dict/en_index.txt";

static const std::string CN_INDEX_FILE = "../../Make_dir/dict/cn_index.txt";

class ReadIndist
{
public:
	ReadIndist() = default;
	~ReadIndist() = default;

	bool LoadDictAndIndex();
	vector<pair<string, int>> ENdict;				 // 英文词典
	vector<pair<string, int>> CNdict;				 // 中文词典
	unordered_map<string, set<int>> ENinvertedIndex; // 英文索引
	unordered_map<string, set<int>> CNinvertedIndex; // 中文索引

private:
	// 读取词典文件
	bool ENReadDict(const std::string &dictPaths);
	bool CNReadDict(const std::string &dictPaths);
	// 读取倒排索引文件
	bool ENReadInvertedIndex(const std::string &indexPaths);
	bool CNReadInvertedIndex(const std::string &indexPaths);
};

#endif // !READINDIST_H