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

#ifndef QUALIFICATION_HPP
#define QUALIFICATION_HPP

#include "network/remote_kart_info.hpp"
#include "network/stk_host.hpp"
#include "network/stk_peer.hpp"
#include "network/network_player_profile.hpp"
#include "utils/cpp2011.hpp"
#include "utils/string_utils.hpp"
#include "utils/log.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <map>

class STQualiGameState
{
private:
    int m_goals_red = 0;
    int m_goals_blue = 0;
    float m_remaining_time = 0.0f;

public:
    STQualiGameState() {};
    virtual ~STQualiGameState() {};

    int getRedGoals() { return m_goals_red; };
    int getBlueGoals() { return m_goals_blue; };
    float getRemainingTime() { return m_remaining_time; };

    bool pending() { return m_remaining_time > 0; };

    void initGoals(int red_goals, int blue_goals) 
    {
        m_goals_red = red_goals;
        m_goals_blue = blue_goals;
    };
    void initRemainingTime(float seconds) 
    { 
        m_remaining_time = seconds;
    };
    void gameStoppedAt(float seconds) { m_remaining_time += seconds; };
    void gameResumedAt(float seconds) { m_remaining_time -= seconds; };
    void reset()
    {
        m_goals_red = 0;
        m_goals_blue = 0;
        m_remaining_time = 0.0f;
    };
};

class SuperTournamentQualification
{
private:
    std::vector<std::string> m_player_list;
    std::map<std::string, int> m_player_elos;
    int m_match_index = 0; // match0 = {player1, player2}, match1 = {player3, player4), ...

    int getListIndex(std::string player_name) const;
    //void updateKartTeams();
    
public:
    SuperTournamentQualification();
    SuperTournamentQualification(std::string config_player_list); // config_player_list = "player1 player2 player3 ..."
    virtual ~SuperTournamentQualification();

    // In case that the game is stopped / resumed
    STQualiGameState gameState;

    const std::vector<std::string>& getPlayerList() const { return m_player_list; }
    void addPlayer(std::string player_name, int elo);
    void removePlayer(std::string player_name);
    void replacePlayer(std::string player_current, std::string player_new, int elo_new);
    bool canPlay(std::string player_name) const;

    void nextMatch();
    void setMatch(int match_id);
    int getCurrentMatchId() const;
    int getMatchId(std::string player_name) const;

    KartTeam getKartTeam(std::string player_name) const;

    int getElo(std::string player_name) const;
    void updateElos(int red_goals, int blue_goals);
    void readElosFromFile();
    void sortPlayersByElo();
};



#endif // QUALIFICATION_HPP