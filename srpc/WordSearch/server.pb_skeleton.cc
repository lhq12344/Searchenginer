#include "wordsearch.srpc.h"
#include "workflow/WFFacilities.h"
#include "ReadIndist.h"
#include "SimilarityComparator.h"
#include <iostream>
#include <ppconsul/ppconsul.h>

using namespace srpc;
using namespace std;
using namespace ppconsul;

ReadIndist readindist; // 全局对象：读取词典和索引的类

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

// 判断一个字节是否是 UTF-8 字符的起始字节，并返回该 UTF-8 字符的总字节数（1-4 字节）
auto is_lead_utf8 = [](unsigned char c) -> int
{
	if ((c & 0x80) == 0)
		return 1; // ASCII英文
	if ((c & 0xE0) == 0xC0)
		return 2; // 2-byte
	if ((c & 0xF0) == 0xE0)
		return 3; // 3-byte (most CJK)中文
	if ((c & 0xF8) == 0xF0)
		return 4; // 4-byte
	return 1;	  // fallback
};

// 从字符串的指定位置提取一个完整的 UTF-8 字符
auto utf8_char = [](const string &s, size_t i) -> pair<string, size_t>
{
	if (i >= s.size())
		return {string(), 0};
	int len = is_lead_utf8((unsigned char)s[i]);
	if (i + len > s.size())
		return {string(), 0};
	return {s.substr(i, len), (size_t)len};
};

// 更稳健的语言判定：如果整个 searchword 都是 ASCII，则认为是英文，否则认为是中文/多字节
auto is_ascii_string = [](const string &s) -> bool
{
	for (unsigned char c : s)
	{
		if (c & 0x80)
			return false;
	}
	return true;
};

class WordSuggestServerServiceImpl : public WordSuggestServer::Service
{
public:
	void WordSuggest(ReqWordSuggest *request, RespWordSuggest *response, srpc::RPCContext *ctx) override
	{
		// 获取用户请求
		string searchword = request->searchword();
		printf("Received searchword: %s\n", searchword.c_str());
		vector<pair<string, size_t>> words;
		// 拆分字符
		const int MAX_WINDOW = 3;
		for (size_t i = 0; i < searchword.size();)
		{
			// read up to MAX_WINDOW utf8 chars into a vector
			auto p = utf8_char(searchword, i);
			if (p.second == 0)
				break;
			words.push_back(p);
			i += p.second;
			printf("Read char: %s\n", p.first.c_str());
		}
		// 查询索引，汇总结果。优先在对应的 EN/CN 倒排索引中查找
		set<int> CNresultSet;
		set<int> ENresultSet;
		for (const auto &word : words)
		{
			printf("Searching for token: %s\n", word.first.c_str());

			if (word.second == 1)
			{
				auto it = readindist.ENinvertedIndex.find(word.first);
				if (it != readindist.ENinvertedIndex.end())
				{
					ENresultSet.insert(it->second.begin(), it->second.end());
				}
			}
			else
			{
				auto it = readindist.CNinvertedIndex.find(word.first);
				if (it != readindist.CNinvertedIndex.end())
				{
					CNresultSet.insert(it->second.begin(), it->second.end());
				}
			}
		}
		// 根据最短编辑距离算法，找出最相似的词
		// 避免 most-vexing-parse：用花括号或额外括号强制为对象构造
		map<string, int, SimilarityComparator> candidates{SimilarityComparator(searchword)}; // 编辑距离 -> 词
		if (!ENresultSet.empty() || !CNresultSet.empty())
		{

			// debug: 打印一下两个结果集合大小
			printf("ENresultSet size: %zu, CNresultSet size: %zu\n", ENresultSet.size(), CNresultSet.size());

			// 根据searchword的类型选择对应的结果集
			if (is_ascii_string(searchword))
			{
				// 英文
				for (int index : ENresultSet)
				{
					if (index < (int)readindist.ENdict.size())
					{
						const auto &p = readindist.ENdict[index];
						candidates[p.first] = index;
					}
					else
					{
						// 索引越界，忽略
						printf("Warning: EN index %d out of bounds (dict size %zu)\n", index, readindist.ENdict.size());
					}
				}
			}
			else
			{
				// 中文
				for (int index : CNresultSet)
				{
					if (index < (int)readindist.CNdict.size())
					{
						const auto &p = readindist.CNdict[index];
						candidates[p.first] = index;
					}
					else
					{
						// 索引越界，忽略
						printf("Warning: CN index %d out of bounds (dict size %zu)\n", index, readindist.CNdict.size());
					}
				}
			}
		}
		else
		{
			printf("没有命中倒排索引\n");
		}

		// 取前10个最相似的词,不够补空字符串
		int count = 0;
		for (const auto &kv : candidates)
		{
			if (count >= 10)
				break;
			response->add_backwords(kv.first); // 添加到响应中
			printf("Suggested word: %s\n", kv.first.c_str());
			count++;
		}
		while (count < 10)
		{
			response->add_backwords(""); // 补足10个
			count++;
		}
		response->set_code(0);
		response->set_message("success");
	}
};

void timerCallback(WFTimerTask *timertask)
{
	agent::Agent *pagent = (agent::Agent *)timertask->user_data;
	pagent->servicePass("WordSuggestService1");
	WFTimerTask *nextTask = WFTaskFactory::create_timer_task(9, 0, timerCallback);
	nextTask->user_data = pagent;
	series_of(timertask)->push_back(nextTask);
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 1500;
	SRPCServer server;

	WordSuggestServerServiceImpl userservice_impl;
	server.add_service(&userservice_impl);
	// 微服务开启后将字典和索引加载到内存中
	bool ret = readindist.LoadDictAndIndex();
	if (ret == false)
	{
		cerr << "Failed to load dictionary and index." << endl;
		return -1;
	}
	// 向consul注册服务（受保护，避免未捕获异常导致进程中止）
	agent::Agent *pagent = nullptr;
	Consul *pconsul = nullptr; // keep consul alive for the lifetime of agent
	pconsul = new Consul("127.0.0.1:8500", ppconsul::kw::dc = "dc1");
	pagent = new agent::Agent(*pconsul);
	pagent->registerService(
		agent::kw::name = "WordSuggestService1",
		agent::kw::id = "WordSuggestService1",
		agent::kw::address = "127.0.0.1",
		agent::kw::port = port,
		agent::kw::check = agent::TtlCheck(std::chrono::seconds(10)));
	// mark service passing once
	pagent->servicePass("WordSuggestService1");
	// 注册定时心跳函数
	WFTimerTask *timerTask = WFTaskFactory::create_timer_task(9, 0, timerCallback);
	if (timerTask)
	{
		timerTask->user_data = pagent;
		timerTask->start();
	}
	else
	{
		fprintf(stderr, "Failed to create timer task for consul heartbeat\n");
	}
	server.start(port);
	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
