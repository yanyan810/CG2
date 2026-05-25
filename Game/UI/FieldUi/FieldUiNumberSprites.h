#pragma once

#include "Sprite.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>

class FieldUiNumberSprites {
public:
	template <size_t DigitCount>
	static void Update(
		std::array<std::unique_ptr<Sprite>, DigitCount>& digits,
		int value,
		float x,
		float y,
		float scale,
		float spacing)
	{
		std::string text = std::to_string((std::max)(0, value));

		for (auto& s : digits) {
			if (s) {
				s->SetColor({ 1.f, 1.f, 1.f, 0.f });
			}
		}

		const int count = static_cast<int>(text.size());
		for (int i = 0; i < count && i < static_cast<int>(DigitCount); ++i) {
			std::string path = "resources/ui/num/";
			path += text[i];
			path += ".png";

			auto& spr = digits[i];
			if (!spr) { continue; }

			spr->SetTextureFilePath(path);
			spr->SetPosition({ x + spacing * i, y });
			spr->SetScale({ scale, scale, 1.0f });
			spr->SetColor({ 1.f, 1.f, 1.f, 1.f });
		}
	}
};
