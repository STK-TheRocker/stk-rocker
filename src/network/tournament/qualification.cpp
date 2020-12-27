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

SuperTournamentQualification::SuperTournamentQualification(std::string config_player_list, int team_size)
{
    gameState = STQualiGameState();
    readElosFromFile();

    std::vector<std::string> splits = StringUtils::split(config_player_list, ' ');
    for (auto &split : splits)
        m_player_list.push_back(split);

    m_team_size = std::max(team_size, 1); // number of players per team cannot be smaller than 1
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

void SuperTournamentQualification::updateKartTeams()
{
    auto peers = STKHost::get()->getPeers();
    for (auto peer : peers)
    {
        for (auto player : peer->getPlayerProfiles())
        {
            std::string player_name = StringUtils::wideToUtf8(player->getName());
            KartTeam kart_team = getKartTeam(player_name);
            player->setTeam(kart_team);
        }
    }
}

void SuperTournamentQualification::addPlayer(std::string player_name, int elo)
{
    if (std::find(m_player_list.begin(), m_player_list.end(), player_name) == m_player_list.end())
        m_player_list.push_back(player_name);

    if (elo != -1)
        m_player_elos[player_name] = elo;
}

void SuperTournamentQualification::removePlayer(std::string player_name)
{
    int player_idx = getListIndex(player_name);
    if (player_idx < 0) return;

    m_player_list.erase(m_player_list.begin() + getListIndex(player_name));
}

void SuperTournamentQualification::replacePlayer(std::string player_current, std::string player_new, int elo_new)
{
    int player_idx = getListIndex(player_current);
    if (player_idx < 0) return;
    if (getListIndex(player_new) >= 0) return;

    m_player_list[player_idx] = player_new;

    if (elo_new != -1)
        m_player_elos[player_new] = elo_new;

    if (m_substitutions.count(player_current))
    {
        if (player_new != m_substitutions[player_current])
            m_substitutions[player_new] = m_substitutions[player_current];
        m_substitutions.erase(player_current);
    }
    else
    {
        m_substitutions[player_new] = player_current;
    }
        
    updateKartTeams();
}

bool SuperTournamentQualification::canPlay(std::string player_name) const
{
    return (getKartTeam(player_name) != KART_TEAM_NONE) && (getMatchId(player_name) == m_match_index);
}

bool SuperTournamentQualification::isAlwaysSpectate(std::string player_name) const
{
    return getListIndex(player_name) < 0;
}

void SuperTournamentQualification::nextMatch()
{
    setMatch(m_match_index + 1);
}

void SuperTournamentQualification::setMatch(int match_id)
{
    gameState.reset();
    int num_matches = m_player_list.size() / (m_team_size * 2);
    m_match_index = match_id % std::max(num_matches, 1);
    updateKartTeams();
}

int SuperTournamentQualification::getCurrentMatchId() const
{
    return m_match_index;
}

int SuperTournamentQualification::getMatchId(std::string player_name) const
{
    int listIndex = getListIndex(player_name);
    return listIndex < 0 ? -2 : listIndex / (m_team_size * 2); // return -2 instead of -1, to avoid conflicts with m_match_index = -1
}

KartTeam SuperTournamentQualification::getKartTeam(std::string player_name) const
{
    if (getMatchId(player_name) == m_match_index)
    {
        int listIndex = getListIndex(player_name);
        if (listIndex % (m_team_size * 2) < m_team_size)
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
    if (gameState.pending()) // after /lobby command, init the goals
    {
        gameState.initGoals(red_goals, blue_goals);
    }
    else
    {
        gameState.reset();

        if (m_player_list.size() < m_team_size * 2 * (m_match_index + 1)) return; // match n can't be evaluated with < t*2*n players in the list :)
        if (m_match_index < 0) return;

        std::vector<std::string> red_players(m_player_list.begin() + m_team_size * 2 * m_match_index, 
                                             m_player_list.begin() + m_team_size * (2 * m_match_index + 1));

        std::vector<std::string> blue_players(m_player_list.begin() + m_team_size * (2 * m_match_index + 1),
                                              m_player_list.begin() + m_team_size * 2 * (m_match_index + 1));

        std::string red_player_str = StringUtils::join(red_players, " ");
        std::string blue_player_str = StringUtils::join(blue_players, " ");

        std::string message = "Match result: " + red_player_str + " " + std::to_string(red_goals) + "-" + std::to_string(blue_goals) + " " + blue_player_str;
        Log::info("SuperTournamentQualification", message.c_str());

        if (m_team_size == 1)
        {
            std::string fitis = "python3 super1vs1quali_update_elo.py " + red_player_str + " " + blue_player_str + " " + std::to_string(getElo(red_player_str)) + " " + std::to_string(getElo(blue_player_str)) + " " + std::to_string(red_goals) + " " + std::to_string(blue_goals);
            system(fitis.c_str());
        }

        if (m_team_size == 2)
        {
            if (!m_substitutions.empty())
            {
                for (int i = 0; i < red_players.size(); i++)
                    if (m_substitutions.count(red_players[i])) red_players[i] = m_substitutions[red_players[i]] + "#" + red_players[i];
                for (int i = 0; i < blue_players.size(); i++)
                    if (m_substitutions.count(blue_players[i])) blue_players[i] = m_substitutions[blue_players[i]] + "#" + blue_players[i];
                red_player_str = StringUtils::join(red_players, " ");
                blue_player_str = StringUtils::join(blue_players, " ");
            }
            std::string fitis = "python3 super2vs2quali_update_elo.py \"" + red_player_str + "\" \"" + blue_player_str + "\" " + std::to_string(getElo(red_players[0])) + " " + std::to_string(getElo(blue_players[0])) + " " + std::to_string(red_goals) + " " + std::to_string(blue_goals);
            system(fitis.c_str());
        }

        readElosFromFile();
    }
}

void SuperTournamentQualification::readElosFromFile()
{
    std::ifstream in_file;
    if (m_team_size == 1) in_file = std::ifstream("super1vs1quali_ranking.txt");
    if (m_team_size == 2) in_file = std::ifstream("super2vs2quali_ranking.txt");
    int elo = 0;
    std::string player = "";
    std::vector<std::string> split;
    if (in_file.is_open())
    {
        std::string line;
        std::getline(in_file, line);
        while (std::getline(in_file, line))
        {
            split = StringUtils::split(line, ' ');
            if (split.size() < 4) continue;
            if (split[1] == "Played_Games") continue;
            elo = int(stof(split[3]));
            player = split[0];
            m_player_elos[player] = elo;
        }
    }
    in_file.close();
}

void SuperTournamentQualification::sortPlayersByElo()
{
    m_match_index = -1; // after sorting the elos, the teams change

    std::map<std::string, int> list_indices;
    for (int i = 0; i < m_player_list.size(); i++) list_indices[m_player_list[i]] = i;

    auto sort_rule = [this, list_indices](std::string const player1, std::string const player2) -> bool
    {
        int elo1 = getElo(player1), elo2 = getElo(player2);
        return elo1 != elo2 ? elo1 > elo2 : list_indices.at(player1) < list_indices.at(player2);
    };
    
    std::sort(m_player_list.begin(), m_player_list.end(), sort_rule);
    updateKartTeams();
}
