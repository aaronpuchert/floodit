#include <queue>
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
		if (priority == min_priority)
			lower.push(std::move(value));
		else if (priority == min_priority + 1)
			upper.push(std::move(value));
		else
			throw std::out_of_range("Priority out of range");
	}

	/// Check if the queue is empty.
	bool empty() const noexcept
	{
		return lower.empty();
	}

	/// Get the next element. The iterator can be used to @ref pop() it later.
	const T& top() const
	{
		assert(!lower.empty());
		return lower.front();
	}

	/// Remove an element obtained by @ref top().
	void pop()
	{
		lower.pop();

		if (lower.empty()) {
			std::swap(lower, upper);
			++min_priority;
		}
	}

private:
	std::queue<T> lower, upper;
	int min_priority;
};
