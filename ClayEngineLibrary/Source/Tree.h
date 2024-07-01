#pragma once

#include <yvals_core.h>
#if _STL_COMPILER_PREPROCESSOR
#include <xmemory>

#if _HAS_CXX17
#include <xpolymorphic_allocator.h>
#endif // _HAS_CXX17

#define _CE_BEGIN namespace ce {
#define _CE_END }

#pragma pack(push, _CRT_PACKING)
#pragma warning(push, _STL_WARNING_LEVEL)
#pragma warning(disable : _STL_DISABLED_WARNINGS)
_STL_DISABLE_CLANG_WARNINGS
#pragma push_macro("new")
#undef new

_STD_BEGIN
template <class _Traits> class _Hash;
_STD_END

_CE_BEGIN
/******************************************************************************/
/*																			  */
/*							Tree Iterator									  */
/*																			  */
/*	Iterator implements depth-first through operator++ and operator--		  */
/*	Does not support bredth-first iteraton at this time						  */
/*																			  */
/******************************************************************************/

struct FakeAllocator {};
struct ContainerBase
{
	inline void _Orphan_all() noexcept {}
	inline void _Swap_proxy_and_iterators(ContainerBase&) noexcept {}
	inline void _Alloc_proxy(const FakeAllocator&) noexcept {}
	inline void _Reload_proxy(const FakeAllocator&, const FakeAllocator&) noexcept {}
};
struct IteratorBase
{
	inline void _Adopt(const void*) noexcept {}
	inline const ContainerBase* _Getcont() const noexcept { return nullptr; }

	static constexpr bool _Unwrap_when_unverified = true;
};

template <class T, class Iter = IteratorBase>
class tree_unchecked_const_iterator : public Iter
{
public:
	using iterator_category = std::bidirectional_iterator_tag;

	using node_ptr = typename T::node_ptr;
	using value_type = typename T::value_type;
	using difference_type = typename T::difference_type;
	using pointer = typename T::const_pointer;
	using reference = const value_type&;

	node_ptr p_node; // pointer to node

	tree_unchecked_const_iterator() noexcept
		: p_node()
	{}
	tree_unchecked_const_iterator(node_ptr node, const T* tree) noexcept
		: p_node(node)
	{
		this->_Adopt(tree);
	}

	[[nodiscard]] reference operator*() const
	{
		return p_node->value;
	}
	[[nodiscard]] pointer operator->() const
	{
		return std::pointer_traits<pointer>::pointer_to(**this);
	}
	[[nodiscard]] bool operator==(const tree_unchecked_const_iterator& rhs) const
	{
		return p_node == rhs.p_node;
	}
	[[nodiscard]] bool operator!=(const tree_unchecked_const_iterator& rhs) const
	{
		return !(*this == rhs);
	}

	tree_unchecked_const_iterator& operator++()
	{
		// First check if node has a child
		if (p_node->children)
		{
			p_node = p_node->children;
			return *this;
		}
		// Second check if node has a forward sibling
		if (p_node->next)
		{
			p_node = p_node->next;
			return *this;
		}
		// Third check if node has forward uncle
		auto p_temp = p_node;
		while (p_node->parent)
		{
			if (p_node->parent->next)
			{
				p_node = p_node->parent->next;
				return *this;
			}
			p_node = p_node->parent;
		}
		p_node = p_temp;
		return *this;
	}
	tree_unchecked_const_iterator operator++(int)
	{
		tree_unchecked_const_iterator temp = *this;

		// First check if node has a child
		if (p_node->children)
		{
			p_node = p_node->children;
			return temp;
		}
		// Second check if node has a forward sibling
		if (p_node->next)
		{
			p_node = p_node->next;
			return temp;
		}
		// Third check if node has forward uncle
		auto p_temp = p_node;
		while (p_node->parent)
		{
			if (p_node->parent->next)
			{
				p_node = p_node->parent->next;
				return temp;
			}
			p_node = p_node->parent;
		}
		p_node = p_temp;
		return temp;
	}
	tree_unchecked_const_iterator& operator--()
	{
		// First check for reverse nephew
		auto p_temp = p_node;
		while (p_node->prev)
		{
			if (p_node->prev->children)
			{
				p_node = p_node->prev->children;
				return *this;
			}
			p_node = p_node->prev;
		}
		p_node = p_temp;

		// Second check for reverse sibling
		if (p_node->prev)
		{
			p_node = p_node->prev;
			return *this;
		}

		// Third check for parent
		if (p_node->parent)
		{
			p_node = p_node->parent;
			return *this;
		}

		// Else, this is a head node at the root of the tree
		return *this;
	}
	tree_unchecked_const_iterator operator--(int)
	{
		tree_unchecked_const_iterator temp = *this;

		// First check for reverse nephew
		while (p_node->prev)
		{
			if (p_node->prev->children)
			{
				p_node = p_node->prev->children;
				return temp;
			}
			p_node = p_node->prev;
		}
		p_node = p_temp;

		// Second check for reverse sibling
		if (p_node->prev)
		{
			p_node = p_node->prev;
			return temp;
		}

		// Third check for parent
		if (p_node->parent)
		{
			p_node = p_node->parent;
			return temp;
		}

		// Else, this is a head node at the root of the tree
		return temp;
	}
};

template <class T>
class tree_unchecked_iterator : public tree_unchecked_const_iterator<T>
{
public:
	using base = tree_unchecked_const_iterator<T>;
	using iterator_category = std::bidirectional_iterator_tag;

	using node_ptr = typename T::node_ptr;
	using value_type = typename T::value_type;
	using difference_type = typename T::difference_type;
	using pointer = typename T::pointer;
	using reference = value_type&;

	using base::base;

	[[nodiscard]]
	reference operator*() const
	{
		return const_cast<reference>(base::operator*());
	}
	[[nodiscard]]
	pointer operator->() const
	{
		return std::pointer_traits<pointer>::pointer_to(**this);
	}

	tree_unchecked_iterator& operator++()
	{
		base::operator++();
		return *this;
	}
	tree_unchecked_iterator operator++(int)
	{
		tree_unchecked_iterator temp = *this;
		base::operator++();
		return temp;
	}
	tree_unchecked_iterator& operator--()
	{
		base::operator--();
		return *this;
	}
	tree_unchecked_iterator operator--(int)
	{
		tree_unchecked_iterator temp = *this;
		base::operator--();
		return temp;
	}
};

template <class T>
class tree_const_iterator : public tree_unchecked_const_iterator<T, IteratorBase>
{
public:
	using base = tree_unchecked_const_iterator<T, IteratorBase>;
	using iterator_category = std::bidirectional_iterator_tag;

	using node_ptr = typename T::node_ptr;
	using value_type = typename T::value_type;
	using difference_type = typename T::difference_type;
	using pointer = typename T::const_pointer;
	using reference = const value_type&;

	using base::base;

	[[nodiscard]]
	reference operator*() const
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		const auto _Mycont = static_cast<const T*>(this->_Getcont());
		_STL_ASSERT(_Mycont, "cannot dereference value-initialized tree iterator");
		_STL_VERIFY(this->p_node != _Mycont->p_node, "cannot dereference end tree iterator");
#endif // _ITERATOR_DEBUG_LEVEL == 2

		return this->p_node->value;
	}
	[[nodiscard]]
	pointer operator->() const
	{
		return std::pointer_traits<pointer>::pointer_to(**this);
	}
	[[nodiscard]]
	bool operator==(const tree_const_iterator& rhs) const
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		_STL_VERIFY(this->_Getcont() == rhs._Getcont(), "tree iterators incompatible");
#endif // _ITERATOR_DEBUG_LEVEL == 2

		return this->p_node == rhs.p_node;
	}
	[[nodiscard]] bool operator!=(const tree_const_iterator& rhs) const
	{
		return !(*this == rhs);
	}

	tree_const_iterator& operator++()
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		const auto _Mycont = static_cast<const T*>(this->_Getcont());
		_STL_ASSERT(_Mycont, "cannot increment value-initialized tree iterator");
		_STL_VERIFY(this->p_node != _Mycont->p_node, "cannot increment end tree iterator");
#endif // _ITERATOR_DEBUG_LEVEL == 2

		this->p_node = this->p_node->_Next;
		return *this;
	}
	tree_const_iterator operator++(int)
	{
		tree_const_iterator temp = *this;
		++* this;
		return temp;
	}
	tree_const_iterator& operator--()
	{
		const auto new_node = this->p_node->_Prev;
#if _ITERATOR_DEBUG_LEVEL == 2
		const auto _Mycont = static_cast<const T*>(this->_Getcont());
		_STL_ASSERT(_Mycont, "cannot decrement value-initialized tree iterator");
		_STL_VERIFY(new_node != _Mycont->p_node, "cannot decrement begin tree iterator");
#endif // _ITERATOR_DEBUG_LEVEL == 2

		this->p_node = new_node;
		return *this;
	}
	tree_const_iterator operator--(int)
	{
		tree_const_iterator temp = *this;
		--* this;
		return temp;
	}

#if _ITERATOR_DEBUG_LEVEL == 2
	friend void _Verify_range(const tree_const_iterator& _First, const tree_const_iterator& _Last)
	{
		_STL_VERIFY(_First._Getcont() == _Last._Getcont(), "tree iterators in range are from different containers");
	}
#endif // _ITERATOR_DEBUG_LEVEL == 2

	using _Prevent_inheriting_unwrap = tree_const_iterator;

	[[nodiscard]]
	tree_unchecked_const_iterator<T> _Unwrapped() const
	{
		return tree_unchecked_const_iterator<T>(this->p_node, static_cast<const T*>(this->_Getcont()));
	}

	void _Seek_to(const tree_unchecked_const_iterator<T> iter)
	{
		this->p_node = iter.p_node;
	}
};

template <class T>
class tree_iterator : public tree_const_iterator<T>
{
public:
	using base = tree_const_iterator<T>;
	using iterator_category = std::bidirectional_iterator_tag;

	using node_ptr = typename T::node_ptr;
	using value_type = typename T::value_type;
	using difference_type = typename T::difference_type;
	using pointer = typename T::pointer;
	using reference = value_type&;

	using base::base;

	[[nodiscard]]
	reference operator*() const
	{
		return const_cast<reference>(base::operator*());
	}
	[[nodiscard]]
	pointer operator->() const
	{
		return std::pointer_traits<pointer>::pointer_to(**this);
	}

	tree_iterator& operator++()
	{
		base::operator++();
		return *this;
	}
	tree_iterator operator++(int)
	{
		tree_iterator temp = *this;
		base::operator++();
		return temp;
	}
	tree_iterator& operator--()
	{
		base::operator--();
		return *this;
	}
	tree_iterator operator--(int)
	{
		tree_iterator temp = *this;
		base::operator--();
		return temp;
	}

	using _Prevent_inheriting_unwrap = tree_iterator;

	[[nodiscard]]
	tree_unchecked_iterator<T> _Unwrapped() const
	{
		return tree_unchecked_iterator<T>(this->p_node, static_cast<const T*>(this->_Getcont()));
	}
};

template <class T, class SizeType, class DepthType, class DiffType, class Ptr, class CPtr, class Ref, class CRef, class NodePtr>
struct tree_iterator_types
{
	using value_type = T;
	using size_type = SizeType;
	using depth_type = DepthType;
	using difference_type = DiffType;
	using pointer = Ptr;
	using const_pointer = CPtr;
	using NodeRaw = NodePtr;
};

/******************************************************************************/
/*																			  */
/*							Tree Node										  */
/*																			  */
/*	This class is used by the iterator to understand the container node		  */
/*	relationships, in order to navigate through the tree properly			  */
/*																			  */
/******************************************************************************/
template <class T, class VoidPtr>
struct tree_node // tree node
{
	using value_type = T;
	using NodeRaw = _STD _Rebind_pointer_t<VoidPtr, tree_node>; // Microsoft STL *** INTERNAL ***

	NodeRaw next; // successor node, or first element if head
	NodeRaw prev; // predecessor node, or last element if head
	NodeRaw parent; // parent node of this node
	NodeRaw children; // head node of child node list

	T value; // the stored value, unused if head (if std::allocator is used to construct this stuct, is this T value value set to nullptr by the std::allocator)

	// No copy/copy assign for Nodes
	tree_node(const tree_node&) = delete;
	tree_node& operator=(const tree_node&) = delete;

	template <class Alloc>
	static NodeRaw _alloc_head_node(Alloc& allocator)
	{
		const auto p_node = allocator.allocate(1);

		_STD _Construct_in_place(p_node->next, p_node); // Microsoft STL *** INTERNAL ***
		_STD _Construct_in_place(p_node->prev, p_node); // Microsoft STL *** INTERNAL ***

		// p_node->root should point at the head node of depth 0
		_STD _Construct_in_place(p_node->root, p_node); // Microsoft STL *** INTERNAL ***
		// p_node->parent should point at the head node of this depth -1
		_STD _Construct_in_place(p_node->parent, p_node); // Microsft STL *** INTERNAL ***
		// p_node->children should point at the head node of this node's children
		_STD _Construct_in_place(p_node->children, p_node); // Microsft STL *** INTERNAL ***

		return p_node;
	}

	template <class Alloc>
	static void _free_head_node(Alloc& allocator, NodeRaw p_node) noexcept // destroy pointer members in first_iter and deallocate with allocator
	{
		static_assert(std::is_same_v<typename Alloc::value_type, tree_node>, "Bad _free_head_node call");

		_STD _Destroy_in_place(p_node->next); // Microsoft STL *** INTERNAL ***
		_STD _Destroy_in_place(p_node->prev); // Microsoft STL *** INTERNAL ***


		// Regarding the following code; root will be a reference to another head node (ring),
		// parent will be a reference to another head node (ring), and
		// children will be a reference to another head node (ring), so
		// if we destroy those head nodes, we abandon their node objects.
		// In order to properly clean a tree, we need to get all of the possible children
		// and somehow remove ourselves from the parent/root's children list.
		// We must modify the next/prev pointers of the parent node
		// We must check what the root node is (ambiguous, I know)
		// We must check all children all children all children all children
		_STD _Destroy_in_place(p_node->root); // Microsoft STL *** INTERNAL ***
		_STD _Destroy_in_place(p_node->parent); // Microsoft STL *** INTERNAL ***
		_STD _Destroy_in_place(p_node->children); // Microsoft STL *** INTERNAL ***



		std::allocator_traits<Alloc>::deallocate(allocator, p_node, 1);
	}

	template <class Alloc>
	static void _free_node_value(Alloc& allocator, NodeRaw p_node) noexcept // destroy all members in first_iter and deallocate with allocator
	{
		std::allocator_traits<Alloc>::destroy(allocator, std::addressof(p_node->value));
		_free_head_node(allocator, p_node);
	}

	template <class Alloc>
	static void _free_non_head_node(Alloc& allocator, NodeRaw p_node) noexcept // free a tree starting at the first and terminated at nullptr
	{
		p_node->prev->next = nullptr; // In relation to the first (*this.begin()) node, the prev is the head, so set the next of the head to nullptr, thus breaking the ring
		//p_node->

		auto p_next = p_node->next;
		for (NodeRaw p_iter; p_next; p_next = p_iter)
		{
			p_iter = p_next->next;
			_free_node_value(allocator, p_next);
		}

		// p_node->root
		// p_node->parent
		// p_node->children
	}
};

/******************************************************************************/
/*																			  */
/*							Tree Value										  */
/*																			  */
/*	This class contains the T data											  */
/*																			  */
/******************************************************************************/
template <class T>
class tree_value : public _STD _Container_base // Microsoft STL *** INTERNAL ***
{
public:
	using NodeRaw = typename T::node_ptr;
	using value_type = typename T::value_type;
	using size_type = typename T::size_type;
	using depth_type = typename T::depth_type;
	using difference_type = typename T::difference_type;
	using pointer = typename T::pointer;
	using const_pointer = typename T::const_pointer;
	using reference = value_type&;
	using const_reference = const value_type&;

	NodeRaw p_node; // pointer to node of this value
	size_type m_size; // number of elements
	depth_type depth; // depth in the tree of this value

	tree_value() noexcept : p_node(), m_size(0) {} // initialize data

#pragma region Microsoft STL Internal
	void _Orphan_ptr2(NodeRaw node) noexcept // orphan iterators with specified node pointers
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		_STD _Lockit _Lock(_LOCK_DEBUG);
		_STD _Iterator_base12** first_iter = &this->_Myproxy->_Myfirstiter;
		const auto c_owner = p_node;
		while (*first_iter)
		{
			_STD _Iterator_base12** next_iter = &(*first_iter)->_Mynextiter;
			const auto c_next = static_cast<tree_const_iterator<tree_value>&>(**first_iter).p_node;
			if (c_next == c_owner || c_next != node)
			{
				// iterator is end() or doesn't point at the one we are orphaning, move on
				first_iter = next_iter;
			}
			else // orphan the iterator
			{
				(*first_iter)->_Myproxy = nullptr;
				*first_iter = *next_iter;
			}
		}
#else // ^^^ _ITERATOR_DEBUG_LEVEL == 2 ^^^ // vvv _ITERATOR_DEBUG_LEVEL != 2 vvv
		(void)_Ptr;
#endif // _ITERATOR_DEBUG_LEVEL == 2
	}

	void _Orphan_non_end() noexcept // orphan iterators except end()
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		_STD _Lockit _Lock(_LOCK_DEBUG);
		_STD _Iterator_base12** first_iter = &this->_Myproxy->_Myfirstiter;
		const auto c_owner = p_node;
		while (*first_iter)
		{
			_STD _Iterator_base12** next_iter = &(*first_iter)->_Mynextiter;
			if (static_cast<tree_const_iterator<tree_value>&>(**first_iter).p_node == c_owner)
			{ // iterator is end(), move on
				first_iter = next_iter;
			}
			else // orphan the iterator
			{
				(*first_iter)->_Myproxy = nullptr;
				*first_iter = *next_iter;
			}
		}
#endif // _ITERATOR_DEBUG_LEVEL == 2
	}

	NodeRaw _Unlinknode(NodeRaw node) noexcept // unlink node at _Where from the tree
	{
		_Orphan_ptr2(node);

		node->prev->next = node->next;
		node->next->prev = node->prev;
		--m_size;

		return node;
	}

#if _ITERATOR_DEBUG_LEVEL == 2
	void _Adopt_unique(tree_value& rhs, NodeRaw node) noexcept
	{
		// adopt iterators pointing to the spliced node
		_STD _Lockit _Lock(_LOCK_DEBUG);
		_STD _Iterator_base12** first_iter = &rhs._Myproxy->_Myfirstiter;
		const auto c_proxy = this->_Myproxy;
		while (*first_iter)
		{
			auto& next_iter = static_cast<tree_const_iterator<tree_value>&>(**first_iter);
			if (next_iter.p_node == node) // adopt the iterator
			{
				*first_iter = next_iter._Mynextiter;
				next_iter._Myproxy = c_proxy;
				next_iter._Mynextiter = c_proxy->_Myfirstiter;
				c_proxy->_Myfirstiter = std::addressof(next_iter);
			}
			else // skip the iterator
			{
				first_iter = &next_iter._Mynextiter;
			}
		}
	}

	void _Adopt_all(tree_value& rhs) noexcept
	{
		// adopt all iterators (except rhs.end())
		_STD _Lockit _Lock(_LOCK_DEBUG);
		_STD _Iterator_base12** first_iter = &rhs._Myproxy->_Myfirstiter;
		const auto c_proxy = this->_Myproxy;
		const auto c_first_node = rhs.p_node;
		while (*first_iter)
		{
			auto& next_iter = static_cast<tree_const_iterator<tree_value>&>(**first_iter);
			if (next_iter.p_node != c_first_node) // adopt the iterator
			{
				*first_iter = next_iter._Mynextiter;
				next_iter._Myproxy = c_proxy;
				next_iter._Mynextiter = c_proxy->_Myfirstiter;
				c_proxy->_Myfirstiter = std::addressof(next_iter);
			}
			else // skip the iterator
			{
				first_iter = &next_iter._Mynextiter;
			}
		}
	}

	void _Adopt_range(tree_value& rhs, const NodeRaw first, const NodeRaw last) noexcept
	{
		// adopt all iterators pointing to nodes in the "range" [first, last) by marking nodes
		_STD _Lockit _Lock(_LOCK_DEBUG);
		_STD _Iterator_base12** first_iter = &rhs._Myproxy->_Myfirstiter;
		const auto c_proxy = this->_Myproxy;
		NodeRaw first_prev = first->prev;
		for (NodeRaw element = first; element != last; element = element->next) // mark prev pointers
		{
			element->prev = nullptr;
		}

		while (*first_iter) // check the iterator
		{
			auto& next_iter = static_cast<tree_const_iterator<tree_value>&>(**first_iter);
			if (next_iter.p_node->prev) // skip the iterator
			{
				first_iter = &next_iter._Mynextiter;
			}
			else // adopt the iterator
			{
				*first_iter = next_iter._Mynextiter;
				next_iter._Myproxy = c_proxy;
				next_iter._Mynextiter = c_proxy->_Myfirstiter;
				c_proxy->_Myfirstiter = std::addressof(next_iter);
			}
		}

		for (NodeRaw element = first; element != last; element = element->next) // restore prev pointers
		{
			element->prev = first_prev;
			first_prev = element;
		}
	}
#endif // _ITERATOR_DEBUG_LEVEL == 2

	static NodeRaw _Unchecked_splice(const NodeRaw before, const NodeRaw first, const NodeRaw last) noexcept
	{
		// splice [first, last) before before; returns last
		_STL_INTERNAL_CHECK(before != first && before != last && first != last);

		const auto last_prev = last->prev;
		last_prev->next = before;
		const auto first_prev = first->prev;
		first_prev->next = last;
		const auto before_prev = before->prev;
		before_prev->next = first;

		before->prev = last_prev;
		last->prev = first_prev;
		first->prev = before_prev;

		return last;
	}

	static NodeRaw _Unchecked_splice(const NodeRaw before, const NodeRaw first) noexcept
	{
		// splice [first, first->next) before before; returns first->next
		_STL_INTERNAL_CHECK(before != first && first->next != before);

		const auto last = first->next;
		first->next = before;
		const auto first_prev = first->prev;
		first_prev->next = last;
		const auto before_prev = before->prev;
		before_prev->next = first;

		before->prev = first;
		last->prev = first_prev;
		first->prev = before_prev;

		return last;
	}

	template <class Ptr>
	static NodeRaw _Merge_same(NodeRaw first, NodeRaw mid, const NodeRaw last, Ptr predicate)
	{
		// Merge the sorted ranges [first, mid) and [mid, last)
		// Returns the new beginning of the range (which won't be first if it was spliced elsewhere)
		_STL_INTERNAL_CHECK(first != mid && mid != last);
		NodeRaw new_first;
		if (_DEBUG_LT_PRED(predicate, mid->value, first->value))
		{
			// mid will be spliced to the front of the range
			new_first = mid;
		}
		else
		{
			// Establish predicate(mid->value, first->value) by skipping over elements from the first
			// range already in position.
			new_first = first;
			do {
				first = first->next;
				if (first == mid)
				{
					return new_first;
				}
			} while (!_DEBUG_LT_PRED(predicate, mid->value, first->value));
		}

		for (;;)
		{ // process one run splice
			auto run_start = mid;
			do { // find the end of the "run" of elements we need to splice from the second range into the first
				mid = mid->next;
			} while (mid != last && _DEBUG_LT_PRED(predicate, mid->value, first->value));

			// [run_start, mid) goes before first->value
			_Unchecked_splice(first, run_start, mid);
			if (mid == last)
			{
				return new_first;
			}

			// Reestablish predicate(mid->value, first->value) by skipping over elements from the first
			// range already in position.
			do {
				first = first->next;
				if (first == mid)
				{
					return new_first;
				}
			} while (!_DEBUG_LT_PRED(predicate, mid->value, first->value));
		}
	}

	template <class Ptr>
	static NodeRaw _Sort(NodeRaw& first, const size_type m_size, Ptr predicate)
	{
		// order [first, last), using predicate, return first + size
		switch (m_size)
		{
		case 0:
			return first;
		case 1:
			return first->next;
		default:
			break;
		}

		auto mid = _Sort(first, m_size >> 1, predicate);
		const auto last = _Sort(mid, m_size - (m_size >> 1), predicate);
		first = _Merge_same(first, mid, last, predicate);
		return last;
	}
#pragma endregion
};

/******************************************************************************/
/*																			  */
/*							Tree											  */
/*																			  */
/*	This class is the top-level API for the container						  */
/*																			  */
/******************************************************************************/
template <class T>
struct tree_simple_types // : public _STD _Simple_types<T> // Microsoft STL *** INTERNAL ***
{
	//
	// Originally implemented in ::std::_Simple_types<T>
	//
	using value_type = T;
	using size_type = size_t;
	using depth_type = size_t;
	using difference_type = ptrdiff_t;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	//
	// End of ::std::_Simple_types<T>
	//

	using node = tree_node<T, void*>;
	using NodeRaw = node*;
};

/*
// STRUCT TEMPLATE _Simple_types
template <class _Value_type>
struct _Simple_types { // wraps types from allocators with simple addressing for use in iterators
					   // and other SCARY machinery
	using value_type = _Value_type;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using pointer = value_type*;
	using const_pointer = const value_type*;
};
*/

template <class T, class Alloc = std::allocator<T>>
class tree
{
private:
	template <class>
	friend class _STD _Hash;
	template <class _Traits>
	friend bool _STD _Hash_equal(const _STD _Hash<_Traits>&, const _STD _Hash<_Traits>&);
#if !_HAS_IF_CONSTEXPR
	template <class _Traits>
	friend bool _Hash_equal_elements(const _Hash<_Traits>& _Left, const _Hash<_Traits>& _Right, false_type);
#endif // _HAS_IF_CONSTEXPR

	using allocate_type = _STD _Rebind_alloc_t<Alloc, T>; // Microsoft STL *** INTERNAL ***
	using allocate_type_traits = std::allocator_traits<allocate_type>;
	using node = tree_node<T, typename std::allocator_traits<Alloc>::void_pointer>;
	using allocate_node = _STD _Rebind_alloc_t<Alloc, node>; // Microsoft STL *** INTERNAL ***
	using allocate_node_traits = std::allocator_traits<allocate_node>;
	using NodeRaw = typename allocate_node_traits::pointer;

	using value_types = std::conditional_t<
		_STD _Is_simple_alloc_v<allocate_node>,
		tree_simple_types<T>,
		tree_iterator_types<T,
		typename allocate_type_traits::size_type,
		typename allocate_type_traits::depth_type,
		typename allocate_type_traits::difference_type,
		typename allocate_type_traits::pointer,
		typename allocate_type_traits::const_pointer,
		T&,
		const T&,
		NodeRaw
		>
	>; // Microsoft STL *** INTERNAL ***

	using value = tree_value<value_types>;

public:
	static_assert(!_ENFORCE_MATCHING_ALLOCATORS || std::is_same_v<T, typename Alloc::value_type>,
		_MISMATCHED_ALLOCATOR_MESSAGE("tree<T, A>", "T"));

	using value_type = T;
	using allocator_type = Alloc;
	using size_type = typename allocate_type_traits::size_type;
	using depth_type = typename allocate_type_traits::depth_type;
	using difference_type = typename allocate_type_traits::difference_type;
	using pointer = typename allocate_type_traits::pointer;
	using const_pointer = typename allocate_type_traits::const_pointer;
	using reference = value_type&;
	using const_reference = const value_type&;

	using iterator = tree_iterator<value>;
	using const_iterator = tree_const_iterator<value>;
	using unchecked_iterator = tree_unchecked_iterator<value>;
	using unchecked_const_iterator = tree_unchecked_const_iterator<value>;

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	_STD _Compressed_pair<allocate_node, value> tree_pair; // Microsoft STL *** INTERNAL ***

#pragma region Internal Functions
	void _Reload_sentinel_and_proxy(const tree& _Right) // reload sentinel / proxy from unequal POCCA rhs
	{
		auto& _allocator = _Getal();
		auto&& _Alproxy = static_cast<_STD _Rebind_alloc_t<Alloc, _STD _Container_proxy>>(_allocator);
		auto& _Right_al = _Right._Getal();
		auto&& _Right_alproxy = static_cast<_STD _Rebind_alloc_t<Alloc, _STD _Container_proxy>>(_Right_al);
		_STD _Container_proxy_ptr<allocate_type> _Proxy(_Right_alproxy, _STD _Leave_proxy_unbound{});
		auto _Right_al_non_const = _Right_al;
		auto _Newhead = node::_alloc_head_node(_Right_al_non_const);
		_Tidy();
		_STD _Pocca(_allocator, _Right_al);
		tree_pair._Myval2.p_node = _Newhead;
		tree_pair._Myval2.m_size = 0;
		_Proxy._Bind(_Alproxy, _STD addressof(tree_pair._Myval2));
	}

	void _Move_assign(tree& _Right, _STD _Equal_allocators) noexcept
	{
		clear();
		_STD _Pocma(_Getal(), _Right._Getal());
		_Swap_val(_Right);
	}

	void _Move_assign(tree& _Right, _STD _Propagate_allocators)
	{
		auto& _allocator = _Getal();
		auto& _Right_al = _Right._Getal();
		if (_allocator == _Right_al)
		{
			_Move_assign(_Right, _STD _Equal_allocators{});
		}
		else
		{
			auto&& _Alproxy = static_cast<_STD _Rebind_alloc_t<allocate_node, _STD _Container_proxy>>(_allocator);
			auto&& _Right_alproxy = static_cast<_STD _Rebind_alloc_t<allocate_node, _STD _Container_proxy>>(_Right_al);
			_STD _Container_proxy_ptr<allocate_type> _Proxy(_Right_alproxy, _STD _Leave_proxy_unbound{});
			auto& _My_data = tree_pair._Myval2;
			auto& _Right_data = _Right.tree_pair._Myval2;
			const auto _Newhead = _STD exchange(_Right_data.p_node, node::_alloc_head_node(_Right_al));
			const auto _Newsize = _STD exchange(_Right_data.m_size, size_type{ 0 });
			_Tidy();
			_STD _Pocma(_allocator, _Right_al);
			_My_data.p_node = _Newhead;
			_My_data.m_size = _Newsize;
			_Proxy._Bind(_Alproxy, _STD addressof(_My_data));
			_My_data._Swap_proxy_and_iterators(_Right_data);
		}
	}

	void _Move_assign(tree& _Right, _STD _No_propagate_allocators)
	{
		if (_Getal() == _Right._Getal())
		{
			_Move_assign(_Right, _STD _Equal_allocators{});
		}
		else
		{
			assign(_STD make_move_iterator(_Right._Unchecked_begin()), _STD make_move_iterator(_Right._Unchecked_end()));
		}
	}

	void _Copy_assign(const tree& _Right, std::false_type)
	{
		_STD _Pocca(_Getal(), _Right._Getal());
		assign(_Right._Unchecked_begin(), _Right._Unchecked_end());
	}

	void _Copy_assign(const tree& _Right, std::true_type)
	{
		if (_Getal() != _Right._Getal()) {
			_Reload_sentinel_and_proxy(_Right);
		}

		assign(_Right._Unchecked_begin(), _Right._Unchecked_end());
	}

	void _Swap_val(tree& _Right) noexcept // swap with rhs, same allocator
	{
		auto& _My_data = tree_pair._Myval2;
		auto& _Right_data = _Right.tree_pair._Myval2;
		_My_data._Swap_proxy_and_iterators(_Right_data);
		_STD _Swap_adl(_My_data.p_node, _Right_data.p_node);
		_STD swap(_My_data.m_size, _Right_data.m_size);
	}

	void _Construct_n(_CRT_GUARDOVERFLOW size_type _Count)
	{
		auto&& _Alproxy = static_cast<_STD _Rebind_alloc_t<allocate_node, _STD _Container_proxy>>(_Getal());
		_STD _Container_proxy_ptr<allocate_type> _Proxy(_Alproxy, tree_pair._Myval2);
		tree_node_insert_op<allocate_node> _Appended(_Getal());
		_Appended._Append_n(_Count);
		_Appended._Attach_head(tree_pair._Myval2);
		_Proxy._Release();
	}

	void _Construct_n(_CRT_GUARDOVERFLOW size_type _Count, const T& _Val) {
		auto&& _Alproxy = static_cast<_STD _Rebind_alloc_t<allocate_node, _STD _Container_proxy>>(_Getal());
		_STD _Container_proxy_ptr<allocate_type> _Proxy(_Alproxy, tree_pair._Myval2);
		tree_node_insert_op<allocate_node> _Appended(_Getal());
		_Appended._Append_n(_Count, _Val);
		_Appended._Attach_head(tree_pair._Myval2);
		_Proxy._Release();
	}

	template <class... VT>
	NodeRaw _Emplace(const NodeRaw _Where, VT&&... _Val) // insert element at _Where
	{
		size_type& m_size = tree_pair._Myval2.m_size;
		if (m_size == max_size())
		{
			_STD _Xlength_error("tree too long");
		}

		tree_node_emplace_op<allocate_node> _Op{ _Getal(), _STD forward<VT>(_Val)... };
		++m_size;
		return _Op._Transfer_before(_Where);
	}

	template <class Iter>
	void _Construct_range_unchecked(Iter _First, Iter _Last)
	{
		auto&& _Alproxy = static_cast<_STD _Rebind_alloc_t<allocate_node, _STD _Container_proxy>>(_Getal());
		_STD _Container_proxy_ptr<allocate_type> _Proxy(_Alproxy, tree_pair._Myval2);
		tree_node_insert_op<allocate_node> _Appended(_Getal());
		_Appended._Append_range_unchecked(_First, _Last);
		_Appended._Attach_head(tree_pair._Myval2);
		_Proxy._Release();
	}

	unchecked_iterator _Unchecked_begin() noexcept
	{
		return unchecked_iterator(tree_pair._Myval2.p_node->_Next, nullptr);
	}

	unchecked_const_iterator _Unchecked_begin() const noexcept
	{
		return unchecked_const_iterator(tree_pair._Myval2.p_node->next, nullptr);
	}

	unchecked_iterator _Unchecked_end() noexcept
	{
		return unchecked_iterator(tree_pair._Myval2.p_node, nullptr);
	}

	unchecked_const_iterator _Unchecked_end() const noexcept
	{
		return unchecked_const_iterator(tree_pair._Myval2.p_node, nullptr);
	}

	iterator _Make_iter(NodeRaw _Where) const noexcept
	{
		return iterator(_Where, std::addressof(tree_pair._Myval2));
	}

	const_iterator _Make_const_iter(NodeRaw _Where) const noexcept
	{
		return const_iterator(_Where, std::addressof(tree_pair._Myval2));
	}

private:
	template <class _Target_ref, class _UIter>
	void _Assign_cast(_UIter _UFirst, const _UIter _ULast)
	{
		// assign [_UFirst, _ULast), casting existing nodes to _Target_ref
		const auto _Myend = tree_pair._Myval2.p_node;
		auto _Old = _Myend->_Next;
		for (;;) // attempt to reuse a node
		{
			if (_Old == _Myend) // no more nodes to reuse, append the rest
			{
				tree_node_insert_op<allocate_node> _Op(_Getal());
				_Op._Append_range_unchecked(_UFirst, _ULast);
				_Op._Attach_at_end(tree_pair._Myval2);
				return;
			}

			if (_UFirst == _ULast)
			{
				// input sequence was shorter than existing tree, destroy and deallocate what's left
				_Unchecked_erase(_Old, _Myend);
				return;
			}

			// reuse the node
			reinterpret_cast<_Target_ref>(_Old->value) = *_UFirst;
			_Old = _Old->_Next;
			++_UFirst;
		}
	}

	NodeRaw _Unchecked_erase(const NodeRaw _Pnode) noexcept // erase element at node
	{
		const auto _Result = _Pnode->_Next;
		tree_pair._Myval2._Orphan_ptr2(_Pnode);
		--tree_pair._Myval2.m_size;
		_Pnode->_Prev->_Next = _Result;
		_Result->_Prev = _Pnode->_Prev;
		node::_free_node_value(_Getal(), _Pnode);
		return _Result;
	}

	NodeRaw _Unchecked_erase(NodeRaw _First, const NodeRaw _Last) noexcept // erase [first, last)
	{
		if (_First == _Last)
		{
			return _Last;
		}

		const auto _Predecessor = _First->_Prev;
#if _ITERATOR_DEBUG_LEVEL == 2
		const auto p_head = tree_pair._Myval2.p_node;
		if (_First == p_head->_Next && _Last == p_head) // orphan all non-end iterators
		{
			tree_pair._Myval2._Orphan_non_end();
		}
		else // orphan erased iterators
		{
			_STD _Lockit _Lock(_LOCK_DEBUG);
			for (auto _Marked = _First; _Marked != _Last; _Marked = _Marked->_Next) // mark erased nodes
			{
				_Marked->_Prev = nullptr;
			}

			_STD _Iterator_base12** _Pnext = &tree_pair._Myval2._Myproxy->_Myfirstiter;
			while (*_Pnext)
			{
				_STD _Iterator_base12** _Pnextnext = &(*_Pnext)->_Mynextiter;
				if (static_cast<const_iterator&>(**_Pnext)._Ptr->_Prev) // node still has a prev, skip
				{
					_Pnext = _Pnextnext;
				}
				else // orphan the iterator
				{
					(*_Pnext)->_Myproxy = nullptr;
					*_Pnext = *_Pnextnext;
				}
			}

			// prev pointers not restored because we're about to delete the nodes of which they are a member anyway
		}
#endif // _ITERATOR_DEBUG_LEVEL == 2

		// snip out the removed range
		_Predecessor->_Next = _Last;
		_Last->_Prev = _Predecessor;

		// count and deallocate the removed nodes
		auto& _allocator = _Getal();
		size_type _Erasures = 0;
		do {
			const auto _Next = _First->_Next;
			node::_free_node_value(_allocator, _First);
			_First = _Next;
			++_Erasures;
		} while (_First != _Last);

		tree_pair._Myval2.m_size -= _Erasures;
		return _Last;
	}

	void _Tidy() noexcept
	{
		auto& _allocator = _Getal();
		auto& _My_data = tree_pair._Myval2;
		_My_data._Orphan_all();
		node::_free_non_head_node(_allocator, _My_data.p_node);
		node::_free_head_node(_allocator, _My_data.p_node);
	}

	template <class Ptr>
	void _Merge1(tree& _Right, Ptr _Pred) // merge in elements from rhs, both ordered by predicate
	{
#if _ITERATOR_DEBUG_LEVEL != 0
		_DEBUG_ORDER_UNWRAPPED(_Unchecked_begin(), _Unchecked_end(), _Pred);
#endif // _ITERATOR_DEBUG_LEVEL != 0
		if (this == _STD addressof(_Right))
		{
			return;
		}

#if _ITERATOR_DEBUG_LEVEL != 0
		_DEBUG_ORDER_UNWRAPPED(_Right._Unchecked_begin(), _Right._Unchecked_end(), _Pred);
		if _CONSTEXPR_IF(!allocate_node_traits::is_always_equal::value)
		{
			_STL_VERIFY(_Getal() == _Right._Getal(), "tree allocators incompatible for merge");
		}
#endif // _ITERATOR_DEBUG_LEVEL != 0

		const auto _Right_size = _Right.tree_pair._Myval2.m_size;
		if (_Right_size == 0) // nothing to do
		{
			return;
		}

		// splice all rhs's nodes on the end of *this
		const auto p_node = tree_pair._Myval2.p_node;
		const auto _Right_head = _Right.tree_pair._Myval2.p_node;
		const auto _Mid = _Right_head->_Next;
		_Splice(p_node, _Right, _Mid, _Right_head, _Right_size);

		// if *this had any elements, run the merge op between the range we just spliced and the old elements
		if (p_node->_Next != _Mid)
		{
			value::_Merge_same(p_node->_Next, _Mid, p_node, _Pred);
		}
	}

	NodeRaw _Splice(const NodeRaw _Where, tree& _Right, const NodeRaw _First, const NodeRaw _Last, const size_type _Count) // splice rhs [first, last) before _Where; returns last
	{
		if (this != _STD addressof(_Right)) // splicing from another tree, adjust counts
		{
#if _ITERATOR_DEBUG_LEVEL != 0
			if _CONSTEXPR_IF(!allocate_node_traits::is_always_equal::value)
			{
				_STL_VERIFY(_Getal() == _Right._Getal(), "tree allocators incompatible for splice");
			}
#endif // _ITERATOR_DEBUG_LEVEL != 0

			auto& _My_data = tree_pair._Myval2;
			if (max_size() - _My_data.m_size < _Count)
			{
				_STD _Xlength_error("tree too long");
			}

			auto& _Right_data = _Right.tree_pair._Myval2;
#if _ITERATOR_DEBUG_LEVEL == 2
			// transfer ownership
			if (_Count == 1)
			{
				_My_data._Adopt_unique(_Right_data, _First);
			}
			else if (_Count == _Right_data.m_size)
			{
				_My_data._Adopt_all(_Right_data);
			}
			else
			{
				_My_data._Adopt_range(_Right_data, _First, _Last);
			}
#endif // _ITERATOR_DEBUG_LEVEL == 2

			_My_data.m_size += _Count;
			_Right_data.m_size -= _Count;
		}

		return value::_Unchecked_splice(_Where, _First, _Last);
	}

	void _Alloc_sentinel_and_proxy()
	{
		auto&& _Alproxy = static_cast<_STD _Rebind_alloc_t<Alloc, _STD _Container_proxy>>(_Getal());
		_STD _Container_proxy_ptr<allocate_type> _Proxy(_Alproxy, tree_pair._Myval2);
		auto& _allocator = _Getal();
		auto _Newhead = _allocator.allocate(1);
		_STD _Construct_in_place(_Newhead->_Next, _Newhead);
		_STD _Construct_in_place(_Newhead->_Prev, _Newhead);
		tree_pair._Myval2.p_node = _Newhead;
		_Proxy._Release();
	}

	void _Orphan_all() noexcept
	{
		tree_pair._Myval2._Orphan_all();
	}

	Alloc& _Getal() noexcept
	{
		return tree_pair._Get_first();
	}

	const Alloc& _Getal() const noexcept
	{
		return tree_pair._Get_first();
	}
#pragma endregion

#pragma region Constructors
public:
	tree() : tree_pair(_STD _Zero_then_variadic_args_t())
	{
		_Alloc_sentinel_and_proxy();
	}

	explicit tree(const Alloc& _allocator) : tree_pair(_STD _One_then_variadic_args_t(), _allocator)
	{
		_Alloc_sentinel_and_proxy();
	}

private:
	template <class _Any_alloc>
	explicit tree(_STD _Move_allocator_tag, _Any_alloc& _allocator) : tree_pair(_STD _One_then_variadic_args_t(), _STD move(_allocator))
	{
		_Alloc_sentinel_and_proxy();
	}

public:
	explicit tree(_CRT_GUARDOVERFLOW size_type _Count) : tree_pair(_STD _Zero_then_variadic_args_t()) // construct tree from count * T()
	{
		_Construct_n(_Count);
	}

	explicit tree(_CRT_GUARDOVERFLOW size_type _Count, const Alloc& _allocator) : tree_pair(_STD _One_then_variadic_args_t(), _allocator) // construct tree from count * T(), with allocator
	{
		_Construct_n(_Count);
	}

	tree(_CRT_GUARDOVERFLOW size_type _Count, const T& _Val) : tree_pair(_STD _Zero_then_variadic_args_t()) // construct tree from count * _Val
	{
		_Construct_n(_Count, _Val);
	}

	tree(_CRT_GUARDOVERFLOW size_type _Count, const T& _Val, const Alloc& _allocator) : tree_pair(_STD _One_then_variadic_args_t(), _allocator) // construct tree from count * _Val, allocator
	{
		_Construct_n(_Count, _Val);
	}

	tree(const tree& rhs) : tree_pair(_STD _One_then_variadic_args_t(), allocate_node_traits::select_on_container_copy_construction(rhs._Getal()))
	{
		_Construct_range_unchecked(rhs._Unchecked_begin(), rhs._Unchecked_end());
	}

	tree(const tree& rhs, const Alloc& _allocator) : tree_pair(_STD _One_then_variadic_args_t(), _allocator)
	{
		_Construct_range_unchecked(rhs._Unchecked_begin(), rhs._Unchecked_end());
	}

	tree& operator=(const tree& rhs)
	{
		if (this != std::addressof(rhs))
		{
			_Copy_assign(rhs, _STD _Choose_pocca<allocate_node>{});
		}

		return *this;
	}

	tree(tree&& rhs) : tree_pair(_STD _One_then_variadic_args_t(), _STD move(rhs._Getal()))
	{
		_Alloc_sentinel_and_proxy();
		_Swap_val(rhs);
	}

	tree(tree&& rhs, const Alloc& _allocator) : tree_pair(_STD _One_then_variadic_args_t(), _allocator)
	{
		if _CONSTEXPR_IF(!allocate_node_traits::is_always_equal::value)
		{
			if (_Getal() != rhs._Getal())
			{
				_Construct_range_unchecked(_STD make_move_iterator(rhs._Unchecked_begin()), _STD make_move_iterator(rhs._Unchecked_end()));
				return;
			}
		}

		_Alloc_sentinel_and_proxy();
		_Swap_val(rhs);
	}

	tree& operator=(tree&& rhs) noexcept(noexcept(_Move_assign(rhs, _STD _Choose_pocma<allocate_node>{}))) /* strengthened */
	{
		if (this != std::addressof(rhs))
		{
			_Move_assign(rhs, _STD _Choose_pocma<allocate_node>{});
		}

		return *this;
	}

	template <class Iter, class = std::enable_if_t<_STD _Is_iterator_v<Iter>>> // Microsoft STL *** INTERNAL ***
	tree(Iter _First, Iter _Last) : tree_pair(_STD _Zero_then_variadic_args_t())
	{
		_STD _Adl_verify_range(_First, _Last);
		_Construct_range_unchecked(_Get_unwrapped(_First), _Get_unwrapped(_Last));
	}

	template <class Iter, class = std::enable_if_t<_STD _Is_iterator_v<Iter>>> // Microsoft STL *** INTERNAL ***
	tree(Iter _First, Iter _Last, const Alloc& _allocator) : tree_pair(_STD _One_then_variadic_args_t(), _allocator)
	{
		_STD _Adl_verify_range(_First, _Last);
		_Construct_range_unchecked(_Get_unwrapped(_First), _Get_unwrapped(_Last));
	}

	tree(std::initializer_list<T> tree) : tree_pair(_STD _Zero_then_variadic_args_t())
	{
		_Construct_range_unchecked(tree.begin(), tree.m_end());
	}

	tree(std::initializer_list<T> tree, const Alloc& _allocator) : tree_pair(_STD _One_then_variadic_args_t(), _allocator)
	{
		_Construct_range_unchecked(tree.begin(), tree.m_end());
	}

	tree& operator=(std::initializer_list<T> tree)
	{
		assign(tree.begin(), tree.m_end());
		return *this;
	}

	~tree() noexcept
	{
		_Tidy();
#if _ITERATOR_DEBUG_LEVEL != 0 // TRANSITION, ABI
		auto&& _Alproxy = static_cast<_STD _Rebind_alloc_t<Alloc, _STD _Container_proxy>>(_Getal());
		_Delete_plain_internal(_Alproxy, tree_pair._Myval2._Myproxy);
#endif // _ITERATOR_DEBUG_LEVEL != 0
	}
#pragma endregion

#pragma region Iterators, Size
public:
	[[nodiscard]] iterator begin() noexcept
	{
		return iterator(tree_pair._Myval2.p_node->next, _STD addressof(tree_pair._Myval2));
	}

	[[nodiscard]] const_iterator begin() const noexcept
	{
		return const_iterator(tree_pair._Myval2.p_node->next, _STD addressof(tree_pair._Myval2));
	}

	[[nodiscard]] iterator m_end() noexcept
	{
		// TODO: Needs to know about the end of the tree
		return iterator(tree_pair._Myval2.p_node, _STD addressof(tree_pair._Myval2));
	}

	[[nodiscard]] const_iterator m_end() const noexcept
	{
		// TODO: Needs to know about the end of the tree
		return const_iterator(tree_pair._Myval2.p_node, _STD addressof(tree_pair._Myval2));
	}

	[[nodiscard]] reverse_iterator rbegin() noexcept
	{
		return reverse_iterator(m_end());
	}

	[[nodiscard]] const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator(m_end());
	}

	[[nodiscard]] reverse_iterator rend() noexcept
	{
		return reverse_iterator(begin());
	}

	[[nodiscard]] const_reverse_iterator rend() const noexcept
	{
		return const_reverse_iterator(begin());
	}

	[[nodiscard]] const_iterator cbegin() const noexcept
	{
		return begin();
	}

	[[nodiscard]] const_iterator cend() const noexcept
	{
		return m_end();
	}

	[[nodiscard]] const_reverse_iterator crbegin() const noexcept
	{
		return rbegin();
	}

	[[nodiscard]] const_reverse_iterator crend() const noexcept
	{
		return rend();
	}

	[[nodiscard]] allocator_type get_allocator() const noexcept
	{
		return static_cast<allocator_type>(_Getal());
	}

	[[nodiscard]] bool empty() const noexcept
	{
		return tree_pair._Myval2.m_size == 0;
	}

	[[nodiscard]] size_type m_size() const noexcept
	{
		return tree_pair._Myval2.m_size;
	}

	[[nodiscard]] size_type max_size() const noexcept
	{
		return _STD _Min_value(static_cast<size_type>((std::numeric_limits<difference_type>::max)()), allocate_node_traits::max_size(_Getal()));
	}

	void resize(_CRT_GUARDOVERFLOW size_type _Newsize) // determine new length, padding with T() elements as needed
	{
		auto& _My_data = tree_pair._Myval2;
		if (_My_data.m_size < _Newsize) // pad to make larger
		{
			tree_node_insert_op<allocate_node> _Op(_Getal());
			_Op._Append_n(_Newsize - _My_data.m_size);
			_Op._Attach_at_end(_My_data);
		}
		else
		{
			while (_Newsize < _My_data.m_size)
			{
				pop_back();
			}
		}
	}

	void resize(_CRT_GUARDOVERFLOW size_type _Newsize, const T& _Val)
	{
		// determine new length, padding with _Val elements as needed
		auto& _My_data = tree_pair._Myval2;
		if (_My_data.m_size < _Newsize) // pad to make larger
		{
			tree_node_insert_op<allocate_node> _Op(_Getal());
			_Op._Append_n(_Newsize - _My_data.m_size, _Val);
			_Op._Attach_at_end(_My_data);
		}
		else
		{
			while (_Newsize < _My_data.m_size)
			{
				pop_back();
			}
		}
	}

	void clear() noexcept // erase all
	{
		auto& _My_data = tree_pair._Myval2;
		_My_data._Orphan_non_end();
		node::_free_non_head_node(_Getal(), _My_data.p_node);
		_My_data.p_node->_Next = _My_data.p_node;
		_My_data.p_node->_Prev = _My_data.p_node;
		_My_data.m_size = 0;
	}
#pragma endregion

#pragma region Front, Back
public:
	[[nodiscard]] reference front() noexcept /* strengthened */
	{
#if _CONTAINER_DEBUG_LEVEL > 0
		_STL_VERIFY(tree_pair._Myval2.m_size != 0, "front() called on empty tree");
#endif // _CONTAINER_DEBUG_LEVEL > 0

		return tree_pair._Myval2.p_node->next->value; // For <list> this returns the value of the node to the right of the head of the list which has no value
	}

	[[nodiscard]] const_reference front() const noexcept /* strengthened */
	{
#if _CONTAINER_DEBUG_LEVEL > 0
		_STL_VERIFY(tree_pair._Myval2.m_size != 0, "front() called on empty tree");
#endif // _CONTAINER_DEBUG_LEVEL > 0

		return tree_pair._Myval2.p_node->next->value;
	}

	[[nodiscard]] reference back() noexcept /* strengthened */
	{
#if _CONTAINER_DEBUG_LEVEL > 0
		_STL_VERIFY(tree_pair._Myval2.m_size != 0, "back() called on empty tree");
#endif // _CONTAINER_DEBUG_LEVEL > 0

		return tree_pair._Myval2.p_node->prev->value;
	}

	[[nodiscard]] const_reference back() const noexcept /* strengthened */
	{
#if _CONTAINER_DEBUG_LEVEL > 0
		_STL_VERIFY(tree_pair._Myval2.m_size != 0, "back() called on empty tree");
#endif // _CONTAINER_DEBUG_LEVEL > 0

		return tree_pair._Myval2.p_node->prev->value;
	}

	void push_front(const T& _Val)
	{
		_Emplace(tree_pair._Myval2.p_node->_Next, _Val);
	}

	void push_back(const T& _Val)
	{
		_Emplace(tree_pair._Myval2.p_node, _Val);
	}

	void push_front(T&& _Val) // insert element at beginning
	{
		_Emplace(tree_pair._Myval2.p_node->_Next, _STD move(_Val));
	}

	void push_back(T&& _Val) // insert element at end
	{
		_Emplace(tree_pair._Myval2.p_node, _STD move(_Val));
	}

	void pop_front() noexcept /* strengthened */
	{
#if _CONTAINER_DEBUG_LEVEL > 0
		_STL_VERIFY(tree_pair._Myval2.m_size != 0, "pop_front called on empty tree");
#endif // _CONTAINER_DEBUG_LEVEL > 0

		_Unchecked_erase(tree_pair._Myval2.p_node->_Next);
	}

	void pop_back() noexcept /* strengthened */
	{
#if _CONTAINER_DEBUG_LEVEL > 0
		_STL_VERIFY(tree_pair._Myval2.m_size != 0, "pop_back called on empty tree");
#endif // _CONTAINER_DEBUG_LEVEL > 0

		_Unchecked_erase(tree_pair._Myval2.p_node->_Prev);
	}
#pragma endregion

#pragma region Insert, Assign, Emplace
public:
	iterator insert(const_iterator _Where, T&& _Val) // insert _Val at _Where
	{
		return emplace(_Where, _STD move(_Val));
	}

	iterator insert(const_iterator _Where, std::initializer_list<T> _Itree) // insert initializer_list
	{
		return insert(_Where, _Itree.begin(), _Itree.m_end());
	}

	iterator insert(const_iterator _Where, const T& _Val) // insert _Val at _Where
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		_STL_VERIFY(_Where._Getcont() == _STD addressof(tree_pair._Myval2), "tree insert iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2
		return _Make_iter(_Emplace(_Where._Ptr, _Val));
	}

	iterator insert(const_iterator _Where, _CRT_GUARDOVERFLOW size_type _Count, const T& _Val) // insert count * _Val before _Where
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		_STL_VERIFY(_Where._Getcont() == _STD addressof(tree_pair._Myval2), "tree insert iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2
		tree_node_insert_op<allocate_node> _Op(_Getal());
		_Op._Append_n(_Count, _Val);
		return _Make_iter(_Op._Attach_before(tree_pair._Myval2, _Where._Ptr));
	}

	template <class Iter, class = std::enable_if_t<_STD _Is_iterator_v<Iter>>>
	iterator insert(const const_iterator _Where, Iter _First, Iter _Last) // insert [first, last) before _Where
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		_STL_VERIFY(_Where._Getcont() == _STD addressof(tree_pair._Myval2), "tree insert iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2
		_STD _Adl_verify_range(_First, _Last);
		tree_node_insert_op<allocate_node> _Op(_Getal());
		_Op._Append_range_unchecked(_Get_unwrapped(_First), _Get_unwrapped(_Last));
		return _Make_iter(_Op._Attach_before(tree_pair._Myval2, _Where._Ptr));
	}

	void assign(std::initializer_list<T> tree)
	{
		assign(tree.begin(), tree.m_end());
	}

	template <class Iter, class = std::enable_if_t<_STD _Is_iterator_v<Iter>>>
	void assign(Iter _First, Iter _Last)
	{
		_Assign_cast<reference>(_Get_unwrapped(_First), _Get_unwrapped(_Last));
	}

	void assign(_CRT_GUARDOVERFLOW size_type _Count, const T& _Val) // assign count * _Val
	{
		const auto _Myend = tree_pair._Myval2.p_node;
		auto _Old = _Myend->_Next;
		for (;;) // attempt to reuse a node
		{
			if (_Old == _Myend) // no more nodes to reuse, append the rest
			{
				tree_node_insert_op<allocate_node> _Op(_Getal());
				_Op._Append_n(_Count, _Val);
				_Op._Attach_at_end(tree_pair._Myval2);
				return;
			}

			if (_Count == 0)
			{
				// input sequence was shorter than existing tree, destroy and deallocate what's left
				_Unchecked_erase(_Old, _Myend);
				return;
			}

			// reuse the node
			_Old->value = _Val;
			_Old = _Old->_Next;
			--_Count;
		}
	}

	template <class... VT>
	decltype(auto) emplace_front(VT&&... _Val) // insert element at beginning
	{
		reference _Result = _Emplace(tree_pair._Myval2.p_node->_Next, _STD forward<VT>(_Val)...)->value;

#if _HAS_CXX17
		return _Result;
#else // ^^^ _HAS_CXX17 // !_HAS_CXX17 vvv
		(void)_Result;
#endif // _HAS_CXX17
	}

	template <class... VT>
	decltype(auto) emplace_back(VT&&... _Val) // insert element at end
	{
		reference _Result = _Emplace(tree_pair._Myval2.p_node, _STD forward<VT>(_Val)...)->value;

#if _HAS_CXX17
		return _Result;
#else // ^^^ _HAS_CXX17 // !_HAS_CXX17 vvv
		(void)_Result;
#endif // _HAS_CXX17
	}

	template <class... VT>
	iterator emplace(const const_iterator _Where, VT&&... _Val) // insert element at _Where
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		_STL_VERIFY(_Where._Getcont() == _STD addressof(tree_pair._Myval2), "tree emplace iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2

		return _Make_iter(_Emplace(_Where._Ptr, _STD forward<VT>(_Val)...));
	}
#pragma endregion

#pragma region Swap, Splice, Merge
public:
	void swap(tree& rhs) noexcept /* strengthened */
	{
		if (this != std::addressof(rhs))
		{
			_STD _Pocs(_Getal(), rhs._Getal());
			_Swap_val(rhs);
		}
	}

	void splice(const const_iterator _Where, tree& _Right) // splice all of rhs at _Where
	{
		auto& _Right_data = _Right.tree_pair._Myval2;
		if (this != _STD addressof(_Right) && _Right_data.m_size != 0) // worth splicing, do it
		{
#if _ITERATOR_DEBUG_LEVEL == 2
			_STL_VERIFY(_Where._Getcont() == _STD addressof(tree_pair._Myval2), "tree splice iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2
			const auto _Right_head = _Right_data.p_node;
			_Splice(_Where._Ptr, _Right, _Right_head->_Next, _Right_head, _Right_data.m_size);
		}
	}

	void splice(const const_iterator _Where, tree&& _Right) // splice all of rhs at _Where
	{
		splice(_Where, _Right);
	}

	void splice(const const_iterator _Where, tree& _Right, const const_iterator _First) // splice rhs [first, first + 1) at _Where
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		_STL_VERIFY(_Where._Getcont() == _STD addressof(tree_pair._Myval2)
			&& _First._Getcont() == _STD addressof(_Right.tree_pair._Myval2), "tree splice iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2
		const auto _UWhere = _Where._Ptr;
		const auto _UFirst = _First._Ptr;

#if _ITERATOR_DEBUG_LEVEL == 2
		if (_UFirst == _Right.tree_pair._Myval2.p_node)
		{
			_STL_REPORT_ERROR("tree splice iterator outside range");
		}
#endif // _ITERATOR_DEBUG_LEVEL == 2

		const auto _ULast = _UFirst->_Next;
		if (this != _STD addressof(_Right) || (_UWhere != _UFirst && _UWhere != _ULast))
		{
			_Splice(_UWhere, _Right, _UFirst, _ULast, 1);
		}
	}

	void splice(const const_iterator _Where, tree&& _Right, const const_iterator _First) // splice rhs [first, first + 1) at _Where
	{
		splice(_Where, _Right, _First);
	}

	void splice(const const_iterator _Where, tree& _Right, const const_iterator _First, const const_iterator _Last) // splice rhs [first, last) at _Where
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		const auto _Right_data_ptr = _STD addressof(_Right.tree_pair._Myval2);
		_STL_VERIFY(_Where._Getcont() == _STD addressof(tree_pair._Myval2) && _First._Getcont() == _Right_data_ptr
			&& _Last._Getcont() == _Right_data_ptr, "tree splice iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2

		const auto _UWhere = _Where._Ptr;
		const auto _UFirst = _First._Ptr;
		const auto _ULast = _Last._Ptr;

		if (_UFirst != _ULast && (this != _STD addressof(_Right) || _UWhere != _ULast)) // worth splicing, do it
		{
			size_type _Count = 0;

			if (this != _STD addressof(_Right))
			{
				const auto _Right_end = _Right.tree_pair._Myval2.p_node;
				if (_UFirst == _Right_end->_Next && _ULast == _Right_end)
				{
					_Count = _Right.tree_pair._Myval2.m_size; // splice in whole list
				}
				else // count nodes and check for knot
				{
					for (auto _To_check = _UFirst; _To_check != _ULast; _To_check = _To_check->_Next, (void) ++_Count)
					{
#if _ITERATOR_DEBUG_LEVEL != 0
						_STL_VERIFY(_To_check != _Right_end, "tree bad splice");
#endif // _ITERATOR_DEBUG_LEVEL != 0
					}
				}
			}

			_Splice(_UWhere, _Right, _UFirst, _ULast, _Count);
		}
	}

	void splice(const const_iterator _Where, tree&& _Right, const const_iterator _First, const const_iterator _Last) // splice rhs [first, last) at _Where
	{
		splice(_Where, _Right, _First, _Last);
	}

	void merge(tree& _Right) // merge in elements from rhs, both ordered by operator<
	{
		_Merge1(_Right, std::less<>());
	}

	void merge(tree&& _Right) // merge in elements from rhs, both ordered by operator<
	{
		_Merge1(_Right, std::less<>());
	}

	template <class Ptr>
	void merge(tree& _Right, Ptr _Pred) // merge in elements from rhs, both ordered by predicate
	{
		_Merge1(_Right, _STD _Pass_fn(_Pred));
	}

	template <class Ptr>
	void merge(tree&& _Right, Ptr _Pred) // merge in elements from rhs, both ordered by predicate
	{
		_Merge1(_Right, _STD _Pass_fn(_Pred));
	}
#pragma endregion

#pragma region Algorithms
	iterator erase(const const_iterator _Where) noexcept /* strengthened */
	{
#if _ITERATOR_DEBUG_LEVEL == 2
		_STL_VERIFY(_Where._Getcont() == _STD addressof(tree_pair._Myval2), "tree erase iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2
		const auto _Result = _Where._Ptr->_Next;
		node::_free_node_value(_Getal(), tree_pair._Myval2._Unlinknode(_Where._Ptr));
		return _Make_iter(_Result);
	}

	iterator erase(const const_iterator _First, const const_iterator _Last) noexcept /* strengthened */
	{
		_STD _Adl_verify_range(_First, _Last);
		return _Make_iter(_Unchecked_erase(_First._Ptr, _Last._Ptr));
	}

	auto remove(const T& _Val) // erase each element matching _Val
	{
		// TODO: Does this need CXX17 to select std vs remove_if defined next?
		return remove_if([&](const T& _Other) { return _Other == _Val; });
	}

	template <class _Pr1>
	auto remove_if(_Pr1 _Pred) // erase each element satisfying predicate
	{
		auto& _My_data = tree_pair._Myval2;
		tree_node_remove_op _Op(*this);
		const auto _Last = _My_data.p_node;
		const size_type _Oldsize = _My_data.m_size;
		for (auto _First = _Last->_Next; _First != _Last;)
		{
			auto _Next = _First->_Next;
			if (_Pred(_First->value))
			{
				_Op._Transfer_back(_First);
			}

			_First = _Next;
		}

#if _HAS_CXX20
		return _Oldsize - _My_data._Mysize;
#else // _HAS_CXX20
		(void)_Oldsize;
#endif // _HAS_CXX20
	}

	auto unique() // erase each element matching previous
	{
		return unique(std::equal_to<>());
	}

	template <class Ptr>
	auto unique(Ptr _Pred) // erase each element satisfying predicate with previous
	{
		tree_node_remove_op _Op(*this);
		const NodeRaw _Phead = tree_pair._Myval2.p_node;
		NodeRaw _Pprev = _Phead->_Next;
		NodeRaw _Pnode = _Pprev->_Next;
		const size_type _Oldsize = tree_pair._Myval2.m_size;
		while (_Pnode != _Phead) {
			if (_Pred(_Pprev->value, _Pnode->value)) // match, remove it
			{
				_Pnode = _Op._Transfer_back(_Pnode);
			}
			else // no match, advance
			{
				_Pprev = _Pnode;
				_Pnode = _Pnode->_Next;
			}
		}

#if _HAS_CXX20
		return _Oldsize - tree_pair._Myval2._Mysize;
#else // _HAS_CXX20
		(void)_Oldsize;
#endif // _HAS_CXX20
	}

	void sort() // order sequence, using operator<
	{
		sort(std::less<>());
	}

	template <class Ptr>
	void sort(Ptr _Pred) // order sequence, using predicate
	{
		auto& _My_data = tree_pair._Myval2;
		value::_Sort(_My_data.p_node->_Next, _My_data.m_size, _Pass_fn(_Pred));
	}

	void reverse() noexcept // reverse sequence
	{
		const NodeRaw _Phead = tree_pair._Myval2.p_node;
		NodeRaw _Pnode = _Phead;

		for (;;) // flip pointers in a node
		{
			const NodeRaw _Pnext = _Pnode->_Next;
			_Pnode->_Next = _Pnode->_Prev;
			_Pnode->_Prev = _Pnext;

			if (_Pnext == _Phead)
			{
				break;
			}

			_Pnode = _Pnext;
		}
	}
#pragma endregion

	/******************************************************************************/
	/*																			  */
	/*							Tree Node										  */
	/*																			  */
	/*	The next three structs represent complex operation brokers for the nodes  */
	/*																			  */
	/******************************************************************************/
	struct tree_node_remove_op
	{
		tree& _tree;
		NodeRaw p_head; // singly linked list of nodes to remove; their prev pointers set to NodeRaw()
		NodeRaw* p_tail;

		explicit tree_node_remove_op(tree& tree) noexcept : _tree(tree), p_head(), p_tail(std::addressof(p_head)) {}

		tree_node_remove_op(const tree_node_remove_op&) = delete;
		tree_node_remove_op& operator=(const tree_node_remove_op&) = delete;

		NodeRaw _Transfer_back(const NodeRaw removed) noexcept
		{
			// extract removed from the tree, and add it to the singly-linked list of nodes to destroy
			// returns the node after removed
			_STL_INTERNAL_CHECK(_tree.tree_pair._Myval2.p_node != removed);

			// snip the node out
			--_tree.tree_pair._Myval2.m_size;
			const auto c_next = std::exchange(removed->next, NodeRaw());
			const auto c_prev = removed->prev;
			c_prev->next = c_next;
			c_next->prev = c_prev;

#if _ITERATOR_DEBUG_LEVEL == 2
			// mark removed node for IDL to snip out later
			removed->prev = NodeRaw();
#endif // _ITERATOR_DEBUG_LEVEL == 2

			* p_tail = removed;
			p_tail = std::addressof(removed->next);

			return c_next;
		}

		~tree_node_remove_op()
		{
			auto& _allocator = _tree._Getal();

#if _ITERATOR_DEBUG_LEVEL == 2
			{
				_STD _Lockit _Lock(_LOCK_DEBUG);
				_STD _Iterator_base12** _Pnext = &_tree.tree_pair._Myval2._Myproxy->_Myfirstiter;
				while (*_Pnext) {
					_STD _Iterator_base12** _Pnextnext = &(*_Pnext)->_Mynextiter;
					const auto _Pnextptr = static_cast<const_iterator&>(**_Pnext)._Ptr;
					if (_Pnextptr->_Prev) // iterator doesn't point to one of the elements we're removing
					{
						_Pnext = _Pnextnext;
					}
					else // orphan the iterator
					{
						(*_Pnext)->_Myproxy = nullptr;
						*_Pnext = *_Pnextnext;
					}
				}
			}
#endif // _ITERATOR_DEBUG_LEVEL == 2

			auto _Target = p_head;
			while (_Target) {
				auto _Next = _Target->_Next;
				_tree.tree_pair._Myval2._Orphan_ptr2(_Target);
				allocate_node_traits::destroy(_allocator, _STD addressof(_Target->_Next));
				allocate_node_traits::destroy(_allocator, _STD addressof(_Target->_Prev));
				allocate_node_traits::destroy(_allocator, _STD addressof(_Target->value));
				_allocator.deallocate(_Target, 1);
				_Target = _Next;
			}
		}
	};
};

template <class Alloc>
struct tree_node_emplace_op : _STD _Alloc_construct_ptr<Alloc> // Microsoft STL *** INTERNAL ***
{
	using allocate_node_traits = std::allocator_traits<Alloc>;
	using pointer = typename allocate_node_traits::pointer;

	template <class... T>
	explicit tree_node_emplace_op(Alloc& allocator, T&&... values) : _STD _Alloc_construct_ptr<Alloc>(allocator) // Microsoft STL *** INTERNAL ***
	{
		this->_Allocate(); // Microsoft STL *** INTERNAL ***
		allocate_node_traits::construct(this->_allocator, std::addressof(this->p_node->value), std::forward<T>(values)...);
	}

	~tree_node_emplace_op()
	{
		if (this->p_node != pointer{})
		{
			allocate_node_traits::destroy(this->_allocator, std::addressof(this->p_node->value));
		}
	}

	tree_node_emplace_op(const tree_node_emplace_op&) = delete;
	tree_node_emplace_op& operator=(const tree_node_emplace_op&) = delete;

	pointer _Transfer_before(const pointer _Insert_before) noexcept
	{
		const pointer _Insert_after = _Insert_before->prev;
		_STD _Construct_in_place(this->p_node->next, _Insert_before);
		_STD _Construct_in_place(this->p_node->prev, _Insert_after);
		const auto _Result = this->p_node;
		this->p_node = pointer{};
		_Insert_before->prev = _Result;
		_Insert_after->next = _Result;
		return _Result;
	}
};

template <class Alloc>
struct tree_node_insert_op
{
	// tree insert operation which maintains exception safety
	using allocate_node_traits = std::allocator_traits<Alloc>;
	using pointer = typename allocate_node_traits::pointer;
	using size_type = typename allocate_node_traits::size_type;
	using value_type = typename allocate_node_traits::value_type;

private:
	Alloc& _allocator;
	pointer p_tail; // points to the most recently appended element; it doesn't have next constructed
	size_type added;

	union
	{
		value_type base; // only ever uses next, other members not constructed
						  // next points to the first appended element
	};

public:
	explicit tree_node_insert_op(Alloc& allocator) : _allocator(allocator), p_tail(std::pointer_traits<pointer>::pointer_to(base)), added(0) {}

	tree_node_insert_op(const tree_node_insert_op&) = delete;
	tree_node_insert_op& operator=(const tree_node_insert_op&) = delete;

	template <class... TArgs>
	void _Append_n(size_type count, const TArgs&... args)
	{
		// Append count Ts constructed from args to this insert operation.
		_STD _Alloc_construct_ptr<Alloc> _Newnode(_allocator);
		for (; 0 < count; --count)
		{
			_Newnode._Allocate(); // throws
			allocate_node_traits::construct(_allocator, _STD addressof(_Newnode._Ptr->value), args...); // throws
			_STD _Construct_in_place(p_tail->_Next, _Newnode._Ptr);
			_STD _Construct_in_place(_Newnode._Ptr->_Prev, p_tail);
			p_tail = _Newnode._Ptr;
			++added;
		}

		_Newnode._Ptr = pointer{};
	}

	template <class _InIt, class _Sentinel>
	void _Append_range_unchecked(_InIt _First, const _Sentinel _Last)
	{
		// Append new elements constructed from [first, last) to this insert operation.
		_STD _Alloc_construct_ptr<Alloc> _Newnode(_allocator);
		for (; _First != _Last; ++_First)
		{
			_Newnode._Allocate(); // throws
			allocate_node_traits::construct(_allocator, _STD addressof(_Newnode._Ptr->value), *_First); // throws
			_STD _Construct_in_place(p_tail->_Next, _Newnode._Ptr);
			_STD _Construct_in_place(_Newnode._Ptr->_Prev, p_tail);
			p_tail = _Newnode._Ptr;
			++added;
		}

		_Newnode._Ptr = pointer{};
	}

	template <class T>
	pointer _Attach_before(tree_value<T>& _My_data, const pointer _Insert_before) noexcept
	{
		// Attach the elements in this insert operation before _Insert_before.
		// If no elements were inserted, returns _Insert_before; otherwise returns a pointer
		// to the first inserted tree node.
		// Resets *this to be empty.
		if (added == 0)
		{
			return _Insert_before;
		}

		_My_data.m_size += _STD exchange(added, size_type{ 0 });

		_STD _Construct_in_place(p_tail->_Next, _Insert_before); // assumed nothrow
		const auto _Insert_after = _STD exchange(_Insert_before->_Prev, p_tail);

		const auto _First_inserted = base._Next;
		_Insert_after->_Next = _First_inserted;
		_First_inserted->_Prev = _Insert_after;

		_STD _Destroy_in_place(base._Next);
		p_tail = std::pointer_traits<pointer>::pointer_to(base);
		return _First_inserted;
	}

	template <class T>
	void _Attach_at_end(tree_value<T>& _My_data) noexcept
	{
		_Attach_before(_My_data, _My_data.element);
	}

	template <class T>
	void _Attach_head(tree_value<T>& _My_data)
	{
		_STD _Alloc_construct_ptr<Alloc> _Newnode(_allocator);
		_Newnode._Allocate(); // throws, assumed nothrow hereafter
		if (added == 0)
		{
			_STD _Construct_in_place(_Newnode._Ptr->_Next, _Newnode._Ptr);
			_STD _Construct_in_place(_Newnode._Ptr->_Prev, _Newnode._Ptr);
			_My_data.m_size = 0;
		}
		else
		{
			_STD _Construct_in_place(_Newnode._Ptr->_Next, base._Next);
			_STD _Construct_in_place(_Newnode._Ptr->_Prev, p_tail);
			base._Next->_Prev = _Newnode._Ptr;
			p_tail->_Next = _Newnode._Ptr;
			_My_data.m_size = _STD exchange(added, size_type{ 0 });
			_STD _Destroy_in_place(p_tail->_Next);
			p_tail = std::pointer_traits<pointer>::pointer_to(base);
		}

		_My_data.element = _Newnode._Release();
	}

	~tree_node_insert_op()
	{
		if (added == 0)
		{
			return;
		}

		_STD _Construct_in_place(p_tail->_Next, nullptr);
		pointer _Subject = base._Next;
		while (_Subject)
		{
			value_type::_free_node_value(_allocator, _STD exchange(_Subject, _Subject->_Next));
		}

		_STD _Destroy_in_place(base._Next);
	}
};

#if _HAS_CXX17
template <class Iter, class Alloc = std::allocator<_STD _Iter_value_t<Iter>>,
	std::enable_if_t<std::conjunction_v<_STD _Is_iterator<Iter>, _STD _Is_allocator<Alloc>>, int> = 0>
	tree(Iter, Iter, Alloc = Alloc())->tree<_STD _Iter_value_t<Iter>, Alloc>;
#endif // _HAS_CXX17

#pragma region tree swap and tree comparison operators implementation
template <class T, class Alloc>
void swap(tree<T, Alloc>& lhs, tree<T, Alloc>& rhs) noexcept /* strengthened */
{
	lhs.swap(rhs);
}

template <class T, class Alloc>
[[nodiscard]] bool operator==(const tree<T, Alloc>& lhs, const tree<T, Alloc>& rhs)
{
	return lhs.m_size() == rhs.m_size() && std::equal(lhs.begin(), lhs.m_end(), rhs.begin());
}

template <class T, class Alloc>
[[nodiscard]] bool operator!=(const tree<T, Alloc>& lhs, const tree<T, Alloc>& rhs)
{
	return !(lhs == rhs);
}

template <class T, class Alloc>
[[nodiscard]] bool operator<(const tree<T, Alloc>& lhs, const tree<T, Alloc>& rhs)
{
	return _STD lexicographical_compare(lhs.begin(), lhs.m_end(), rhs.begin(), rhs.m_end());
}

template <class T, class Alloc>
[[nodiscard]] bool operator>(const tree<T, Alloc>& lhs, const tree<T, Alloc>& rhs)
{
	return rhs < lhs;
}

template <class T, class Alloc>
[[nodiscard]] bool operator<=(const tree<T, Alloc>& lhs, const tree<T, Alloc>& rhs)
{
	return !(rhs < lhs);
}

template <class T, class Alloc>
[[nodiscard]] bool operator>=(const tree<T, Alloc>& lhs, const tree<T, Alloc>& rhs)
{
	return !(lhs < rhs);
}
#pragma endregion

_CE_END

_STD_BEGIN
#if _HAS_CXX17
namespace pmr
{
	template <class T>
	using tree = ce::tree<T, polymorphic_allocator<T>>;
} // namespace pmr
#endif // _HAS_CXX17
_STD_END

#pragma pop_macro("new")
_STL_RESTORE_CLANG_WARNINGS
#pragma warning(pop)
#pragma pack(pop)
#endif // _STL_COMPILER_PREPROCESSOR

#endif // _CE_TREE_
