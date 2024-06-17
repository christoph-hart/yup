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

String Clipboard::paste()
{
	if(auto cl = ScopedClipboardLoader())
	{
		ScopedDataAccess data(cl.getClipboardData());
		return data.toString();
	}

	return {};
}

void Clipboard::copy(const String& text)
{
	if(auto cl = ScopedClipboardLoader())
	{
		if(auto buffer = cl.allocate(text))
		{
			ScopedDataAccess data(buffer);
			data.writeString(text);
			cl.writeToClipboard(data.getData());
		}
	}
}

const wchar_t* Clipboard::ScopedDataAccess::getData() const
{
	return static_cast<const wchar_t*>(data);
}

wchar_t* Clipboard::ScopedDataAccess::getData()
{
	return static_cast<wchar_t*>(data);
}

void Clipboard::ScopedDataAccess::writeString(const String& text)
{
	auto bytesNeeded = CharPointer_UTF16::getBytesRequiredFor (text.getCharPointer()) + 4;
	text.copyToUTF16(getData(), bytesNeeded);
}

String Clipboard::ScopedDataAccess::toString() const
{
	return String(getData(), getNumBytes());
}

} // namespace yup