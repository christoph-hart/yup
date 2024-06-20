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

namespace yup
{

DataModelTree::PropertyListener::PropertyListener(Type type_, const Array<Identifier>& ids_):
	ListenerBase(type_),
	ids(ids_)
{}

DataModelTree::ChildIterator::ChildIterator(const DataModelTree& p)
{
	if(p)
	{
		children.reserve(p.data->children.size());

		for(auto c: p.data->children)
		{
			DataModelTree copy(c);
			children.push_back(copy);
		}
	}
}

DataModelTree* DataModelTree::ChildIterator::begin()
{
	return children.data();
}

DataModelTree* DataModelTree::ChildIterator::end()
{
	return children.data() + children.size();
}

const DataModelTree* DataModelTree::ChildIterator::begin() const
{
	return children.data();
}

const DataModelTree* DataModelTree::ChildIterator::end() const
{
	return children.data() + children.size();
}

DataModelTree::ChildIterator DataModelTree::getChildren() const
{ return ChildIterator(*this); }

bool DataModelTree::forEach(const std::function<bool(DataModelTree&)>& f)
{
	if(*this)
	{
		if(f(*this))
			return true;

		for(auto c: data->children)
		{
			DataModelTree cv(c);

			if(cv.forEach(f))
				return true;
		}
	}

	return false;
}

bool DataModelTree::forEachParent(const std::function<bool(DataModelTree&)>& f)
{
	if(!*this)
		return false;

	if(f(*this))
		return true;

	if(auto p = getParent())
		return p.forEachParent(f);

	return false;
}

void DataModelTree::addPropertyListener(PropertyListener* pl)
{
	auto l = getListeners(true);
	l->propertyListeners.add(pl);
	pl->attach(true);
}

void DataModelTree::removePropertyListener(PropertyListener* pl)
{
	if(auto l = getListeners(false))
	{
		l->propertyListeners.remove(pl);
		pl->attach(false);
	}
}

void DataModelTree::addChildListener(ChildListener* cl)
{
	auto l = getListeners(true);
	l->childListeners.add(cl);
	cl->attach(true);
}

void DataModelTree::removeChildListener(ChildListener* cl)
{
	if(auto l = getListeners(false))
	{
		l->childListeners.remove(cl);
		cl->attach(false);
	}
}



DataModelTree::DataModelTree(const Identifier& id, UndoManager* um):
	data(new DataObject(id, um))
{}

DataModelTree::~DataModelTree()
{
#if JUCE_DEBUG
	if(debugDataListener != nullptr)
	{
		removeChildListener(debugDataListener.get());
		removePropertyListener(dynamic_cast<PropertyListener*>(debugDataListener.get()));
		debugDataListener.reset();
	}
#endif
}

DataModelTree& DataModelTree::operator=(const DataModelTree& other)
{
	data = other.data;
	return *this;
}

DataModelTree::DataModelTree(DataModelTree&& other) noexcept:
	data(std::move(other.data))
{}

DataModelTree& DataModelTree::operator=(DataModelTree&& other) noexcept
{
	std::swap(data, other.data);
	return *this;
}

DataModelTree DataModelTree::getChild(int index) const
{
	if(*this)
	{
		if(auto c = data->children[index])
		{
			return DataModelTree(c);;
		}
	}

	return {};
}

DataModelTree DataModelTree::getChildWithName(const Identifier& id) const
{
	if(*this)
	{
		for(auto c: data->children)
		{
			if(c->id == id)
				return DataModelTree(c);
		}
	}

	return {};
}

DataModelTree DataModelTree::getParent() const
{
	if(*this && data->parent.get())
		return DataModelTree(data->parent.get());

	return {};
}

Identifier DataModelTree::getType() const
{
	if(*this)
		return data->id;

	return {};
}

DataModelTree DataModelTree::getRoot() const
{
	auto p = *this;

	auto parent = p.getParent();

	while(parent)
	{
		p = parent;
		parent = p.getParent();
	}

	return p;
}

DataModelTree DataModelTree::getOrCreateChildWithName(const Identifier& id)
{
	if(auto c = getChildWithName(id))
		return c;

	DataModelTree newChild(id);
	addChild(newChild, -1);
	return newChild;
}

DataModelTree DataModelTree::addChild(const DataModelTree& child, int index)
{
	if(child && *this)
	{
		jassert(child.data->parent == nullptr);
		auto childData = child.data;

		perform([childData, index](DataModelTree& v, bool isUndo)
		{
			jassert(v);

			DataModelTree c(childData);

			if(isUndo)
			{
				v.sendChildChangeMessage(c, false);
				v.data->remove(childData); 
			}
			else
			{
				v.data->add(childData, index);
				v.sendChildChangeMessage(c, true);
			}
                
			return true;
		});
	}

	return child;
}

bool DataModelTree::removeChild(const DataModelTree& child)
{
	if(child && *this)
	{
		auto childData = child.data;
		auto idx = data->children.indexOf(child.data);
		if(idx != -1)
		{
			perform([childData, idx](DataModelTree& v, bool isUndo)
			{
				DataModelTree c(childData);

				if(isUndo)
				{
					v.data->add(childData, idx);
					v.sendChildChangeMessage(c, true);
				}
		                
				else
				{
					v.sendChildChangeMessage(c, false);
					v.data->remove(childData);
				}
		            
				return true; 
			});
		}
	}

	return false;
}

DataModelTree::DataObject::DataObject(const Identifier& id_, UndoManager* um):
	undoManager(um),
	id(id_)
{}

void DataModelTree::DataObject::add(Ptr child, int index)
{
	child->parent = this;
	child->undoManager = undoManager;

	if(index == -1)
		children.add(child);
	else
		children.insert(index, child);
}

bool DataModelTree::DataObject::remove(Ptr child)
{
	if(children.contains(child))
	{
		child->children.removeObject(child);
		child->parent = nullptr;
		child->undoManager = nullptr;
		return true;
	}

	return false;
}

DataModelTree::Property<DataModelTree> DataModelTree::operator[](const Identifier& id)
{
	if(data != nullptr)
		return { *this, id };

	return {};
}

DataModelTree::Property<const DataModelTree> DataModelTree::operator[](const Identifier& id) const
{
	if(data != nullptr)
		return { *this, id };

	return {};
}

DataModelTree::Property<DataModelTree> DataModelTree::operator[](const char* id)
{
	return (*this)[Identifier(id)];
}

DataModelTree::Property<const DataModelTree> DataModelTree::operator[](const char* id) const
{
	return (*this)[Identifier(id)];
}

DataModelTree::PropertyIterator<DataModelTree> DataModelTree::getProperties()
{ return PropertyIterator(*this); }

DataModelTree::PropertyIterator<const DataModelTree> DataModelTree::getProperties() const
{ return PropertyIterator(*this); }

void DataModelTree::sendChildChangeMessage(DataModelTree& child, bool wasAdded)
{
	if(auto l = getListeners(false))
	{
		l->childListeners.call([child, wasAdded](ChildListener& cl)
		{
			DataModelTree copy(child);
			cl.childAddedOrRemoved(copy, wasAdded);
		});
	}
}

void DataModelTree::sendPropertyChangeMessage(const DataModelTree& v, const Identifier& id)
{
	if(auto l = getListeners(false))
	{
		for(int i = 0; i < l->lambdaPropertyListeners.size(); i++)
		{
			if(l->lambdaPropertyListeners[i]->isDangling())
			{
				auto pl = l->lambdaPropertyListeners.removeAndReturn(i--);
				l->propertyListeners.remove(pl);
			}
		}

		l->propertyListeners.call([this, v, id](PropertyListener& p)
		{
			auto shouldFire = p.getListenerType() == ListenerBase::Type::NotifyAtChildEvents;
			shouldFire |= v.data == this->data;
			shouldFire &= p.matches(id);

			if(shouldFire)
				p.propertyChanged(*this, id);
		});
	}

	if(auto p = getParent())
	{
		p.sendPropertyChangeMessage(v, id);
	}
}

DataModelTree::DataObject::Listeners* DataModelTree::getListeners(bool createIfNotExist)
{
	if(*this)
	{
		if(data->listeners == nullptr && createIfNotExist)
			data->listeners.reset(new DataObject::Listeners());

		return data->listeners.get();
	}

	return nullptr;
}

#if JUCE_DEBUG

struct DataModelTree::DebugDataModel
{
    struct property
    {
        std::string id;
        std::string value;
    };
    std::vector<property> properties;
    std::vector<DebugDataModel> children;
};

struct DataModelTree::DebugDataListener: public ChildListener,
									 public PropertyListener
{
    DebugDataListener(DataModelTree& v_):
	  PropertyListener(ListenerBase::Type::NotifyAtChildEvents),
	  ChildListener(ListenerBase::Type::NotifyAtChildEvents),
      v(v_)
    {
        v.addPropertyListener(this);
        v.addChildListener(this);

		data = createDebugDataModel(v);

#if JUCE_DEBUG
		v.debugData = &data;
#endif
    };

    ~DebugDataListener()
    {
        v.removePropertyListener(this);
        v.removeChildListener(this);
    }

	static DebugDataModel createDebugDataModel(DataModelTree& vt)
    {
        DebugDataModel root;

        for(auto& v: vt.getProperties())
        {
	        DebugDataModel::property p;
            p.id = v.getIdentifier().toString().toStdString();
            p.value = v.get().toString().toStdString();
            root.properties.push_back(p);
        }

        for(auto c: vt.getChildren())
        {
	        root.children.push_back(createDebugDataModel(c));
        }

        return root;
    }


    void propertyChanged(const DataModelTree&, const Identifier&) override
    {

	    data = createDebugDataModel(v);
		v.debugData = &data;
    }

    void childAddedOrRemoved(DataModelTree& newChild, bool wasAdded) override
    {
	    data = createDebugDataModel(v);
		v.debugData = &data;
    }

    DebugDataModel data;
    DataModelTree& v;
};

#endif

void DataModelTree::setCreateDebugModel()
{
#if JUCE_DEBUG
	auto l = getListeners(true);
	debugDataListener.reset(new DebugDataListener(*this));
#endif
}

std::unique_ptr<juce::XmlElement> DataModelTree::createXml() const
{
	auto xml = std::make_unique<juce::XmlElement>(getType());

	for(const auto& p: getProperties())
		xml->setAttribute(p.getIdentifier(), p.get().toString());

	for(const auto& c: getChildren())
		xml->addChildElement(c.createXml().release());

	return xml;
}

DataModelTree DataModelTree::fromXml(XmlElement& xml, UndoManager* um)
{
	DataModelTree v(xml.getTagName(), um);

	for(int i = 0; i < xml.getNumAttributes(); i++)
	{
		v[Identifier(xml.getAttributeName(i))] = xml.getAttributeValue(i);
	}

	for(int i = 0; i < xml.getNumChildElements(); i++)
	{
		auto childTree = fromXml(*xml.getChildElement(i), um);
		v.addChild(childTree, -1);
	}

	return v;
}

void DataModelTree::writeToStream(OutputStream& out)
{
	static constexpr uint8 DataModelTreeStartMarker = 50;
	static constexpr uint8 DataModelTreeEndMarker = 50;


}

DataModelTree DataModelTree::fromInputStream(InputStream& input)
{
	return {};
}

void DataModelTree::dump()
{
#if JUCE_DEBUG
	DBG("Dumping DataModelTree " + getType() + "====================");
	forEach([&](DataModelTree& v)
	{
		String path;
		v.forEachParent([&](DataModelTree& parent)
		{
			path = parent.getType() + "." + path;
			return false;
		});

		for(auto& p: v.getProperties())
		{
			String line = "  " + path;
			line << p.getIdentifier();
			line << ": " << p.get().toString();
			DBG(line);
		}
            
		return false;
	});

	DBG("End of dump ==================================");
#endif
}

void DataModelTree::perform(const std::function<bool(DataModelTree&, bool)>& f)
{
	if(*this)
	{
		if(data->undoManager != nullptr)
			data->undoManager->perform<DataModelTree>(*this, f);
		else
			f(*this, false);
	}
}
} // namespace yup

/** This class is testing the yup::ValueTreeAdapter and requires the GPL licensed juce_data_structures module
 *  for comparing the behaviour against the juce::ValueTree. Set this to 1 to enable it and make sure that
 *  the project has the module added in the Projucer.
 */
#ifndef TEST_VALUE_TREE
#define TEST_VALUE_TREE 0
#endif

#if TEST_VALUE_TREE
#include <juce_data_structures/juce_data_structures.h>

template <bool useUndo> struct ValueTreeAdapterTest: public juce::UnitTest
{
    template <typename T> struct ListenerBase: public T::Listener
    {
        juce::Identifier lastId;
        juce::var lastValue;
        int numCalls = 0;

        template <typename Other> bool operator==(const ListenerBase<Other>& o) const
        {
	        return lastId == o.lastId &&
                   lastValue == o.lastValue &&
                   numCalls == o.numCalls;
        }

	    virtual void valueTreePropertyChanged (T& t, const juce::Identifier& id)
	    {
		    numCalls++;
            lastId = id;
            lastValue = t[id];
	    }

        virtual void valueTreeChildAdded (T& p,T& c) override
	    {
		    numCalls++;
            lastId = p.getType();
            lastValue = c.getType().toString();
	    }

        virtual void valueTreeChildRemoved (T& p, T& c, int index) override
    	{
            numCalls++;
            lastId = p.getType();
            lastValue = c.getType().toString() + ":" + juce::String(index);
        };

        void valueTreeChildOrderChanged (T& p, int oldIndex, int newIndex) override
        {
	        jassertfalse;
        }

        void valueTreeParentChanged (T& treeWhoseParentHasChanged) override
        {
	        jassertfalse;
        }

        void valueTreeRedirected (T& treeWhichHasBeenChanged) override
        {
	        jassertfalse;
        }
    };
    
    juce::UndoManager juceUndoManager;
    juce::ValueTree juceTree;
    juce::UndoManager* um;

    yup::UndoManager yupUndoManager;
    yup::DataModelTree yupTree;
    yup::ValueTreeAdapter adapter;

    ValueTreeAdapterTest(const juce::Identifier& id):
      UnitTest("Testing ValueTree adapter"),
      juceTree(id),
      yupTree(id, useUndo ? &yupUndoManager : nullptr),
      adapter(yupTree),
      yupUndoManager(false),
      um(useUndo ? &juceUndoManager : nullptr)
    {
        yupTree.setCreateDebugModel();
    }

    void test(juce::String t)
    {
	    t << (useUndo ? " with" : " without") << " Undo";
        beginTest(t);
    }

    void runTest() override
    {
        testIdAndBasicValidation();
        testSetProperty();
        testListeners();
    }

#define UNDO_MANAGER(x) juceUndoManager.x; yupUndoManager.x;

#define TREE(x) juceTree.x; adapter.x

    void testIdAndBasicValidation()
    {
        test("testing basic validation");

	    expect(juceTree.getType() == adapter.getType(), "type not equal");
        expect((juceTree == juceTree) == (adapter == adapter), "operator== doesn't work");
        expect((juceTree != juceTree) == (adapter != adapter), "operator!= doesn't work");
        expect(juceTree.isValid() == adapter.isValid(), "valid not equal");
        expect(juceTree.isEquivalentTo(juceTree) == adapter.isEquivalentTo(adapter), "isEquivalentTo not equal");
        expect((juceTree.getRoot() == juceTree) == (adapter.getRoot() == adapter), "getRoot not equal");
    }

#define EXPECT(x, message) expect(juceTree.x == adapter.x, message)

    void testSetProperty()
    {
        test("test property methods");

        UNDO_MANAGER(beginNewTransaction());

        TREE(setProperty("myProperty", 4, um));

        EXPECT(getProperty("myProperty"), "property operator[] not equal");
        expect(juceTree["myProperty"] == adapter["myProperty"], "property operator[] not equal");

        UNDO_MANAGER(beginNewTransaction());
        TREE(setProperty("myProperty", 123, um));
        EXPECT(getProperty("myProperty"), "property not equal");
        UNDO_MANAGER(undo());

        EXPECT(getProperty("myProperty"), "property not equal after undo");
        
        expectEquals<int>(juceTree["myProperty"], useUndo ? 4 : 123, "juce undo manager doesn't work (lol)");
        expectEquals<int>(adapter["myProperty"], useUndo ? 4 : 123, "adapter undo manager doesn't work");

        UNDO_MANAGER(beginNewTransaction());

        TREE(setProperty("myProperty", 62, um));
        TREE(setProperty("myProperty", 125, um));
        TREE(setProperty("myProperty", 52, um));
        TREE(setProperty("myProperty", 10, um));
        
        EXPECT(getProperty("myProperty"), "property not equal before group undo");
        UNDO_MANAGER(undo());
        EXPECT(getProperty("myProperty"), "property not equal after group undo");
    }

    void testListeners()
    {
	    test("testing listeners");

        ListenerBase<juce::ValueTree> juceListener;
        ListenerBase<yup::ValueTreeAdapter> adapterListener;

        expect(juceListener == adapterListener, "not equal");

        adapter.addListener(&adapterListener);
        juceTree.addListener(&juceListener);

        TREE(setProperty("test", 90, um));

        expect(juceListener == adapterListener, "not equal after property change");
        expectEquals<int>(juceListener.lastValue, 90, "value doesn't match");

        auto jCopy = juceTree;
        auto yCopy = adapter;

        UNDO_MANAGER(beginNewTransaction());

        jCopy.setProperty("test", 91, um);
        yCopy.setProperty("test", 91, um);

        expect(juceListener == adapterListener, "not equal after tree copy property change");
        expectEquals<int>(juceListener.lastValue, 91, "value doesn't match");

        UNDO_MANAGER(undo());

        expect(juceListener == adapterListener, "not equal after undo");
        expectEquals<int>(juceListener.lastValue, um != nullptr ? 90 : 91, "value doesn't match");
        
        adapter.removeListener(&adapterListener);
        juceTree.removeListener(&juceListener);
    }
};


static ValueTreeAdapterTest<true> test("AdapterTest");
static ValueTreeAdapterTest<false> test2("AdapterTest");

#endif
