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

namespace yup {

Label::Label(const String& id):
	Component(id.isEmpty() ? "Label" : id),
	updater(*this),
	downCursor(*this),
	dragCursor(*this)
{
	MouseCursor mc(MouseCursor::StandardCursorTypes::IBeamCursor);
	setMouseCursor(mc);
	setWantsKeyboardFocus(true);
	startTimer(15);
}

void Label::setText(const juce::String& newText, juce::NotificationType notifyListeners)
{
	content = newText;

	if(!listeners.isEmpty() && notifyListeners != dontSendNotification)
	{
		if(!MessageManager::getInstance()->isThisTheMessageThread())
			updater.triggerAsyncUpdate();
		else
		{
			switch(notifyListeners)
			{
			case dontSendNotification: break;
			case sendNotification: break;
			case sendNotificationAsync: updater.triggerAsyncUpdate(); break;
			case sendNotificationSync: updater.handleAsyncUpdate();
			default: ;
			}
		}
	}

	rebuildText();
}

void Label::timerCallback()
{
	if(dragCursor)
	{
		alpha += 0.1f;
		repaint();
	}
}

void Label::setPadding(float newPadding)
{
	padding = newPadding;
	rebuildText();
}

void Label::keyDown(const KeyPress& keys, const Point<float>& position)
{
	if(keys.getKey() == KeyPress::escapeKey)
	{
		leaveFocus();
		dragCursor.clear();
		downCursor.clear();
		repaint();
		return;
	}
	if(keys.getKey() == KeyPress::enterKey)
	{
		if(multiline)
		{
			insert("\n");
		}
		else
		{
			leaveFocus();
			dragCursor.clear();
			downCursor.clear();
			repaint();
		}

		return;
	}

	KeyModifiers ctrl(2);

	std::vector<std::pair<std::pair<int, KeyModifiers>, int>> navCom;
	std::vector<std::pair<std::pair<int, KeyModifiers>, int>> functions;

	navCom.push_back({{ KeyPress::leftKey, {} }, -1});
	navCom.push_back({{ KeyPress::rightKey, {} }, 1});
	navCom.push_back({{ KeyPress::upKey, {} }, multiline ? PrevLine : BeginPos});
	navCom.push_back({{ KeyPress::downKey, {} }, multiline ? NextLine : EndPos});
	navCom.push_back({{ KeyPress::homeKey, {} }, LineStart});
	navCom.push_back({{ KeyPress::endKey, {} }, LineEnd});
	navCom.push_back({{ KeyPress::pageUpKey, {} }, BeginPos});
	navCom.push_back({{ KeyPress::pageDownKey, {} }, EndPos});
	navCom.push_back({{ KeyPress::leftKey, ctrl }, BeginPos});
	navCom.push_back({{ KeyPress::rightKey, ctrl }, EndPos});

	functions.push_back({{ KeyPress::textCKey, ctrl }, Copy});
	functions.push_back({{ KeyPress::textXKey, ctrl }, Cut});
	functions.push_back({{ KeyPress::textVKey, ctrl }, Paste});
	functions.push_back({{ KeyPress::textYKey, ctrl }, Undo});
	functions.push_back({{ KeyPress::textZKey, ctrl }, Redo}); // this is switched on german keyboards lol...
	functions.push_back({{ KeyPress::textAKey, ctrl }, SelectAll});

	for(const auto& nc: navCom)
	{
		if(nc.first.first == keys.getKey() && nc.first.second == keys.getModifiers().withoutFlags(KeyModifiers::shiftMask))
			return navigate(keys.getModifiers().isShiftDown(), nc.second);
	}

	for(const auto& f: functions)
	{
		if(f.first.first == keys.getKey() && f.first.second == keys.getModifiers())
			return perform(f.second);
	}

	if(keys.getKey() == KeyPress::deleteKey)
		return deleteSelection(1);
	if(keys.getKey() == KeyPress::backspaceKey)
		return deleteSelection(-1);

	auto code = keys.getKey();

	if(isPositiveAndBelow(code, 127) && CharacterFunctions::isPrintable((char)code))
	{
		if(!keys.getModifiers().isShiftDown())
			code = CharacterFunctions::toLowerCase((char)code);

		insert((char)code);
	}
}

void Label::mouseDoubleClick(const MouseEvent& e)
{
	perform(SelectAll);
}

void Label::mouseDown(const MouseEvent& event)
{
	alpha = 0.0f;

	if(!hasFocus())
	{
		isBeingEdited = true;
		takeFocus();
		perform(SelectAll);
	}
	else
	{
		if(event.getModifiers().isShiftDown())
		{
			dragCursor.updateFromMouseEvent(event);
		}
		else
		{
			downCursor.updateFromMouseEvent(event);
			dragCursor.moveTo(downCursor);
		}
	}

	repaint();
}

void Label::mouseDrag(const MouseEvent& event)
{
	dragCursor.updateFromMouseEvent(event);
	repaint();
}

void Label::paint(Graphics& g)
{
	g.setFillColor(0xFF555555);
	g.fillAll();

	if(!downCursor.getSelection(dragCursor).isEmpty())
	{
		auto selections = downCursor.getSelectionRectangles(dragCursor);

		g.setFillColor(0xFF888888);

		for(auto& s: selections)
			g.fillRoundedRect(s.enlarged(0.0f, 6.0f), 1.0f);
	}

	auto& cTouse = dragCursor ? dragCursor : downCursor;

	if(cTouse)
	{
		auto b = cTouse.getPosition();
		g.setFillColor(Colors::white.withAlpha(0.5f * std::cos(alpha) + 0.5f));
		g.fillRect(b.enlarged(0.0f, 4.0f).translated(-2.0f, 0.0f));
	}

	g.setStrokeColor(Colors::white);
	g.strokeFittedText(text, getLocalBounds(), StyledText::getRiveTextAlign(alignment));

	
}

void Label::resized()
{
	auto b = getLocalBounds();

	if(!b.area())
		return;

	b = b.reduced(padding);

	if(!multiline)
		b = b.withSizeKeepingCentre(b.getWidth(), fontSize);
	
	auto baseLineY = b.getY();
	lineInformation = text.layout (b, alignment);

	xPosRanges.clear();
	

	if(!text.getGlyphs().empty())
	{
		auto first = text.getGlyphs()[0].bounds().left();

		int lineNumber = 0;

		while(auto p = text.getParagraph(lineNumber))
		{
			float lastPos = 0.0f;
			float nextPos = 0.0f;

			Array<Range<float>> xPos;

			for(auto& r: p->runs)
			{
				for(auto& x: r.xpos)
				{
					lastPos = nextPos;
					nextPos = x;
					xPos.add({lastPos + first, nextPos + first});
				}
			}

			xPos.remove(0);

			xPosRanges.add(xPos);
			lineNumber++;
		}
	}
        
	repaint();
}

void Label::setFont(const yup::Font& f, float newFontSize)
{
	font = f;
	fontSize = newFontSize;
	rebuildText();
}

void Label::setJustification(StyledText::Alignment newAlignment)
{
	alignment = newAlignment;
	rebuildText();
}

void Label::addListener(Listener* l)
{
	listeners.add(l);
}

void Label::removeListener(Listener* l)
{
	listeners.remove(l);
}

void Label::insert(const String& text)
{
	auto range = downCursor.getSelection(dragCursor);
	auto left = range.getStart();
	auto right = range.getEnd();

	auto oldText = content.substring(left, right);
	auto posAfter = right - oldText.length() + text.length();

	um.perform<Label>(*this, [left, right, posAfter, oldText, text](Label& l, bool isUndo)
	{
		String newContent;

		auto& t = isUndo ? oldText : text;
		auto& r = isUndo ? right : posAfter;

		newContent.preallocateBytes(left + r + t.length());

		newContent << l.content.substring(0, left);
		newContent << t;
		newContent << l.content.substring(isUndo ? posAfter : right);

		l.setText(newContent, sendNotification);

		l.downCursor.moveTo(isUndo ? left : r);
		l.dragCursor.moveTo(r);
            
		l.alpha = 0.0f;

		return true;
	});
}

void Label::deleteSelection(int delta)
{
	if(downCursor.getSelection(dragCursor).isEmpty())
		downCursor.move(delta);

	insert("");
}

void Label::insert(char textChar)
{
	String t;
	t << textChar;
	insert(t);
}

void Label::navigate(bool select, int delta)
{
	alpha = 0.0f;

	dragCursor.move(delta);

	if(!select)
		downCursor.moveTo(dragCursor);
}

String Label::getSelection() const
{
	auto r = downCursor.getSelection(dragCursor);
	return content.substring(r.getStart(), r.getEnd());
}

void Label::perform(int command)
{
	switch(command)
	{
	case Cut: 
		Clipboard::copy(getSelection()); 
		deleteSelection(0);
		break;
	case Copy: 
		Clipboard::copy(getSelection());
		break;
	case Paste: 
		insert(Clipboard::paste());
		break;
	case SelectAll:
		downCursor.moveToStart();
		dragCursor.moveToEnd();
		break;
	case Undo:
		um.undo();
		break;
	case Redo:
		um.redo();
		break;
	}
}

void Label::rebuildText()
{
	text.clear();

	if(font.getFont() != nullptr && !content.isEmpty())
		text.appendText(font, fontSize, fontSize, content.toRawUTF8());

	resized();
}

Label::Updater::Updater(Label& parent_):
	parent(parent_)
{}

void Label::Updater::handleAsyncUpdate()
{
	parent.listeners.call([this](Label::Listener& l){ l.labelTextChanged(parent); });
}

Label::Cursor::Cursor(const Label& l):
	charIndex(-1),
	parent(l)
{}

bool Label::Cursor::moveToStart()
{
	return moveTo(0);
}

bool Label::Cursor::moveToEnd()
{
	return moveTo(INT_MAX);
}

bool Label::Cursor::updateFromMouseEvent(const MouseEvent& e)
{
	auto tl = parent.getBounds().getTopLeft();
	auto mousePos = e.getPosition();
	auto lp = mousePos - tl;

	lineNumber = 0;

	for(int i = 0; i < parent.lineInformation.size(); i++)
	{
		if(lp.getY() > parent.lineInformation[i].first)
		{
			lineNumber = i;
		}
	}
	
	if(parent.content.isEmpty())
		return moveToStart();

	auto xPos = parent.getXPositions(lineNumber);

	if(xPos[0].getStart() > lp.getX())
		return moveToStart();

	int posIndex = 0;

	if(xPos.getLast().getEnd() < lp.getX())
		return moveToEnd();
	         
	for(auto& p: xPos)
	{
		if(p.contains(lp.getX()))
		{
			auto normPos = (lp.getX() - p.getStart()) / (p.getLength());
			auto newIndex = normPos > 0.5 ? posIndex + 1 : posIndex;

			newIndex += parent.lineInformation[lineNumber].second.getStart();

			return moveTo(newIndex);
		}

		++posIndex;
	}

	return false;
}

Rectangle<float> Label::Cursor::getPosition() const
{
	Rectangle<float> area(0.0, parent.lineInformation[lineNumber].first, 2.0f, parent.fontSize);

	if(parent.content.isEmpty())
	{
		area.setY(parent.padding);

		switch(parent.alignment)
		{
		case StyledText::left: area.setX(parent.padding); break;
		case StyledText::center: area.setX(parent.getLocalBounds().getCenter().getX()); break;
		case StyledText::right: area.setX(parent.getLocalBounds().getWidth() - parent.padding);break;
		default: return {};
		}

		return area;
	}

	auto xpos = parent.xPosRanges[lineNumber];

	auto indexInLine = charIndex - parent.lineInformation[lineNumber].second.getStart();

	if(isPositiveAndBelow(indexInLine, xpos.size()))
		area.setX(xpos[indexInLine].getStart());
			    
	else
		area.setX(xpos.getLast().getEnd());

	return area;
}

bool Label::Cursor::moveTo(const Cursor& other)
{
	auto prev = charIndex;
	charIndex = other.charIndex;
	lineNumber = other.lineNumber;
	return prev != charIndex;
}

bool Label::Cursor::moveTo(int pos)
{
	auto prev = charIndex;
	charIndex = jlimit(0, parent.content.length(), pos);

	updateLineNumber();

	return prev != charIndex;
}

bool Label::Cursor::move(int delta)
{
	if(delta == LineEnd)
		return moveToEndOfLine();
	if(delta == LineStart)
		return moveToStartOfLine();
	if(delta == NextLine)
		return moveLine(1);
	if(delta == PrevLine)
		return moveLine(-1);

	auto prev = charIndex;

	charIndex = jlimit(0, parent.content.length(), charIndex + delta);

	updateLineNumber();

	return prev != charIndex;
}

void Label::Cursor::clear()
{
	charIndex = -1;
}

Range<int> Label::Cursor::getSelection(const Cursor& other) const
{
	auto l = jmin(charIndex, other.charIndex);
	auto r = jmax(charIndex, other.charIndex);

	return { l, r };
}
} // namespace yup