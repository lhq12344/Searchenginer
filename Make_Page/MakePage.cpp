#include "MakePage.h"

vector<string> MakePage::cut(const string &line)
{
	vector<string> words;
	jieba.Cut(line, words);
	return words;
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

void MakePage::makeinverseindex()
{
	int N = rss.idex; // 记录总的文档数
	// 记录文档中每个词出现的次数
	unordered_map<int, map<string, int>> wordCount;
	// 记录包含该词语的文档数
	unordered_map<string, int> docFrequency;
	for (int i = 0; i < N; i++)
	{
		vector<string> words = cut(rss._rss[i]._description);
		for (const auto &word : words)
		{
			if (wordCount[i].find(word) == wordCount[i].end()) // 第一次插入
			{
				wordCount[i][word] = 1;
				docFrequency[word]++;
				inverseinde[word].insert({i + 1, 0.0}); // 初始化权重为0.0
			}
			else
			{
				wordCount[i][word]++;
			}
		}
	}

	// 计算权重并存储倒排索引
	for (int i = 0; i < N; i++)
	{
		double norm = 0.0;		// 用于归一化
		vector<double> weights; // 存储每个词的权重，顺序与 wordCount[i] 中的词顺序一致

		// 归一化权重
		for (const auto &kv : wordCount[i])
		{
			const string &word = kv.first;
			int tf = kv.second;
			int df = docFrequency[word];
			double idf = log2((double)N / (df + 1)); // 避免除零错误
			double weight = tf * idf;
			weights.push_back(weight);
			norm += weight * weight;
		}
		norm = sqrt(norm);
		// 更新权重
		for (const auto &kv : wordCount[i])
		{
			const string &word = kv.first;
			double weight = weights.front();
			weights.erase(weights.begin());
			weight /= norm; // 归一化
			inverseinde[word].erase({i + 1, 0.0});
			inverseinde[word].insert({i + 1, weight});
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
			ofs << "(" << docWeight.first << ", " << docWeight.second << ") ";
		}
		ofs << endl;
	}
	cout << "倒排索引创建完毕！" << endl;
}