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
#include "../State/PlayerCreateOptions.h"

// C++ standard includes

// Falltergeist includes
#include "../Audio/Mixer.h"
#include "../functions.h"
#include "../Game/Game.h"
#include "../Graphics/Renderer.h"
#include "../ResourceManager.h"
#include "../State/ExitConfirm.h"
#include "../State/LoadGame.h"
#include "../State/Location.h"
#include "../State/SaveGame.h"
#include "../State/SettingsMenu.h"
#include "../UI/Image.h"
#include "../UI/ImageButton.h"
#include "../UI/TextArea.h"

// Third party includes

namespace Falltergeist
{
namespace State
{

PlayerCreateOptions::PlayerCreateOptions() : State()
{
}

PlayerCreateOptions::~PlayerCreateOptions()
{
}

void PlayerCreateOptions::init()
{
    if (_initialized) return;
    State::init();

    setModal(true);
    setFullscreen(false);

    auto background = new UI::Image("art/intrface/opbase.frm");

    Point backgroundPos = Point((Game::getInstance()->renderer()->size() - background->size()) / 2);
    int backgroundX = backgroundPos.x();
    int backgroundY = backgroundPos.y();

    auto saveButton = new UI::ImageButton(UI::ImageButton::Type::OPTIONS_BUTTON, backgroundX+14, backgroundY+18);
    auto loadButton = new UI::ImageButton(UI::ImageButton::Type::OPTIONS_BUTTON, backgroundX+14, backgroundY+18+37);
    auto printToFileButton = new UI::ImageButton(UI::ImageButton::Type::OPTIONS_BUTTON, backgroundX+14, backgroundY+18+37*2);
    auto eraseButton = new UI::ImageButton(UI::ImageButton::Type::OPTIONS_BUTTON, backgroundX+14, backgroundY+18+37*3);
    auto doneButton = new UI::ImageButton(UI::ImageButton::Type::OPTIONS_BUTTON, backgroundX+14, backgroundY+18+37*4);

    saveButton->addEventHandler("mouseleftclick",    [this](Event::Event* event){ this->onSaveButtonClick(dynamic_cast<Event::Mouse*>(event)); });
    loadButton->addEventHandler("mouseleftclick",    [this](Event::Event* event){ this->onLoadButtonClick(dynamic_cast<Event::Mouse*>(event)); });
    printToFileButton->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->onPrintToFileButtonClick(dynamic_cast<Event::Mouse*>(event)); });
    eraseButton->addEventHandler("mouseleftclick",       [this](Event::Event* event){ this->onEraseButtonClick(dynamic_cast<Event::Mouse*>(event)); });
    doneButton->addEventHandler("mouseleftclick",        [this](Event::Event* event){ this->onDoneButtonClick(dynamic_cast<Event::Mouse*>(event)); });

    auto font = ResourceManager::getInstance()->font("font3.aaf", 0xb89c28ff);

    // label: save
    auto saveButtonLabel = new UI::TextArea(_t(MSG_EDITOR, 600), backgroundX+8, backgroundY+26);
    saveButtonLabel->setFont(font);
    saveButtonLabel->setWidth(150);
    saveButtonLabel->setHorizontalAlign(UI::TextArea::HorizontalAlign::CENTER);

    // label: load
    auto loadButtonLabel = new UI::TextArea(_t(MSG_EDITOR, 601), backgroundX+8, backgroundY+26+37);
    loadButtonLabel->setFont(font);
    loadButtonLabel->setWidth(150);
    loadButtonLabel->setHorizontalAlign(UI::TextArea::HorizontalAlign::CENTER);

    // label: print to file
    auto printToFileButtonLabel = new UI::TextArea(_t(MSG_EDITOR, 602), backgroundX+8, backgroundY+26+37*2);
    printToFileButtonLabel->setFont(font);
    printToFileButtonLabel->setWidth(150);
    printToFileButtonLabel->setHorizontalAlign(UI::TextArea::HorizontalAlign::CENTER);

    // label: erase
    auto eraseButtonLabel = new UI::TextArea(_t(MSG_EDITOR, 603), backgroundX+8, backgroundY+26+37*3);
    eraseButtonLabel->setFont(font);
    eraseButtonLabel->setWidth(150);
    eraseButtonLabel->setHorizontalAlign(UI::TextArea::HorizontalAlign::CENTER);

    // label: done
    auto doneButtonLabel = new UI::TextArea(_t(MSG_EDITOR, 604), backgroundX+8, backgroundY+26+37*4);
    doneButtonLabel->setFont(font);
    doneButtonLabel->setWidth(150);
    doneButtonLabel->setHorizontalAlign(UI::TextArea::HorizontalAlign::CENTER);

    background->setPosition(backgroundPos);

    addUI(background);

    addUI(saveButton);
    addUI(loadButton);
    addUI(printToFileButton);
    addUI(eraseButton);
    addUI(doneButton);

    addUI(saveButtonLabel);
    addUI(loadButtonLabel);
    addUI(printToFileButtonLabel);
    addUI(eraseButtonLabel);
    addUI(doneButtonLabel);
}

void PlayerCreateOptions::onSaveButtonClick(Event::Mouse* event)
{
//    Game::getInstance()->pushState(new SavePlayerStatState());
}

void PlayerCreateOptions::onLoadButtonClick(Event::Mouse* event)
{
//    Game::getInstance()->pushState(new LoadPlayerStatState());
}

void PlayerCreateOptions::onPrintToFileButtonClick(Event::Mouse* event)
{
//    Game::getInstance()->pushState(new SettingsMenu());
}

void PlayerCreateOptions::onEraseButtonClick(Event::Mouse* event)
{
//    Game::getInstance()->pushState(new EraseConfirmState());
}

void PlayerCreateOptions::onDoneButtonClick(Event::Mouse* event)
{
    Game::getInstance()->popState();
}

void PlayerCreateOptions::onKeyDown(Event::Keyboard* event)
{
    switch (event->keyCode())
    {
        case SDLK_ESCAPE:
        case SDLK_RETURN:
        case SDLK_d:
            Game::getInstance()->popState();
            break;
    }
}

}
}
