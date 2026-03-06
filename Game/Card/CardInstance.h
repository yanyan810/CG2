#pragma once

enum class CardSuit
{
    Spade = 0,
    Heart,
    Diamond,
    Club,
};

struct CardInstance
{
    int defId = 0;         // どのカードか
    int number = 1;        // 1~13
    CardSuit suit = CardSuit::Spade;
};