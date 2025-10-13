#include "MakeDictAndIndex.h"
#include <filesystem>
#include <unordered_set>

/*左修剪（去除开头空格）*/
static inline std::string ltrim_copy(std::string s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
									{ return !std::isspace(ch); }));
	return s;
}
/*右修剪（去除结尾空格）*/
static inline std::string rtrim_copy(std::string s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
						 { return !std::isspace(ch); })
				.base(),
			s.end());
	return s;
}
/*去除两端空格*/
static inline std::string trim_copy(std::string s)
{
	return rtrim_copy(ltrim_copy(std::move(s)));
}
/*去BOM头 BOM是文件开头的特殊标记 EF BB BF，某些编辑器会自动添加*/
static inline void strip_bom(std::string &s)
{
	if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF)
		s.erase(0, 3);
}
/* 转换为小写 */
static inline std::string tolower_ascii(std::string s)
{
	for (char &c : s)
		c = (char)std::tolower((unsigned char)c);
	return s;
}

vector<string> MakeDictAndIndex::ENcut(const string &line)
{
	vector<string> words;
	string word;
	for (char ch : line)
	{
		if (isalpha(ch))
		{
			word += tolower(ch);
		}
		else
		{
			if (!word.empty())
			{
				words.push_back(word);
				word.clear();
			}
		}
	}
	if (!word.empty())
	{
		words.push_back(word);
	}
	return words;
}

vector<string> MakeDictAndIndex::CNcut(const string &line)
{
	vector<string> words;
	jieba.Cut(line, words);
	return words;
}

void MakeDictAndIndex::makeENdict()
{
	// 确保输出目录存在
	std::error_code __ec1; //// 用于接收错误码
	std::filesystem::create_directories("dict", __ec1);

	// 读取英文停用词（trim + 去BOM + 小写归一化）
	std::unordered_set<std::string> en_stopset;
	ifstream stopWordsFile(ENStopWordsfilepath[0]);
	string stopword;
	bool firstLine = true;
	while (getline(stopWordsFile, stopword))
	{
		if (firstLine)
		{
			strip_bom(stopword);
			firstLine = false;
		}
		auto t = trim_copy(stopword);
		if (!t.empty())
			en_stopset.insert(tolower_ascii(t));
	}
	//  读取英文语料库
	for (const auto &filepath : ENfilepath)
	{
		ifstream file(filepath);
		string line;
		while (getline(file, line))
		{
			// 分词并加入字典
			vector<string> words = ENcut(line);
			for (const auto &word : words)
			{
				// 统计词频
				ENdict[word]++;
			}
		}
	}
	// 生成英文词典
	ofstream dictFile("dict/en_dict.txt");
	if (!dictFile.is_open())
	{
		cerr << "[Error] 打开输出文件失败: dict/en_dict.txt (请确认有写权限)\n";
		return;
	}
	for (const auto &kv : ENdict)
	{
		// 输出前过滤停用词
		if (!en_stopset.count(kv.first))
		{
			dictFile << kv.first << " " << kv.second << endl;
		}
	}
	printf("英文词典生成完毕！\n");
}

void MakeDictAndIndex::makeCNdict()
{
	// 确保输出目录存在
	std::error_code __ec2;
	std::filesystem::create_directories("dict", __ec2);

	// 读取中文停用词（trim + 去BOM）
	std::unordered_set<std::string> cn_stopset;
	ifstream stopWordsFile(CNStopWordsfilepath[0]);
	string stopword;
	bool firstLineCN = true;
	while (getline(stopWordsFile, stopword))
	{
		if (firstLineCN)
		{
			strip_bom(stopword);
			firstLineCN = false;
		}
		auto t = trim_copy(stopword);
		if (!t.empty())
			cn_stopset.insert(t);
	}
	// 读取中文语料库
	// 打开中文语料目录
	namespace fs = std::filesystem;
	for (const auto &entry : fs::directory_iterator("yuliao/art"))
	{
		ifstream file(entry.path());
		string line;
		while (getline(file, line))
		{
			// 分词并加入字典
			vector<string> words = CNcut(line);
			for (const auto &word : words)
			{
				// 统计词频
				CNdict[word]++;
			}
		}
	}

	// 生成中文词典
	ofstream dictFile("dict/cn_dict.txt");
	if (!dictFile.is_open())
	{
		cerr << "[Error] 打开输出文件失败: dict/cn_dict.txt (请确认有写权限)\n";
		return;
	}

	for (const auto &kv : CNdict)
	{
		if (!cn_stopset.count(kv.first))
		{
			dictFile << kv.first << " " << kv.second << endl;
		}
	}
	printf("中文词典生成完毕！\n");
}

void MakeDictAndIndex::makeENindex()
{
	// 读取英文词典文件
	ifstream dictFile("dict/en_dict.txt");
	string line;
	while (getline(dictFile, line))
	{
		auto pos = line.find(' ');
		if (pos != string::npos)
		{
			string word = line.substr(0, pos);
			int freq = stoi(line.substr(pos + 1));
			ENdictread.emplace_back(word, freq);
		}
	}
	// 创建字典索引
	int index = 0;
	for (const auto &p : ENdictread)
	{
		string firetschar = p.first.substr(0, 1);
		ENinvertedIndex[firetschar].insert(index++);
	}
	// 保存倒排索引文件
	ofstream indexFile("dict/en_index.txt");
	if (!indexFile.is_open())
	{
		cerr << "[Error] 打开输出文件失败: dict/en_index.txt (请确认有写权限)\n";
		return;
	}
	for (const auto &kv : ENinvertedIndex)
	{
		indexFile << kv.first << " ";
		for (const auto &docId : kv.second)
		{
			indexFile << docId << " ";
		}
		indexFile << endl;
	}
	printf("英文倒排索引生成完毕！\n");
}

void MakeDictAndIndex::makeCNindex()
{
	// 读取中文词典文件
	ifstream dictFile("dict/cn_dict.txt");
	string line;
	while (getline(dictFile, line))
	{
		auto pos = line.find(' ');
		if (pos != string::npos)
		{
			string word = line.substr(0, pos);
			int freq = stoi(line.substr(pos + 1));
			CNdictread.emplace_back(word, freq);
		}
	}
	// 创建字典索引
	int index = 0;
	for (const auto &p : CNdictread)
	{
		string firstchar = p.first.substr(0, 3);
		CNinvertedIndex[firstchar].insert(index++);
	}
	// 保存倒排索引文件
	ofstream indexFile("dict/cn_index.txt");
	if (!indexFile.is_open())
	{
		cerr << "[Error] 打开输出文件失败: dict/cn_index.txt (请确认有写权限)\n";
		return;
	}
	for (const auto &kv : CNinvertedIndex)
	{
		indexFile << kv.first << " ";
		for (const auto &docId : kv.second)
		{
			indexFile << docId << " ";
		}
		indexFile << endl;
	}
	printf("中文倒排索引生成完毕！\n");
}