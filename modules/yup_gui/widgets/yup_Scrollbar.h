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
/** A UI widget that positions itself at the edge of a parent component and allows
 *  to scroll within a bigger area than the component bounds.
 *
 */
class JUCE_API Scrollbar
    : public Component,
      public Timer
{
public:
    enum class Type
    {
        verticalScrollbar,
        //< right edge
        horizontalScrollbar //< bottom edge
    };

    /** A listener that will be notified when the scroll position has changed. */
    struct Listener
    {
        virtual void onScroll (Type t, double newPos) = 0;
    };

    /** This class will calculate the content area and can be used
     *  to simulate a bigger content than the actual bounds.
     *
     *  This is a different implementation than the juce::Viewport,
     *  but I found it to be easier to handle than always dragging around
     *  another component for the content.
     */
    struct InternalViewport
        : private Listener,
          private MouseListener
    {
        /** Creates an internal viewport. Usually this is a member variable
         *  of the main component that houses the scrollbar. */
        InternalViewport (Scrollbar& sb_);

        ~InternalViewport () override;

        void setResizeOnScroll (bool shouldCallResize)
        {
            resizeOnScroll = shouldCallResize;
        }

        // =======================================================================

        /** Call this when you know what size you want to have as a content.
         *  This is independent of the actual content size and will show a scrollbar if it
         *  exceeds the dimensions.
         *
         */
        void setContentArea (float width, float height);

        /** Call this in the resized() method of the main component and it will update the handle size
         *  and the scrollbar position.
         */
        void updatePosition ();

        /** Call this in the resized() or paint() method to get the content area that you then can populate
         *  or draw onto. It will be translated to match the scrollbar position.
         */
        yup::Rectangle<float> getViewport () const;

        /** Returns the area that is visible in the parent. */
        yup::Rectangle<float> getVisibleArea () const;

        AffineTransform getTransform () const { return transform; }

        void scrollToShow (Range<float> yPos) const;

    private:
        /** This forwards the mouse wheel event to the scroll bar. */
        bool mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;

        void onScroll (Type t, double newPos) override;

        void positionViewport (yup::Scrollbar::Type t, Component& c, double newPos);

        Scrollbar& sb;
        yup::AffineTransform transform;
        yup::Rectangle<float> content;

        bool resizeOnScroll = true;
    };

    /** Creates a scrollbar and adds it to the given component. */
    Scrollbar (Type t_, Component& parent_);;

    // =======================================================================

    /** Sets the size of the draggable handle (from 0 to 1). */
    void setHandleSize (double normalisedHandleSize);

    /** Sets the position and notifies the listeners. */
    void setPosition (double normalisedHandlePosition, juce::NotificationType notify);

    /** Call this in the resized() method to position the scrollbar at the edge */
    void updatePosition ();

    // =======================================================================

    void paint (Graphics& g) override;
    void timerCallback () override;

    // =======================================================================

    void addListener (Listener* l);
    void removeListener (Listener* l);

private:
    // =======================================================================

    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseEnter (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;

    // =======================================================================

    static constexpr int Thickness = 11;

    void updateFromEvent (const MouseEvent& e, bool updateIfInside);

    bool down = false;
    bool over = false;

    double handleSize = 0.25;
    double currentPosition = 0.0;
    double lastPos = 0.0f;
    double velocity = 0.0f;

    ListenerList<Listener> listeners;

    Component& parent;
    const Type t;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scrollbar);
};
}
