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

#ifndef FALLTERGEIST_UI_SMALLCOUNTER_H
#define FALLTERGEIST_UI_SMALLCOUNTER_H

// C++ standard includes
#include <memory>

// Falltergeist includes
#include "../UI/Base.h"

// Third party includes

namespace Falltergeist
{
namespace UI
{

class Image;

class SmallCounter : public Falltergeist::UI::Base
{
public:
    enum class Color
    {
        WHITE = 1,
        YELLOW,
        RED
    };
    enum class Type
    {
        UNSIGNED = 0,
        SIGNED
    };

    SmallCounter(const Point& pos);
    ~SmallCounter() override;

    Graphics::Texture* texture() const override;

    Color color() const;
    void setColor(Color color);

    unsigned int length() const;
    void setLength(unsigned int length);

    signed int number() const;
    void setNumber(signed int number);

    Type type() const;
    void setType(Type type);

protected:
    Color _color = Color::WHITE;
    signed int _number = 0;
    unsigned int _length = 3;
    Type _type = Type::UNSIGNED;
    mutable std::unique_ptr<Graphics::Texture> _textureOnDemand;

    void setTexture(Graphics::Texture* texture) override;

private:
    // Hide unused field from childs.
    using Falltergeist::UI::Base::_texture;

    SmallCounter(const SmallCounter&) = delete;
    void operator=(const SmallCounter&) = delete;
    
};

}
}
#endif // FALLTERGEIST_UI_SMALLCOUNTER_H
