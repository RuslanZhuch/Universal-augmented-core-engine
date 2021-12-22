#pragma once

#include <algorithm>
#include <deque>
#include <vector>

namespace cexpr
{

    template <class _Ty, class _Container = std::deque<_Ty>>
    class queue {
    public:
        using value_type = typename _Container::value_type;
        using reference = typename _Container::reference;
        using const_reference = typename _Container::const_reference;
        using size_type = typename _Container::size_type;
        using container_type = _Container;

        static_assert(std::is_same_v<_Ty, value_type>, "container adaptors require consistent types");

        constexpr queue() = default;

        explicit constexpr queue(const _Container& _Cont) : c(_Cont) {}

        explicit constexpr queue(_Container&& _Cont) noexcept(std::is_nothrow_move_constructible_v<_Container>) // strengthened
            : c(_STD move(_Cont)) {}

        template <class _Alloc, std::enable_if_t<std::uses_allocator_v<_Container, _Alloc>, int> = 0>
        explicit constexpr queue(const _Alloc& _Al) noexcept(std::is_nothrow_constructible_v<_Container, const _Alloc&>) // strengthened
            : c(_Al) {}

        template <class _Alloc, std::enable_if_t<std::uses_allocator_v<_Container, _Alloc>, int> = 0>
        constexpr queue(const _Container& _Cont, const _Alloc& _Al) : c(_Cont, _Al) {}

        template <class _Alloc, std::enable_if_t<std::uses_allocator_v<_Container, _Alloc>, int> = 0>
        constexpr queue(_Container&& _Cont, const _Alloc& _Al) noexcept(
            std::is_nothrow_constructible_v<_Container, _Container, const _Alloc&>) // strengthened
            : c(_STD move(_Cont), _Al) {}

        template <class _Alloc, std::enable_if_t<std::uses_allocator_v<_Container, _Alloc>, int> = 0>
        constexpr queue(const queue& _Right, const _Alloc& _Al) : c(_Right.c, _Al) {}

        template <class _Alloc, std::enable_if_t<std::uses_allocator_v<_Container, _Alloc>, int> = 0>
        constexpr queue(queue&& _Right, const _Alloc& _Al) noexcept(
            std::is_nothrow_constructible_v<_Container, _Container, const _Alloc&>) // strengthened
            : c(_STD move(_Right.c), _Al) {}

        _NODISCARD constexpr bool empty() const noexcept(noexcept(c.empty())) /* strengthened */ {
            return c.empty();
        }

        _NODISCARD constexpr size_type size() const noexcept(noexcept(c.size())) /* strengthened */ {
            return c.size();
        }

        _NODISCARD constexpr reference front() noexcept(noexcept(c.front())) /* strengthened */ {
            return c.front();
        }

        _NODISCARD constexpr const_reference front() const noexcept(noexcept(c.front())) /* strengthened */ {
            return c.front();
        }

        _NODISCARD constexpr reference back() noexcept(noexcept(c.back())) /* strengthened */ {
            return c.back();
        }

        _NODISCARD constexpr const_reference back() const noexcept(noexcept(c.back())) /* strengthened */ {
            return c.back();
        }

        constexpr void push(const value_type& _Val) {
            c.push_back(_Val);
        }

        constexpr void push(value_type&& _Val) {
            c.push_back(_STD move(_Val));
        }

        template <class... _Valty>
        decltype(auto) constexpr emplace(_Valty&&... _Val) {
            return c.emplace_back(_STD forward<_Valty>(_Val)...);
        }

        constexpr void pop() noexcept(noexcept(c.pop_front())) /* strengthened */ {
            c.pop_front();
        }

        constexpr void swap(queue& _Right) noexcept(_Is_nothrow_swappable<_Container>::value) {
            _Swap_adl(c, _Right.c);
        }

        _NODISCARD constexpr const _Container& _Get_container() const noexcept {
            return c;
        }

    protected:
        _Container c{};
    };

    template <class _Container, std::enable_if_t<!std::_Is_allocator<_Container>::value, int> = 0>
    queue(_Container)->queue<typename _Container::value_type, _Container>;

    template <class _Container, class _Alloc,
        std::enable_if_t<
        std::conjunction_v<std::negation<std::_Is_allocator<_Container>>, std::_Is_allocator<_Alloc>, std::uses_allocator<_Container, _Alloc>>,
        int> = 0>
        queue(_Container, _Alloc)->queue<typename _Container::value_type, _Container>;

    template <class _Ty, class _Container>
    _NODISCARD constexpr bool operator==(const queue<_Ty, _Container>& _Left, const queue<_Ty, _Container>& _Right) {
        return _Left._Get_container() == _Right._Get_container();
    }

    template <class _Ty, class _Container>
    _NODISCARD constexpr bool operator!=(const queue<_Ty, _Container>& _Left, const queue<_Ty, _Container>& _Right) {
        return _Left._Get_container() != _Right._Get_container();
    }

    template <class _Ty, class _Container>
    _NODISCARD constexpr bool operator<(const queue<_Ty, _Container>& _Left, const queue<_Ty, _Container>& _Right) {
        return _Left._Get_container() < _Right._Get_container();
    }

    template <class _Ty, class _Container>
    _NODISCARD constexpr bool operator>(const queue<_Ty, _Container>& _Left, const queue<_Ty, _Container>& _Right) {
        return _Left._Get_container() > _Right._Get_container();
    }

    template <class _Ty, class _Container>
    _NODISCARD constexpr bool operator<=(const queue<_Ty, _Container>& _Left, const queue<_Ty, _Container>& _Right) {
        return _Left._Get_container() <= _Right._Get_container();
    }

    template <class _Ty, class _Container>
    _NODISCARD constexpr bool operator>=(const queue<_Ty, _Container>& _Left, const queue<_Ty, _Container>& _Right) {
        return _Left._Get_container() >= _Right._Get_container();
    }

    template <class _Ty, std::three_way_comparable _Container>
    _NODISCARD constexpr std::compare_three_way_result_t<_Container> operator<=>(
        const queue<_Ty, _Container>& _Left, const queue<_Ty, _Container>& _Right) {
        return _Left._Get_container() <=> _Right._Get_container();
    }
    template <class _Ty, class _Container, std::enable_if_t<std::_Is_swappable<_Container>::value, int> = 0>
    constexpr void swap(queue<_Ty, _Container>& _Left, queue<_Ty, _Container>& _Right) noexcept(noexcept(_Left.swap(_Right))) {
        _Left.swap(_Right);
    }

    template <class _Ty, class _Container, class _Alloc>
    struct std::uses_allocator<queue<_Ty, _Container>, _Alloc> : uses_allocator<_Container, _Alloc>::type {};

};
