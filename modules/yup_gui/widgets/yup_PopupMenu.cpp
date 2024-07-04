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
NativePopupMenu::NativePopupMenu (Component& parent)
{
    pimpl = new Pimpl (parent);
}

NativePopupMenu::~NativePopupMenu ()
{
    delete pimpl;
}

void NativePopupMenu::addSeparator ()
{
    pimpl->addSeparator();
}

void NativePopupMenu::addItem (int itemId,
                               const String& text,
                               const String& shortCutString,
                               bool isTicked,
                               bool isActive)
{
    pimpl->addItem (itemId, text, shortCutString, isTicked, isActive);
}

void NativePopupMenu::show (const std::function<bool  (int)>& resultCallback)
{
    pimpl->show (resultCallback);
}
} // namespace yup
