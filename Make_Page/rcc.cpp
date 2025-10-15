#include "rcc.h"

RSS::RSS(size_t capa)
{
	_rss.reserve(capa);
}

/*
 *避免在获取文本时因空指针导致的程序崩溃
 *即使 XML 结构不完整（例如某个 item 缺少 title 元素），
 *代码也不会崩溃，而是会得到一个空字符串
 */
static const char *safe_get_text(XMLElement *e)
{
	if (!e)
		return nullptr;
	const XMLNode *fc = e->FirstChild();
	if (!fc)
		return nullptr;
	return e->GetText();
}

// 读文件
void RSS::read(const string &filename)
{
	XMLDocument doc;
	XMLError err = doc.LoadFile(filename.c_str());
	if (err != XML_SUCCESS)
	{
		std::cerr << "RSS::read: loadFile fail '" << filename << "' err=" << err << endl;
		return;
	}

	XMLElement *root = doc.FirstChildElement("rss");
	if (!root)
	{
		std::cerr << "RSS::read: missing <rss> in '" << filename << "'" << endl;
		return;
	}
	XMLElement *channel = root->FirstChildElement("channel");
	if (!channel)
	{
		std::cerr << "RSS::read: missing <channel> in '" << filename << "'" << endl;
		return;
	}

	XMLElement *itemNode = channel->FirstChildElement("item"); // 按照 RSS 格式的层级结构，获取第一个 item 节点
	size_t idx = 0;
	while (itemNode)
	{
		const char *title_c = safe_get_text(itemNode->FirstChildElement("title"));
		const char *link_c = safe_get_text(itemNode->FirstChildElement("link"));
		const char *desc_c = safe_get_text(itemNode->FirstChildElement("description"));
		const char *cont_c = safe_get_text(itemNode->FirstChildElement("content:encoded"));

		if (!title_c)
		{
			std::cerr << "RSS::read: item missing <title> in '" << filename << "' (item idx=" << idx << ")" << endl;
		}
		if (!link_c)
		{
			std::cerr << "RSS::read: item missing <link> in '" << filename << "' (item idx=" << idx << ")" << endl;
		}

		string title = title_c ? string(title_c) : string();
		string link = link_c ? string(link_c) : string();
		string description = desc_c ? string(desc_c) : string();
		string content = cont_c ? string(cont_c) : string();

		std::regex reg("<[^>]*>"); // 通用正则表达式 <[^>]*>
		description = regex_replace(description, reg, "");
		content = regex_replace(content, reg, "");

		RSSIteam rssItem;
		rssItem._title = title;
		rssItem._link = link;
		rssItem._description = description;
		rssItem._content = content;

		_rss.push_back(rssItem);
		++idx;

		itemNode = itemNode->NextSiblingElement("item");
	}
}

// 写文件
void RSS::store(const string &filename, const string &storeFile, const string &offsetFile)
{
	ofstream ofs(filename);
	if (!ofs)
	{
		std::cerr << "open " << filename << " fail!" << endl;
		return;
	}

	ofstream offsetofs(offsetFile);
	if (!offsetofs)
	{
		std::cerr << "open " << offsetFile << " fail!" << endl;
		return;
	}

	ofstream textofs(storeFile);
	if (!textofs)
	{
		std::cerr << "open " << storeFile << " fail!" << endl;
		return;
	}

	// 通过simhash进行去重
	// 写入 XML 根元素开始
	ofs << "<root>" << '\n';

	auto xml_escape = [](const std::string &s) -> std::string
	{
		std::string out;
		out.reserve(s.size());
		for (char c : s)
		{
			switch (c)
			{
			case '&':
				out += "&amp;";
				break;
			case '<':
				out += "&lt;";
				break;
			case '>':
				out += "&gt;";
				break;
			case '"':
				out += "&quot;";
				break;
			case '\'':
				out += "&apos;";
				break;
			default:
				out.push_back(c);
			}
		}
		return out;
	};

	for (const auto &item : _rss)
	{
		size_t topN = 10; // 取前 10 个关键词
		uint64_t u64 = 0;
		// 如果 description 为空，则跳过 simhash 计算以避免 noisy failures
		if (!item._description.empty())
		{
			vector<pair<string, double>> words;
			simhasher.extract(item._description, words, topN);
			if (!simhasher.make(item._description, topN, u64))
			{
				std::cerr << "simhash make failed! description: " << item._title << std::endl;
				continue; // 如果计算 simhash 失败，跳过该条目
			}
		}
		else
		{
			continue; // 如果 description 为空，跳过该条目
		}
		// 检查 simhash 是否已经存在
		bool isDuplicate = false;
		for (const auto &entry : simhashIndex)
		{
			if (simhasher.isEqual(entry.second, u64))
			{
				isDuplicate = true;
				break; // 找到相似的 simhash，标记为重复
			}
		}
		if (isDuplicate)
		{
			continue; // 如果是重复项，跳过该条目
		}
		simhashIndex[idex + 1] = u64; // 将新的 simhash 添加到索引中

		// 写入文件（对输出做 XML 转义以保证合法性）
		ofs << "<doc><docid>" << (idex + 1) << "</docid>"
			<< "<title>" << xml_escape(item._title) << "</title>"
			<< "<link>" << xml_escape(item._link) << "</link>"
			<< "<description>" << xml_escape(item._description) << "</description>"
			<< "<content>" << xml_escape(item._content) << "</content></doc>" << '\n';

		// 记录偏移量（text.dat 使用原始 description，偏移基于原始长度）
		offsetofs << start << " " << (item._description.size() + start) << '\n';

		// 写入text
		textofs << item._description << '\n';
		start += item._description.size() + 1; // 更新起始偏移量
		++idex;								   // 仅对非重复项递增索引
	}
	// 关闭根元素并关闭文件
	ofs << "</root>" << '\n';
	ofs.close();
	offsetofs.close();
	textofs.close();
}