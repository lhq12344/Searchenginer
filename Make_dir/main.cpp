
#include "MakeDictAndIndex.h"
// 对字典和索引文件的建立，中英文分开建立

int main()
{
	MakeDictAndIndex mdi;
	mdi.makeENdict();
	mdi.makeCNdict();
	mdi.makeENindex();
	mdi.makeCNindex();
	return 0;
}
