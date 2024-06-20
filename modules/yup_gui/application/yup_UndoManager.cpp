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
UndoManager::UndoManager(bool startTimer)
{
	if(startTimer)
		setEnabled(true);
	else
		currentlyBuiltAction = new CoallascatedItem();
}

bool UndoManager::perform(ActionBase::Ptr f)
{
	if(isSynchronous)
		flushCurrentAction();

	if(f->call(false))
	{
		if(!suspended.get())
			dynamic_cast<CoallascatedItem*>(currentlyBuiltAction.get())->childItems.add(f);

		return true;
	}

	return false;
}

bool UndoManager::undo()
{
	return internalUndo(true);
}

bool UndoManager::redo()
{
	return internalUndo(false);
}

void UndoManager::setEnabled(bool shouldBeEnabled)
{
	if(isTimerRunning() != shouldBeEnabled)
	{
		if(shouldBeEnabled)
		{
			startTimer(500);
			currentlyBuiltAction = new CoallascatedItem();
		}
		else
		{
			stopTimer();
			currentlyBuiltAction = nullptr;
			undoHistory.clear();
		}
	}
	    
}

bool UndoManager::CoallascatedItem::call(bool isUndo)
{
	if(isUndo)
	{
		for(int i = childItems.size()-1; i >= 0; i--)
		{
			if(!childItems[i]->call(isUndo))
			{
				childItems.remove(i);
			}
		}
	}
	else
	{
		for(int i = 0; i < childItems.size(); i++)
		{
			if(!childItems[i]->call(isUndo))
			{
				childItems.remove(i--);
			}
		}
	}

	return !childItems.isEmpty();
}

bool UndoManager::CoallascatedItem::isEmpty() const
{
	return childItems.isEmpty();
}

void UndoManager::timerCallback()
{
	flushCurrentAction();
}

bool UndoManager::internalUndo(bool isUndo)
{
	flushCurrentAction();

	auto& idx = isUndo ? nextUndoAction : nextRedoAction;

	if(auto current = undoHistory[idx])
	{
		current->call(isUndo);

		auto delta = isUndo ? -1 : 1;
		nextUndoAction += delta;
		nextRedoAction += delta;
		return true;
	}

	return false;
}

bool UndoManager::flushCurrentAction()
{
	if(currentlyBuiltAction->isEmpty())
	{
		return false;
	}

	// Remove all future actions
	if(nextRedoAction < undoHistory.size())
		undoHistory.removeRange(nextRedoAction, undoHistory.size() - nextRedoAction);

	undoHistory.add(currentlyBuiltAction);
	currentlyBuiltAction = new CoallascatedItem();

	// Clean up to keep the undo history in check
	int numToRemove = jmax(0, undoHistory.size() - HistorySize);

	if(numToRemove > 0)
		undoHistory.removeRange(0, numToRemove);

	nextUndoAction = undoHistory.size() - 1;
	nextRedoAction = undoHistory.size();

	return true;
}
}
