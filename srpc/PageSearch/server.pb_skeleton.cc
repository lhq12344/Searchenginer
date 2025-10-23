#include "pagesearch.srpc.h"
#include "workflow/WFFacilities.h"
#include "ReadPage.h"
#include "cut_weight.h"
#include "Cache.h"
#include <iostream>
#include <shared_mutex>
#include <ppconsul/ppconsul.h>
#include <faiss/IndexIDMap.h>

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

// 计算关键词的向量表示（简单平均）
std::vector<float> countwords(vector<string> &words, fasttext::FastText &model)
{
	int dim = static_cast<int>(model.getDimension());
	std::vector<float> accum(dim, 0.f);
	fasttext::Vector wv(dim);
	int used = 0;
	for (const auto &w : words)
	{
		if (w.empty())
			continue;
		model.getWordVector(wv, w);
		// 累加
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
	// L2 归一化（FAISS 用内积时等价余弦）
	double s = 0.0;
	for (float x : accum)
		s += double(x) * double(x);
	if (s <= 0)
		return std::vector<float>(dim, 0.f);
	float inv = 1.0f / static_cast<float>(std::sqrt(s));
	for (auto &x : accum)
		x *= inv;

	return accum;
}

// 使用 TF-IDF 权重计算查询向量：对词向量做加权求和并 L2 归一化
std::vector<float> query_vector_tfidf(const std::vector<std::pair<std::string, double>> &words_weight,
									  fasttext::FastText &model)
{
	int dim = static_cast<int>(model.getDimension());
	std::vector<float> accum(dim, 0.f);
	if (words_weight.empty())
		return accum;
	fasttext::Vector wv(dim);
	for (const auto &p : words_weight)
	{
		const std::string &w = p.first;
		double wt = p.second; // 已经在 cut_weight 中做了归一化
		if (w.empty() || wt == 0.0)
			continue;
		model.getWordVector(wv, w);
		for (int i = 0; i < dim; ++i)
			accum[i] += static_cast<float>(wv[i] * wt);
	}
	// L2 归一化
	double s = 0.0;
	for (float x : accum)
		s += double(x) * double(x);
	if (s > 0)
	{
		float inv = 1.0f / static_cast<float>(std::sqrt(s));
		for (auto &x : accum)
			x *= inv;
	}
	return accum;
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
		// 先用 TF-IDF 计算查询词权重；若失败则回退到简单平均
		std::vector<std::pair<std::string, double>> words_weight = cutweight.cut_weight(searchstring, readPage.totalpage, readPage);
		std::vector<float> vector;
		if (!words_weight.empty())
		{
			vector = query_vector_tfidf(words_weight, readPage.model);
		}
		else
		{
			// fallback：只做分词 + 简单平均
			std::vector<std::string> words = cutweight.cut(searchstring);
			vector = countwords(words, readPage.model);
		}
		// 查询faiss索引
		int topk = 10;					   // 查找最相似的前10篇文章
		std::vector<faiss::idx_t> I(topk); // 存储结果索引
		std::vector<float> D(topk);		   // 存储相似度得分

		// 查询（第一个参数是查询向量数量，这里是1）
		readPage.index->search(1, vector.data(), topk, D.data(), I.data());

		// 输出结果
		for (int i = 0; i < topk; ++i)
		{
			if (I[i] < 0)
				continue; // -1 表示无结果
			std::cout << "Rank " << i + 1 << ": docid=" << I[i]
					  << "  similarity=" << D[i]
					  << std::endl;
		}
		// 构建优先队列以排序结果
		priority_queue<pair<double, int>> pq; // (similarity, docid)
		for (int i = 0; i < topk; ++i)
		{
			if (I[i] < 0)
				continue;
			pq.push({D[i], static_cast<int>(I[i])});
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
	// 加载fastText模型
	readPage.model.loadModel("../../Make_Page/page/small_model.bin");
	// 加载fastText向量
	std::vector<VecEntry> entries;
	int dim = 0;
	if (!readPage.LoadVectors("../../Make_Page/page/vectors.dat", entries, dim))
	{
		std::cerr << "Failed to load vectors.dat\n";
		return -1;
	}

	// Build IndexFlatIP and wrap with IndexIDMap so search returns original docids
	faiss::IndexFlatIP *flat = new faiss::IndexFlatIP(dim);
	faiss::IndexIDMap *idmap = new faiss::IndexIDMap(flat);
	// flatten vectors into a single array and collect ids
	std::vector<float> allvecs;
	allvecs.reserve((size_t)entries.size() * dim);
	std::vector<faiss::idx_t> ids;
	ids.reserve(entries.size());
	for (const auto &e : entries)
	{
		allvecs.insert(allvecs.end(), e.vec.begin(), e.vec.end());
		ids.push_back(static_cast<faiss::idx_t>(e.docid));
	}
	idmap->add_with_ids(entries.size(), allvecs.data(), ids.data());
	readPage.index = idmap;

	std::cout << "FAISS index built: dim=" << dim << " n=" << entries.size() << std::endl;
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
