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
		// 拆分字符
		vector<string> words;
		for (size_t i = 0; i < searchword.size();)
		{
			if (searchword[i] & 0x80)
			{ // 中文字符
				if (i + 3 <= searchword.size())
				{
					words.push_back(searchword.substr(i, 3));
					i += 3;
				}
				else
				{
					response->set_code(1);
					response->set_message("incomplete character");
					return;
				}
			}
			else
			{ // 英文字符
				words.push_back(searchword.substr(i, 1));
				i++;
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
	if (!pagent)
	{
		// No consul agent available, skip heartbeat
		return;
	}
	try
	{
		// Use the same service id we registered
		pagent->servicePass("WordSuggestService1");
	}
	catch (const ppconsul::NotFoundError &e)
	{
		fprintf(stderr, "Consul NotFoundError in servicePass: %s\n", e.what());
	}
	catch (const std::exception &e)
	{
		fprintf(stderr, "Consul error in servicePass: %s\n", e.what());
	}
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
	try
	{
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
	}
	catch (const ppconsul::NotFoundError &e)
	{
		fprintf(stderr, "Consul NotFoundError while registering service: %s\n", e.what());
		if (pagent)
		{
			delete pagent;
			pagent = nullptr;
		}
		if (pconsul)
		{
			delete pconsul;
			pconsul = nullptr;
		}
	}
	catch (const std::exception &e)
	{
		fprintf(stderr, "Consul error while registering service: %s\n", e.what());
		if (pagent)
		{
			delete pagent;
			pagent = nullptr;
		}
	}
	server.start(port);
	wait_group.wait();
	server.stop();
	if (pagent)
	{
		// best-effort cleanup
		try
		{
			delete pagent;
		}
		catch (...)
		{
		}
		pagent = nullptr;
	}
	if (pconsul)
	{
		try
		{
			delete pconsul;
		}
		catch (...)
		{
		}
		pconsul = nullptr;
	}
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
