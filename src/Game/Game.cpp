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
#include "../Game/Game.h"

// C++ standard includes
#include <algorithm>
#include <sstream>

// Falltergeist includes
#include "../Audio/Mixer.h"
#include "../Base/StlFeatures.h"
#include "../CrossPlatform.h"
#include "../Event/Dispatcher.h"
#include "../Event/State.h"
#include "../Exception.h"
#include "../Game/DudeObject.h"
#include "../Game/Time.h"
#include "../Graphics/AnimatedPalette.h"
#include "../Graphics/Renderer.h"
#include "../Input/Mouse.h"
#include "../Logger.h"
#include "../Lua/Script.h"
#include "../ResourceManager.h"
#include "../Settings.h"
#include "../State/State.h"
#include "../State/Location.h"
#include "../UI/FpsCounter.h"
#include "../UI/TextArea.h"

// Third patry includes
#include <libfalltergeist/Gam/File.h>
#include <SDL_image.h>

namespace Falltergeist
{
namespace Game
{

using namespace Base;

Game* getInstance()
{
    return ::Falltergeist::Game::Game::getInstance();
}

Game::Game()
{
}

Game* Game::getInstance()
{
    return Base::Singleton<Game>::get();
}

void Game::init(std::unique_ptr<Settings> settings)
{
    if (_initialized) return;
    _initialized = true;

    _settings = std::move(settings);

    _eventDispatcher = make_unique<Event::Dispatcher>();

    _renderer = make_unique<Graphics::Renderer>(_settings->screenWidth(), _settings->screenHeight());

    Logger::info("GAME") << CrossPlatform::getVersion() << std::endl;
    Logger::info("GAME") << "Opensource Fallout 2 game engine" << std::endl;

    SDL_setenv("SDL_VIDEO_CENTERED", "1", 1);

    // Force ResourceManager to initialize instance.
    (void)ResourceManager::getInstance();

    renderer()->init();

    std::string version = CrossPlatform::getVersion();
    renderer()->setCaption(version.c_str());

    _mixer = make_unique<Audio::Mixer>();
    _mouse = make_unique<Input::Mouse>();
    _fpsCounter = make_unique<UI::FpsCounter>(renderer()->width() - 42, 2);

    version += " " + to_string(renderer()->size());
    version += " " + renderer()->name();

    _falltergeistVersion = make_unique<UI::TextArea>(version, 3, renderer()->height() - 10);
    _mousePosition = make_unique<UI::TextArea>("", renderer()->width() - 55, 14);
    _animatedPalette = make_unique<Graphics::AnimatedPalette>();
    _currentTime = make_unique<UI::TextArea>("", renderer()->size() - Point(150, 10));

    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
}

Game::~Game()
{
    shutdown();
}

void Game::shutdown()
{
    _mixer.reset();
    ResourceManager::getInstance()->shutdown();
    while (!_states.empty()) popState();
    _settings.reset();
}

void Game::pushState(State::State* state)
{
    _states.push_back(std::unique_ptr<State::State>(state));
    if (!state->initialized()) state->init();
    state->setActive(true);
    state->emitEvent(make_unique<Event::State>("activate"));
}

void Game::popState()
{
    if (_states.size() == 0) return;

    State::State* state = _states.back().get();
    _statesForDelete.emplace_back(std::move(_states.back()));
    _states.pop_back();
    state->setActive(false);
    state->emitEvent(make_unique<Event::State>("deactivate"));
}

void Game::setState(State::State* state)
{
    while (!_states.empty()) popState();
    pushState(state);
}

void Game::run()
{
    Logger::info("GAME") << "Starting main loop" << std::endl;
    _frame = 0;
    while (!_quit)
    {
        handle();
        think();
        render();
        SDL_Delay(1);
        _statesForDelete.clear();
        _frame++;
    }
    Logger::info("GAME") << "Stopping main loop" << std::endl;
}

void Game::quit()
{
    _quit = true;
}

void Game::setPlayer(std::unique_ptr<DudeObject> player)
{
    _player = std::move(player);
}

DudeObject* Game::player()
{
    return _player.get();
}

Input::Mouse* Game::mouse() const
{
    return _mouse.get();
}

State::Location* Game::locationState()
{
    for (auto& state : _states)
    {
        auto location = dynamic_cast<State::Location*>(state.get());
        if (location)
        {
            return location;
        }
    }
    return 0;
}

void Game::setGVAR(unsigned int number, int value)
{
    _initGVARS();
    if (number >= _GVARS.size())
    {
        throw Exception("Game::setGVAR(num, value) - num out of range: " + std::to_string(number));
    }
    _GVARS.at(number) = value;
}

int Game::GVAR(unsigned int number)
{
    _initGVARS();
    if (number >= _GVARS.size())
    {
        throw Exception("Game::GVAR(num) - num out of range: " + std::to_string(number));
    }
    return _GVARS.at(number);
}

void Game::_initGVARS()
{
    if (_GVARS.size() > 0) return;
    auto gam = ResourceManager::getInstance()->gamFileType("data/vault13.gam");
    for (auto gvar : *gam->GVARS())
    {
        _GVARS.push_back(gvar.second);
    }
}

State::State* Game::topState(unsigned offset) const
{
    return (_states.rbegin() + offset)->get();
}

std::vector<State::State*> Game::_getVisibleStates()
{
    std::vector<State::State*> subset;
    if (_states.size() == 0)
    {
        return subset;
    }
    // we must render all states from last fullscreen state to the top of stack
    auto it = _states.end();
    do
    {
        --it;
    }
    while (it != _states.begin() && !(*it)->fullscreen());

    for (; it != _states.end(); ++it)
    {
        subset.push_back((*it).get());
    }
    return subset;
}

std::vector<State::State*> Game::_getActiveStates()
{
    // we must handle all states from top to bottom of stack
    std::vector<State::State*> subset;

    auto it = _states.rbegin();
    // active states
    for (; it != _states.rend(); ++it)
    {
        auto state = it->get();
        if (!state->active())
        {
            state->emitEvent(make_unique<Event::State>("activate"));
            state->setActive(true);
        }
        subset.push_back(state);
        if (state->modal() || state->fullscreen())
        {
            ++it;
            break;
        }
    }
    // not active states
    for (; it != _states.rend(); ++it)
    {
        auto state = it->get();
        if (state->active())
        {
            state->emitEvent(make_unique<Event::State>("deactivate"));
            state->setActive(false);
        }
    }
    return subset;
}

Graphics::Renderer* Game::renderer()
{
    return _renderer.get();
}

Settings* Game::settings() const
{
    return _settings.get();
}

std::unique_ptr<Event::Event> Game::_createEventFromSDL(const SDL_Event& sdlEvent)
{
    switch (sdlEvent.type)
    {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            SDL_Keymod mods = SDL_GetModState();
            auto mouseEvent = make_unique<Event::Mouse>((sdlEvent.type == SDL_MOUSEBUTTONDOWN) ? "mousedown" : "mouseup");
            mouseEvent->setPosition({sdlEvent.button.x, sdlEvent.button.y});
            mouseEvent->setLeftButton(sdlEvent.button.button == SDL_BUTTON_LEFT);
            mouseEvent->setRightButton(sdlEvent.button.button == SDL_BUTTON_RIGHT);
            mouseEvent->setShiftPressed(mods & KMOD_SHIFT);
            mouseEvent->setControlPressed(mods & KMOD_CTRL);
            return std::move(mouseEvent);
        }
        case SDL_MOUSEMOTION:
        {
            auto mouseEvent = make_unique<Event::Mouse>("mousemove");
            mouseEvent->setPosition({sdlEvent.motion.x, sdlEvent.motion.y});
            mouseEvent->setOffset({sdlEvent.motion.xrel,sdlEvent.motion.yrel});
            return std::move(mouseEvent);
        }
        case SDL_KEYDOWN:
        {
            auto keyboardEvent = make_unique<Event::Keyboard>("keydown");
            keyboardEvent->setKeyCode(sdlEvent.key.keysym.sym);
            keyboardEvent->setAltPressed(sdlEvent.key.keysym.mod & KMOD_ALT);
            keyboardEvent->setShiftPressed(sdlEvent.key.keysym.mod & KMOD_SHIFT);
            keyboardEvent->setControlPressed(sdlEvent.key.keysym.mod & KMOD_CTRL);
            return std::move(keyboardEvent);
        }
        case SDL_KEYUP:
        {
            auto keyboardEvent = make_unique<Event::Keyboard>("keyup");
            keyboardEvent->setKeyCode(sdlEvent.key.keysym.sym);
            keyboardEvent->setAltPressed(sdlEvent.key.keysym.mod & KMOD_ALT);
            keyboardEvent->setShiftPressed(sdlEvent.key.keysym.mod & KMOD_SHIFT);
            keyboardEvent->setControlPressed(sdlEvent.key.keysym.mod & KMOD_CTRL);;

            // TODO: maybe we should make Game an EventTarget too?
            if (keyboardEvent->keyCode() == SDLK_F12)
            {
                auto texture = renderer()->screenshot();
                std::string name = std::to_string(SDL_GetTicks()) +  ".bmp";
                SDL_SaveBMP(texture->sdlSurface(), name.c_str());
                Logger::info("GAME") << "Screenshot saved to " + name << std::endl;
            }
            return std::move(keyboardEvent);
        }
    }
    return std::unique_ptr<Event::Event>();
}

void Game::handle()
{
    if (_renderer->fading()) return;

    while (SDL_PollEvent(&_event))
    {
        if (_event.type == SDL_QUIT)
        {
            _quit = true;
        }
        else
        {
            auto event = _createEventFromSDL(_event);
            if (event)
            {
                for (auto state : _getActiveStates()) state->handle(event.get());
            }
        }
        // process events generate during handle()
        _eventDispatcher->processScheduledEvents();
    }
}

void Game::think()
{
    _fpsCounter->think();
    _mouse->think();

    _animatedPalette->think();

    *_mousePosition = "";
    *_mousePosition << mouse()->position().x() << " : " << mouse()->position().y();

    *_currentTime = "";
    *_currentTime << _gameTime.year()  << "-" << _gameTime.month()   << "-" << _gameTime.day() << " "
                  << _gameTime.hours() << ":" << _gameTime.minutes() << ":" << _gameTime.seconds() << " " << _gameTime.ticks();

    if (_renderer->fading())
    {
        return;
    }

    for (auto state : _getActiveStates())
    {
        state->think();
    }
    // process custom events
    _eventDispatcher->processScheduledEvents();
}

void Game::render()
{
    renderer()->beginFrame();

    for (auto state : _getVisibleStates())
    {
        state->render();
    }

    if (settings()->displayFps())
    {
        _fpsCounter->render();
    }

    _falltergeistVersion->render();

    if (settings()->displayMousePosition())
    {
        _mousePosition->render();
    }

    _currentTime->render();
    _mouse->render();
    renderer()->endFrame();
}

Graphics::AnimatedPalette* Game::animatedPalette()
{
    return _animatedPalette.get();
}

Time* Game::gameTime()
{
    return &_gameTime;
}

Audio::Mixer* Game::mixer()
{
    return _mixer.get();
}

Event::Dispatcher* Game::eventDispatcher()
{
    return _eventDispatcher.get();
}

unsigned int Game::frame() const
{
    return _frame;
}
}
}
