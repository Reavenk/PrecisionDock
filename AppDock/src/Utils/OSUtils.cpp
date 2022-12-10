#include "OSUtils.h"

namespace OSUtils
{
	bool IsToplevel(HWND hwnd)
	{
		return (hwnd == GetAncestor(hwnd, GA_ROOT));
	}
}