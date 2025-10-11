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

class WordSuggestServerServiceImpl : public WordSuggestServer::Service
{
public:
	void WordSuggest(ReqWordSuggest *request, RespWordSuggest *response, srpc::RPCContext *ctx) override
	{
		// 获取用户请求
		string searchword = request->searchword();
		printf("Received searchword: %s\n", searchword.c_str());
		// 拆分字符（对中文使用最大匹配法，优先匹配词典中的多字词）
		vector<string> words;
		auto is_lead_utf8 = [](unsigned char c) -> int
		{
			if ((c & 0x80) == 0)
				return 1; // ASCII
			if ((c & 0xE0) == 0xC0)
				return 2; // 2-byte
			if ((c & 0xF0) == 0xE0)
				return 3; // 3-byte (most CJK)
			if ((c & 0xF8) == 0xF0)
				return 4; // 4-byte
			return 1;	  // fallback
		};

		// Helper to get a UTF-8 character substring at byte index i
		auto utf8_char = [&](const string &s, size_t i) -> pair<string, size_t>
		{
			if (i >= s.size())
				return {string(), 0};
			int len = is_lead_utf8((unsigned char)s[i]);
			if (i + len > s.size())
				return {string(), 0};
			return {s.substr(i, len), (size_t)len};
		};

		// Max match window (in number of UTF-8 characters)
		const int MAX_WINDOW = 3;
		for (size_t i = 0; i < searchword.size();)
		{
			// read up to MAX_WINDOW utf8 chars into a vector
			size_t j = i;
			vector<pair<string, size_t>> chars;
			for (int k = 0; k < MAX_WINDOW && j < searchword.size(); ++k)
			{
				auto p = utf8_char(searchword, j);
				if (p.second == 0)
					break;
				chars.push_back(p);
				j += p.second;
			}

			bool matched = false;
			// try longest match first
			for (int w = (int)chars.size(); w > 0; --w)
			{
				string token;
				size_t len_bytes = 0;
				for (int t = 0; t < w; ++t)
				{
					token += chars[t].first;
					len_bytes += chars[t].second;
				}
				// if token exists in dict (we'll use readindist.dict for lookup via unordered_map later),
				// here we check if invertedIndex contains token as a quick heuristic
				if (readindist.invertedIndex.find(token) != readindist.invertedIndex.end())
				{
					words.push_back(token);
					i += len_bytes;
					matched = true;
					break;
				}
			}
			if (!matched)
			{
				// fallback to single utf8 char
				if (!chars.empty())
				{
					words.push_back(chars[0].first);
					i += chars[0].second;
				}
				else
				{
					// malformed utf8
					response->set_code(1);
					response->set_message("invalid utf8 in searchword");
					return;
				}
			}
		}
		// 查询索引，汇总结果
		set<int> resultSet;
		for (const auto &word : words)
		{
			auto it = readindist.invertedIndex.find(word);
			if (it != readindist.invertedIndex.end())
			{
				resultSet.insert(it->second.begin(), it->second.end());
			}
		}
		// 根据最短编辑距离算法，找出最相似的词
		// 避免 most-vexing-parse：用花括号或额外括号强制为对象构造
		map<string, int, SimilarityComparator> candidates{SimilarityComparator(searchword)}; // 编辑距离 -> 词
		for (int index : resultSet)
		{
			if (index >= 0 && index < (int)readindist.dict.size())
			{
				const string &candidateWord = readindist.dict[index].first;
				candidates[candidateWord] = index;
			}
			else
			{
				// 索引无效，跳过
				response->set_code(1);
				response->set_message("invalid index");
				return;
			}
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
	readindist.LoadDictAndIndex();
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
