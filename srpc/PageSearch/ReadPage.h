#ifndef READPAGE_H
#define READPAGE_H

#include "../../include/tinyxml2.h"
#include <fstream>
#include <regex>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <shared_mutex>
#include <faiss/IndexFlat.h>
#include <fasttext/fasttext.h>
using namespace std;
using namespace tinyxml2;
// using namespace simhash;
struct Item
{
	int docid;
	string title;
	string link;
};

static const std::string PAGE_LIB_FILE = "../../Make_Page/page/pagelib.dat";
static const std::string OFFSET_LIB_FILE = "../../Make_Page/page/offset.dat";
static const std::string INVERTED_INDEX_FILE = "../../Make_Page/page/inverseindex.dat";
static const std::string FASTTEXT_LIB_FILE = "../../Make_Page/page/vectors.dat";

struct VecEntry
{
	int32_t docid;
	std::vector<float> vec;
};

class ReadPage
{
public:
	ReadPage() = default;
	~ReadPage() = default;

	// 加载网页库、偏移库、倒排索引到内存
	bool LoadPageAndOffset();
	int totalpage = 0;
	// 加载 fastText 向量
	bool LoadVectors(const std::string &path, std::vector<VecEntry> &entries, int &dim);
	// 网页库
	std::vector<Item> pageLib;

	// 偏移库
	std::vector<pair<int, int>> offsetLib; // (start, end)

	// 倒排索引
	std::unordered_map<std::string, std::set<pair<int, double>>> invertedIndex;

	// 读写锁：Load 时写入，运行时并发读取使用 shared_lock
	mutable std::shared_mutex mtx;

	// fasttext计算关键词的向量
	fasttext::FastText model;

	faiss::Index *index = nullptr;

private:
	// 读取网页库文件
	bool ReadPageLib(const std::string &pageLibPath);
	// 读取偏移库文件
	bool ReadOffsetLib(const std::string &offsetLibPath);
	// 读取倒排索引文件
	bool ReadInvertedIndex(const std::string &indexPath);
};

#endif // !READPAGE_H
