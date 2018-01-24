//-----------------------------------------------------------------------------------------------------------------------------
// think-cell public library
// Copyright (C) 2016-2018 think-cell Software GmbH
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. 
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. 
//
// You should have received a copy of the GNU General Public License along with this program. 
// If not, see <http://www.gnu.org/licenses/>. 
//-----------------------------------------------------------------------------------------------------------------------------

#pragma once

#include "range_defines.h"
#include "range_fwd.h"

#include "range_adaptor.h"
#include "sub_range.h"
#include "meta.h"

#include "tc_move.h"
#include "transform.h"

namespace tc {
	namespace transform_adaptor_impl {
		struct transform_adaptor_access final {
			template< typename Func, typename Rng, bool bHasIterator >
			static Func&& get_func(transform_adaptor<Func,Rng,bHasIterator>&& rng) noexcept {
				return tc_move(rng).m_func;
			}
		};


		template< typename Func, typename Rng >
		struct transform_adaptor<Func,Rng,false> : public range_adaptor<transform_adaptor<Func,Rng>, Rng > {
		private:
			using base_ = range_adaptor<transform_adaptor<Func,Rng>, Rng >;

		protected:
			using range_adaptor = base_;

			static_assert( tc::is_decayed< Func >::value );
			Func m_func;

		private:
			friend struct range_adaptor_impl::range_adaptor_access;
			friend struct transform_adaptor_impl::transform_adaptor_access;
			template< typename Apply, typename... Args>
			auto apply(Apply&& apply, Args&& ... args) const& MAYTHROW return_decltype (
				tc::continue_if_not_break(std::forward<Apply>(apply), m_func(std::forward<Args>(args)...))
			)

		public:
			// other ctors
			template< typename RngOther, typename FuncOther >
			explicit transform_adaptor( RngOther&& rng, FuncOther&& func ) noexcept
				: base_(aggregate_tag(), std::forward<RngOther>(rng))
				, m_func(std::forward<FuncOther>(func))
			{}

			template< typename Rng2 = tc::index_range_t<Rng>, std::enable_if_t<tc::size_impl::has_size<Rng2>::value>* = nullptr >
			auto size() const& noexcept {
				return tc::size_impl::size(this->base_range());
			}
		};

		template< typename Func, typename Rng >
		struct transform_adaptor<Func,Rng,true> : public transform_adaptor<Func,Rng,false> {
			static_assert( 
				std::is_same< Rng, view_by_value_t<Rng> >::value,
				"adaptors must hold ranges by value"
			);
        private:
			using base_ = transform_adaptor<Func,Rng,false>;
			using range_adaptor = typename base_::range_adaptor; // using not accepted by MSVC

			friend struct range_adaptor_impl::range_adaptor_access;
		public:
			using typename base_::index;

			// ctor from range and functor
			template< typename RngOther, typename FuncOther >
			explicit transform_adaptor( RngOther&& rng, FuncOther&& func ) noexcept
				: base_(std::forward<RngOther>(rng),std::forward<FuncOther>(func))
			{}

			// ctors forwarding to sub_range
			template< typename RngOther, typename FuncOther >
			explicit transform_adaptor( transform_adaptor< FuncOther, RngOther, true >&& rng
				, typename boost::range_iterator< transform_adaptor< FuncOther, RngOther, true > >::type itBegin
				, typename boost::range_iterator< transform_adaptor< FuncOther, RngOther, true > >::type itEnd
			) noexcept
				: base_(tc::slice(tc_move(rng).base_range_move(),itBegin.border_base(),itEnd.border_base()), transform_adaptor_access::get_func(tc_move(rng)))
			{}

			template<typename Func2=Func/*enable SFINAE*/>
			auto STATIC_VIRTUAL_METHOD_NAME(dereference_index)(index const& idx) & MAYTHROW -> tc::transform_return_t<
				Func2,
				decltype(std::declval<Func2 const&>()(std::declval<range_adaptor &>().STATIC_VIRTUAL_METHOD_NAME(dereference_index)(std::declval<index const&>()))),
				decltype(std::declval<range_adaptor &>().STATIC_VIRTUAL_METHOD_NAME(dereference_index)(std::declval<index const&>()))
			> {
				// always call operator() const, which is assumed to be thread-safe
				return tc::as_const(this->m_func)(base_::STATIC_VIRTUAL_METHOD_NAME(dereference_index)(idx));
			}

			template<typename Func2=Func/*enable SFINAE*/>
			auto STATIC_VIRTUAL_METHOD_NAME(dereference_index)(index const& idx) const& MAYTHROW -> tc::transform_return_t<
				Func2,
				decltype(std::declval<Func2 const&>()(std::declval<range_adaptor const&>().STATIC_VIRTUAL_METHOD_NAME(dereference_index)(std::declval<index const&>()))),
				decltype(std::declval<range_adaptor const&>().STATIC_VIRTUAL_METHOD_NAME(dereference_index)(std::declval<index const&>()))
			> {
				// always call operator() const, which is assumed to be thread-safe
				return tc::as_const(this->m_func)(base_::STATIC_VIRTUAL_METHOD_NAME(dereference_index)(idx));
			}

			auto separator_base_index(index const& idx) const& noexcept {
				return idx;
			}

			auto element_base_index(index const& idx) const& noexcept {
				return idx;
			}
		};

		template<typename Func, typename Rng, bool b>
		auto constexpr_size(transform_adaptor<Func, Rng, b> const& rng) -> decltype(constexpr_size(rng.base_range()));
	}

	namespace range_reference_adl_barrier {
		template< typename Func, typename Rng, bool bConst >
		struct range_reference_transform_adaptor {
			using type = decltype(
				std::declval<Func&>()(
					std::declval<
						tc::range_reference_t<
							tc::apply_if_t<
								bConst,
								std::add_const,
								range_adaptor<transform_adaptor<Func,Rng>, Rng >
							>
						>
					>()
				)
			);
		};


		template< typename Func, typename Rng >
		struct range_reference<transform_adaptor<Func, Rng, false> > : range_reference_transform_adaptor<Func, Rng, false> {};

		template< typename Func, typename Rng >
		struct range_reference<transform_adaptor<Func, Rng, false> const> : range_reference_transform_adaptor<Func, Rng, true> {};
	}

	namespace replace_if_impl {
		template< typename Func, typename T >
		struct replace_if final {
		private:
			tc::decay_t<Func> m_func;
			tc::decay_t<T> m_t;
			
		public:
			replace_if(Func&& func, T&& t) noexcept
				: m_func(std::forward<Func>(func))
				, m_t(std::forward<T>(t))
			{}
			template< typename S >
			auto operator()(S&& s) const& MAYTHROW ->decltype(auto) {
				return CONDITIONAL(m_func(s),m_t,std::forward<S>(s));
			}
		};
	}

	template<typename Rng, typename Func, typename T>
	auto replace_if(Rng&& rng, Func func, T&& t) noexcept {
		return tc::transform( std::forward<Rng>(rng), replace_if_impl::replace_if<Func,T>(std::forward<Func>(func),std::forward<T>(t) ) );
	}

	template<typename Rng, typename S, typename T>
	auto replace(Rng&& rng, S&& s, T&& t) noexcept {
		return tc::replace_if( std::forward<Rng>(rng), [s_=tc::decay_copy(std::forward<S>(s))](auto const& _) noexcept { return tc::equal_to(_, s_); }, std::forward<T>(t) );
	}

	template <typename Rng, typename Func, typename T>
	Rng& replace_if_inplace(Rng& rng, Func func, T const& t) noexcept {
		for_each(rng, [&](decltype(*boost::begin(rng)) v) noexcept {
			if (func(tc::as_const(v))) {
				v = t;
			}
		});
		return rng;
	}

	template<typename Rng, typename S, typename T>
	Rng& replace_inplace(Rng& rng, S&& s, T const& t) noexcept {
		return tc::replace_if_inplace( rng, [&](auto const& _) noexcept { return tc::equal_to(_, s); }, t );
	}
}

