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

// C++ standard includes
#include <sstream>

// Falltergeist includes
#include "../functions.h"
#include "../Game/Game.h"
#include "../Graphics/Renderer.h"
#include "../ResourceManager.h"
#include "../Event/State.h"
#include "../Game/DudeObject.h"
#include "../State/NewGame.h"
#include "../State/Location.h"
#include "../State/PlayerCreate.h"
#include "../UI/Image.h"
#include "../UI/ImageButton.h"
#include "../UI/ImageList.h"
#include "../UI/TextArea.h"

// Third party includes

namespace Falltergeist
{
namespace State
{

NewGame::NewGame() : State()
{
}

NewGame::~NewGame()
{
    while(!_characters.empty())
    {
        if (_characters.back() != Game::getInstance()->player()) delete _characters.back();
        _characters.pop_back();
    }
}

void NewGame::init()
{
    if (_initialized) return;
    State::init();

    setFullscreen(true);
    setModal(true);

    auto renderer = Game::getInstance()->renderer();

    setX((renderer->width()  - 640)*0.5);
    setY((renderer->height() - 480)*0.5);

    addUI("background", new Image("art/intrface/pickchar.frm"));

    auto beginGameButton = addUI(new ImageButton(ImageButton::Type::SMALL_RED_CIRCLE, 81, 322));
    beginGameButton->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->onBeginGameButtonClick(dynamic_cast<Event::Mouse*>(event)); });

    auto editButton = addUI(new ImageButton(ImageButton::Type::SMALL_RED_CIRCLE, 436, 319));
    editButton->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->onEditButtonClick(dynamic_cast<Event::Mouse*>(event)); });

    auto createButton = addUI(new ImageButton(ImageButton::Type::SMALL_RED_CIRCLE, 81, 424));
    createButton->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->onCreateButtonClick(dynamic_cast<Event::Mouse*>(event)); });

    auto backButton = addUI(new ImageButton(ImageButton::Type::SMALL_RED_CIRCLE, 461, 424));
    backButton->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->onBackButtonClick(dynamic_cast<Event::Mouse*>(event)); });

    auto prevCharacterButton = addUI(new ImageButton(ImageButton::Type::LEFT_ARROW, 292, 320));
    prevCharacterButton->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->onPrevCharacterButtonClick(dynamic_cast<Event::Mouse*>(event)); });

    auto nextCharacterButton = addUI(new ImageButton(ImageButton::Type::RIGHT_ARROW, 318, 320));
    nextCharacterButton->addEventHandler("mouseleftclick", [this](Event::Event* event){ this->onNextCharacterButtonClick(dynamic_cast<Event::Mouse*>(event)); });

    addUI("images", new ImageList({
                                    "art/intrface/combat.frm",
                                    "art/intrface/stealth.frm",
                                    "art/intrface/diplomat.frm"
                                    }, 27, 23));

    addUI("name", new TextArea(300, 40));

    addUI("stats_1", new TextArea(0, 70));
    getTextArea("stats_1")->setWidth(362);
    getTextArea("stats_1")->setHorizontalAlign(TextArea::HorizontalAlign::RIGHT);

    addUI("stats_2", new TextArea(374, 70));
    addUI("bio",     new TextArea(437, 40));

    addUI("stats_3", new TextArea(294, 150));
    getTextArea("stats_3")->setWidth(85);
    getTextArea("stats_3")->setHorizontalAlign(TextArea::HorizontalAlign::RIGHT);

    addUI("stats3_values", new TextArea(383, 150));

    auto combat = new Game::DudeObject();
    combat->loadFromGCDFile(ResourceManager::getInstance()->gcdFileType("premade/combat.gcd"));
    combat->setBiography(ResourceManager::getInstance()->bioFileType("premade/combat.bio")->text());
    _characters.push_back(combat);

    auto stealth = new Game::DudeObject();
    stealth->loadFromGCDFile(ResourceManager::getInstance()->gcdFileType("premade/stealth.gcd"));
    stealth->setBiography(ResourceManager::getInstance()->bioFileType("premade/stealth.bio")->text());
    _characters.push_back(stealth);

    auto diplomat = new Game::DudeObject();
    diplomat->loadFromGCDFile(ResourceManager::getInstance()->gcdFileType("premade/diplomat.gcd"));
    diplomat->setBiography(ResourceManager::getInstance()->bioFileType("premade/diplomat.bio")->text());
    _characters.push_back(diplomat);

    _changeCharacter();
}

void NewGame::think()
{
    State::think();
}

void NewGame::doBeginGame()
{
    auto player = _characters.at(_selectedCharacter);
    Game::getInstance()->setPlayer(player);
    Game::getInstance()->setState(new Location());
}

void NewGame::doEdit()
{
    Game::getInstance()->setPlayer(_characters.at(_selectedCharacter));
    Game::getInstance()->pushState(new PlayerCreate());
}

void NewGame::doCreate()
{
    auto none = new Game::DudeObject();
    none->loadFromGCDFile(ResourceManager::getInstance()->gcdFileType("premade/blank.gcd"));
    Game::getInstance()->setPlayer(none);
    Game::getInstance()->pushState(new PlayerCreate());
}

void NewGame::doBack()
{
    removeEventHandlers("fadedone");
    addEventHandler("fadedone", [this](Event::Event* event){ this->onBackFadeDone(dynamic_cast<Event::State*>(event)); });
    Game::getInstance()->renderer()->fadeOut(0,0,0,1000);
}

void NewGame::doNext()
{
    if (_selectedCharacter < 2)
    {
        _selectedCharacter++;
    }
    else
    {
        _selectedCharacter = 0;
    }
    _changeCharacter();
}

void NewGame::doPrev()
{
    if (_selectedCharacter > 0)
    {
        _selectedCharacter--;
    }
    else
    {
        _selectedCharacter = 2;
    }
    _changeCharacter();
}

void NewGame::onBackButtonClick(Event::Mouse* event)
{
    doBack();
}

void NewGame::onBackFadeDone(Event::State* event)
{
    removeEventHandlers("fadedone");
    Game::getInstance()->popState();
}

void NewGame::onPrevCharacterButtonClick(Event::Mouse* event)
{
    doPrev();
}

void NewGame::onNextCharacterButtonClick(Event::Mouse* event)
{
    doNext();
}

void NewGame::_changeCharacter()
{
    auto dude = _characters.at(_selectedCharacter);
   *getTextArea("stats_1") = "";
   *getTextArea("stats_1")
        << _t(MSG_STATS, 100) << " " << (dude->stat(STAT::STRENGTH)     < 10 ? "0" : "") << dude->stat(STAT::STRENGTH)     << "\n"
        << _t(MSG_STATS, 101) << " " << (dude->stat(STAT::PERCEPTION)   < 10 ? "0" : "") << dude->stat(STAT::PERCEPTION)   << "\n"
        << _t(MSG_STATS, 102) << " " << (dude->stat(STAT::ENDURANCE)    < 10 ? "0" : "") << dude->stat(STAT::ENDURANCE)    << "\n"
        << _t(MSG_STATS, 103) << " " << (dude->stat(STAT::CHARISMA)     < 10 ? "0" : "") << dude->stat(STAT::CHARISMA)     << "\n"
        << _t(MSG_STATS, 104) << " " << (dude->stat(STAT::INTELLIGENCE) < 10 ? "0" : "") << dude->stat(STAT::INTELLIGENCE) << "\n"
        << _t(MSG_STATS, 105) << " " << (dude->stat(STAT::AGILITY)      < 10 ? "0" : "") << dude->stat(STAT::AGILITY)      << "\n"
        << _t(MSG_STATS, 106) << " " << (dude->stat(STAT::LUCK)         < 10 ? "0" : "") << dude->stat(STAT::LUCK)         << "\n";

    *getTextArea("stats_2") = "";
    *getTextArea("stats_2")
        << _t(MSG_STATS, dude->stat(STAT::STRENGTH)     + 300) << "\n"
        << _t(MSG_STATS, dude->stat(STAT::PERCEPTION)   + 300) << "\n"
        << _t(MSG_STATS, dude->stat(STAT::ENDURANCE)    + 300) << "\n"
        << _t(MSG_STATS, dude->stat(STAT::CHARISMA)     + 300) << "\n"
        << _t(MSG_STATS, dude->stat(STAT::INTELLIGENCE) + 300) << "\n"
        << _t(MSG_STATS, dude->stat(STAT::AGILITY)      + 300) << "\n"
        << _t(MSG_STATS, dude->stat(STAT::LUCK)         + 300) << "\n";

    getTextArea("bio")->setText(dude->biography());
    getTextArea("name")->setText(dude->name());
    getImageList("images")->setCurrentImage(_selectedCharacter);

    std::string stats3  = _t(MSG_MISC,  16)  + "\n"    // Hit Points
                        + _t(MSG_STATS, 109) + "\n"     // Armor Class
                        + _t(MSG_MISC,  15)  + "\n"     // Action Points
                        + _t(MSG_STATS, 111) + "\n";    // Melee Damage

    std::string stats3_values   = std::to_string(dude->hitPointsMax()) + "/" + std::to_string(dude->hitPointsMax()) + "\n"
                                + std::to_string(dude->armorClass())   + "\n"
                                + std::to_string(dude->actionPoints()) + "\n"
                                + std::to_string(dude->meleeDamage())  + "\n";

    for (unsigned i = (unsigned)SKILL::SMALL_GUNS; i <= (unsigned)SKILL::OUTDOORSMAN; i++)
    {
        if (dude->skillTagged((SKILL)i))
        {
            stats3 += "\n" + _t(MSG_SKILLS, 100 + i);
            stats3_values += "\n" + std::to_string(dude->skillValue((SKILL)i)) + "%";
        }
    }
    for (unsigned i = (unsigned)TRAIT::FAST_METABOLISM; i <= (unsigned)TRAIT::GIFTED; i++)
    {
        if (dude->traitTagged((TRAIT)i))
        {
            stats3 += "\n" + _t(MSG_TRAITS, 100 + i);
        }
    }
    getTextArea("stats_3")->setText(stats3);
    getTextArea("stats3_values")->setText(stats3_values);
}

void NewGame::onEditButtonClick(Event::Mouse* event)
{
    doEdit();
}

void NewGame::onCreateButtonClick(Event::Mouse* event)
{
    doCreate();
}

void NewGame::onBeginGameButtonClick(Event::Mouse* event)
{
    doBeginGame();
}

void NewGame::onKeyDown(Event::Keyboard* event)
{
    switch (event->keyCode())
    {
        case SDLK_ESCAPE:
        case SDLK_b:
            doBack();
            break;
        case SDLK_t:
            doBeginGame();
            break;
        case SDLK_c:
            doCreate();
            break;
        case SDLK_m:
            doEdit();
            break;
        case SDLK_LEFT:
            doPrev();
            break;
        case SDLK_RIGHT:
            doNext();
            break;
    }
}

void NewGame::onStateActivate(Event::State* event)
{
    Game::getInstance()->renderer()->fadeIn(0,0,0,1000);
}


}
}
