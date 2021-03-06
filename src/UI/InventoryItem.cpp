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

// Related headers
#include "../UI/InventoryItem.h"

// C++ standard includes

// Falltergeist includes
#include "../Audio/Mixer.h"
#include "../Base/StlFeatures.h"
#include "../Event/Event.h"
#include "../Event/Mouse.h"
#include "../Game/DudeObject.h"
#include "../Game/Game.h"
#include "../Game/ItemObject.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Texture.h"
#include "../Input/Mouse.h"
#include "../Logger.h"
#include "../Point.h"
#include "../UI/Image.h"
#include "../UI/ItemsList.h"

// Third party includes

namespace Falltergeist
{
namespace UI
{
using namespace Base;

InventoryItem::InventoryItem(Game::ItemObject *item, const Point& pos) : Falltergeist::UI::Base(pos)
{
    _item = item;
    addEventHandler("mouseleftdown", [this](Event::Event* event){ this->onMouseLeftDown(dynamic_cast<Event::Mouse*>(event)); });
    addEventHandler("mousedragstart", [this](Event::Event* event){ this->onMouseDragStart(dynamic_cast<Event::Mouse*>(event)); });
    addEventHandler("mousedrag", [this](Event::Event* event){ this->onMouseDrag(dynamic_cast<Event::Mouse*>(event)); });
    addEventHandler("mousedragstop", [this](Event::Event* event){ this->onMouseDragStop(dynamic_cast<Event::Mouse*>(event)); });
}

InventoryItem::~InventoryItem()
{
}

InventoryItem::Type InventoryItem::type() const
{
    return _type;
}

void InventoryItem::setType(Type value)
{
    _type = value;
}

Graphics::Texture* InventoryItem::texture() const
{
    //if (!_texture)
    //{
    //    _texture = new Texture(this->width(), this->height());
    //    _texture->fill(0x000000ff);
    //}
    //return _texture;

    if (!_item) return 0;

    switch (_type)
    {
        case Type::SLOT:
            return _item->inventorySlotUi()->texture();
        case Type::DRAG:
            return _item->inventoryDragUi()->texture();
        default: {}
    }

    return _item->inventoryUi()->texture();
}

void InventoryItem::render(bool eggTransparency)
{
    //return ActiveUI::render();
    if (!_item) return;
    auto game = Game::getInstance();
    Size texSize = Size(texture()->width(), texture()->height());
    game->renderer()->drawTexture(texture(), position() + (this->size() - texSize) / 2);
}

unsigned int InventoryItem::pixel(const Point& pos)
{
    if (!_item) return 0;
    return Rect::inRect(pos, this->size()) ? 1 : 0;
}

Game::ItemObject* InventoryItem::item()
{
    return _item;
}

void InventoryItem::setItem(Game::ItemObject* item)
{
    _item = item;
}

void InventoryItem::onMouseLeftDown(Event::Mouse* event)
{
}

void InventoryItem::onMouseDragStart(Event::Mouse* event)
{
    Game::getInstance()->mouse()->pushState(Input::Mouse::Cursor::NONE);
    Game::getInstance()->mixer()->playACMSound("sound/sfx/ipickup1.acm");
    _oldType = type();
    setType(Type::DRAG);
}

void InventoryItem::onMouseDrag(Event::Mouse* event)
{
    setOffset(offset() + event->offset());
}

void InventoryItem::onMouseDragStop(Event::Mouse* event)
{
    Game::getInstance()->mouse()->popState();
    Game::getInstance()->mixer()->playACMSound("sound/sfx/iputdown.acm");
    setOffset({0, 0});
    setType(_oldType);

    auto itemevent = make_unique<Event::Mouse>("itemdragstop");
    itemevent->setPosition(event->position());
    itemevent->setTarget(this);
    emitEvent(std::move(itemevent));
}

void InventoryItem::onArmorDragStop(Event::Mouse* event)
{
    // Check if mouse is over this item
    if (!Rect::inRect(event->position(), position(), size()))
    {
        return;
    }

    if (ItemsList* itemsList = dynamic_cast<ItemsList*>(event->target()))
    {

        InventoryItem* draggedItem = itemsList->draggedItem();
        auto itemObject = draggedItem->item();
        itemsList->removeItem(draggedItem, 1);
        // place current armor back to inventory
        if (_item)
        {
            itemsList->addItem(this, 1);
        }
        this->setItem(itemObject);
        if (auto armor = dynamic_cast<Game::ArmorItemObject*>(itemObject))
        {
            Game::getInstance()->player()->setArmorSlot(armor);
        }
    }
}

void InventoryItem::onHandDragStop(Event::Mouse* event)
{
    // Check if mouse is over this item
    if (!Rect::inRect(event->position(), position(), size()))
    {
        return;
    }

    if (ItemsList* itemsList = dynamic_cast<ItemsList*>(event->target()))
    {
        InventoryItem* itemUi = itemsList->draggedItem();
        auto item = itemUi->item();
        itemsList->removeItem(itemUi, 1);
        // place current weapon back to inventory
        if (_item)
        {
            itemsList->addItem(this, 1);
        }
        this->setItem(item);
    }
}

Size InventoryItem::size() const
{
    switch (_type)
    {
        case Type::INVENTORY:
            return Size(70, 49);
        case Type::SLOT:
            return Size(90, 63);
        default:
            return Base::size();
    }
}

}
}
