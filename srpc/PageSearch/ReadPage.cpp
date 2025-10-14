#include "ReadPage.h"

bool ReadPage::LoadPageAndOffset()
{
	int ret = 0;
	// 读取网页库文件
	if (!ReadPageLib(PAGE_LIB_FILE))
	{
		cerr << "Failed to read page library." << endl;
		ret = -1;
	}
	// 读取偏移库文件
	if (!ReadOffsetLib(OFFSET_LIB_FILE))
	{
		cerr << "Failed to read offset library." << endl;
		ret = -1;
	}
	// 读取倒排索引文件
	if (!ReadInvertedIndex(INVERTED_INDEX_FILE))
	{
		cerr << "Failed to read inverted index." << endl;
		ret = -1;
	}
	return ret == 0;
}

// 读取网页库文件
bool ReadPage::ReadPageLib(const std::string &pageLibPath)
{
	XMLDocument doc;
	XMLError err = doc.LoadFile(pageLibPath.c_str());
	if (err != XML_SUCCESS)
	{
		std::cerr << "ReadPage::ReadPageLib: loadFile fail '" << pageLibPath << "' err=" << err << endl;
		return false;
	}
	XMLElement *root = doc.FirstChildElement("doc");
	while (root)
	{
		Item item;
		item.docid = root->IntAttribute("docid");
		item.title = root->FirstChildElement("title")->GetText();
		item.link = root->FirstChildElement("link")->GetText();
		pageLib.push_back(item);
		root = root->NextSiblingElement("doc");
	}
	return true;
}

// 读取偏移库文件
bool ReadPage::ReadOffsetLib(const std::string &offsetLibPath)
{
	std::ifstream ifs(offsetLibPath);
	if (!ifs)
	{
		std::cerr << "open " << offsetLibPath << " fail!" << endl;
		return false;
	}
	string line;
	while (getline(ifs, line))
	{
		auto pos = line.find(' ');
		if (pos != string::npos)
		{
			int start = stoi(line.substr(0, pos));
			int end = stoi(line.substr(pos + 1));
			offsetLib.push_back(make_pair(start, end));
		}
	}
}
// 读取倒排索引文件
bool ReadPage::ReadInvertedIndex(const std::string &indexPath)
{
	std::ifstream ifs(indexPath);
	if (!ifs)
	{
		std::cerr << "open " << indexPath << " fail!" << endl;
		return false;
	}
	string line;
	while (getline(ifs, line))
	{
		auto pos = line.find(' ');
		if (pos != string::npos)
		{
			string word = line.substr(0, pos);
			string rest = line.substr(pos + 1);
			istringstream iss(rest);
			int docid;
			double weight;
			while (iss >> docid >> weight)
			{
				invertedIndex[word].insert(make_pair(docid, weight));
			}
		}
	}
	return true;
}