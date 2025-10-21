#include "pagesearch.srpc.h"
#include "workflow/WFFacilities.h"
#include "ReadPage.h"
#include "cut_weight.h"
#include "Cache.h"
#include <iostream>
#include <shared_mutex>
#include <ppconsul/ppconsul.h>

using namespace srpc;
using namespace std;
using namespace ppconsul;

/*全局对象：*/
ReadPage readPage;									 // 读取网页库、偏移库、倒排索引的类
CutWeight cutweight;								 // 分词并计算权重的类
MyLRUCache<int, shared_ptr<string>> textCache(1000); // LRU缓存，缓存网页内容，容量1000

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

map<int, set<double>> getintersection(vector<pair<string, double>> &words_weight)
{
	map<int, set<double>> result_set;
	// 并发安全：加共享锁以保护对 readPage 的读取
	std::shared_lock<std::shared_mutex> rlock(readPage.mtx);

	// 不要在遍历时修改 words_weight；先建立过滤后的词表
	vector<pair<string, double>> filtered;
	for (const auto &word_weight : words_weight)
	{
		const string &word = word_weight.first;
		auto it = readPage.invertedIndex.find(word);
		if (it == readPage.invertedIndex.end())
		{
			cout << "word not found: " << word << endl;
			continue; // 该词不在倒排索引中
		}
		const set<pair<int, double>> &docSet = it->second; // 包含该词的所有文档及其权重
		for (const auto &doc : docSet)
		{
			result_set[doc.first].insert(doc.second);
			printf("DocID: %d, Word: %s, Weight: %f\n", doc.first, word.c_str(), doc.second);
		}
		filtered.push_back(word_weight);
	}

	// 替换原始词表为过滤后的词表
	words_weight.swap(filtered);

	// 过滤出包含所有分词的文档；先收集需要保留的docid，避免在迭代时修改map
	int required_count = words_weight.size();
	vector<int> keep;
	for (const auto &entry : result_set)
	{
		if ((int)entry.second.size() == required_count)
			keep.push_back(entry.first);
	}
	map<int, set<double>> final_map;
	for (int d : keep)
	{
		final_map[d] = std::move(result_set[d]);
	}
	printf("Total documents containing all words: %zu\n", final_map.size());
	return final_map;
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
		vector<pair<string, double>> words_weight = cutweight.cut_weight(searchstring, readPage.totalpage, readPage);
		if (words_weight.empty())
		{
			response->set_code(2);
			response->set_message("cut no valid words");
			printf("cut no valid words: %s\n", searchstring.c_str());
			return;
		}

		// 计算模长
		double norm = cutweight.get_norm(); // 模长
		printf("关键句的Norm: %f\n", norm);

		// 计算每个文档与查询的内积
		unordered_map<int, pair<double, double>> inner_products; // docid -> (累加的内积, 文档模长)
		unordered_map<int, int> docid_set;						 // 记录docid的集合
		int words_count = words_weight.size();
		// 加共享锁保护对 readPage 的多次读取
		std::shared_lock<std::shared_mutex> rlock(readPage.mtx);
		for (auto &[word, wq] : words_weight)
		{
			auto it = readPage.invertedIndex.find(word);
			if (it == readPage.invertedIndex.end())
			{
				words_count--;
				continue; // 词不在索引中
			}
			for (auto &[docid, wd] : it->second)
			{
				inner_products[docid].first += wq * wd;	 // 按词累积内积
				inner_products[docid].second += wd * wd; // 计算文档模长
				docid_set[docid]++;
			}
		}

		// 计算内积大小并且排序
		priority_queue<pair<double, int>> pq; // (inner_product, docid)priority_queue默认以pair的first降序排列
		for (const auto &entry : inner_products)
		{
			// 删除关键词数量过少的文档
			int docid = entry.first;
			if (docid_set[docid] < words_count) // 包含全部关键词
				continue;
			double inner_product = entry.second.first;
			double norm2 = sqrt(entry.second.second);
			// 计算余弦相似度
			if (norm2 == 0 || norm == 0)
				continue; // 避免除以零
			double cosine_similarity = inner_product / (norm * norm2);
			pq.push({cosine_similarity, docid});
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
			const Item &page = readPage.pageLib[docid - 1];
			pair<int, int> offset = readPage.offsetLib[docid - 1];
			// 先从缓存中查找网页内容
			shared_ptr<string> pageContentPtr;
			auto cachedContent = textCache.get(docid);
			if (cachedContent)
			{
				pageContentPtr = *cachedContent;
				printf("Cache hit for DocID: %d\n", docid);
			}
			else
			{
				ifstream ifs("../../Make_Page/page/text.dat", ios::binary);
				if (!ifs)
				{
					response->set_code(1);
					response->set_message("Failed to open text.dat");
					printf("Failed to open text.dat\n");
					break;
				}
				size_t len = offset.second - offset.first;
				ifs.seekg(offset.first);
				string buffer(len, '\0'); // 预分配空间
				ifs.read(&buffer[0], len);
				// 放入缓存
				pageContentPtr = make_shared<string>(std::move(buffer));
				textCache.put(docid, pageContentPtr);
			}

			Item_proto *item_proto = response->add_itemproto();
			item_proto->set_title(page.title);
			item_proto->set_link(page.link);
			item_proto->set_description(*pageContentPtr);
			printf("DocID: %d, Title: %s, Link: %s,Similarity: %f\n", docid, page.title.c_str(), page.link.c_str(), item.first);
			count++;
		}
	}
};

void timerCallback(WFTimerTask *timertask)
{
	agent::Agent *pagent = (agent::Agent *)timertask->user_data;
	pagent->servicePass("PageSuggestService1");
	WFTimerTask *nextTask = WFTaskFactory::create_timer_task(9, 0, timerCallback);
	nextTask->user_data = pagent;
	series_of(timertask)->push_back(nextTask);
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 1600;
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
	//  向consul注册服务（受保护，避免未捕获异常导致进程中止）
	agent::Agent *pagent = nullptr;
	Consul *pconsul = nullptr; // keep consul alive for the lifetime of agent
	pconsul = new Consul("127.0.0.1:8500", ppconsul::kw::dc = "dc1");
	pagent = new agent::Agent(*pconsul);
	pagent->registerService(
		agent::kw::name = "PageSuggestService1",
		agent::kw::id = "PageSuggestService1",
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
