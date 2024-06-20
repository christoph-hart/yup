/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 Christoph Hart

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

namespace juce
{
struct UndoManager;
}

namespace yup
{

/** A tree data model with undo support & listeners.
 *
 *  This is a recreation of the juce::ValueTree with a few modifications:
 *
 *  - listeners are divided into property & child listeners and can be set to be
 *    recursive or not (so that changes of child value trees will also fire the listener callbacks)
 *  - you can subclass and implement virtual methods for creating a listener or just
 *    pass in a lambda with a weak referenceable object
 *  - you can supply an UndoManager in the constructor of the ValueTree which will then be used
 *    for all operations (and adding a child to a tree will use the undomanager from the parent)
 *  - there is a property object that is supposed to be used on the stack with easy operator[] read & write
 *  - implicit cast to bool for checking the validity
 *  - forEach() function for quick iteration over all child elements
 *  - a simple std::string as member variable for properties when debugging, as well as a method
 *    to create (and update) a data model POD using std types for nice debug inspections
 *
 *  Note: for a 1:1 drop in replacement for the juce::ValueTree wrap this into a yup::ValueTreeAdapter
 */
class JUCE_API DataModelTree
{
public:
    
    // ==============================================================================================
    // Constructors & Destructor

    /** Creates an empty value tree. */
    DataModelTree() = default;

    /** Creates a ValueTree with a given type and an undo manager. */
    DataModelTree(const Identifier& id, UndoManager* um=nullptr);

    /** Creates a value tree that references another value tree. It will not copy its listeners!. */
    DataModelTree(const DataModelTree& other):
	  data(other.data)
    {};

    ~DataModelTree();

    /** Assigns a value tree to another value tree. It will not copy its listeners!. */
    DataModelTree& operator=(const DataModelTree& other);

    /** Moves a value tree to another value tree and transfers the listeners. */
    DataModelTree(DataModelTree&& other) noexcept;;
    
    /** Moves a value tree to another value tree and transfers the listeners. */
    DataModelTree& operator=(DataModelTree&& other) noexcept;

    // ==============================================================================================
    // Methods for ID

    /** This will check whether the value tree points to a valid data object. */
    operator bool() const { return data != nullptr; }

    /** Returns the type of the ValueTree. */
    Identifier getType() const;

    // ==============================================================================================
    // Methods for children

    /** Returns the number of (immediate) child trees. */
    int getNumChildren() const noexcept
    {
	    if(*this) 
            return data->children.size();
        else
            return 0;
    }

    /** Returns the child at the given index. */
    DataModelTree getChild(int index) const;

    DataModelTree getChildWithName(const Identifier& id) const;

    /** Gets the parent node. */
    DataModelTree getParent() const;

    /** Gets the root node. */
    DataModelTree getRoot() const;

    /** Checks if a child with the given ID exists and creates one if not, then returns the child. */
    DataModelTree getOrCreateChildWithName(const Identifier& id);

    /** Adds the child to the tree at the specified position. */
    DataModelTree addChild(const DataModelTree& child, int index=-1);

    bool isChildOf(const DataModelTree& possibleParent) const
    {
	    if(*this)
            return data->parent.get() == possibleParent.data.get();

        return false;
    }

    int indexOf(const DataModelTree& child)
    {
	    if(*this)
		    return data->children.indexOf(child.data);

        return -1;
    }
    
    /** Removes the child from the tree. */
    bool removeChild(const DataModelTree& child);

    struct ChildIterator
    {
        ChildIterator() = default;

	    ChildIterator(const DataModelTree& p);

	    DataModelTree* begin();
	    DataModelTree* end();
	    const DataModelTree* begin() const;
	    const DataModelTree* end() const;

    private:

	    std::vector<DataModelTree> children;
    };

    /** Returns an object that can be used to iterate through the children with a range-based for-loop */
    ChildIterator getChildren() const;

    // ==============================================================================================
    // Recursive iteration

    /** Recursively calls the given lambda for this value tree and every child tree. */
    bool forEach(const std::function<bool(DataModelTree&)>& f);

    /** Recursively calls the given lambda for this value tree and every parent. */
    bool forEachParent(const std::function<bool(DataModelTree&)>& f);

    // ==============================================================================================
    // Listener methods & class definitions

    class ListenerBase
    {
    public:

        enum Type
	    {
		    NotifyAtChildEvents,
	        NotifyAtOwnEvents
	    };

        ListenerBase(Type t):
          type(t)
        {};

	    virtual ~ListenerBase()
	    {
            jassert(numAttachments == 0);
	    }

        Type getListenerType() const { return type; }

        virtual bool isDangling() const { return false; }

        void attach(bool addAsListener)
        {
	        numAttachments += addAsListener ? 1 : -1;
            numAttachments = jmax(0, numAttachments);
        }

    private:

        int numAttachments = 0;

        const Type type;
    };

    class ChildListener: public ListenerBase
    {
    public:

        ChildListener(Type type_=Type::NotifyAtOwnEvents):
          ListenerBase(type_)
        {}

        ~ChildListener() override {};

        virtual void childAddedOrRemoved(DataModelTree& v, bool wasAdded) = 0;
    };

    // ==============================================================================================

    class PropertyListener: public ListenerBase
    {
    public:

    	~PropertyListener() override {};

        PropertyListener(Type t=Type::NotifyAtOwnEvents, const Array<Identifier>& ids_={});

	    virtual void propertyChanged(const DataModelTree& changedTree, const Identifier& id) = 0;

        bool matches(const Identifier& id) const noexcept { return ids.isEmpty() || ids.contains(id); }

    private:

        Array<Identifier> ids;
    };

    // ==============================================================================================

    template <typename WeakReferenceable> class LambdaPropertyListener: public PropertyListener
    {
    public:

        using Callback = std::function<void(WeakReferenceable& obj, const DataModelTree& v, const Identifier& id)>;

        LambdaPropertyListener(WeakReferenceable& obj_, Type type, const Array<Identifier>& ids, const Callback& f_);

        bool isDangling() const override { return obj.get() == nullptr; }

    private:

        void propertyChanged(const DataModelTree& changedTree, const Identifier& id) override;

        WeakReference<WeakReferenceable> obj;
        Callback f;
        
    };

    /** Adds a listener that will be notified whenever a property changes. */
    void addPropertyListener(PropertyListener* pl);

    /** Removes the listener. */
    void removePropertyListener(PropertyListener* pl);

    /** Adds a listener that will be notified whenever a child was added. */
    void addChildListener(ChildListener* cl);

    /** Removes the listener. */
    void removeChildListener(ChildListener* cl);
    
    /** Adds a listener that will check if the weak referenceable obj exists, and then call the given lambda if one of the properties
     *  have changed.
     */
    template <typename T> void addLambdaPropertyListenerT(T& obj, ListenerBase::Type type, const Array<Identifier>& ids, const typename LambdaPropertyListener<T>::Callback& f);

    void addLambdaPropertyListener(ListenerBase::Type type, const Array<Identifier>& ids,
                                   const LambdaPropertyListener<DataModelTree>::Callback& f)
    {
        addLambdaPropertyListenerT<DataModelTree>(*this, type, ids, f);
    }

    // ==============================================================================================

    /** A temporary (aka stack-used) helper class that accesses a property.
     *
     *  You will never create this yourself, but use the operator[] or the range-based iterator from
     *  getProperties() for accessing the property.
     */
    template <typename ValueTreeRefType> struct Property
    {
        /** Creates an empty property. */
        Property() = default;

        /** Creates a property that can be used to access the parent data. */
		Property(ValueTreeRefType& parent_, const Identifier& id_);

        /** Writes the property using the undo manager. */
	    Property<DataModelTree>& operator=(const var& newValue);

        /** Checks if the value is defined. */
        bool isDefined() const noexcept;

        /** Checks if the property is pointing to a valid object. It can stil be undefined. */
        operator bool() const;

        /** Returns the id of the property. */
        Identifier getIdentifier() const;

        /** Returns the (current) value of the property. */
        var get(const var& defaultValue={}) const;

        /** Returns the (current) value of the property. */
        explicit operator var() const;

    private:

        Identifier id;
        ValueTreeRefType* parent;
    };

    /** This is a class that can be used to iterate through the properties of a ValueTree. */
    template <typename ValueTreeRefType> struct PropertyIterator
    {
        /** Creates a value tree iterator. */
	    PropertyIterator(ValueTreeRefType& parent);

        /** Returns the property for the given ID. */
        Property<ValueTreeRefType> operator[](const Identifier& id);

        /** Returns the number of properties. */
	    int size() const;

        /** Returns the property reference at the given index. The index must be 0 < index < numProperties. */
	    Property<ValueTreeRefType>& operator[](int index)
	    {
		    jassert(isPositiveAndBelow(index, properties.size()));
            return *(properties.getRawDataPointer() + index);
	    }

	    Property<ValueTreeRefType>* begin();
	    Property<ValueTreeRefType>* end();

    private:

        Array<Property<ValueTreeRefType>> properties;
        ValueTreeRefType& parent;
    };

    // ==============================================================================================
    // Property methods

    /** Creates a write access property with the given Identifier. */
    Property<DataModelTree> operator[](const Identifier& id);

    /** Creates a readonly access property with the given Identifier. */
    Property<const DataModelTree> operator[](const Identifier& id) const;

    /** Creates a write access property with the given id. */
    Property<DataModelTree> operator[](const char* id);

    /** Creates a readonly access property with the given id. */
    Property<const DataModelTree> operator[](const char* id) const;

    /** Creates a write access property iterator. */
    PropertyIterator<DataModelTree> getProperties();

    /** Creates a read access property iterator. */
    PropertyIterator<const DataModelTree> getProperties() const;

    // ==============================================================================================
    // I/O methods

    /** This will create a POD model that copies the content (if JUCE_DEBUG is enabled) so
     *  that you can easily inspect the ValueTree in the debugger. It uses std::types for
     *  best visualisation.
     *
     *  Call this on the root value tree and it will register an internal listener that updates
     *  the data on each change.
     */
    void setCreateDebugModel();

    /** Creates a XML element from the ValueTree. */
    std::unique_ptr<juce::XmlElement> createXml() const;

    /** Creates a ValueTree from the XML element. */
    static DataModelTree fromXml(XmlElement& xml, UndoManager* um=nullptr);

    void writeToStream(OutputStream& out);

    static DataModelTree fromInputStream(InputStream& input);

    void dump();

private:

    friend class ValueTreeAdapter;

    struct DebugDataListener;
    struct DebugDataModel;

    struct DataObject: public ReferenceCountedObject
	{
        using Ptr = ReferenceCountedObjectPtr<DataObject>;
        using List = ReferenceCountedArray<DataObject>;

        DataObject(const Identifier& id_, UndoManager* um);;

        void add(Ptr child, int index);

        bool remove(Ptr child);

        Identifier id;
		NamedValueSet properties;
        yup::UndoManager* undoManager;

        List children;
        WeakReference<DataObject> parent;

        struct Listeners
        {
	        OwnedArray<PropertyListener> lambdaPropertyListeners;
		    ListenerList<PropertyListener> propertyListeners;
		    ListenerList<ChildListener> childListeners;
        };

    	std::unique_ptr<Listeners> listeners;
        
        JUCE_DECLARE_WEAK_REFERENCEABLE(DataObject);
        
	};

    /** @internal */
    explicit DataModelTree(DataObject::Ptr p):
      data(p)
    {}

    /** @internal */
    void perform(const std::function<bool(DataModelTree& , bool)>& f);

    /** @internal */
    void sendChildChangeMessage(DataModelTree& child, bool wasAdded);

    /** @internal */
    void sendPropertyChangeMessage(const DataModelTree& v, const Identifier& id);

    /** @internal */
    DataObject::Listeners* getListeners(bool createIfNotExist);

#if JUCE_DEBUG
	DebugDataModel* debugData;
    std::unique_ptr<ChildListener> debugDataListener;
#endif

    DataObject::Ptr data;

    JUCE_DECLARE_WEAK_REFERENCEABLE(DataModelTree);
};


/** This class can be used for porting a data model based on a juce::ValueTree.
 *
 *  It wraps around the DataModelTree and provides the same API as juce::ValueTree.
 *
 *  You can then use aliases for porting the codebase:
 *
 *  // old code base
 *  juce::ValueTree data;
 *
 *  data.setPropertyExcludingListener("id", 12, getUndoManagerFromSomewhere());
 *
 *
 *  // new code base
 *  using ValueTree = yup::ValueTreeAdapter;
 *
 *  yup::DataModelTree dmt;
 *  ValueTree data(dmt);
 *
 *  data.setPropertyExcludingListener("id", 12, getUndoManagerFromSomewhere());
 *
 *
 */
struct ValueTreeAdapter
{
    struct Listener: public DataModelTree::PropertyListener,
					 public DataModelTree::ChildListener
    {
        Listener():
          PropertyListener(Type::NotifyAtChildEvents, {}),
          ChildListener(Type::NotifyAtChildEvents)
        {}

        virtual ~Listener() {};

	    virtual void valueTreePropertyChanged (ValueTreeAdapter& t, const Identifier& id) {};
        virtual void valueTreeChildAdded (ValueTreeAdapter& p, ValueTreeAdapter& c) {};
        virtual void valueTreeChildRemoved (ValueTreeAdapter& p, ValueTreeAdapter& c, int index) {};
        virtual void valueTreeChildOrderChanged (ValueTreeAdapter& p, int oldIndex, int newIndex) {};
        virtual void valueTreeParentChanged (ValueTreeAdapter& p) {};
        virtual void valueTreeRedirected (ValueTreeAdapter& t) {};

        void propertyChanged(const DataModelTree& changedTree, const Identifier& id) override
        {
	        ValueTreeAdapter t(changedTree);
            valueTreePropertyChanged(t, id);
        }

        void childAddedOrRemoved(DataModelTree& v, bool wasAdded) override
        {
	        ValueTreeAdapter c(v);
            ValueTreeAdapter p(v.getParent());
            auto index = p.indexOf(c);

            if(wasAdded)
                valueTreeChildAdded(p, c);
            else
                valueTreeChildRemoved(p, c, index);

        }
    };

    ValueTreeAdapter(DataModelTree& data_):
      data(data_)
    {}

    ~ValueTreeAdapter()
    {
	    for(auto& l: listeners)
	    {
		    removeListener(l);
	    }

        listeners.clear();
    }

    ValueTreeAdapter(const DataModelTree& data_):
      data(*const_cast<DataModelTree*>(&data_))
    {}

    var getProperty(const Identifier& id) const
    {
	    return data[id].get();
    }

	void setProperty(const Identifier& id, const var& newValue, juce::UndoManager* um)
	{
        performWithUndo(um, [id, newValue](DataModelTree& d)
        {
	        d[id] = newValue;
        });
	}

    void addChild(const ValueTreeAdapter& child, int index, juce::UndoManager* um)
    {
	    performWithUndo(um, [child, index](DataModelTree& d)
	    {
		    d.addChild(child.get(), index);
	    });
    }

    void removeChild(const ValueTreeAdapter& child, juce::UndoManager* um)
    {
	    performWithUndo(um, [child](DataModelTree& d)
	    {
		    d.removeChild(child.get());
	    });
    }

    int indexOf(const ValueTreeAdapter& child)
    {
	    return get().indexOf(child.get());
    }

    ValueTreeAdapter getChildWithName(const Identifier& id) const
    {
        auto c = data.getChildWithName(id);
	    return ValueTreeAdapter(c);
    }

    Identifier getType() const noexcept { return data.getType(); }

    bool operator==(const ValueTreeAdapter& other) const noexcept
    {
	    return getData() == other.getData();
    }

    bool operator !=(const ValueTreeAdapter& other) const noexcept
    {
	    return !((*this) == other);
    }

    bool isValid() const noexcept
    {
	    return (bool)data;
    }

    bool isEquivalentTo(const ValueTreeAdapter& other) const noexcept
    {
	    return getData() == other.getData();
    }

    ValueTreeAdapter getRoot() const
    {
        auto r = data.getRoot();
	    return ValueTreeAdapter(r);
    }

    var operator[](const Identifier& id) const noexcept
    {
	    return data[id].get();
    }

    void addListener(Listener* l)
    {
        if(listeners.addIfNotAlreadyThere(l))
        {
	        get().addPropertyListener(l);
			get().addChildListener(l);
        }
	    
    }

    void removeListener(Listener* l)
    {
        if(listeners.removeAllInstancesOf(l) == 1)
        {
	        get().removePropertyListener(l);
			get().removeChildListener(l);
        }
    }

private:

    Array<Listener*> listeners;

    DataModelTree::DataObject::Ptr getData() const { return data.data; }
    
    DataModelTree& get() { return data; }
    const DataModelTree& get() const { return data; }

    /** @internal */
    void performWithUndo(juce::UndoManager* um, const std::function<void(DataModelTree&)>& f)
    {
        // This function will check whether the call is supposed to be undoable
        // and then either use the undo function of the DataModelTree or suspend the undo manager
        // for the action
	    bool wantsUndoManager = um != nullptr;
        bool dataModelHasUndoManager = data.data->undoManager != nullptr;

        if(wantsUndoManager == dataModelHasUndoManager)
        {
            // has undomanager, expects undomanager, everything's fine
            f(data);
        }
        else if (!dataModelHasUndoManager && wantsUndoManager)
        {
            // nope, you've called this method assuming that there's an undo manager!
	        jassertfalse;
        }
        else if (dataModelHasUndoManager && !wantsUndoManager)
        {
            // There's an undo manager, but we don't want to use it...
            yup::UndoManager::ScopedDeactivator sd(*data.data->undoManager);
	        f(data);
        }
    }

    DataModelTree& data;
};


} // namespace yup;

#include "yup_ValueTree_impl.h"