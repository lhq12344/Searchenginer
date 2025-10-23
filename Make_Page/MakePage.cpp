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

// 创建训练文本文件
void MakePage::makefasttextfile()
{
	// 读取文本库text.dat
	std::ifstream ifs("page/text.dat");
	if (!ifs)
	{
		std::cerr << "makefasttextfile: failed to open text.dat" << std::endl;
		return;
	}
	std::ofstream ofs("page/fasttext.txt");
	if (!ofs)
	{
		std::cerr << "makefasttextfile: failed to open fasttext.txt" << std::endl;
		return;
	}
	std::string line;
	while (std::getline(ifs, line))
	{
		// 处理每一行，进行分词等操作
		std::vector<std::string> words = cut(line);
		for (const auto &word : words)
		{
			ofs << word << " ";
		}
		ofs << std::endl;
	}
}

void MakePage::makefasttextmodel()
{
	// 1) 加载 fastText 预训练词向量模型（可换为你的实际路径或通过环境变量覆盖）
	fasttext::FastText model;
	model.loadModel("./page/small_model.bin");
	// 2) 读取 pagelib，准备导出文档向量
	std::vector<Item> pages;
	if (!ReadPageLibLocal("./page/pagelib.dat", pages))
	{
		std::cerr << "makefasttextmodel: failed to read pagelib locally." << std::endl;
		return;
	}
	int N = static_cast<int>(pages.size());

	//  维度：若使用 fastText 则取模型维度；否则取环境变量 VEC_DIM（默认 2048）
	int32_t dim = static_cast<int32_t>(model.getDimension());
	std::cout << "[fasttext] :" << " docs=" << N << " dim=" << dim << std::endl;
	if (N <= 0 || N > 10000000)
	{
		std::cerr << "makefasttextmodel: abnormal totalpage=" << N << ", abort." << std::endl;
		return;
	}
	// 3) 输出二进制向量文件：VEC1 + dim + nvec + (docid, vector[dim])*nvec
	std::error_code ec;
	std::filesystem::create_directories("page", ec);
	std::ofstream ofs("page/vectors.dat", std::ios::binary);
	if (!ofs)
	{
		std::cerr << "makefasttextmodel: cannot open page/vectors.dat for write" << std::endl;
		return;
	}
	// 写头
	const char magic[4] = {'V', 'E', 'C', '1'};
	ofs.write(magic, 4);
	int32_t nvec = static_cast<int32_t>(N);
	// 向量数量
	ofs.write(reinterpret_cast<const char *>(&dim), sizeof(int32_t));
	ofs.write(reinterpret_cast<const char *>(&nvec), sizeof(int32_t));
	// 4) 为每个文档构建向量：使用倒排索引中的 TF-IDF 权重对词向量加权 -> L2 归一化
	// 读取倒排索引文件 page/inverseindex.dat，格式为：word \t docid1 weight1 docid2 weight2 ...
	unordered_map<int, vector<pair<string, double>>> docWeights; // docid -> list of (word, weight)
	{
		std::ifstream invifs("page/inverseindex.dat");
		if (invifs)
		{
			string line;
			while (std::getline(invifs, line))
			{
				if (line.empty())
					continue;
				// 切分：词语 在第一个 \t 之前，后面是若干 "docid weight"
				size_t tab = line.find('\t');
				if (tab == string::npos)
					continue;
				string word = line.substr(0, tab);
				string rest = line.substr(tab + 1);
				std::istringstream iss(rest);
				int docid;
				double weight;
				while (iss >> docid >> weight)
				{
					docWeights[docid].push_back({word, weight});
				}
			}
		}
		else
		{
			std::cerr << "makefasttextmodel: cannot open page/inverseindex.dat, will fallback to simple averaging" << std::endl;
		}
	}

	fasttext::Vector wv(dim);
	auto l2_normalize = [](std::vector<float> &v)
	{
		double s = 0.0;
		for (float x : v)
			s += double(x) * double(x);
		if (s <= 0)
			return;
		float inv = 1.0f / static_cast<float>(std::sqrt(s));
		for (auto &x : v)
			x *= inv;
	};

	std::vector<float> accum(dim);
	for (const auto &item : pages)
	{
		std::fill(accum.begin(), accum.end(), 0.f);
		int docid = item.docid;
		bool usedWeights = false;
		auto it = docWeights.find(docid);
		if (it != docWeights.end() && !it->second.empty())
		{
			usedWeights = true;
			double totalWeight = 0.0;
			for (const auto &p : it->second)
				totalWeight += p.second > 0 ? p.second : 0.0;
			if (totalWeight <= 0.0)
				totalWeight = 1.0; // 防止除以零

			for (const auto &p : it->second)
			{
				const string &w = p.first;
				double wt = p.second;
				if (wt <= 0)
					continue;
				model.getWordVector(wv, w);
				for (int i = 0; i < dim; ++i)
					accum[i] += static_cast<float>(wv[i] * (wt / totalWeight));
			}
		}
		else
		{
			// fallback: simple average of word vectors (原有逻辑)
			vector<string> words = cut(item.description);
			int used = 0;
			for (const auto &w : words)
			{
				if (w.empty())
					continue;
				model.getWordVector(wv, w);
				for (int i = 0; i < dim; ++i)
					accum[i] += wv[i];
				++used;
			}
			if (used > 0)
			{
				float inv = 1.0f / static_cast<float>(used);
				for (auto &x : accum)
					x *= inv; // 平均
			}
			if (used == 0)
			{
				std::istringstream iss(item.description);
				wv.zero();
				model.getSentenceVector(iss, wv);
				for (int i = 0; i < dim; ++i)
					accum[i] = wv[i];
			}
		}

		// L2 归一化
		l2_normalize(accum);

		// 写入：docid + 向量
		int32_t docid32 = static_cast<int32_t>(item.docid);
		ofs.write(reinterpret_cast<const char *>(&docid32), sizeof(int32_t));
		ofs.write(reinterpret_cast<const char *>(accum.data()), sizeof(float) * dim);
	}
	ofs.flush();
	std::cout << "[fasttext] vectors exported -> page/vectors.dat" << std::endl;
}
