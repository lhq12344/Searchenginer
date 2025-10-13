#ifndef MAKEPAGE_H
#define MAKEPAGE_H

#include "rcc.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <regex>
#include <filesystem>
#include <sstream>
#include <cctype>
using namespace std;

class MakePage
{
public:
	MakePage(/* args */) = default;
	~MakePage() = default;

	void makepage(); // 创建网页库和偏移库

private:
	// 结构体存储网页内容
	unordered_map<int, pair<int, int>> offsetMap; // docid -> (start, end)存储网页偏移库
};

#endif // !MAKEPAGE_H
