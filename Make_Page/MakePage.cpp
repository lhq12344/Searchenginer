#include "MakePage.h"

// 读取并且清洗网页
void MakePage::makepage()
{
	// 确保输出目录存在
	std::error_code __ec2;
	std::filesystem::create_directories("page", __ec2);

	// rcc构造参数
	RSS rss(4000);

	for (const auto &entry : std::filesystem::directory_iterator("./yuliao"))
	{
		ifstream file(entry.path());
		rss.read(entry.path().string());
	}
	rss.store("./page/pagelib.dat", "./page/text.dat", "./page/offset.dat");
}
