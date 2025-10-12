#include "ReadIndist.h"
#include <sstream>

bool ReadIndist::LoadDictAndIndex()
{
	int ret = 0;
	// 读取词典和索引文件
	if (!ENReadDict(EN_DICT_FILE))
	{
		cerr << "Failed to read English dictionary." << endl;
		ret = -1;
	}

	if (!CNReadDict(CN_DICT_FILE))
	{
		cerr << "Failed to read Chinese dictionary." << endl;
		ret = -1;
	}

	if (!ENReadInvertedIndex(EN_INDEX_FILE))
	{
		cerr << "Failed to read English inverted index." << endl;
		ret = -1;
	}

	if (!CNReadInvertedIndex(CN_INDEX_FILE))
	{
		cerr << "Failed to read Chinese inverted index." << endl;
		ret = -1;
	}

	return ret == 0;
}
// 读取词典文件
bool ReadIndist::ENReadDict(const std::string &dictPaths)
{

	printf("Loading dictionary from: %s\n", dictPaths.c_str());
	ifstream dictFile(dictPaths);
	if (!dictFile.is_open())
	{
		cerr << "[Error] 打开词典文件失败: " << dictPaths << " (请确认文件存在且有读权限)\n";
		return false;
	}
	string line;
	while (getline(dictFile, line))
	{
		auto pos = line.find(' ');
		if (pos != string::npos)
		{
			string word = line.substr(0, pos);
			int freq = stoi(line.substr(pos + 1));
			ENdict.emplace_back(word, freq);
		}
	}

	return true;
}

bool ReadIndist::CNReadDict(const std::string &dictPaths)
{

	printf("Loading dictionary from: %s\n", dictPaths.c_str());
	ifstream dictFile(dictPaths);
	if (!dictFile.is_open())
	{
		cerr << "[Error] 打开词典文件失败: " << dictPaths << " (请确认文件存在且有读权限)\n";
		return false;
	}
	string line;
	while (getline(dictFile, line))
	{
		auto pos = line.find(' ');
		if (pos != string::npos)
		{
			string word = line.substr(0, pos);
			int freq = stoi(line.substr(pos + 1));
			CNdict.emplace_back(word, freq);
		}
	}

	return true;
}
// 读取倒排索引文件
bool ReadIndist::ENReadInvertedIndex(const std::string &indexPaths)
{

	printf("Loading inverted index from: %s\n", indexPaths.c_str());
	ifstream dictFile(indexPaths);
	if (!dictFile.is_open())
	{
		cerr << "[Error] 打开倒排索引文件失败: " << indexPaths << " (请确认文件存在且有读权限)\n";
		return false;
	}
	string line;
	while (getline(dictFile, line))
	{
		auto pos = line.find(' ');
		if (pos != string::npos)
		{
			string word = line.substr(0, pos);
			// printf("Indexing word: %s\n", word.c_str());
			string rest = line.substr(pos + 1);
			istringstream iss(rest);
			int docid;
			while (iss >> docid)
			{
				ENinvertedIndex[word].insert(docid);
			}
		}
	}

	return true;
}

bool ReadIndist::CNReadInvertedIndex(const std::string &indexPaths)
{

	printf("Loading inverted index from: %s\n", indexPaths.c_str());
	ifstream dictFile(indexPaths);
	if (!dictFile.is_open())
	{
		cerr << "[Error] 打开倒排索引文件失败: " << indexPaths << " (请确认文件存在且有读权限)\n";
		return false;
	}
	string line;
	while (getline(dictFile, line))
	{
		auto pos = line.find(' ');
		if (pos != string::npos)
		{
			string word = line.substr(0, pos);
			// printf("Indexing word: %s\n", word.c_str());
			string rest = line.substr(pos + 1);
			istringstream iss(rest);
			int docid;
			while (iss >> docid)
			{
				CNinvertedIndex[word].insert(docid);
			}
		}
	}

	return true;
}
