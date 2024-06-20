/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

namespace yup
{

template <typename WeakReferenceable>
DataModelTree::LambdaPropertyListener<WeakReferenceable>::LambdaPropertyListener(WeakReferenceable& obj_, DataModelTree::ListenerBase::Type type,
	const Array<Identifier>& ids, const Callback& f_):
	PropertyListener(type, ids),
	obj(&obj_),
	f(f_)
{}

template <typename WeakReferenceable>
void DataModelTree::LambdaPropertyListener<WeakReferenceable>::propertyChanged(const DataModelTree& v, const Identifier& id)
{
	if(obj.get())
		f(*obj, v, id);
}

template <typename T>
void DataModelTree::addLambdaPropertyListenerT(T& obj, DataModelTree::ListenerBase::Type type, const Array<Identifier>& ids,
	const typename LambdaPropertyListener<T>::Callback& f)
{
	auto l = getListeners(true);
	
	auto n = new LambdaPropertyListener<T>(obj, type, ids, f);
	l->lambdaPropertyListeners.add(n);
	l->propertyListeners.add(n);
}


template <typename DataModelTreeRefType>
DataModelTree::Property<DataModelTreeRefType>::Property(DataModelTreeRefType& parent_, const Identifier& id_):
  parent(&parent_),
  id(id_)
{}

template <typename DataModelTreeRefType>
DataModelTree::Property<DataModelTree>& DataModelTree::Property<DataModelTreeRefType>::operator=(const var& newValue)
{
	// If this throws an compile error, you tried to write a property from a read only access.
	// Go up the stack trace (usually one or two levels) to find the culprit...
	static_assert(!std::is_const<DataModelTreeRefType>(), "not a const property");

	if constexpr (!std::is_const<DataModelTreeRefType>())
	{
		auto oldValue = parent->data->properties[id];
		auto thisId = id;

		if(parent)
		{
			parent->perform([thisId, oldValue, newValue](DataModelTree& obj, bool isUndo)
			{
				auto v = isUndo ? oldValue : newValue;
				obj.data->properties.set(thisId, v);
				obj.sendPropertyChangeMessage(obj, thisId);
				return true; 
			});
		}
	}
	
	return *this;
}

template <typename DataModelTreeRefType>
bool DataModelTree::Property<DataModelTreeRefType>::isDefined() const noexcept
{
	if(parent)
		return parent->data->properties.contains(id);

	return false;
}

template <typename DataModelTreeRefType>
DataModelTree::Property<DataModelTreeRefType>::operator bool() const
{ return (bool)parent; }

template <typename DataModelTreeRefType>
Identifier DataModelTree::Property<DataModelTreeRefType>::getIdentifier() const
{ return id; }

template <typename DataModelTreeRefType>
var DataModelTree::Property<DataModelTreeRefType>::get(const var& defaultValue) const
{
	if(parent)
	{
		return parent->data->properties[id];
	}

	return defaultValue;
}

template <typename DataModelTreeRefType>
DataModelTree::Property<DataModelTreeRefType>::operator var() const
{
	return get();
}

template <typename DataModelTreeRefType>
DataModelTree::PropertyIterator<DataModelTreeRefType>::PropertyIterator(DataModelTreeRefType& parent_):
  parent(parent_)
{
	if(parent)
	{
		int numToCreate = parent.data->properties.size();

		properties.ensureStorageAllocated(numToCreate);

		for(int i = 0; i < numToCreate; i++)
		{
			auto id = parent.data->properties.getName(i);
			properties.add(Property(parent, id));
		}
	}
}

template <typename DataModelTreeRefType>
DataModelTree::Property<DataModelTreeRefType> DataModelTree::PropertyIterator<DataModelTreeRefType>::operator[](const Identifier& id)
{
	for(auto& p: properties)
	{
		if(p.getIdentifier() == id)
			return p;
	}

	return { parent, id };
}

template <typename DataModelTreeRefType>
int DataModelTree::PropertyIterator<DataModelTreeRefType>::size() const
{ return properties.size(); }

template <typename DataModelTreeRefType>
DataModelTree::Property<DataModelTreeRefType>* DataModelTree::PropertyIterator<DataModelTreeRefType>::begin()
{ return properties.begin();}

template <typename DataModelTreeRefType>
DataModelTree::Property<DataModelTreeRefType>* DataModelTree::PropertyIterator<DataModelTreeRefType>::end()
{ return properties.end(); }

} // namespace yup;
