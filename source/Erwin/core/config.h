#pragma once

#include "wtypes.h"

namespace erwin
{

class Config
{
public:
	class ItemBase;

	template<typename T>
	class Item;

	Config();
	~Config();

private:
};

class Config::ItemBase
{
public:
	ItemBase(const char* name): name_(name) {}

	inline hash_t get_hash() const { return H_(name_); }

private:
	const char* name_;
};

template<typename T>
class Config::Item: public Config::ItemBase
{
public:
	Item(const char* name, T&& value):      ItemBase(name), value_(std::move(value)) {}
	Item(const char* name, const T& value): ItemBase(name), value_(value) {}

private:
	T value_;
};

} // namespace erwin