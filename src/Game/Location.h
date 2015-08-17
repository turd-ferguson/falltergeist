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

#ifndef FALLTERGEIST_GAME_LOCATION_H
#define FALLTERGEIST_GAME_LOCATION_H

// C++ standard includes
#include <map>
#include <string>
#include <vector>

// Falltergeist includes

// Third party includes

namespace Falltergeist
{
namespace Game
{

class Location
{
public:
    Location();
    ~Location();

    std::string name();
    void setName(const std::string& value);

    std::string filename();
    void setFilename(const std::string& value);

    std::string music();
    void setMusic(const std::string& value);

    std::map<std::string, unsigned int>* ambient();

    bool saveable();
    void setSaveable(bool value);

    bool removeBodies();
    void setRemoveBodies(bool value);

    bool pipboy();
    void setPipboy(bool value);

    std::map<unsigned int, unsigned int>* startPoints();

protected:
    std::string _name;
    std::string _filename;
    std::string _music;
    std::map<std::string, unsigned int> _ambient;
    bool _saveable = false;
    bool _removeBodies = false;
    bool _pipboy = true;
    std::map<unsigned int, unsigned int> _startPoints;
};

}
}
#endif // FALLTERGEIST_GAME_LOCATION_H
