/*
 * Copyright 2012-2015 Falltergeist Developers.
 *
 * This file is part of Falltergeist.
 *
 * Falltergeist is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Falltergeist is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Falltergeist.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FALLTERGEIST_UI_BASE_H
#define FALLTERGEIST_UI_BASE_H

// C++ standard includes
#include <memory>

// Falltergeist includes
#include "../Event/EventTarget.h"
#include "../Point.h"

// Third party includes

namespace Falltergeist
{
namespace Graphics
{
    class Texture;
}

namespace UI
{

class Base : public Event::EventTarget
{
public:
    Base(int x = 0, int y = 0);
    Base(const Point& pos);
    ~Base() override;

    int x() const;
    void setX(int value);

    int y() const;
    void setY(int value);

    virtual unsigned width() const;
    virtual unsigned height() const;

    virtual Point position() const;
    virtual void setPosition(const Point& pos);

    virtual Point offset() const;
    virtual void setOffset(const Point& pos);
    void setOffset(int x, int y);

    virtual Graphics::Texture* texture() const;
    /**
     * Set to use pre-existing Texture object
     */
    virtual void setTexture(Graphics::Texture* texture);

    virtual bool visible() const;
    virtual void setVisible(bool value);

    /**
     * @brief Handles OS events coming from the State::handle().
     * Used in Event Capturing process.
     * This method is called first in the main loop (before think() and render()).
     */
    virtual void handle(Event::Event* event);
    /**
     * @brief Process any real-time actions at each frame.
     * This method is called after handle() but before render() in the main loop.
     */
    virtual void think();
    /**
     * @brief Render this UI element on game window.
     * This method is called last in the main loop (after handle() and think()).
     */
    virtual void render(bool eggTransparency = false);

    virtual Size size() const;

    virtual unsigned int pixel(const Point& pos);
    unsigned int pixel(unsigned int x, unsigned int y);

protected:
    Point _position;
    Point _offset;

    /**
     * Generate and set new blank texture with given size
     */
    void _generateTexture(unsigned int width, unsigned int height);

    Graphics::Texture* _texture = nullptr;
    std::unique_ptr<Graphics::Texture> _tmptex;
    bool _leftButtonPressed = false;
    bool _rightButtonPressed = false;
    bool _drag = false;
    bool _hovered = false;
    bool _visible = true;
    // @todo Should it really be here?
    std::string _downSound = "";
    std::string _upSound = "";

private:
    std::unique_ptr<Graphics::Texture> _generatedTexture;
};

}
}
#endif // FALLTERGEIST_UI_BASE_H
