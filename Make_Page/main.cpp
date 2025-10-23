#include "MakePage.h"
#include <cstdlib>
#include <string>

int main()
{
	MakePage makePage;

	makePage.makepage();
	makePage.makeinverseindex();
	// makePage.makefasttextfile();
	makePage.makefasttextmodel();

	return 0;
}