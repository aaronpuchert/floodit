#include <unordered_set>
#include <stdexcept>

/**
 * Special-purpose priority queue.
 *
 * Such a queue can be used if the priorities of new elements are either the
 * same as those of the current top, or greater by 1. This would be the case if
 * a graph has only weights 0 and 1.
 */
template<typename T>
class special_priority_queue
{
public:
	explicit special_priority_queue(int min_priority)
		: min_priority(min_priority) {}

	/**
	 * Add element to queue.
	 *
	 * @param value    Value to add.
	 * @param priority Priority, which must be in next().second + {0, 1}.
	 */
	void push(T &&value, int priority)
	{
		if (priority == min_priority) {
			auto old = upper.find(value);
			if (old != upper.end())
				upper.erase(old);
			lower.insert(std::move(value));
		}
		else if (priority == min_priority + 1) {
			if (lower.find(value) == lower.end())
				upper.insert(std::move(value));
		}
		else
			throw std::out_of_range("Priority out of range");
	}

	/// Check if the queue is empty.
	bool empty() const noexcept
	{
		return lower.empty();
	}

	/// Get the next element. The iterator can be used to @ref pop() it later.
	std::pair<typename std::unordered_set<T>::const_iterator, int> top() const
	{
		assert(!lower.empty());
		return std::make_pair(lower.cbegin(), min_priority);
	}

	/// Remove an element obtained by @ref top().
	void pop(typename std::unordered_set<T>::const_iterator it)
	{
		lower.erase(it);

		if (lower.empty()) {
			std::swap(lower, upper);
			++min_priority;
		}
	}

private:
	std::unordered_set<T> lower, upper;
	int min_priority;
};
