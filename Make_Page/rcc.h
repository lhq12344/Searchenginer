#include "../include/tinyxml2.h"
#include "./include/simhash/Simhasher.hpp"
#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <fstream>

using std::cout;
using std::endl;
using std::ofstream;
using std::string;
using std::vector;

using namespace tinyxml2;
using namespace simhash;

struct RSSIteam
{
	string _title;
	string _link;
	string _description;
	string _content;
};

class RSS
{
public:
	RSS(size_t capa);

	// 读xml文件并且清洗
	void read(const string &filename);

	// 储存
	void store(const string &filename, const string &storeFile, const string &offsetFile);

private:
	Simhasher simhasher{"./include/simhash/dict/jieba.dict.utf8",
						"./include/simhash/dict/hmm_model.utf8",
						"./include/simhash/dict/idf.utf8",
						"./include/simhash/dict/stop_words.utf8"};
	size_t idex = 0;
	size_t start = 0;
	vector<RSSIteam> _rss;
	unordered_map<int, uint64_t> simhashIndex; //  文档 ID 列表 -> simhash 值
};
