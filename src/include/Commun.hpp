#pragma once

#include <queue>
#include <stack>
#include <bitset>
#include <string>
#include <iostream>

// NE PAS METTRE DANS UN CPP

template <size_t N>
void diviser(const std::bitset <N>& original,
    std::bitset <N/2>& gauche, std::bitset <N/2>& droite)
{
	droite = std::bitset<N/2> ((original).to_ulong());
	gauche = std::bitset<N/2> ((original >> N/2).to_ulong());
}

template <size_t N>
void diviser(std::bitset <N> original, std::bitset <N/3>& gauche,
    std::bitset <N/3>& milieu, std::bitset <N/3>& droite)
{
    droite = std::bitset<N/3> ((original).to_ulong());
    milieu = std::bitset<N/3> ((original >>= N/3).to_ulong());
    gauche = std::bitset<N/3> ((original >> N/3).to_ulong());
}

template <size_t N1, size_t N2>
std::bitset<N1 + N2> concat(const std::bitset <N1> gauche,
    const std::bitset <N2> droite)
{
    std::string gaucheStr = gauche.to_string();
    std::string droiteStr = droite.to_string();
    return std::bitset <N1 + N2>(gaucheStr + droiteStr);
}

void afficher(std::stack<std::bitset<16>> pile);

void afficher(std::queue<std::stack<std::bitset<16>>> file);

void afficher(std::deque<std::stack<std::bitset<16>>> doubleFile);
