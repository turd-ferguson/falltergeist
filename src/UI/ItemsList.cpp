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
#include "../UI/ItemsList.h"

// C++ standard includes

// Falltergeist includes
#include "../Audio/Mixer.h"
#include "../Base/StlFeatures.h"
#include "../Event/Event.h"
#include "../Event/Mouse.h"
#include "../Game/ArmorItemObject.h"
#include "../Game/DudeObject.h"
#include "../Game/Game.h"
#include "../Game/ItemObject.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Texture.h"
#include "../Input/Mouse.h"
#include "../Logger.h"
#include "../Point.h"
#include "../UI/InventoryItem.h"

// Third party includes

namespace Falltergeist
{
namespace UI
{

using namespace Base;

ItemsList::ItemsList(const Point& pos) : Falltergeist::UI::Base(pos)
{
    _generateTexture(_slotWidth, _slotHeight * _slotsNumber);
    _texture->fill(0x000000FF);

    addEventHandler("mouseleftdown",  [this](Event::Event* event){ this->onMouseLeftDown(dynamic_cast<Event::Mouse*>(event)); });
    addEventHandler("mousedragstart", [this](Event::Event* event){ this->onMouseDragStart(dynamic_cast<Event::Mouse*>(event)); });
    addEventHandler("mousedrag",      [this](Event::Event* event){ this->onMouseDrag(dynamic_cast<Event::Mouse*>(event)); });
    addEventHandler("mousedragstop",  [this](Event::Event* event){ this->onMouseDragStop(dynamic_cast<Event::Mouse*>(event)); });
}

void ItemsList::setItems(std::vector<Game::ItemObject*>* items)
{
    _items = items;
    update();
}

std::vector<Game::ItemObject*>* ItemsList::items()
{
    return _items;
}

void ItemsList::update()
{
    _inventoryItems.clear();

    for (unsigned int i = _slotOffset; i < items()->size() && i != _slotOffset + _slotsNumber; i++)
    {
        _inventoryItems.push_back(std::unique_ptr<InventoryItem>(new InventoryItem(items()->at(i))));
    }
}

void ItemsList::render(bool eggTransparency)
{
    //ActiveUI::render();
    unsigned int i = 0;
    for (auto& item : _inventoryItems)
    {
        item->setPosition(position() + Point(0, _slotHeight*i));
        item->render();
        i++;
    }
}

unsigned int ItemsList::pixel(const Point& pos)
{
    unsigned int i = 0;
    for (auto& item : _inventoryItems)
    {
        unsigned int pixel = item->pixel(pos - Point(0, _slotHeight*i));
        if (pixel) return pixel;
        i++;
    }
    return 0;
}

std::vector<std::unique_ptr<InventoryItem>>& ItemsList::inventoryItems()
{
    return _inventoryItems;
}

void ItemsList::onMouseLeftDown(Event::Mouse* event)
{
    Logger::critical() << "mouseleftdown" << std::endl;
}

void ItemsList::onMouseDragStart(Event::Mouse* event)
{
    unsigned int index = (event->position().y() - y())/_slotHeight;
    if (index < _inventoryItems.size())
    {
        Game::getInstance()->mouse()->pushState(Input::Mouse::Cursor::NONE);
        Game::getInstance()->mixer()->playACMSound("sound/sfx/ipickup1.acm");
        _draggedItem = _inventoryItems.at(index).get();
        _draggedItem->setType(InventoryItem::Type::DRAG);
        _draggedItem->setOffset((event->position() - _draggedItem->position()) - (_draggedItem->size() / 2));
        Logger::critical() << "mousedragstart at " << index << " (" << _draggedItem->item()->name() << ")" << std::endl;
    }
    else
    {
        _draggedItem = nullptr;
    }
}

void ItemsList::onMouseDrag(Event::Mouse* event)
{
    if (_draggedItem)
    {
        _draggedItem->setOffset(_draggedItem->offset() + event->offset());
    }
    Logger::critical() << "mousedrag " << event->position() << ", " << event->offset() << std::endl;
}

void ItemsList::onMouseDragStop(Event::Mouse* event)
{
    if (_draggedItem)
    {
        Game::getInstance()->mouse()->popState();
        Game::getInstance()->mixer()->playACMSound("sound/sfx/iputdown.acm");
        _draggedItem->setOffset(0, 0);
        _draggedItem->setType(_type);
        auto itemevent = make_unique<Event::Mouse>("itemdragstop");
        itemevent->setPosition(event->position());
        itemevent->setTarget(this);
        emitEvent(std::move(itemevent));
    }
    Logger::critical() << "mousedragstop" << std::endl;
}

void ItemsList::onItemDragStop(Event::Mouse* event)
{
    Logger::critical() << "itemdragstop" << std::endl;

    // check if mouse is in this item list
    if (!Rect::inRect(event->position(), position(), Size(_slotWidth, _slotHeight*_slotsNumber)))
    {
        return;
    }

    if (auto itemsList = dynamic_cast<ItemsList*>(event->target()))
    {
        // @todo create addItem method
        this->addItem(itemsList->draggedItem(), 1);

        itemsList->removeItem(itemsList->draggedItem(), 1);
        itemsList->update();
    }

    if (auto inventoryItem = dynamic_cast<UI::InventoryItem*>(event->target()))
    {
        // @todo create addItem method
        this->addItem(inventoryItem, 1);

        if (dynamic_cast<Game::ArmorItemObject*>(inventoryItem->item()) && inventoryItem->type() == InventoryItem::Type::SLOT)
        {
            Game::getInstance()->player()->setArmorSlot(nullptr);
        }
        inventoryItem->setItem(0);
    }

    Logger::critical() << "IN!" << std::endl;
}

InventoryItem* ItemsList::draggedItem()
{
    return _draggedItem;
}

void ItemsList::addItem(InventoryItem* item, unsigned int amount)
{
    _items->push_back(item->item());
    this->update();
}

void ItemsList::removeItem(InventoryItem* item, unsigned int amount)
{
    for (auto it = _items->begin(); it != _items->end(); ++it)
    {
        Game::ItemObject* object = *it;
        if (object == item->item())
        {
            _items->erase(it);
            break;
        }
    }
    this->update();
}

bool ItemsList::canScrollUp()
{
    return _slotOffset > 0;
}

bool ItemsList::canScrollDown()
{
    return _slotOffset + _slotsNumber < _items->size();
}

void ItemsList::scrollUp()
{
    _slotOffset--;
    this->update();
}

void ItemsList::scrollDown()
{
    _slotOffset++;
    this->update();
}

}
}
