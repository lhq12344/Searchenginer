#include "ReadPage.h"
#include <mutex>
#include <shared_mutex>

bool ReadPage::LoadPageAndOffset()
{
	int ret = 0;
	// 独占写锁，保证加载/更新期间没有读取竞争
	unique_lock<std::shared_mutex> wlock(mtx);
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
	// Diagnostic: print counts
	cerr << "ReadPage: pageLib size=" << pageLib.size()
		 << " offsetLib size=" << offsetLib.size()
		 << " invertedIndex size=" << invertedIndex.size() << endl;
	return ret == 0;
}

// 读取网页库文件
bool ReadPage::ReadPageLib(const std::string &pageLibPath)
{
	// Read file into string so we can optionally wrap multiple top-level <doc> nodes
	std::ifstream ifs(pageLibPath);
	if (!ifs)
	{
		std::cerr << "ReadPage::ReadPageLib: open file '" << pageLibPath << "' fail" << std::endl;
		return false;
	}
	std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

	XMLDocument doc;
	XMLError err = doc.Parse(content.c_str());
	if (err != XML_SUCCESS)
	{
		// Try wrapping with a single root element in case the file contains multiple top-level <doc> entries
		std::string wrapped = std::string("<root>") + content + std::string("</root>");
		err = doc.Parse(wrapped.c_str());
		if (err != XML_SUCCESS)
		{
			std::cerr << "ReadPage::ReadPageLib: parse fail '" << pageLibPath << "' err=" << err
					  << " msg=" << (doc.ErrorStr() ? doc.ErrorStr() : "") << std::endl;
			// Fallback: try manual, forgiving extraction of <doc>...</doc> fragments without strict XML parsing
			size_t cur = 0;
			while (true)
			{
				size_t start = content.find("<doc", cur);
				if (start == std::string::npos)
					break;
				size_t startTagEnd = content.find('>', start);
				if (startTagEnd == std::string::npos)
					break;
				size_t end = content.find("</doc>", startTagEnd);
				if (end == std::string::npos)
					break;
				std::string fragment = content.substr(startTagEnd + 1, end - (startTagEnd + 1));

				auto extract_between = [&](const std::string &s, const std::string &a, const std::string &b) -> std::string
				{
					size_t pa = s.find(a);
					if (pa == std::string::npos)
						return std::string();
					pa += a.size();
					size_t pb = s.find(b, pa);
					if (pb == std::string::npos)
						return std::string();
					return s.substr(pa, pb - pa);
				};

				Item item;
				std::string docid_s = extract_between(fragment, "<docid>", "</docid>");
				if (!docid_s.empty())
				{
					try
					{
						item.docid = std::stoi(docid_s);
					}
					catch (...)
					{
						item.docid = 0;
					}
				}
				item.title = extract_between(fragment, "<title>", "</title>");
				item.link = extract_between(fragment, "<link>", "</link>");

				pageLib.push_back(item);
				cur = end + 6; // move past </doc>
			}

			if (!pageLib.empty())
				return true;
			return false;
		}
	}

	// Find the first <doc> element. If we wrapped, the document's first child will be <root>
	XMLElement *firstChild = doc.FirstChildElement();
	XMLElement *root = nullptr;
	if (firstChild && std::string(firstChild->Name()) == "root")
	{
		root = firstChild->FirstChildElement("doc");
	}
	else
	{
		root = doc.FirstChildElement("doc");
	}

	while (root)
	{
		Item item;
		// docid is stored as a child element in the data; be defensive when accessing elements
		XMLElement *docidElem = root->FirstChildElement("docid");
		if (docidElem && docidElem->GetText())
		{
			try
			{
				item.docid = std::stoi(docidElem->GetText());
			}
			catch (...)
			{
				item.docid = 0;
			}
		}
		else
		{
			item.docid = 0;
		}

		XMLElement *titleElem = root->FirstChildElement("title");
		item.title = (titleElem && titleElem->GetText()) ? titleElem->GetText() : std::string();

		XMLElement *linkElem = root->FirstChildElement("link");
		item.link = (linkElem && linkElem->GetText()) ? linkElem->GetText() : std::string();

		pageLib.push_back(item);
		root = root->NextSiblingElement("doc");
		// printf("item.docid: %d, title: %s, link: %s\n", item.docid, item.title.c_str(), item.link.c_str());
	}
	totalpage = pageLib.size();
	printf("Loaded %zu pages from %s\n", pageLib.size(), pageLibPath.c_str());
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
		// printf("start: %d, end: %d\n", offsetLib.back().first, offsetLib.back().second);
	}
	printf("Loaded %zu offsets from %s\n", offsetLib.size(), offsetLibPath.c_str());
	return true;
}

// 读取倒排索引文件
bool ReadPage::ReadInvertedIndex(const std::string &indexPath)
{
	std::ifstream ifs(indexPath);
	if (!ifs.is_open())
	{
		std::cerr << "无法打开文件: " << indexPath << std::endl;
		return false;
	}

	std::string line;
	int lineNum = 0;
	int wordCount = 0;

	while (std::getline(ifs, line))
	{
		++lineNum;
		if (line.empty())
			continue;

		// 去掉行尾回车符
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		std::istringstream iss(line);
		std::string word;
		if (!(iss >> word))
			continue; // 读取词语部分

		std::set<std::pair<int, double>> postings;
		int docid;
		double weight;

		// 后面交替读取 docid 和 weight
		while (iss >> docid >> weight)
		{
			postings.insert({docid, weight});
		}

		if (!postings.empty())
		{
			invertedIndex[word] = std::move(postings);
			++wordCount;
		}
		else
		{
			std::cerr << "第 " << lineNum << " 行 (" << word << ") 没有有效数据" << std::endl;
		}
	}

	std::cout << "成功载入倒排索引，共 " << wordCount
			  << " 个词，总计 " << invertedIndex.size() << " 个索引项。" << std::endl;

	// 调试输出前5个词
	int debugCount = 0;
	for (auto &kv : invertedIndex)
	{
		if (debugCount++ >= 5)
			break;
		std::cout << "Word: [" << kv.first << "] → ";
		for (auto &p : kv.second)
			std::cout << "(" << p.first << ", " << p.second << ") ";
		std::cout << std::endl;
	}

	return true;
}