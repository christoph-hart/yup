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

/** A base class for handling popup menus (context menus). You can either use the OS native menus or a custom component (TODO). */
class JUCE_API PopupMenuBase
{
public:

	virtual ~PopupMenuBase() {};

	virtual void addSeparator() = 0;

    virtual void addItem(int itemId, const String& text, const String& shortCutString, bool isTicked=false, bool isActive=false) = 0;

    virtual void show(const std::function<bool(int)>& resultCallback) = 0;
};

class JUCE_API NativePopupMenu: public PopupMenuBase
{
public:

	NativePopupMenu(Component& parent);

	~NativePopupMenu() override;

	void addSeparator() override;

	void addItem(int itemId, const String& text, const String& shortCutString={}, bool isTicked=false, bool isActive=true) override;

	void show(const std::function<bool(int)>& resultCallback) override;

private:

    // The implementation is in native/yup_Windows.cpp / native/yup_macOS.mm
	struct Pimpl;
	Pimpl* pimpl;
};


}
