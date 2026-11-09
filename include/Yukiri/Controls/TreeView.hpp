#pragma once

#include <Yukiri/Controls/ListView.hpp>

namespace Yukiri
{
class TreeView : public ListView
{
public:
	TreeView() : ListView(ViewKind::Tree) {}
};
}
