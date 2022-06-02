/*
**  Xbox360 USB Gamepad Userspace Driver
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HEADER_XBOXDRV_BUTTON_COMBINATION_MAP_HPP
#define HEADER_XBOXDRV_BUTTON_COMBINATION_MAP_HPP

#include <memory>
#include <vector>

#include "button_combination.hpp"

namespace xboxdrv {

typedef std::shared_ptr<ButtonCombination> ButtonCombinationPtr;

template<typename C>
class ButtonCombinationMap
{
private:
  struct Mapping
  {
    ButtonCombinationPtr m_combo;
    std::vector<ButtonCombinationPtr> m_supersets;
    bool m_state;
    C m_data;

    Mapping() :
      m_combo(),
      m_supersets(),
      m_state(),
      m_data()
    {}
  };

  typedef std::vector<Mapping> Mappings;
  Mappings m_mappings;

public:
  ButtonCombinationMap() :
    m_mappings()
  {}

  void add(const ButtonCombination& combo, const C& data)
  {
    Mapping mapping;
    mapping.m_combo.reset(new ButtonCombination(combo));
    mapping.m_state = false;
    mapping.m_data  = data;
    m_mappings.push_back(mapping);
  }

  void init(const ControllerMessageDescriptor& desc)
  {
    // init all bindings and clear obsolete superset mappings
    for(typename Mappings::iterator it = m_mappings.begin(); it != m_mappings.end(); ++it)
    {
      it->m_combo->init(desc);
      it->m_supersets.clear();
    }

    // BROKEN: need to filter out duplicate bindings

    // regenerate superset mappings
    for(typename Mappings::iterator it = m_mappings.begin(); it != m_mappings.end(); ++it)
    {
      // find which already bound combinations the new one is a
      // superset of and add it to the list
      for(typename Mappings::iterator j = m_mappings.begin(); j != m_mappings.end(); ++j)
      {
        if (&*it != &*j)
        {
          if (it->m_combo->is_subset_of(*j->m_combo))
          {
            it->m_supersets.push_back(j->m_combo);
          }
        }
      }
    }
  }

  void update(const std::bitset<256>& button_state)
  {
    for(typename Mappings::iterator i = m_mappings.begin(); i != m_mappings.end(); ++i)
    {
      if (i->m_combo->match(button_state))
      {
        // check if a superset matches
        bool superset_matches = false;
        for(std::vector<ButtonCombinationPtr>::iterator j = i->m_supersets.begin(); j != i->m_supersets.end(); ++j)
        {
          if ((*j)->match(button_state))
          {
            superset_matches = true;
            break;
          }
        }

        if (superset_matches)
        {
          i->m_state = false;
        }
        else
        {
          i->m_state = true;
        }
      }
      else
      {
        i->m_state = false;
      }
    }
  }

  typedef typename Mappings::iterator iterator;
  typedef typename Mappings::const_iterator const_iterator;

  iterator begin() { return m_mappings.begin(); }
  iterator end()   { return m_mappings.end(); }
};

} // namespace xboxdrv

#endif

/* EOF */
