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

/** A class that allows writing and reading text from the system clipboard. */
class JUCE_API Clipboard
{
public:

	static String paste();
	static void copy(const String& text);

private:

    struct ScopedClipboardLoader
    {
	    ScopedClipboardLoader();
		~ScopedClipboardLoader();

        void* getClipboardData() const { return handle; }

	    bool isEmpty() const noexcept;
	    void writeToClipboard(void* data);
	    void* allocate(const String& text);

	    operator bool() const noexcept { return ok; }

    private:

        void* handle = nullptr;
        const bool ok;
    };

    struct ScopedDataAccess
    {
	    ScopedDataAccess(void* handle_);
		~ScopedDataAccess();

        const wchar_t* getData() const;

	    wchar_t* getData();
	    size_t getNumBytes() const;
	    void writeString(const String& text);
	    String toString() const;

    private:

	    void* data;
        void* handle;
    };
};


} // namespace yup