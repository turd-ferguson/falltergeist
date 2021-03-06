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
#include "../State/Location.h"

// C++ standard includes
#include <algorithm>
#include <cmath>
#include <list>

// Falltergeist includes
#include "../Audio/Mixer.h"
#include "../Base/StlFeatures.h"
#include "../Event/Mouse.h"
#include "../Exception.h"
#include "../Game/ContainerItemObject.h"
#include "../Game/Defines.h"
#include "../Game/DoorSceneryObject.h"
#include "../Game/DudeObject.h"
#include "../Game/ExitMiscObject.h"
#include "../Game/Game.h"
#include "../Game/Object.h"
#include "../Game/ObjectFactory.h"
#include "../Game/Time.h"
#include "../Game/WeaponItemObject.h"
#include "../Graphics/Renderer.h"
#include "../Input/Mouse.h"
#include "../LocationCamera.h"
#include "../Logger.h"
#include "../PathFinding/Hexagon.h"
#include "../PathFinding/HexagonGrid.h"
#include "../Point.h"
#include "../ResourceManager.h"
#include "../Settings.h"
#include "../State/CursorDropdown.h"
#include "../State/ExitConfirm.h"
#include "../State/MainMenu.h"
#include "../UI/Animation.h"
#include "../UI/AnimationQueue.h"
#include "../UI/Image.h"
#include "../UI/ImageButton.h"
#include "../UI/PlayerPanel.h"
#include "../UI/SmallCounter.h"
#include "../UI/TextArea.h"
#include "../UI/Tile.h"
#include "../UI/TileMap.h"
#include "../VM/VM.h"

// Third party includes
#include <libfalltergeist/Gam/File.h>
#include <libfalltergeist/Map/Elevation.h>
#include <libfalltergeist/Map/File.h>
#include <libfalltergeist/Map/Object.h>

namespace Falltergeist
{
namespace State
{

using namespace Base;

const int Location::DROPDOWN_DELAY = 350;
const int Location::KEYBOARD_SCROLL_STEP = 35;

Location::Location() : State()
{
    auto game = Game::getInstance();
    game->mouse()->setState(Input::Mouse::Cursor::ACTION);

    _camera = make_unique<LocationCamera>(game->renderer()->size(), Point(0, 0));

    _hexagonInfo = make_unique<UI::TextArea>("", game->renderer()->width() - 135, 25);
    _hexagonInfo->setHorizontalAlign(UI::TextArea::HorizontalAlign::RIGHT);

}

Location::~Location()
{
}

void Location::init()
{
    if (initialized()) return;
    State::init();

    setFullscreen(true);
    setModal(true);

    auto game = Game::getInstance();
    setLocation("maps/" + game->settings()->initialLocation() + ".map");

    _playerPanel = make_unique<UI::PlayerPanel>();
}

void Location::onStateActivate(Event::State* event)
{
}

void Location::onStateDeactivate(Event::State* event)
{
    _objectUnderCursor = nullptr;
    _actionCursorTicks = 0;
}

void Location::setLocation(const std::string& name)
{
    _floor = make_unique<UI::TileMap>();
    _roof = make_unique<UI::TileMap>();
    _hexagonGrid = make_unique<HexagonGrid>();
    _objects.clear();

    auto mapFile = ResourceManager::getInstance()->mapFileType(name);

    if (mapFile == nullptr)
    {
        auto defaultSettings = new Settings;
        Logger::warning() << "No such map: `" << name << "`; using default map" << std::endl;
        mapFile = ResourceManager::getInstance()->mapFileType("maps/" + defaultSettings->initialLocation() + ".map");
    }

    _currentElevation = mapFile->defaultElevation();

    // Set camera position on default
    camera()->setCenter(hexagonGrid()->at(mapFile->defaultPosition())->position());

    // Initialize MAP vars
    if (mapFile->MVARS()->size() > 0)
    {
        auto name = mapFile->name();
        auto gam = ResourceManager::getInstance()->gamFileType("maps/" + name.substr(0, name.find(".")) + ".gam");
        for (auto mvar : *gam->MVARS())
        {
            _MVARS.push_back(mvar.second);
        }
    }

    auto mapObjects = mapFile->elevations()->at(_currentElevation)->objects();

    auto ticks = SDL_GetTicks();
    // @todo remove old objects from hexagonal grid
    for (auto mapObject : *mapObjects)
    {
        auto object = Game::ObjectFactory::getInstance()->createObject(mapObject->PID());
        if (!object)
        {
            Logger::error() << "Location::setLocation() - can't create object with PID: " << mapObject->PID() << std::endl;
            continue;
        }

        object->setFID(mapObject->FID());
        object->setElevation(_currentElevation);
        object->setOrientation(mapObject->orientation());
        object->setLightRadius(mapObject->lightRadius());
        object->setLightIntensity(mapObject->lightIntensity());
        object->setFlags(mapObject->flags());

        if (auto exitGrid = dynamic_cast<Game::ExitMiscObject*>(object))
        {
            exitGrid->setExitMapNumber(mapObject->exitMap());
            exitGrid->setExitElevationNumber(mapObject->exitElevation());
            exitGrid->setExitHexagonNumber(mapObject->exitPosition());
            exitGrid->setExitDirection(mapObject->exitOrientation());
        }

        if (auto container = dynamic_cast<Game::ContainerItemObject*>(object))
        {
            for (auto child : *mapObject->children())
            {
                auto item = dynamic_cast<Game::ItemObject*>(Game::ObjectFactory::getInstance()->createObject(child->PID()));
                if (!item)
                {
                    Logger::error() << "Location::setLocation() - can't create object with PID: " << child->PID() << std::endl;
                    continue;
                }
                item->setAmount(child->ammount());
                container->inventory()->push_back(item);
            }
        }

        if (mapObject->scriptId() > 0)
        {
            auto intFile = ResourceManager::getInstance()->intFileType(mapObject->scriptId());
            if (intFile) object->setScript(new VM(intFile,object));
        }
        if (mapObject->mapScriptId() > 0 && mapObject->mapScriptId() != mapObject->scriptId())
        {
            auto intFile = ResourceManager::getInstance()->intFileType(mapObject->mapScriptId());
            if (intFile) object->setScript(new VM(intFile, object));
        }

        auto hexagon = hexagonGrid()->at(mapObject->hexPosition());
        Location::moveObjectToHexagon(object, hexagon);

        _objects.emplace_back(object);
    }
    Logger::info("GAME") << "Objects loaded in " << (SDL_GetTicks() - ticks) << std::endl;

    // Adding dude
    {
        auto player = Game::getInstance()->player();
        player->setArmorSlot(nullptr);
        // Just for testing
        {
            player->inventory()->push_back((Game::ItemObject*)Game::ObjectFactory::getInstance()->createObject(0x00000003)); // power armor
            player->inventory()->push_back((Game::ItemObject*)Game::ObjectFactory::getInstance()->createObject(0x0000004A)); // leather jacket
            player->inventory()->push_back((Game::ItemObject*)Game::ObjectFactory::getInstance()->createObject(0x00000011)); // combat armor
            player->inventory()->push_back((Game::ItemObject*)Game::ObjectFactory::getInstance()->createObject(0x00000071)); // purple robe
            auto leftHand = Game::ObjectFactory::getInstance()->createObject(0x0000000C); // minigun
            player->setLeftHandSlot(dynamic_cast<Game::WeaponItemObject*>(leftHand));
            auto rightHand = Game::ObjectFactory::getInstance()->createObject(0x00000007); // spear
            player->setRightHandSlot(dynamic_cast<Game::WeaponItemObject*>(rightHand));
        }
        player->setPID(0x01000001);
        player->setOrientation(mapFile->defaultOrientation());

        // Player script
        player->setScript(new VM(ResourceManager::getInstance()->intFileType(0), player));

        auto hexagon = hexagonGrid()->at(mapFile->defaultPosition());
        Location::moveObjectToHexagon(player, hexagon);
    }

    // Location script
    if (mapFile->scriptId() > 0)
    {
        _locationScript = make_unique<VM>(ResourceManager::getInstance()->intFileType(mapFile->scriptId()-1), nullptr);
    }

    // Generates floor and roof images
    {
        for (unsigned int i = 0; i != 100*100; ++i)
        {
            unsigned int tileX = std::ceil(((double)i)/100);
            unsigned int tileY = i%100;
            unsigned int x = (100 - tileY - 1)*48 + 32*(tileX - 1);
            unsigned int y = tileX*24 +(tileY - 1)*12 + 1;

            unsigned int tileNum = mapFile->elevations()->at(_currentElevation)->floorTiles()->at(i);
            if (tileNum > 1)
            {
                _floor->tiles().push_back(make_unique<UI::Tile>(tileNum, Point(x, y)));
            }

            tileNum = mapFile->elevations()->at(_currentElevation)->roofTiles()->at(i);
            if (tileNum > 1)
            {
                _roof->tiles().push_back(make_unique<UI::Tile>(tileNum, Point(x, y - 104)));
            }
        }
    }
}

std::vector<Input::Mouse::Icon> Location::getCursorIconsForObject(Game::Object* object)
{
    std::vector<Input::Mouse::Icon> icons;
    if (object->script() && object->script()->hasFunction("use_p_proc"))
    {
        icons.push_back(Input::Mouse::Icon::USE);
    }
    else if (dynamic_cast<Game::DoorSceneryObject*>(object))
    {
        icons.push_back(Input::Mouse::Icon::USE);
    }
    else if (dynamic_cast<Game::ContainerItemObject*>(object))
    {
        icons.push_back(Input::Mouse::Icon::USE);
    }

    switch (object->type())
    {
        case Game::Object::Type::ITEM:
            break;
        case Game::Object::Type::DUDE:
            icons.push_back(Input::Mouse::Icon::ROTATE);
            break;
        case Game::Object::Type::SCENERY:
            break;
        case Game::Object::Type::CRITTER:
            icons.push_back(Input::Mouse::Icon::TALK);
            break;
        default:
            break;
    }
    icons.push_back(Input::Mouse::Icon::LOOK);
    icons.push_back(Input::Mouse::Icon::INVENTORY);
    icons.push_back(Input::Mouse::Icon::SKILL);
    icons.push_back(Input::Mouse::Icon::CANCEL);
    return icons;
}


void Location::onObjectMouseEvent(Event::Event* event, Game::Object* object)
{
    if (!object) return;
    if (event->name() == "mouseleftdown")
    {
        _objectUnderCursor = object;
        _actionCursorTicks = SDL_GetTicks();
        _actionCursorButtonPressed = true;
    }
    else if (event->name() == "mouseleftclick")
    {
        auto icons = getCursorIconsForObject(object);
        if (icons.size() > 0)
        {
            handleAction(object, icons.front());
            _actionCursorButtonPressed = false;
        }
    }
}

void Location::onObjectHover(Event::Event* event, Game::Object* object)
{
    if (event->name() == "mouseout")
    {
        if (_objectUnderCursor == object)
            _objectUnderCursor = nullptr;
    }
    else
    {
        if (_objectUnderCursor == nullptr || event->name() == "mousein")
        {
            _objectUnderCursor = object;
            _actionCursorButtonPressed = false;
        }
        _actionCursorTicks = SDL_GetTicks();
    }
}

void Location::onBackgroundClick(Event::Mouse* event)
{
}

void Location::render()
{
    _floor->render();

    //render only flat objects first
    for (auto hexagon : _hexagonGrid->hexagons())
    {
        hexagon->setInRender(false);
        for (auto object : *hexagon->objects())
        {
            if (object->flat())
            {
                object->render();
                if (object->inRender())
                {
                    hexagon->setInRender(true);
                }
            }
        }
    }

    // now render all other objects
    for (auto hexagon : _hexagonGrid->hexagons())
    {
        hexagon->setInRender(false);
        for (auto object : *hexagon->objects())
        {
            if (!object->flat())
            {
                object->render();
                if (object->inRender())
                {
                    hexagon->setInRender(true);
                }
            }
        }
    }

    for (auto hexagon : _hexagonGrid->hexagons())
    {
        for (auto object : *hexagon->objects())
        {
            object->renderText();
        }
    }
    //_roof->render();
    if (active())
    {
        _hexagonInfo->render();
    }

    _playerPanel->render();
}

void Location::think()
{
    Game::getInstance()->gameTime()->think();

    _playerPanel->think();

    auto player = Game::getInstance()->player();

    for (auto& object : _objects)
    {
        object->think();
    }
    player->think();

    // location scrolling
    if (_scrollTicks + 10 < SDL_GetTicks())
    {
        _scrollTicks = SDL_GetTicks();
        int scrollDelta = 5;

        //Game::getInstance()->mouse()->setType(Mouse::ACTION);

        Point pScrollDelta = Point(
            _scrollLeft ? -scrollDelta : (_scrollRight ? scrollDelta : 0),
            _scrollTop ? -scrollDelta : (_scrollBottom ? scrollDelta : 0)
        );
        _camera->setCenter(_camera->center() + pScrollDelta);

        auto mouse = Game::getInstance()->mouse();

        // if scrolling is active
        if (_scrollLeft || _scrollRight || _scrollTop || _scrollBottom)
        {
            Input::Mouse::Cursor state;
            if (_scrollLeft)   state = Input::Mouse::Cursor::SCROLL_W;
            if (_scrollRight)  state = Input::Mouse::Cursor::SCROLL_E;
            if (_scrollTop)    state = Input::Mouse::Cursor::SCROLL_N;
            if (_scrollBottom) state = Input::Mouse::Cursor::SCROLL_S;
            if (_scrollLeft && _scrollTop)     state = Input::Mouse::Cursor::SCROLL_NW;
            if (_scrollLeft && _scrollBottom)  state = Input::Mouse::Cursor::SCROLL_SW;
            if (_scrollRight && _scrollTop)    state = Input::Mouse::Cursor::SCROLL_NE;
            if (_scrollRight && _scrollBottom) state = Input::Mouse::Cursor::SCROLL_SE;
            if (mouse->state() != state)
            {
                if (mouse->scrollState())
                {
                    mouse->popState();
                }
                mouse->pushState(state);
            }
        }
        // scrolling is not active
        else
        {
            if (mouse->scrollState())
            {
                mouse->popState();
            }
        }

    }
    if (_locationEnter)
    {
        _locationEnter = false;

        if (_locationScript) _locationScript->initialize();

        for (auto& object : _objects)
        {
            if (object->script())
            {
                object->script()->initialize();
            }
        }
        if (player->script())
        {
            player->script()->initialize();
        }

        if (_locationScript) _locationScript->call("map_enter_p_proc");

        // By some reason we need to use reverse iterator to prevent scripts problems
        // If we use normal iterators, some exported variables are not initialized on the moment
        // when script is called
        player->map_enter_p_proc();
        for (auto it = _objects.rbegin(); it != _objects.rend(); ++it)
        {
            (*it)->map_enter_p_proc();
        }
    }
    else
    {
        if (_scriptsTicks + 10000 < SDL_GetTicks())
        {
            _scriptsTicks = SDL_GetTicks();
            if (_locationScript)
            {
                _locationScript->call("map_update_p_proc");
            }
            for (auto& object : _objects)
            {
                object->map_update_p_proc();
            }
            player->map_update_p_proc();
        }
    }

    // action cursor stuff
    if (_objectUnderCursor && _actionCursorTicks && _actionCursorTicks + DROPDOWN_DELAY < SDL_GetTicks())
    {
        auto game = Game::getInstance();
        if (_actionCursorButtonPressed || game->mouse()->state() == Input::Mouse::Cursor::ACTION)
        {
            if (!_actionCursorButtonPressed && (_actionCursorLastObject != _objectUnderCursor))
            {
                _objectUnderCursor->look_at_p_proc();
                _actionCursorLastObject = _objectUnderCursor;
            }
            auto icons = getCursorIconsForObject(_objectUnderCursor);
            if (icons.size() > 0)
            {
                if (dynamic_cast<CursorDropdown*>(game->topState()) != nullptr)
                {
                    game->popState();
                }
                auto state = new CursorDropdown(std::move(icons), !_actionCursorButtonPressed);
                state->setObject(_objectUnderCursor);
                Game::getInstance()->pushState(state);
            }
        }
        _actionCursorButtonPressed = false;
        _actionCursorTicks = 0;
    }
}

void Location::toggleCursorMode()
{
    auto game = Game::getInstance();
    auto mouse = game->mouse();
    switch (mouse->state())
    {
        case Input::Mouse::Cursor::NONE: // just for testing
        {
            mouse->pushState(Input::Mouse::Cursor::ACTION);
            break;
        }
        case Input::Mouse::Cursor::ACTION:
        {
            auto hexagon = hexagonGrid()->hexagonAt(mouse->position() + _camera->topLeft());
            if (!hexagon)
            {
                break;
            }
            mouse->pushState(Input::Mouse::Cursor::HEXAGON_RED);
            mouse->ui()->setPosition(hexagon->position() - _camera->topLeft());
            _objectUnderCursor = nullptr;
            break;
        }
        case Input::Mouse::Cursor::HEXAGON_RED:
        {
            mouse->popState();
            break;
        }
        default:
            break;
    }
}

void Location::handle(Event::Event* event)
{
    _playerPanel->handle(event);
    if (event->handled()) return;

    auto game = Game::getInstance();
    if (auto mouseEvent = dynamic_cast<Event::Mouse*>(event))
    {
        auto mouse = Game::getInstance()->mouse();

        // Right button pressed
        if (mouseEvent->name() == "mousedown" && mouseEvent->rightButton())
        {
            toggleCursorMode();
            event->setHandled(true);
        }

        // Left button up
        if (mouseEvent->name() == "mouseup" && mouseEvent->leftButton())
        {
            switch (mouse->state())
            {
                case Input::Mouse::Cursor::HEXAGON_RED:
                {
                    // Here goes the movement
                    auto hexagon = hexagonGrid()->hexagonAt(mouse->position() + _camera->topLeft());
                    if (!hexagon)
                    {
                        break;
                    }

                    auto path = hexagonGrid()->findPath(game->player()->hexagon(), hexagon);
                    if (path.size())
                    {
                        game->player()->stopMovement();
                        game->player()->setRunning((_lastClickedTile != 0 && hexagon->number() == _lastClickedTile) || (mouseEvent->shiftPressed() != game->settings()->running()));
                        for (auto hexagon : path)
                        {
                            game->player()->movementQueue()->push_back(hexagon);
                        }
                        //moveObjectToHexagon(game->player(), hexagon);
                    }
                    event->setHandled(true);
                    _lastClickedTile = hexagon->number();
                    break;
                }
                default:
                    break;
            }
        }

        if (mouseEvent->name() == "mousemove")
        {
            auto hexagon = hexagonGrid()->hexagonAt(mouse->position() + _camera->topLeft());

            switch (mouse->state())
            {
                case Input::Mouse::Cursor::HEXAGON_RED:
                {
                    if (!hexagon)
                    {
                        break;
                    }
                    mouse->ui()->setPosition(hexagon->position() - _camera->topLeft());
                    break;
                }
                case Input::Mouse::Cursor::ACTION:
                {
                    // optimization to prevent FPS drops on mouse move
                    auto ticks = SDL_GetTicks();
                    if (ticks - _mouseMoveTicks < 50)
                    {
                        event->setHandled(true);
                    }
                    else
                    {
                        _mouseMoveTicks = ticks;
                    }
                    break;
                }
                default:
                    break;
            }

            int scrollArea = 8;
            Point evPos = mouseEvent->position();
            _scrollLeft = (evPos.x() < scrollArea);
            _scrollRight = (evPos.x() > game->renderer()->width()- scrollArea);
            _scrollTop = (evPos.y() < scrollArea);
            _scrollBottom = (evPos.y() > game->renderer()->height() - scrollArea);

            if (hexagon)
            {
                std::string text = "Hex number: " + std::to_string(hexagon->number()) + "\n";
                text += "Hex position: " + std::to_string(hexagon->number()%200) + "," + std::to_string((unsigned int)(hexagon->number()/200)) + "\n";
                text += "Hex coords: " + std::to_string(hexagon->position().x()) + "," + std::to_string(hexagon->position().y()) + "\n";
                auto dude = Game::getInstance()->player();
                auto hex = dude->hexagon();
                text += "Hex delta:\n dx=" + std::to_string(hex->cubeX()-hexagon->cubeX()) + "\n dy="+ std::to_string(hex->cubeY()-hexagon->cubeY()) + "\n dz=" + std::to_string(hex->cubeZ()-hexagon->cubeZ());
                _hexagonInfo->setText(text);
            }
            else
            {
                _hexagonInfo->setText("No hex");
            }
        }
        // let event fall down to all objects when using action cursor and within active view
        if (mouse->state() != Input::Mouse::Cursor::ACTION && mouse->state() != Input::Mouse::Cursor::NONE)
        {
            event->setHandled(true);
        }
    }

    if (auto keyboardEvent = dynamic_cast<Event::Keyboard*>(event))
    {
        if (event->name() == "keyup")
        {
            switch (keyboardEvent->keyCode())
            {
                // JUST FOR ANIMATION TESTING
                case SDLK_r:
                {
                    game->player()->setRunning(!game->player()->running());
                    break;
                }
            }
        }
        else if (event->name() == "keydown")
        {
            onKeyDown(keyboardEvent);
        }
        event->setHandled(true);
    }
    auto hexagons = _hexagonGrid->hexagons();
    for (auto it = hexagons.rbegin(); it != hexagons.rend(); ++it)
    {
        Hexagon* hexagon = *it;
        if (!hexagon->inRender()) continue;
        auto objects = hexagon->objects();
        for (auto itt = objects->rbegin(); itt != objects->rend(); ++itt)
        {
            if (event->handled()) return;
            auto object = *itt;
            if (!object->inRender()) continue;
            object->handle(event);
        }
    }
}

void Location::onKeyDown(Event::Keyboard* event)
{
    switch (event->keyCode())
    {
        case SDLK_m:
            toggleCursorMode();
            break;
        case SDLK_COMMA:
        {
            auto player = Game::getInstance()->player();
            player->setOrientation(player->orientation() + 5); // rotate left
            break;
        }
        case SDLK_PERIOD:
        {
            auto player = Game::getInstance()->player();
            player->setOrientation(player->orientation() + 1); // rotate right
            break;
        }
        case SDLK_HOME:
            centerCameraAtHexagon(Game::getInstance()->player()->hexagon());
            break;
        case SDLK_PLUS:
        case SDLK_KP_PLUS:
            // @TODO: increase brightness
            break;
        case SDLK_MINUS:
        case SDLK_KP_MINUS:
            // @TODO: decrease brightness
            break;
        case SDLK_1:
            // @TODO: use skill: sneak
            break;
        case SDLK_2:
            // @TODO: use skill: lockpick
            break;
        case SDLK_3:
            // @TODO: use skill: steal
            break;
        case SDLK_4:
            // @TODO: use skill: traps
            break;
        case SDLK_5:
            // @TODO: use skill: first aid
            break;
        case SDLK_6:
            // @TODO: use skill: doctor
            break;
        case SDLK_7:
            // @TODO: use skill: science
            break;
        case SDLK_8:
            // @TODO: use skill: repair
            break;
        case SDLK_LEFT:
            _camera->setCenter(_camera->center() + Point(-KEYBOARD_SCROLL_STEP, 0));
            break;
        case SDLK_RIGHT:
            _camera->setCenter(_camera->center() + Point(KEYBOARD_SCROLL_STEP, 0));
            break;
        case SDLK_UP:
            _camera->setCenter(_camera->center() + Point(0, -KEYBOARD_SCROLL_STEP));
            break;
        case SDLK_DOWN:
            _camera->setCenter(_camera->center() + Point(0, KEYBOARD_SCROLL_STEP));
            break;
    }
}


LocationCamera* Location::camera()
{
    return _camera.get();
}

void Location::setMVAR(unsigned int number, int value)
{
    if (number >= _MVARS.size())
    {
        throw Exception("Location::setMVAR(num, value) - num out of range: " + std::to_string((int)number));
    }
    _MVARS.at(number) = value;
}

int Location::MVAR(unsigned int number)
{
    if (number >= _MVARS.size())
    {
        throw Exception("Location::MVAR(num) - num out of range: " + std::to_string((int)number));
    }
    return _MVARS.at(number);
}

std::map<std::string, VMStackValue>* Location::EVARS()
{
    return &_EVARS;
}

void Location::moveObjectToHexagon(Game::Object* object, Hexagon* hexagon)
{
    auto oldHexagon = object->hexagon();
    if (oldHexagon)
    {
        for (auto it = oldHexagon->objects()->begin(); it != oldHexagon->objects()->end(); ++it)
        {
            if (*it == object)
            {
                oldHexagon->objects()->erase(it);
                break;
            }
        }

        /* JUST FOR EXIT GRIDS TESTING
        for (auto obj : *hexagon->objects())
        {
            if (auto exitGrid = dynamic_cast<GameExitMiscObject*>(obj))
            {
                auto &debug = Logger::critical("LOCATION");
                debug << " PID: 0x" << std::hex << exitGrid->PID() << std::dec << std::endl;
                debug << " name: " << exitGrid->name() << std::endl;
                debug << " exitMapNumber: " << exitGrid->exitMapNumber() << std::endl;
                debug << " exitElevationNumber: " << exitGrid->exitElevationNumber() << std::endl;
                debug << " exitHexagonNumber: " << exitGrid->exitHexagonNumber() << std::endl;
                debug << " exitDirection: " << exitGrid->exitDirection() << std::endl << std::endl;
            }
        }
        */
    }

    object->setHexagon(hexagon);
    hexagon->objects()->push_back(object);
}

void Location::destroyObject(Game::Object* object)
{
    auto objectsAtHex = object->hexagon()->objects();
    object->destroy_p_proc();
    for (auto it = objectsAtHex->begin(); it != objectsAtHex->end(); ++it)
    {
        if (*it == object)
        {
            objectsAtHex->erase(it);
            break;
        }
    }
    if (_objectUnderCursor == object) _objectUnderCursor = nullptr;
    for (auto it = _objects.begin(); it != _objects.end(); ++it)
    {
        if ((*it).get() == object)
        {
            _objects.erase(it);
            break;
        }
    }
}

void Location::centerCameraAtHexagon(Hexagon* hexagon)
{
    _camera->setCenter(hexagon->position());
}

void Location::centerCameraAtHexagon(int tileNum)
{
    try
    {
        centerCameraAtHexagon(_hexagonGrid->at((unsigned int)tileNum));
    }
    catch (std::out_of_range ex)
    {
        throw Exception(std::string("Tile number out of range: ") + std::to_string(tileNum));
    }
}

void Location::handleAction(Game::Object* object, Input::Mouse::Icon action)
{
    switch (action)
    {
        case Input::Mouse::Icon::LOOK:
        {
            object->description_p_proc();
            break;
        }
        case Input::Mouse::Icon::USE:
        {
            auto player = Game::getInstance()->player();
            auto animation = player->setActionAnimation("al");
            animation->addEventHandler("actionFrame", [object, player](Event::Event* event){ object->onUseAnimationActionFrame(event, player); });
            break;
        }
        case Input::Mouse::Icon::ROTATE:
        {
            auto dude = dynamic_cast<Game::DudeObject*>(object);
            if (!dude) throw Exception("Location::handleAction() - only Dude can be rotated");

            auto orientation = dude->orientation() + 1;
            if (orientation > 5) orientation = 0;
            dude->setOrientation(orientation);

            break;
        }
        case Input::Mouse::Icon::TALK:
        {
            if (auto critter = dynamic_cast<Game::CritterObject*>(object))
            {
                critter->talk_p_proc();
            }
            else
            {
                throw Exception("Location::handleAction() - can talk only with critters!");
            }
        }
        default: {}
    }
}

void Location::displayMessage(const std::string& message)
{
    Game::getInstance()->mixer()->playACMSound("sound/sfx/monitor.acm");
    Logger::info("MESSAGE") << message << std::endl;
}

HexagonGrid* Location::hexagonGrid()
{
    return _hexagonGrid.get();
}

UI::PlayerPanel* Location::playerPanel()
{
    return _playerPanel.get();
}

}
}
