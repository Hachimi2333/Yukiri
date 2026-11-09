#pragma once

#include <algorithm>
#include <memory>
#include <vector>

namespace Yukiri::Detail
{
template<typename T, typename Predicate>
void RetireMatching(std::vector<std::unique_ptr<T>>& objects, Predicate predicate)
{
	std::vector<std::unique_ptr<T>> retired;
	std::erase_if(objects, [&](auto& object)
	{
		if (!predicate(*object)) return false;
		retired.push_back(std::move(object));
		return true;
	});
	// Destructors may mutate the source collection after its iterators retire.
}
}
