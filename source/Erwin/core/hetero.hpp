#pragma once

// Adapted from: https://gieseanw.wordpress.com/2017/05/03/a-true-heterogeneous-container-in-c/

#include <vector>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <experimental/type_traits>
#include <cassert>

#include "cereal/types/unordered_map.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/archives/json.hpp"

namespace erwin
{

template<typename...>
struct type_list{};
  
template<typename... TYPES>
struct visitor_base
{
    using types = type_list<TYPES...>;
};
        
struct HeteroMap
{
public:
	HeteroMap() = default;
	HeteroMap(const HeteroMap& other)
	{
	    *this = other;
	}

	HeteroMap& operator=(const HeteroMap& other)
	{
	    clear();
	    deleters = other.deleters;
	    cloners = other.cloners;
	    for (auto&& cloner: cloners)
	        cloner(other, *this);

	    return *this;
	}

	template<typename T>
	void set(const char* key, const T& value)
	{
		// don't have it yet, so create functions for printing, copying, moving, and destroying
		if(items<T>.find(this) == std::end(items<T>))
		{   
			deleters.emplace_back([](HeteroMap& hm){items<T>.erase(&hm);});

			// if someone copies me, they need to call each copy_function and pass themself
			cloners.emplace_back([](const HeteroMap& from, HeteroMap& to)
			{
		 		items<T>[&to] = items<T>[&from];
			});
		}
		items<T>[this].insert(std::make_pair(key, value));
	}

    template<typename T>
    const T& get(const char* key) const
    {
    	// look for the registry
		auto it = items<T>.find(this);
		if(it == std::end(items<T>))
			assert(false && "Can't find registry for this type.");

		// look for key in registry
		auto it2 = items<T>[this].find(key);
		if(it2 == std::end(items<T>[this]))
			assert(false && "Can't find key in registry.");

		return it2->second;
    }


	void clear()
	{
	    for (auto&& deleter: deleters)
	        deleter(*this);
	}

	~HeteroMap()
	{
	    clear();
	}   

	template<typename T>
	void visit(T&& visitor)
	{
	    visit_impl(visitor, typename std::decay_t<T>::types{});
	}

	/*template <class Archive>
	void serialize(Archive& ar)
	{
		serialize_impl(ar, type_list<int,float>{});
	}*/

private:
	// template variable -> one instance for each type
	template<typename T>
	static std::unordered_map<const HeteroMap*, std::unordered_map<std::string, T>> items;

	// visitor that takes a key and a value
	template<typename T, typename U>
	using kv_visit_function = decltype(std::declval<T>().operator()(std::declval<const std::string&>(), std::declval<U&>()));
	// visitor that takes a value only
	template<typename T, typename U>
	using v_visit_function = decltype(std::declval<T>().operator()(std::declval<U&>()));

	template<typename T, typename U>
	static constexpr bool has_kv_visit_v = std::experimental::is_detected<kv_visit_function, T, U>::value;
	template<typename T, typename U>
	static constexpr bool has_v_visit_v = std::experimental::is_detected<v_visit_function, T, U>::value;

	template<typename T, template<typename...> typename TLIST, typename... TYPES>
	void visit_impl(T&& visitor, TLIST<TYPES...>)
	{
	    (..., visit_impl_help<std::decay_t<T>, TYPES>(visitor));
	}

	template<typename VisitorT, typename U>
	void visit_impl_help(VisitorT& visitor)
	{
	    if constexpr(has_kv_visit_v<VisitorT, U>)
	    	for(auto&& [key,element]: items<U>[this])
	        	visitor(key, element);
	    else if(has_v_visit_v<VisitorT, U>)
	    	for(auto&& [key,element]: items<U>[this])
	        	visitor(element);
	}

	/*template<typename Archive, template<typename...> typename TLIST, typename... TYPES>
	void serialize_impl(Archive& ar, TLIST<TYPES...>)
	{
	    //(..., serialize_impl_help<std::decay_t<T>, TYPES>(visitor));
	}*/

	std::vector<std::function<void(HeteroMap&)>> deleters;
	std::vector<std::function<void(const HeteroMap&, HeteroMap&)>> cloners;
	std::vector<std::function<size_t(const HeteroMap&)>> size_functions;
};

template<typename T>
std::unordered_map<const HeteroMap*, std::unordered_map<std::string, T>> HeteroMap::items;


} // namespace erwin