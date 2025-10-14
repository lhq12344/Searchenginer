#include "../include/Pageserver.h"

// URL解码函数
std::string urlDecode(const std::string &encoded)
{
	std::string decoded;
	for (size_t i = 0; i < encoded.size(); ++i)
	{
		if (encoded[i] == '%' && i + 2 < encoded.size())
		{
			// 解析%后面的两位十六进制数
			char hex1 = encoded[i + 1];
			char hex2 = encoded[i + 2];

			int val = 0;
			if (isxdigit(hex1) && isxdigit(hex2))
			{
				std::stringstream ss;
				ss << std::hex << std::string(1, hex1) << std::string(1, hex2);
				ss >> val;
				decoded += static_cast<char>(val);
				i += 2; // 跳过已经处理的两个字符
				continue;
			}
		}
		// 普通字符直接添加
		decoded += encoded[i];
	}
	return decoded;
}

void pageSuggestCallback(RespPageSuggest *response, srpc::RPCContext *ctx, HttpResp *resp,
						 ReqPageSuggest *word_suggest_req)
{

	if (response->code() == 0)
	{
	}
	else
	{
	}

	// 清理申请的资源
	delete word_suggest_req;
}

// 访问CONSUL获取服务信息
void ConsulPageSearchCallback(WFHttpTask *task, const HttpReq *req,
							  HttpResp *resp, SeriesWork *series)
{
	auto consul_resp = task->get_resp();
	if (task->get_state() == WFT_STATE_SUCCESS)
	{
		const void *raw_body = nullptr;
		size_t body_size = 0;

		if (consul_resp->get_parsed_body(&raw_body, &body_size))
		{
			// 将 body 转成 std::string
			std::string body(static_cast<const char *>(raw_body), body_size);
			std::string json_candidate = extract_first_json(body);
			try
			{
				// 解析 JSON
				auto j = json::parse(json_candidate);
				// Consul 的返回格式是一个对象，key 是服务名

				std::string ip;
				unsigned short port = 0;
				for (auto &[serviceName, serviceInfo] : j.items())
				{
					if (serviceName == "PageSuggestService1" &&
						serviceInfo.contains("Address") && serviceInfo.contains("Port"))
					{
						ip = serviceInfo["Address"].get<std::string>();
						port = static_cast<unsigned short>(serviceInfo["Port"].get<int>());
						break; // 如果只取一个，找到后跳出
					}
				}
				if (ip.empty() || port == 0)
				{
					std::cerr << "PageSuggestService1 not found in consul response"
							  << std::endl;
					resp->String("FAILED");
					return;
				}

				std::cout << "IP: " << ip << ", Port: " << port << std::endl;
				auto formKV = req->form_kv();
				std::string urlsearchstring = formKV["searchstring"];
				std::string searchstring = urlDecode(urlsearchstring);
				if (searchstring.empty())
				{
					resp->String("FAILED: searchstring is empty");
					return;
				}
				std::cout << "searchstring: " << searchstring.c_str() << std::endl;
				GOOGLE_PROTOBUF_VERIFY_VERSION;
				// 1) 建立 SRPC Client（放到堆或让 task 管理其生命周期）
				auto client =
					std::make_shared<PageSuggestServer::SRPCClient>(ip.c_str(), (int)port);

				// 2) 组装请求
				auto *page_suggest_req = new ReqPageSuggest();
				page_suggest_req->set_searchstring(searchstring);

				// 3) 创建一个 SRPC 任务，并把它挂到当前 HTTP 的 series 上
				auto *task = client->create_PageSuggest_task(
					std::bind(pageSuggestCallback, std::placeholders::_1,
							  std::placeholders::_2, resp, page_suggest_req));
				task->serialize_input(page_suggest_req);

				// 4) 把 SRPC 任务加入本次 HTTP 的序列 —— 关键！
				series->push_back(task);
			}
			catch (const std::exception &e)
			{
				std::cerr << "JSON parse error: " << e.what() << std::endl;
				resp->String("FAILED");
			}
		}
	}
	else
	{
		std::cerr << "HTTP request failed, state=" << task->get_state()
				  << std::endl;
		resp->String("FAILED");
	}
}