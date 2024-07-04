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
Scrollbar::InternalViewport::InternalViewport (Scrollbar& sb_)
    : sb (sb_)
{
    sb.addListener (this);
    sb.parent.addMouseListener (this);
}

Scrollbar::InternalViewport::~InternalViewport ()
{
    sb.removeListener (this);
    sb.parent.removeMouseListener (this);
}

void Scrollbar::InternalViewport::setContentArea (float width, float height)
{
    content.setSize ({ width, height });
    updatePosition();
}

void Scrollbar::InternalViewport::updatePosition ()
{
    if (content.getHeight() > 0)
    {
        if (sb.t == Type::verticalScrollbar)
            sb.setHandleSize (sb.parent.getHeight() / content.getHeight());

        sb.updatePosition();
    }
}

yup::Rectangle<float> Scrollbar::InternalViewport::getViewport () const
{
    return content.transformed (transform);
}

yup::Rectangle<float> Scrollbar::InternalViewport::getVisibleArea () const
{
    return sb.parent.getLocalBounds().transformed (transform.inverted());
}

void Scrollbar::InternalViewport::scrollToShow (Range<float> yPos) const
{
    auto visibleArea = getVisibleArea();

    Range<float> yRange (visibleArea.getY(), visibleArea.getBottomLeft().getY());

    if (! yRange.intersects (yPos))
    {
        if (yPos.getStart() < visibleArea.getY())
        {
            auto normPos = yPos.getStart() / (content.getHeight() - yRange.getLength());
            sb.setPosition (normPos, sendNotification);
        }
        else
        {
            auto normPos = (yPos.getEnd() - yRange.getLength()) / (content.getHeight() - yRange.getLength());
            sb.setPosition (normPos, sendNotification);
        }
    }
}

bool Scrollbar::InternalViewport::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    auto before = sb.currentPosition;
    sb.mouseWheel (event, wheelData);
    return sb.currentPosition != before;
}

void Scrollbar::InternalViewport::onScroll (Type t, double newPos)
{
    positionViewport (t, sb.parent, newPos);
}

void Scrollbar::InternalViewport::positionViewport (yup::Scrollbar::Type t, Component& c, double newPos)
{
    if (t == yup::Scrollbar::Type::verticalScrollbar)
        transform = transform.translation (transform.getTranslateX(), -1.0f * newPos * (content.getHeight() - c.getHeight()));

    if (resizeOnScroll)
        c.resized();
    else
        c.repaint();
}

Scrollbar::Scrollbar (Type t_, Component& parent_)
    : t (t_)
    , parent (parent_)
{
    parent.addAndMakeVisible (*this);
}

void Scrollbar::mouseDown (const MouseEvent& event)
{
    updateFromEvent (event, false);
    down = true;
    lastPos = currentPosition;
    repaint();
    startTimer (15);
}

void Scrollbar::mouseUp (const MouseEvent& event)
{
    down = false;
    repaint();
}

void Scrollbar::setHandleSize (double normalisedHandleSize)
{
    handleSize = normalisedHandleSize;
    repaint();
    setVisible (handleSize < 1.0);

    if (! isVisible() && currentPosition != 0.0)
        setPosition (0.0, sendNotification);
}

void Scrollbar::timerCallback ()
{
    velocity = 0.6f * velocity + 0.4f * (currentPosition - lastPos);
    lastPos = currentPosition;

    if (! down)
    {
        velocity *= 0.9f;
        currentPosition += velocity;

        setPosition (currentPosition, sendNotification);

        if (std::abs (velocity) < 0.002f)
            stopTimer();
    }
}

void Scrollbar::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    auto newPos = jlimit (0.0, 1.0, currentPosition - (double) wheelData.getDeltaY() * 0.05);
    setPosition (newPos, sendNotification);
}

void Scrollbar::setPosition (double normalisedHandlePosition, juce::NotificationType notify)
{
    currentPosition = jlimit (0.0, 1.0, normalisedHandlePosition);

    if (notify)
        listeners.call ([this](Listener& l) { l.onScroll (this->t, this->currentPosition); });

    repaint();
}

void Scrollbar::updateFromEvent (const MouseEvent& e, bool updateIfInside)
{
    auto p = getBounds().getTopLeft();
    auto ep = e.getPosition();
    auto lp = ep - p;

    double normPos;

    if (t == Type::verticalScrollbar)
        normPos = lp.getY() / ((1.0 - handleSize) * getHeight());
    else
        normPos = lp.getX() / ((1.0 - handleSize) * getWidth());

    normPos -= 0.5 * handleSize;
    normPos = jlimit (0.0, 1.0, normPos);

    if (updateIfInside || ! Range<double> (currentPosition - 0.5 * handleSize, currentPosition + 0.5 * handleSize).contains (normPos))
        setPosition (normPos, sendNotification);
}

void Scrollbar::mouseDrag (const MouseEvent& event)
{
    updateFromEvent (event, true);
}

void Scrollbar::mouseEnter (const MouseEvent& event)
{
    over = true;
    repaint();
}

void Scrollbar::mouseExit (const MouseEvent& event)
{
    over = false;
    repaint();
}

void Scrollbar::updatePosition ()
{
    auto b = parent.getLocalBounds();

    if (t == Type::verticalScrollbar)
        b = b.removeFromRight (Thickness);
    else
        b = b.removeFromBottom (Thickness);

    setBounds (b);
}

void Scrollbar::paint (Graphics& g)
{
    auto b = getLocalBounds().reduced (1);

    g.setFillColor (Colors::white.withAlpha (over ? 0.1f : 0.0f));
    g.fillAll();

    float alpha = 0.2f;

    if (over)
        alpha += 0.2f;

    if (down)
        alpha += 0.2f;

    g.setFillColor (Colors::white.withAlpha (alpha));
    auto cornerSize = jmin (b.getWidth() * 0.5f, b.getHeight() * 0.5f);

    b = b.removeFromTop (handleSize * b.getHeight()).withY (currentPosition * (getHeight() * (1.0 - handleSize)));

    g.fillRoundedRect (b, cornerSize);
}

void Scrollbar::addListener (Listener* l)
{
    listeners.add (l);
}

void Scrollbar::removeListener (Listener* l)
{
    listeners.remove (l);
}
} // namespace yup
