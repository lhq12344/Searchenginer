#include "MakePage.h"

vector<string> MakePage::cut(const string &line)
{
	vector<string> words;
	jieba.Cut(line, words);
	return words;
}

const char *safe_get_text(XMLElement *elem)
{
	if (!elem)
	{
		return nullptr;
	}
	const char *text = elem->GetText();
	return text ? text : nullptr;
}

// 本地读取 pagelib 到局部容器，避免触碰成员 pageLib 以规避潜在的内存破坏
static bool ReadPageLibLocal(const std::string &pageLibPath, std::vector<Item> &outPages)
{
	tinyxml2::XMLDocument doc;
	XMLError err = doc.LoadFile(pageLibPath.c_str());
	if (err != XML_SUCCESS)
	{
		std::cerr << "[local] Load XML file failed: '" << pageLibPath << "' error=" << err << std::endl;
		return false;
	}

	XMLElement *root = doc.FirstChildElement("root");
	if (!root)
	{
		std::cerr << "[local] Missing <root> element" << std::endl;
		return false;
	}

	XMLElement *itemNode = root->FirstChildElement("doc");
	int processed = 0;
	while (itemNode)
	{
		XMLElement *docidElem = itemNode->FirstChildElement("docid");
		XMLElement *titleElem = itemNode->FirstChildElement("title");
		XMLElement *linkElem = itemNode->FirstChildElement("link");
		XMLElement *descriptionElem = itemNode->FirstChildElement("description");

		const char *docid_c = safe_get_text(docidElem);
		const char *title_c = safe_get_text(titleElem);
		const char *link_c = safe_get_text(linkElem);
		const char *description_c = safe_get_text(descriptionElem);
		if (!docid_c || !title_c || !link_c || !description_c)
		{
			itemNode = itemNode->NextSiblingElement("doc");
			continue;
		}

		Item page;
		page.docid = std::atoi(docid_c);
		page.title = title_c;
		page.link = link_c;
		page.description = description_c;
		outPages.push_back(std::move(page));
		processed++;

		// if (processed % 2000 == 0)
		// {
		// 	std::cout << "[local] parsed " << processed << " docs" << std::endl;
		// }

		itemNode = itemNode->NextSiblingElement("doc");
	}

	// std::cout << "[local] ReadPageLibLocal done. pages=" << outPages.size() << std::endl;
	return true;
}

// 读取并且清洗网页
void MakePage::makepage()
{
	// 确保输出目录存在
	std::error_code __ec2;
	std::filesystem::create_directories("page", __ec2);

	for (const auto &entry : std::filesystem::directory_iterator("./yuliao"))
	{
		ifstream file(entry.path());
		rss.read(entry.path().string());
	}
	rss.store("./page/pagelib.dat", "./page/text.dat", "./page/offset.dat");
	cout << "网页库和偏移库创建完毕！" << endl;
}

// 创建倒排索引
void MakePage::makeinverseindex()
{
	// 使用局部解析结果，避免成员容器潜在问题
	std::vector<Item> pages;
	if (!ReadPageLibLocal("./page/pagelib.dat", pages))
	{
		std::cerr << "makeinverseindex: failed to read pagelib locally." << std::endl;
		return;
	}
	int N = static_cast<int>(pages.size());
	std::cout << "[debug] makeinverseindex N=" << N << std::endl;
	if (N <= 0 || N > 10000000)
	{
		std::cerr << "makeinverseindex: abnormal totalpage=" << N << ", aborting to avoid OOM/crash." << std::endl;
		return;
	}

	// 记录文档中每个词出现的次数（每篇文档一个词频表）
	unordered_map<int, map<string, int>> wordCount;
	// 记录包含该词语的文档数（df）
	unordered_map<string, int> docFrequency;

	for (int i = 0; i < N; i++)
	{
		const int docid = pages[i].docid;
		const string &text = pages[i].description;
		if (text.empty())
		{
			continue;
		}
		vector<string> words = cut(text);
		if (words.empty())
		{
			continue;
		}
		std::unordered_set<string> seenInDoc;
		for (const auto &word : words)
		{
			// 跳过空词
			if (word.empty())
				continue;
			// 词频统计
			wordCount[i][word]++;
			// df 只在文档内首次出现时 +1
			if (seenInDoc.insert(word).second)
			{
				docFrequency[word]++;
				// 初始化倒排项
				inverseinde[word].insert({docid, 0.0});
			}
		}
	}

	// 计算 TF-IDF 并更新倒排索引
	for (int i = 0; i < N; i++)
	{
		if (wordCount.find(i) == wordCount.end())
		{
			continue; // 该文档可能为空或分词失败
		}
		const int docid = pages[i].docid;

		// 先计算每个词的 tf-idf
		unordered_map<string, double> tfidf;
		double norm = 0.0;
		for (const auto &kv : wordCount[i])
		{
			const string &word = kv.first;
			int tf = kv.second;
			auto itdf = docFrequency.find(word);
			int df = (itdf == docFrequency.end()) ? 0 : itdf->second;
			// idf = log2(N/(df+1))，确保分母>0
			double denom = static_cast<double>(df) + 1.0;
			double ratio = static_cast<double>(N) / denom;
			if (ratio <= 0.0)
			{
				continue; // 极端异常保护
			}
			double idf = log2(ratio);
			double weight = static_cast<double>(tf) * idf;
			tfidf[word] = weight;
			norm += weight * weight;
		}

		// 归一化，避免除零
		norm = sqrt(norm);
		if (norm <= 0.0)
		{
			// 全为0权重，保持初始化的0.0 即可
			continue;
		}

		for (const auto &kv : tfidf)
		{
			const string &word = kv.first;
			double weight = kv.second / norm;
			// 更新：先擦除初始的 {docid, 0.0}，再插入新权重
			inverseinde[word].erase({docid, 0.0});
			inverseinde[word].insert({docid, weight});
		}
	}

	// 保存倒排索引到文件
	ofstream ofs("page/inverseindex.dat");
	if (!ofs)
	{
		cerr << "无法打开文件 page/inverseindex.dat 进行写入。" << endl;
		return;
	}
	for (const auto &kv : inverseinde)
	{
		const string &word = kv.first;
		const auto &docWeights = kv.second;
		ofs << word << "\t";
		for (const auto &docWeight : docWeights)
		{
			ofs << docWeight.first << "  " << docWeight.second << " ";
			// printf("Word: %s, DocID: %d, Weight: %f\n", word.c_str(), docWeight.first, docWeight.second);
		}
		ofs << endl;
	}
	cout << "倒排索引创建完毕！" << endl;
}