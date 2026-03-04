#include "CardDatabase.h"

void CardDatabase::BuildSample() {
    defs_.clear();

    CardDef fire{};
    fire.id = 1;
    fire.name = "Fireball";
    fire.cost = 2;
    fire.frameModel = "cards/models/frame.obj";
    fire.artModel = "cards/models/art_plane.obj";
    fire.artTex = "resources/cards/art/fireball.png";
    fire.desc = "敵単体に10ダメージ";
    fire.effects.push_back({ "Damage", 10 });
    defs_[fire.id] = fire;

    CardDef draw{};
    draw.id = 2;
    draw.name = "Quick Draw";
    draw.cost = 1;
    draw.frameModel = "cards/models/frame.obj";
    draw.artModel = "cards/models/art_plane.obj";
    draw.artTex = "resources/cards/art/draw.png";
    draw.desc = "2枚引く";
    draw.effects.push_back({ "Draw", 2 });
    defs_[draw.id] = draw;
}

const CardDef* CardDatabase::Find(int id) const {
    auto it = defs_.find(id);
    return (it == defs_.end()) ? nullptr : &it->second;
}