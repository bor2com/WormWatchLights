#include "mbed.h"
#include "ChainableLED.h"
#include <cstdlib>
#include <vector>

const PinName clockPin = D6;
const PinName dataPin = D7;
const uint32_t ledCount = 8;

enum SwappedColors {
	NORMAL,
	GREEN_WITH_BLUE,
	RED_WITH_BLUE
};

struct Strip {
	int32_t index;
	SwappedColors swapScenario;
	Strip(const SwappedColors& swapScenario_, const int32_t& index_) :
			index(index_), swapScenario(swapScenario_) {
	}
};

const std::vector<Strip> verticalBus = {
	Strip(NORMAL, 2),
	Strip(NORMAL, 1),
	Strip(NORMAL, 6),
	Strip(NORMAL, 5),
	Strip(NORMAL, 0),
	Strip(GREEN_WITH_BLUE, 3),
	Strip(NORMAL, 4)
};

const std::vector<Strip> horizontalBus = {
	Strip(NORMAL, 1),
	Strip(NORMAL, 6),
	Strip(NORMAL, 0),
	Strip(RED_WITH_BLUE, 2),
	Strip(NORMAL, 5),
	Strip(NORMAL, 9),
	Strip(NORMAL, 3),
	Strip(NORMAL, 14),
	Strip(NORMAL, 10),
	Strip(NORMAL, 8),
	Strip(NORMAL, 13),
	Strip(NORMAL, 12),
	Strip(NORMAL, 4),
	Strip(NORMAL, 7),
	Strip(NORMAL, 11)
};

class WormSegments {
private:
	ChainableLED leds;
	std::vector<Strip> strips;
	WormSegments(const ChainableLED& leds_, const std::vector<Strip>& strips_) :
			leds(leds_), strips(strips_) {
		leds.ledsOff();
	}
public:
	void flush() {
		leds.flush();
	}
	void setColorRGB(uint32_t stripi, uint8_t red, uint8_t green,
			uint8_t blue) {
		const Strip& strip = strips[stripi];
		switch (strip.swapScenario) {
		case GREEN_WITH_BLUE:
			leds.setColorRGB(strip.index, red, blue, green);
			break;
		case RED_WITH_BLUE:
			leds.setColorRGB(strip.index, blue, green, red);
			break;
		default:
			leds.setColorRGB(strip.index, red, green, blue);
			break;
		}
	}
	uint32_t size() {
		return strips.size();
	}
	static WormSegments create(const std::vector<Strip>& strips) {
		return WormSegments(ChainableLED(clockPin, dataPin, strips.size()),
				strips);
	}
};

std::vector<Strip> mergeBuses(const std::vector<Strip>& head,
		const std::vector<Strip>& tail) {
	std::vector<Strip> result(head.begin(), head.end());
	for (const Strip& s : tail) {
		result.emplace_back(Strip(s.swapScenario, s.index + head.size()));
	}
	return result;
}

int main() {
	WormSegments wormSegments = WormSegments::create(
			mergeBuses(verticalBus, horizontalBus));

	for (uint32_t i = 0; i < wormSegments.size(); ++i) {
		wormSegments.setColorRGB(i, 0, 0, 0);
	}

	while (true) {
		uint8_t r = std::rand() % 0x100,
				g = std::rand() % 0x100,
				b = std::rand() % 0x100;
		for (uint32_t i = 0; i < wormSegments.size(); ++i) {
			wormSegments.setColorRGB(i, r, g, b);
			wormSegments.flush();
			wait(0.2);
			wormSegments.setColorRGB(i, 0, 0, 0);
		}
	}
}
