//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2018 SuperTuxKart-Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "network/tournament/qualification.hpp"


SuperTournamentQualification::SuperTournamentQualification()
{ 

}

SuperTournamentQualification::SuperTournamentQualification(std::string config_player_list)
{
    std::vector<std::string> splits = StringUtils::split(config_player_list, ' ');
    for (auto &split : splits)
        m_player_list.push_back(split);
}

SuperTournamentQualification::~SuperTournamentQualification() { }


int SuperTournamentQualification::getListIndex(std::string player_name) const
{
    auto it = std::find(m_player_list.begin(), m_player_list.end(), player_name);
    if (it == m_player_list.end())
        return -1;
    else
        return std::distance(m_player_list.begin(), it);
}

void SuperTournamentQualification::addPlayer(std::string player_name, int elo)
{
    if (std::find(m_player_list.begin(), m_player_list.end(), player_name) != m_player_list.end())
        m_player_list.push_back(player_name);
    m_player_elos[player_name] = elo;
}

void SuperTournamentQualification::removePlayer(std::string player_name)
{
    m_player_list.erase(m_player_list.begin() + getListIndex(player_name));
    m_player_elos.erase(player_name);
}

bool SuperTournamentQualification::canPlay(std::string player_name) const
{
    return m_match_opened && getMatchId(player_name) == m_match_index;
}

void SuperTournamentQualification::nextMatch()
{
    int num_matches = m_player_list.size() / 2;
    m_match_index = (m_match_index + 1) % std::max(num_matches, 1);
}

void SuperTournamentQualification::setMatch(int match_id)
{
    m_match_index = match_id;
}

int SuperTournamentQualification::getMatchId(std::string player_name) const
{
    int listIndex = getListIndex(player_name);
    return listIndex < 0 ? -1 : listIndex / 2;
}

void SuperTournamentQualification::openMatch()
{
    m_match_opened = true;
}

void SuperTournamentQualification::closeMatch()
{
    m_match_opened = false;
}

KartTeam SuperTournamentQualification::getKartTeam(std::string player_name) const
{
    if (canPlay(player_name))
    {
        int listIndex = getListIndex(player_name);
        if (listIndex % 2 == 0)
            return KART_TEAM_RED;
        else
            return KART_TEAM_BLUE;
    }
    return KART_TEAM_NONE;
}

int SuperTournamentQualification::getElo(std::string player_name) const
{
    return m_player_elos.count(player_name) ? m_player_elos.at(player_name) : 1500;
}

void SuperTournamentQualification::updateElos(int red_goals, int blue_goals)
{
    if (m_player_list.size() < 2 * (m_match_index + 1)) return; // match n can't be evaluated with < 2*n players in the list :)

    std::string red_player = m_player_list[2 * m_match_index];
    std::string blue_player = m_player_list[2 * m_match_index + 1];

    std::string message = "Match result: " + red_player + " " + std::to_string(red_goals) + "-" + std::to_string(blue_goals) + " " + blue_player;
    Log::info("SuperTournamentQualification", message.c_str());

    // TODO: Use the elo formula in order to update the elo for player1 and player2

    //m_player_elos[red_player] = newElo(red_player);
    //m_player_elos[blue_player] = newElo(blue_player);

    // TODO: If needed, send the updated elos to the database
}

void SuperTournamentQualification::readElosFromFile()
{
    // TODO: Read all elos from a text file and store them like in this example

    //std::string player_name = "TheRocker";
    //m_player_elos[player_name] = 1900;
}