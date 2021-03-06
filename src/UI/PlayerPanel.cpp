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
#include "../UI/PlayerPanel.h"

// C++ standard includes

// Falltergeist includes
#include "../Audio/Mixer.h"
#include "../Event/Event.h"
#include "../Event/Keyboard.h"
#include "../Game/Game.h"
#include "../Game/DudeObject.h"
#include "../Graphics/Renderer.h"
#include "../Input/Mouse.h"
#include "../State/ExitConfirm.h"
#include "../State/GameMenu.h"
#include "../State/Inventory.h"
#include "../State/LoadGame.h"
#include "../State/PipBoy.h"
#include "../State/PlayerEdit.h"
#include "../State/SaveGame.h"
#include "../State/Skilldex.h"
#include "../State/WorldMap.h"
#include "../UI/Image.h"
#include "../UI/ImageButton.h"
#include "../UI/SmallCounter.h"

// Third party includes

namespace Falltergeist
{
namespace UI
{

PlayerPanel::PlayerPanel() : UI::Base()
{
    auto game = Game::getInstance();
    auto renderer = game->renderer();
    auto mouse = game->mouse();

    _background = std::make_shared<Image>("art/intrface/iface.frm");
    _ui.push_back(_background);

    setX((renderer->width() - 640) / 2);
    setY(renderer->height() - _background->height());

    _background->setPosition(this->position());

    addEventHandler("mousein", [this, mouse](Event::Event* event)
    {
        mouse->pushState(Input::Mouse::Cursor::BIG_ARROW);
    });

    addEventHandler("mouseout", [this, mouse](Event::Event* event)
    {
        if (mouse->scrollState())
        {
            // this trick is needed for correct cursor type returning on scrolling
            auto state = mouse->state();
            mouse->popState();
            mouse->popState();
            mouse->pushState(state);
        }
        else
        {
            mouse->popState();
        }
    });

    // Change hand button
    _ui.push_back(std::make_shared<ImageButton>(ImageButton::Type::BIG_RED_CIRCLE, position() + Point(218, 5)));
    _ui.back()->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->changeHand(); });

    // Inventory button
    _ui.push_back(std::make_shared<ImageButton>(ImageButton::Type::PANEL_INVENTORY, position() + Point(211, 40)));
    _ui.back()->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->openInventory(); });

    // Options button
    _ui.push_back(std::make_shared<ImageButton>(ImageButton::Type::PANEL_OPTIONS, position() + Point(210, 61)));
    _ui.back()->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->openGameMenu(); });

    // Attack button
    _ui.push_back(std::make_shared<ImageButton>(ImageButton::Type::PANEL_ATTACK, position() + Point(267, 25)));

    // Hit points
    _hitPoints = std::make_shared<SmallCounter>(position() + Point(471, 40));
    _hitPoints->setType(SmallCounter::Type::SIGNED);
    _hitPoints->setNumber(game->player()->hitPoints());
    _ui.push_back(_hitPoints);

    // Armor class
    _armorClass = std::make_shared<SmallCounter>(position() + Point(472, 76));
    _armorClass->setType(SmallCounter::Type::SIGNED);
    _armorClass->setNumber(game->player()->armorClass());
    _ui.push_back(_armorClass);

    // Skilldex button
    _ui.push_back(std::make_shared<ImageButton>(ImageButton::Type::BIG_RED_CIRCLE, position() + Point(523, 5)));
    _ui.back()->addEventHandler("mouseleftclick", [this](Event::Event* event){
        this->openSkilldex();
    });

    // MAP button
    _ui.push_back(std::make_shared<ImageButton>(ImageButton::Type::PANEL_MAP, position() + Point(526, 39)));
    _ui.back()->addEventHandler("mouseleftclick", [this](Event::Event* event){
        this->openMap();
    });

    // CHA button
    _ui.push_back(std::make_shared<ImageButton>(ImageButton::Type::PANEL_CHA, position() + Point(526, 58)));
    _ui.back()->addEventHandler("mouseleftclick", [this](Event::Event* event){
        this->openCharacterScreen();
    });

    // PIP button
    _ui.push_back(std::make_shared<ImageButton>(ImageButton::Type::PANEL_PIP, position() + Point(526, 77)));
    _ui.back()->addEventHandler("mouseleftclick", [this](Event::Event* event){
        this->openPipBoy();
    });

    addEventHandler("keydown", [this](Event::Event* event) {
        this->onKeyDown(dynamic_cast<Event::Keyboard*>(event));
    });
}

PlayerPanel::~PlayerPanel()
{
}

Size PlayerPanel::size() const
{
    return _background->size();
}

void PlayerPanel::render(bool eggTransparency)
{
    for (auto it = _ui.begin(); it != _ui.end(); ++it)
    {
        (*it)->render();
    }

    // object in hand
    if (auto item = Game::getInstance()->player()->currentHandSlot())
    {
        auto itemUi = item->inventoryDragUi();
        itemUi->setPosition(position() + Point(360, 60) - itemUi->size() / 2);
        itemUi->render();
    }
}

void PlayerPanel::handle(Event::Event *event)
{
    UI::Base::handle(event);
    if (auto mouseEvent = dynamic_cast<Event::Mouse*>(event))
    {
        mouseEvent->setObstacle(false);
        mouseEvent->setHandled(false);
    }

    // object in hand
    if (auto item = Game::getInstance()->player()->currentHandSlot())
    {
        auto itemUi = item->inventoryDragUi();
        itemUi->handle(event);
    }

    for (auto it = _ui.rbegin(); it != _ui.rend(); ++it)
    {
        if (event->handled()) return;
        (*it)->handle(event);
    }
}

void PlayerPanel::think()
{
    UI::Base::think();

    auto game = Game::getInstance();

    for (auto it = _ui.begin(); it != _ui.end(); ++it)
    {
        (*it)->think();
    }

    // object in hand
    if (auto item = game->player()->currentHandSlot())
    {
        auto itemUi = item->inventoryDragUi();
        itemUi->think();
    }

}

void PlayerPanel::playWindowOpenSfx()
{
    Game::getInstance()->mixer()->playACMSound("sound/sfx/ib1p1xx1.acm");
}

void PlayerPanel::changeHand()
{
    auto player = Game::getInstance()->player();
    player->setCurrentHand(player->currentHand() == HAND::RIGHT ? HAND::LEFT : HAND::RIGHT);
    playWindowOpenSfx();

}

void PlayerPanel::openGameMenu()
{
    Game::getInstance()->pushState(new State::GameMenu());
    playWindowOpenSfx();
}

void PlayerPanel::openInventory()
{
    Game::getInstance()->pushState(new State::Inventory());
    playWindowOpenSfx();
}

void PlayerPanel::openSkilldex()
{
    Game::getInstance()->pushState(new State::Skilldex());
    playWindowOpenSfx();
}

void PlayerPanel::openMap()
{
    Game::getInstance()->pushState(new State::WorldMap());
    playWindowOpenSfx();
}

void PlayerPanel::openCharacterScreen()
{
    Game::getInstance()->pushState(new State::PlayerEdit());
    playWindowOpenSfx();
}

void PlayerPanel::openPipBoy()
{
    Game::getInstance()->pushState(new State::PipBoy());
    playWindowOpenSfx();
}

void PlayerPanel::onKeyDown(Event::Keyboard* event)
{
    switch (event->keyCode())
    {
        case SDLK_a:
            // @TODO: initiateCombat();
            break;
        case SDLK_c:
            openCharacterScreen();
            break;
        case SDLK_i:
            openInventory();
            break;
        case SDLK_p:
            if (event->controlPressed())
            {
                // @TODO: pause game
            }
            else
            {
                openPipBoy();
            }
            break;
        case SDLK_z:
            openPipBoy(); // @TODO: go to clock
            break;
        case SDLK_ESCAPE:
        case SDLK_o:
            openGameMenu();
            break;
        case SDLK_b:
            changeHand();
            break;
        // M button is handled in State::Location
        case SDLK_n:
            // @TODO: toggleItemMode();
            break;
        case SDLK_s:
            if (event->controlPressed())
            {
                openSaveGame();
            }
            else
            {
                openSkilldex();
            }
            break;
        case SDLK_l:
            if (event->controlPressed())
            {
                openLoadGame();
            }
            break;
        case SDLK_x:
            if (event->controlPressed())
            {
                Game::getInstance()->pushState(new State::ExitConfirm());
                playWindowOpenSfx();
            }
        case SDLK_SLASH:
            // @TODO: printCurrentTime();
            break;
        case SDLK_TAB:
            openMap();
            break;
        case SDLK_F1:
            // @TODO: help screen
            break;
        case SDLK_F2:
            // @TODO: volume down
            break;
        case SDLK_F3:
            // @TODO: volume up
            break;
        case SDLK_F4:
            if (!event->altPressed())
            {
                openSaveGame();
            }
            break;
        case SDLK_F5:
            openLoadGame();
            break;
        case SDLK_F6:
            // @TODO: quick save logic
            break;
        case SDLK_F7:
            // @TODO: quick load logic
            break;
        case SDLK_F10:
            Game::getInstance()->pushState(new State::ExitConfirm());
            playWindowOpenSfx();
            break;
        case SDLK_F12:
            // @TODO: save screenshot
            break;
    }
}

void PlayerPanel::openSaveGame()
{
    Game::getInstance()->pushState(new State::SaveGame());
    playWindowOpenSfx();
}

void PlayerPanel::openLoadGame()
{
    Game::getInstance()->pushState(new State::LoadGame());
    playWindowOpenSfx();
}

unsigned int PlayerPanel::pixel(const Point& pos)
{
    return _background->pixel(pos);
}

}
}
