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
#include <iostream>
#include <fstream>
#include <string>
#include "utils/string_utils.hpp"

SuperTournamentQualification::SuperTournamentQualification()
{ 
    gameState = STQualiGameState();
}

SuperTournamentQualification::SuperTournamentQualification(std::string config_player_list)
{
    gameState = STQualiGameState();

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

/*
void SuperTournamentQualification::updateKartTeams()
{
    auto peers = STKHost::get()->getPeers();
    for (auto peer : peers)
    {
        for (auto player : peer->getPlayerProfiles())
        {
            std::string player_name = StringUtils::wideToUtf8(player->getName());
            player->setTeam(getKartTeam(player_name));
        }
    }
}
*/

void SuperTournamentQualification::addPlayer(std::string player_name, int elo)
{
    if (std::find(m_player_list.begin(), m_player_list.end(), player_name) == m_player_list.end())
        m_player_list.push_back(player_name);
    m_player_elos[player_name] = elo;
}

void SuperTournamentQualification::removePlayer(std::string player_name)
{
    int player_idx = getListIndex(player_name);
    if (player_idx == -1) return;

    m_player_list.erase(m_player_list.begin() + getListIndex(player_name));
    m_player_elos.erase(player_name);
}

void SuperTournamentQualification::replacePlayer(std::string player_current, std::string player_new, int elo_new)
{
    int player_idx = getListIndex(player_current);
    if (player_idx == -1) return;

    m_player_list[player_idx] = player_new;
    m_player_elos.erase(player_current);
    m_player_elos[player_new] = elo_new;
}

bool SuperTournamentQualification::canPlay(std::string player_name) const
{
    return (getKartTeam(player_name) != KART_TEAM_NONE) && (getMatchId(player_name) == m_match_index);
}

void SuperTournamentQualification::nextMatch()
{
    setMatch(m_match_index + 1);
}

void SuperTournamentQualification::setMatch(int match_id)
{
    gameState.reset();
    int num_matches = m_player_list.size() / 2;
    m_match_index = match_id % std::max(num_matches, 1);
    //updateKartTeams();
}

int SuperTournamentQualification::getCurrentMatchId() const
{
    return m_match_index;
}

int SuperTournamentQualification::getMatchId(std::string player_name) const
{
    int listIndex = getListIndex(player_name);
    return listIndex < 0 ? -1 : listIndex / 2;
}

KartTeam SuperTournamentQualification::getKartTeam(std::string player_name) const
{
    if (getMatchId(player_name) == m_match_index)
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
    if (gameState.pending())
    {
        gameState.initGoals(red_goals, blue_goals);
    }
    else
    {
        gameState.reset();

        if (m_player_list.size() < 2 * (m_match_index + 1)) return; // match n can't be evaluated with < 2*n players in the list :)

        std::string red_player = m_player_list[2 * m_match_index];
        std::string blue_player = m_player_list[2 * m_match_index + 1];

        std::string message = "Match result: " + red_player + " " + std::to_string(red_goals) + "-" + std::to_string(blue_goals) + " " + blue_player;
        Log::info("SuperTournamentQualification", message.c_str());

        std::string fitis = "python3 super1vs1quali_update_elo.py " + red_player + " " + blue_player + " " + std::to_string(m_player_elos[red_player]) + " " + std::to_string(m_player_elos[blue_player]) + " " + std::to_string(red_goals) + " " + std::to_string(blue_goals);
        system(fitis.c_str());
        readElosFromFile();
    }
}

void SuperTournamentQualification::readElosFromFile()
{
     std::ifstream in_file("super1vs1quali_ranking.txt");
     int elo;
     std::string player;
     std::string test="test test";
     auto split=StringUtils::split(test,' ');
     if (in_file.is_open())
     {
         std::string line;
         std::getline(in_file, line);
         while (std::getline(in_file, line))
         {
             split = StringUtils::split(line, ' ');
             if (split.size()<4) continue;
             if (split[1]=="Played_Games") continue;
             elo=int(stof(split[3]));
             player=split[0];
             m_player_elos[player]=elo;
         }
     }
     in_file.close();
}

void SuperTournamentQualification::sortPlayersByElo()
{
    m_match_index = 0; // after sorting the elos, the teams change

    auto sort_rule = [this](std::string const player1, std::string const player2) -> bool
    {
        return getElo(player1) > getElo(player2);
    };
    
    std::sort(m_player_list.begin(), m_player_list.end(), sort_rule);
}
