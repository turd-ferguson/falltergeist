/*
 * Copyright 2012-2014 Falltergeist Developers.
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

// Falltergeist includes
#include "../../Logger.h"
#include "../../VM/Handlers/Opcode8131Handler.h"
#include "../../VM/VM.h"
#include "../../Game/ContainerItemObject.h"
#include "../../Game/DoorSceneryObject.h"

// Third party includes

namespace Falltergeist
{

Opcode8131Handler::Opcode8131Handler(VM* vm) : OpcodeHandler(vm)
{
}

void Opcode8131Handler::_run()
{
    Logger::debug("SCRIPT") << "[8131] [+] void obj_open(GameDoorSceneryObject* object) " << std::endl;
    auto object = _vm->dataStack()->popObject();
    if (!object)
    {
        _error("obj_open: object is NULL");
    }
    // @TODO: need some refactoring to get rid of this ugly if-elses
    if (auto door = dynamic_cast<Game::DoorSceneryObject*>(object))
    {
        door->setOpened(true);
    }
    else if (auto container = dynamic_cast<Game::ContainerItemObject*>(object))
    {
        container->setOpened(true);
    }
    else
    {
        _error("obj_open: object is not openable type!");
    }
}

}


