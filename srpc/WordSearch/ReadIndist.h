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

static const std::vector<std::string> DICT_FILE = {
	"../../Make_dir/dict/en_dict.txt",
	"../../Make_dir/dict/cn_dict.txt"};

static const std::vector<std::string> INDEX_FILE = {
	"../../Make_dir/dict/en_index.txt",
	"../../Make_dir/dict/cn_index.txt"};

class ReadIndist
{
public:
	ReadIndist() = default;
	~ReadIndist() = default;

	bool LoadDictAndIndex();
	vector<pair<string, int>> dict;				   // 词典
	unordered_map<string, set<int>> invertedIndex; // 索引

private:
	// 读取词典文件
	bool ReadDict(const std::vector<std::string> &dictPaths);
	// 读取倒排索引文件
	bool ReadInvertedIndex(const std::vector<std::string> &indexPaths);
};

#endif // !READINDIST_H