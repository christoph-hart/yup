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

/** This file contains some platform specific implementations for Windows. */
#if JUCE_WINDOWS

#include <windows.h>

namespace yup
{

// Clipboard ====================================================================

Clipboard::ScopedClipboardLoader::ScopedClipboardLoader():
	ok(OpenClipboard (nullptr))
{
	if(ok)
	{
		handle = GetClipboardData (CF_UNICODETEXT);
	}
}

Clipboard::ScopedClipboardLoader::~ScopedClipboardLoader()
{
	if(ok)
	{
		CloseClipboard();
	}
}

bool Clipboard::ScopedClipboardLoader::isEmpty() const noexcept
{
	return EmptyClipboard();
}

void Clipboard::ScopedClipboardLoader::writeToClipboard(void* data)
{
	SetClipboardData (CF_UNICODETEXT, data);
}

void* Clipboard::ScopedClipboardLoader::allocate(const String& text)
{
	auto bytesNeeded = CharPointer_UTF16::getBytesRequiredFor (text.getCharPointer()) + 4;

	return GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT, bytesNeeded + sizeof (WCHAR));
}

Clipboard::ScopedDataAccess::ScopedDataAccess(void* handle_):
	handle(handle_)
{
	data = GlobalLock((HGLOBAL)handle);
}



size_t Clipboard::ScopedDataAccess::getNumBytes() const
{
	return static_cast<size_t>(GlobalSize((HGLOBAL)data));
}



Clipboard::ScopedDataAccess::~ScopedDataAccess()
{
	GlobalUnlock ((HGLOBAL)handle);
}

// MouseCursor

void MouseCursor::setCursor(void* nativeHandle)
{
    auto cursorId = IDC_ARROW;

    switch(type)
    {
    case StandardCursorTypes::NormalCursor: break;
    case StandardCursorTypes::DraggingHandCursor: cursorId = IDC_HAND; break;
    case StandardCursorTypes::IBeamCursor: cursorId = IDC_IBEAM; break;
    case StandardCursorTypes::numCursorTypes: break;
    default: ;
    }

    auto c = LoadCursor ((HINSTANCE)nullptr, cursorId);
    SetCursor(c);
}

} // namespace yup

#endif
