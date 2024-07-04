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
/** A class that will perform actions in a linear timeline that can be navigated using undo() / redo().
 *
 *  This is (of course) inspired by the juce::UndoManager but has the following differences:
 *
 *  - inbuilt timer to coallascate actions that occur within a short time window
 *  - templated perform() function using a weak-referenceable object and a lambda
 *  - automatic lifetime management of objects in the undo history
 *  - can be enabled / disabled.
 *  - can be temporarily suspended with RAII using the ScopedSuspender class
 *  - the grouping of the actions can be temporarily suspended using the ScopedActionIsolator class
 *
 */
class JUCE_API UndoManager : private juce::Timer
{
public:
    /** The base class for all actions in the timeline.
     *
     *  You can subclass from this class to define your actions,
     *  but a better way is to just use a lambda with a WeakReferenceable object.
    */
    struct ActionBase : public juce::ReferenceCountedObject
    {
        using List = ReferenceCountedArray<ActionBase>;
        using Ptr = ReferenceCountedObjectPtr<ActionBase>;

        ~ActionBase () override
        {
        };

        /** This should return true if the action is invalidated (eg. because the object it operates on was deleted. */
        virtual bool isEmpty () const = 0;

        /** This should perform the undo action with the direction specified in the isUndo parameter. */
        virtual bool call (bool isUndo) = 0;
    };

public:
    /** Use this helper class if you want to ensure that certain actions
     *  will be grouped as a single action.
     *
     *  By default the undo manager will group all actions within a 500ms
     *  time window into one action, but if you need to have a separate item
     *  in the timeline for certain actions, you can use this class.
     *
     *  @code
     *  void doSomething()
     *  {
     *      UndoManager::ScopedActionIsolator isolator(um);
     *
     *      um.perform(action1);
     *      um.perform(action2);
     *  }
     *  @endcode 
     */
    struct ScopedActionIsolator
    {
        ScopedActionIsolator (UndoManager& um_);
        ~ScopedActionIsolator ();

        UndoManager& um;
    };

    struct ScopedDeactivator
    {
        ScopedDeactivator (UndoManager& um_);
        ~ScopedDeactivator ();

        UndoManager& um;
        bool prevValue;
    };

    /** Create a new UndoManager. It will also start the timer. */
    UndoManager (bool startTimer = true);

    /** Adds a new action to the timeline and performs it with isUndo=false.
     *
     */
    bool perform (ActionBase::Ptr f);

    // The alias for the lambda callback that can be used to define a undoable action
    template <typename WeakReferenceable>
    using ActionCallback = std::function<bool  (WeakReferenceable&, bool)>;

    /** This will create a action using the weak referencable object and a lambda that will
     *  be performed if the object is still alive.
     */
    template <typename T>
    bool perform (T& obj, const ActionCallback<T>& f)
    {
        ActionBase::Ptr newObject = new Item<T> (obj, f);
        return perform (newObject);
    }

    /** This will reverse the action in the current timeline position. Returns true if it performed something. */
    bool undo ();

    /** This will perform the action in the current timeline position. Return true if it performed something. */
    bool redo ();

    /** This will enable the undo manager. If it is enabled, the timer will run. Disabling the undomanager will clear the history and stop the timer. */
    void setEnabled (bool shouldBeEnabled);

    /** This will disable the grouping of the messages (useful for testing in a non UI mode). */
    void setSynchronousMode (bool shouldBeSynchronous);

    void beginNewTransaction ();

private:
    bool isSynchronous = false;

    /** this is the number of items kept in the history. */
    static constexpr int HistorySize = 30;

    template <typename T>
    struct Item : public ActionBase
    {
        Item (T& obj_, const std::function<bool  (T&, bool)>& f_)
            : obj (&obj_)
            , f (f_)
        {
        };

        bool call (bool isUndo) override
        {
            if (obj.get() != nullptr)
                return f (*obj, isUndo);

            return false;
        }

        bool isEmpty () const override
        {
            return obj.get() == nullptr;
        }

        WeakReference<T> obj;
        std::function<bool  (T&, bool)> f;
    };

    struct CoallascatedItem : public ActionBase
    {
        bool call (bool isUndo) override;
        bool isEmpty () const override;

        List childItems;
    };

    /** @internal */
    void timerCallback () override;
    /** @internal */
    bool internalUndo (bool isUndo);
    /** @internal */
    bool flushCurrentAction ();

    // the list of actions
    ActionBase::List undoHistory;

    // the current CoallascatedItem that collects all new actions.
    ActionBase::Ptr currentlyBuiltAction;

    // the position in the timeline for the next actions.
    int nextUndoAction = -1;
    int nextRedoAction = -1;

    // The undomanager can be temporarily suspended using a ScopedSuspender

    ThreadLocalValue<bool> suspended;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UndoManager);
};
}
