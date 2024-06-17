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

/** A UI widget that displays text and lets you edit the content.
 *
 *  This is a merger of the juce::TextEditor and the juce::Label class. It implements most
 *  of the text editing logic from scratch, including:
 *
 *  - navigation with the mouse / key buttons, shows a cursor
 *  - entering text
 *  - clipboard access (copy / paste)
 *  - selection
 *  - undo support
 *
 *  Current limitations:
 *
 *  - windows only
 *  - no multiline support
 *  - it doesn't really detect the key strokes correctly (so you can't enter special characters).
 *  - the keyboard layout is not recognized (eg. Y / Z mixup between german and US layout).
 *
 */
class JUCE_API Label: public yup::Component,
					  public Timer
{
public:

    struct Listener
    {
        virtual ~Listener() {};

        /** This will be called on the message thread whenever the text changes. */
	    virtual void labelTextChanged(Label& label) = 0;
    };

    // ===============================================================================

    Label(const String& id={});;

    // ===============================================================================

    void keyDown(const KeyPress& keys, const Point<float>& position) override;
    void mouseDoubleClick(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;

    // ===============================================================================

    void paint(Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // ===============================================================================

    void setText(const juce::String& newText, juce::NotificationType notifyListeners);
    void setPadding(float newPadding);
    void setFont(const yup::Font& f, float newFontSize=16.0f);
    void setJustification(StyledText::Alignment newAlignment);

    void setMultiline(bool shouldBeMultiline)
    {
	    multiline = shouldBeMultiline;
    }

    // ===============================================================================

    void addListener(Listener* l);
    void removeListener(Listener* l);

    // ===============================================================================

private:

    void insert(const String& text);
    void insert(char textChar);
    void deleteSelection(int delta);
    void navigate(bool select, int delta);
    String getSelection() const;
    void perform(int command);

    void rebuildText();

    // A magic number for navigating to the start of the text
    static constexpr int BeginPos = -100000;

    // A magic number for navigating to the end of the text
    static constexpr int EndPos = 100000;

    // Home
    static constexpr int LineStart = -50000;

    // End
    static constexpr int LineEnd = 50000;

    // Down
    static constexpr int NextLine = 40000;

    // Up
    static constexpr int PrevLine = -40000;
    
    // Ctrl + C
    static constexpr int Copy = 100001;

    // Ctrl + X
    static constexpr int Cut = 100002;

    // Ctrl + V
    static constexpr int Paste = 100003;

    // Ctrl + Z
    static constexpr int Undo = 100004;

    // Ctrl + Y
    static constexpr int Redo = 100005;

    // Ctrl + A
    static constexpr int SelectAll = 100006;
    
    struct Updater: public AsyncUpdater
    {
        Updater(Label& parent_);

	    void handleAsyncUpdate() override;

    private:
        Label& parent;
    } updater;

    // Internal class for handling the cursor selection.
    struct Cursor
    {
        Cursor(const Label& l);;

        operator bool() const { return charIndex != -1; }

        bool moveToStart();
        bool moveToEnd();

        bool moveToStartOfLine()
        {
            auto prevIndex = charIndex;
            charIndex = parent.lineInformation[lineNumber].second.getStart();
            return prevIndex != charIndex;
        }

        bool moveToEndOfLine()
        {
            auto prevIndex = charIndex;
            charIndex = parent.lineInformation[lineNumber].second.getEnd();
            return prevIndex != charIndex;
        }

        bool moveTo(const Cursor& other);
        bool moveTo(int pos);
        bool move(int delta);

        bool moveLine(int delta)
        {
            auto xPos = getPosition().getX();

            auto prev = charIndex;

            auto newLineNumber = jlimit(0, parent.lineInformation.size()-1, lineNumber + delta);

            

            auto positionsInNewLine = parent.xPosRanges[newLineNumber];

            if(xPos > positionsInNewLine.getLast().getEnd())
            {
	            charIndex = parent.lineInformation[newLineNumber].second.getEnd()-1;
            }
            else
            {
	            for(int i = 0; i < positionsInNewLine.size(); i++)
	            {
		            if(positionsInNewLine[i].contains(xPos))
		            {
	                    auto normPos = (xPos - positionsInNewLine[i].getStart()) / positionsInNewLine[i].getLength();

	                    if(normPos > 0.5)
	                        i++;

			            charIndex = parent.lineInformation[newLineNumber].second.getStart() + i;
	                    break;
		            }
	            }
            }

            

            lineNumber = newLineNumber;

            return prev != charIndex;
        }


        bool updateFromMouseEvent(const MouseEvent& e);
        Rectangle<float> getPosition() const;
        void clear();

        Range<int> getSelection(const Cursor& other) const;

        RectangleList<float> getSelectionRectangles(const Cursor& other) const
        {
            RectangleList<float> list;

            auto firstPos = getPosition().withWidth(0.0f);
			auto secondPos = other.getPosition().withWidth(0.0f);

            if(charIndex > other.charIndex)
                std::swap(firstPos, secondPos);

            if(lineNumber == other.lineNumber)
            {
	            list.addWithoutMerge(firstPos.smallestContainingRectangle(secondPos));
            }
            else
            {
                auto minChar = jmin(charIndex, other.charIndex);
                auto maxChar = jmax(charIndex, other.charIndex);

                Cursor minLineEnd(parent);

                minLineEnd.moveTo(minChar);
                minLineEnd.moveToEndOfLine();
                auto firstLineEnd = minLineEnd.getPosition().withWidth(0.0f);

                Cursor maxLineStart(parent);
                maxLineStart.moveTo(maxChar);
                maxLineStart.moveToStartOfLine();

                auto lastLineStart = maxLineStart.getPosition().withWidth(0.0f);

                list.addWithoutMerge(firstPos.smallestContainingRectangle(firstLineEnd));
                list.addWithoutMerge(secondPos.smallestContainingRectangle(lastLineStart));
            }

            Range<int> lineRange(jmin(lineNumber, other.lineNumber), jmax(lineNumber, other.lineNumber));

            for(int i = lineRange.getStart() + 1; i < lineRange.getEnd(); i++)
            {
	            auto left = parent.xPosRanges[i].getFirst().getStart();
                auto y = parent.lineInformation[i].first;
                auto right = parent.xPosRanges[i].getLast().getEnd();
                auto w = right - left;
                auto h = parent.fontSize;

                list.addWithoutMerge({left, y, w, h});
            }

            return list;
        }

    private:

        void updateLineNumber()
        {
	        for(int i = 0; i < parent.lineInformation.size(); i++)
	        {
		        if(parent.lineInformation[i].second.contains(charIndex))
		        {
			        lineNumber = i;
                    break;
		        }
	        }
        }

        const Label& parent;
        int charIndex = -1;
        int lineNumber = 0;
    };

    yup::UndoManager um;

    bool isBeingEdited = false;

    bool multiline = false;

    juce::Array<Range<float>> getXPositions(int lineNumber) const
    {
	    return xPosRanges[lineNumber];
    }

    juce::ListenerList<Listener> listeners;
    juce::Array<juce::Array<Range<float>>> xPosRanges;
    Array<std::pair<float, Range<int>>> lineInformation;

    Cursor downCursor;
    Cursor dragCursor;
    
    StyledText::Alignment alignment = StyledText::Alignment::center;
    Font font;
    float fontSize = 16.0f;
    String content;
    StyledText text;

    float padding = 5.0f;
    float alpha = 0.0f;

    JUCE_DECLARE_WEAK_REFERENCEABLE(Label);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Label);
};


}
