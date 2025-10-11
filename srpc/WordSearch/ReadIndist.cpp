#include "ReadIndist.h"
#include <sstream>

bool ReadIndist::LoadDictAndIndex()
{
	int ret = 0;
	// 读取词典和索引文件
	if (!ReadDict(DICT_FILE))
	{
		cerr << "Failed to read English dictionary." << endl;
		ret = -1;
	}

	if (!ReadInvertedIndex(INDEX_FILE))
	{
		cerr << "Failed to read English inverted index." << endl;
		ret = -1;
	}

	return ret == 0;
}
// 读取词典文件
bool ReadIndist::ReadDict(const std::vector<std::string> &dictPaths)
{
	for (const auto &path : dictPaths)
	{
		ifstream dictFile(path);
		if (!dictFile.is_open())
		{
			cerr << "[Error] 打开词典文件失败: " << path << " (请确认文件存在且有读权限)\n";
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
				dict.emplace_back(word, freq);
			}
		}
	}
	return true;
}

// 读取倒排索引文件
bool ReadIndist::ReadInvertedIndex(const std::vector<std::string> &indexPaths)
{
	for (const auto &path : indexPaths)
	{
		ifstream dictFile(path);
		if (!dictFile.is_open())
		{
			cerr << "[Error] 打开倒排索引文件失败: " << path << " (请确认文件存在且有读权限)\n";
			return false;
		}
		string line;
		while (getline(dictFile, line))
		{
			auto pos = line.find(' ');
			if (pos != string::npos)
			{
				string word = line.substr(0, pos);
				string rest = line.substr(pos + 1);
				istringstream iss(rest);
				int docid;
				while (iss >> docid)
				{
					invertedIndex[word].insert(docid);
				}
			}
		}
	}
	return true;
}
