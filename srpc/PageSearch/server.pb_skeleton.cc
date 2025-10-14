#include "pagesearch.srpc.h"
#include "workflow/WFFacilities.h"
#include "ReadPage.h"
#include "cut_weight.h"
#include <iostream>
#include <ppconsul/ppconsul.h>

using namespace srpc;
using namespace std;
using namespace ppconsul;

/*全局对象：*/
ReadPage readPage;	 // 读取网页库、偏移库、倒排索引的类
CutWeight cutweight; // 分词并计算权重的类

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

map<int, set<double>> getintersection(vector<pair<string, double>> &words_weight)
{
	map<int, set<double>> result_set;
	for (const auto &word_weight : words_weight)
	{
		const string &word = word_weight.first;
		double weight = word_weight.second;
		auto it = readPage.invertedIndex.find(word);
		if (it == readPage.invertedIndex.end())
		{
			// 删除这个分词   remove_if 会把所有需要删除的元素“移到末尾”；
			words_weight.erase(remove_if(words_weight.begin(), words_weight.end(),
										 [&word](const pair<string, double> &p)
										 { return p.first == word; }),
							   words_weight.end());
			continue; // 该词不在倒排索引中
		}
		const set<pair<int, double>> &docSet = it->second; // 包含该词的所有文档及其权重
		for (const auto &doc : docSet)
		{
			result_set[doc.first].insert(doc.second);
		}
	}
	// 过滤出包含所有分词的文档
	int required_count = words_weight.size();
	for (const auto &entry : result_set)
	{
		int docid = entry.first;
		if (entry.second.size() != required_count)
		{
			// 不包含所有分词，删除
			result_set.erase(docid);
		}
	}
	return result_set;
}
double countnorm(const set<double> &weights)
{
	double norm = 0.0;
	for (double w : weights)
	{
		norm += w * w;
	}
	return sqrt(norm);
}
class PageSuggestServerServiceImpl : public PageSuggestServer::Service
{
public:
	void PageSuggest(ReqPageSuggest *request, RespPageSuggest *response, srpc::RPCContext *ctx) override
	{
		// 获取用户请求
		string searchstring = request->searchstring();
		printf("Received searchstring: %s\n", searchstring.c_str());

		// 分词并且计算权重
		vector<pair<string, double>> words_weight = cutweight.cut_weight(searchstring);
		double norm = cutweight.get_norm(); // 模长
		printf("关键句的Norm: %f\n", norm);

		// 查倒排索引，取出交集（包含所有分词的page）
		map<int, set<double>> result_set = getintersection(words_weight);

		// 计算内积大小并且排序
		priority_queue<pair<double, int>> pq; // (inner_product, docid)priority_queue默认以pair的first降序排列
		for (const auto &entry : result_set)
		{
			int docid = entry.first;
			const set<double> &weights = entry.second;
			double norm2 = countnorm(weights);
			double inner_product = 0.0;
			int index = 0;
			for (double w : weights)
			{
				inner_product += w * words_weight[index].second; // 计算内积
				index++;
			}
			inner_product /= (norm * norm2); // 余弦相似度
			pq.push({inner_product, docid});
		}

		// 返回前10个结果
		int count = 0;
		response->set_code(0);
		response->set_message("OK");
		while (!pq.empty() && count < 10)
		{
			auto item = pq.top();
			pq.pop();
			// item.first 是余弦相似度，item.second 是 docid
			// 根据 docid 取出网页信息
			int docid = item.second;
			if (docid <= 0 || docid > (int)readPage.pageLib.size())
			{
				printf("Invalid docid: %d\n", docid);
				continue; // docid 无效
			}
			const Item &page = readPage.pageLib[docid];
			Item *item1 = response->add_item();
			item1->set_title(page.title);
			item1->set_link(page.link);
			printf("DocID: %d, Title: %s, Link: %s,Similarity: %f\n", docid, page.title.c_str(), page.link.c_str(), item.first);
			count++;
		}
	}
};

void timerCallback(WFTimerTask *timertask)
{
	agent::Agent *pagent = (agent::Agent *)timertask->user_data;
	pagent->servicePass("PPageSuggestService1");
	WFTimerTask *nextTask = WFTaskFactory::create_timer_task(9, 0, timerCallback);
	nextTask->user_data = pagent;
	series_of(timertask)->push_back(nextTask);
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 1501;
	SRPCServer server;

	PageSuggestServerServiceImpl pagesuggestserver_impl;
	server.add_service(&pagesuggestserver_impl);
	// 微服务开启后将网页库、网页偏移库、倒排索引加载到内存中
	bool ret = readPage.LoadPageAndOffset();
	if (ret == false)
	{
		cerr << "Failed to load page." << endl;
		return -1;
	}
	// 向consul注册服务（受保护，避免未捕获异常导致进程中止）
	agent::Agent *pagent = nullptr;
	Consul *pconsul = nullptr; // keep consul alive for the lifetime of agent
	pconsul = new Consul("127.0.0.1:8500", ppconsul::kw::dc = "dc1");
	pagent = new agent::Agent(*pconsul);
	pagent->registerService(
		agent::kw::name = "PageSuggestService1",
		agent::kw::id = "PPageSuggestService1",
		agent::kw::address = "127.0.0.1",
		agent::kw::port = port,
		agent::kw::check = agent::TtlCheck(std::chrono::seconds(10)));
	// mark service passing once
	pagent->servicePass("PageSuggestService1");
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
