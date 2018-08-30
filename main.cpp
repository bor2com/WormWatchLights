#include "mbed.h"
#include "ChainableLED.h"
#include <cmath>
#include <cstdlib>
#include <list>
#include <vector>

const PinName clockPin = D6;
const PinName dataPin = D7;

// LED used to indicate error or invalid program state.
DigitalOut errorLED(LED1);

// Makes sure the evaluated condition is true. If not, blocks the program
// and blinks the errorLED to indicate that program got to an invalid state.
void checkState(bool condition) {
	while (!condition) {
		errorLED = !errorLED;
		wait(2);
	}
}

// Some strips have been soldered incorrectly. This enum indicates how exactly.
enum SwappedColors {
	NORMAL,
	GREEN_WITH_BLUE,
	RED_WITH_BLUE
};

// A strip of LEDs that's wrapped around a single segment.
class Strip {
	int32_t index_;
	Strip(const bool& is_real_, const SwappedColors& swapScenario_,
			const int32_t& index__) :
			index_(index__), is_real(is_real_), swapScenario(swapScenario_) {
	}
public:
	// Indicates if the strip covers a real or fake (underground) segment.
	bool is_real;
	// If strip has been incorrectly soldered, provides information to
	// adjust color output.
	SwappedColors swapScenario;
	// This strip position in global communication bus.
	const int32_t& index() const {
		checkState(is_real);
		return index_;
	}
	static Strip createReal(const SwappedColors& swapScenario,
			const int32_t& index) {
		return Strip(true, swapScenario, index);
	}
	static Strip createFake() {
		return Strip(false, NORMAL, -1);
	}
};

const std::vector<Strip> verticalBus = {
	Strip::createReal(NORMAL, 2),
	Strip::createReal(NORMAL, 1),
	Strip::createReal(NORMAL, 6),
	Strip::createReal(NORMAL, 5),
	Strip::createReal(NORMAL, 0),
	Strip::createReal(GREEN_WITH_BLUE, 3),
	Strip::createReal(NORMAL, 4)
};

const std::vector<Strip> horizontalBus = {
	Strip::createReal(NORMAL, 1),
	Strip::createReal(NORMAL, 6),
	Strip::createReal(NORMAL, 0),
	Strip::createReal(RED_WITH_BLUE, 2),
	Strip::createReal(NORMAL, 5),
	Strip::createReal(NORMAL, 9),
	Strip::createReal(NORMAL, 3),
	Strip::createReal(NORMAL, 14),
	Strip::createReal(NORMAL, 10),
	Strip::createReal(NORMAL, 8),
	Strip::createReal(NORMAL, 13),
	Strip::createReal(NORMAL, 12),
	Strip::createReal(NORMAL, 4),
	Strip::createReal(NORMAL, 7),
	Strip::createReal(NORMAL, 11)
};

const std::vector<Strip> tailBus = {
	Strip::createReal(NORMAL, 2),
	Strip::createReal(NORMAL, 4),
	Strip::createReal(NORMAL, 1),
	Strip::createReal(NORMAL, 6),
	Strip::createReal(NORMAL, 0),
	Strip::createReal(NORMAL, 5),
	Strip::createReal(NORMAL, 3),
};

class WormSegments {
	ChainableLED leds;
	std::vector<Strip> strips;
public:
	WormSegments(const std::vector<Strip>& strips_) :
			leds(ChainableLED(clockPin, dataPin, strips_.size())), strips(
					strips_) {
	}
	// Flushes stored colors of each segment to an actual hardware.
	void flush() {
		leds.flush();
	}
	// Sets color of a strip adjusting for incorrectly soldered colors.
	void setColorRGB(int32_t stripi, uint8_t red, uint8_t green, uint8_t blue) {
		checkState(stripi >= 0);
		checkState(stripi < size());

		const Strip& strip = strips[stripi];
		if (!strip.is_real) {
			return;
		}
		switch (strip.swapScenario) {
		case GREEN_WITH_BLUE:
			leds.setColorRGB(strip.index(), red, blue, green);
			break;
		case RED_WITH_BLUE:
			leds.setColorRGB(strip.index(), blue, green, red);
			break;
		default:
			leds.setColorRGB(strip.index(), red, green, blue);
			break;
		}
	}
	// All segments including fake ones.
	int32_t size() {
		return strips.size();
	}
};

// Combines two strips into one adjusting indexes and handling fake segments.
std::vector<Strip> mergeBuses(const std::vector<Strip>& head,
		const std::vector<Strip>& tail) {
	std::vector<Strip> result;
	int32_t indexShift = 0;
	for (const Strip& s : head) {
		if (s.is_real) {
			++indexShift;
		}
		result.emplace_back(s);
	}
	for (const Strip& s : tail) {
		if (s.is_real) {
			result.emplace_back(
					Strip::createReal(s.swapScenario, s.index() + indexShift));
		} else {
			result.emplace_back(Strip::createFake());
		}
	}
	return result;
}

/* A single light Runner of a single color. It has length and distance behind it
 * that should not be lit (tail).
 *
 *                       V -- shift
 * | tail |    length    |
 * -5 -4 -3 -2 -1  0  1  2  3  4  5  6
 *  .  .  .  #  #  #  #  #  .  .  .  .
 */
class Runner {
	int32_t length, tail, shift;
	uint8_t red, green, blue;
	// Provides color of the segment for given part of the wave length.
	// Calculates a slight color fade towards the end of the wave.
	void getColor(const int32_t& i, uint8_t& r, uint8_t& g, uint8_t& b) {
		double fraction = double(i) / double(length);
		double damper = exp(1.0 - fraction) / M_E;
		r = (uint8_t) (red * damper);
		g = (uint8_t) (green * damper);
		b = (uint8_t) (blue * damper);
	}
	Runner(const int32_t& length_, const int32_t& tail_, const uint8_t& r,
			const uint8_t& g, const uint8_t& b) :
			length(length_), tail(tail_), shift(-1), red(r), green(g), blue(b) {
	}
public:
	static Runner createRandom() {
		double randomFraction = double(rand()) / double(RAND_MAX);
		int32_t length = (uint32_t) ceil(-log2(randomFraction)) + 2;
		int32_t tail = (uint32_t) floor(log2((double) length)) + 3;
		uint8_t red = std::rand() & 0xff;
		uint8_t green = std::rand() & 0xff;
		uint8_t blue = std::rand() & 0xff;
		return Runner(length, tail, red, green, blue);
	}
	// Makes one step in wave motion. Paints each segment of the wave and
	// blacks out it's tail.
	// Returns true if this runner keeps going or false if it's gone
	// beyond bounds of the worm.
	bool progressOne(WormSegments& wormSegments) {
		++shift;
		for (int32_t j, i = 0; i < length; ++i) {
			j = shift - i;
			if (j >= 0 && j < wormSegments.size()) {
				uint8_t red, green, blue;
				getColor(i, red, green, blue);
				wormSegments.setColorRGB(j, red, green, blue);
			}
		}
		for (int32_t j, i = 0; i < tail; ++i) {
			j = shift - length - i;
			if (j >= 0 && j < wormSegments.size()) {
				wormSegments.setColorRGB(j, 0, 0, 0);
			}
		}
		return shift - length - tail < wormSegments.size();
	}
	// Returns position on the WormSegments strip immediately after the tail
	// of this Runner. When this returns 0 we can start another Runner.
	int32_t end() const {
		return shift - length - tail;
	}
};

int main() {
	WormSegments wormSegments = WormSegments(
			mergeBuses(
					mergeBuses(
							mergeBuses(
									verticalBus,
									std::vector<Strip> {
										Strip::createFake()
									}),
							mergeBuses(
									horizontalBus,
									std::vector<Strip> {
										Strip::createFake(),
										Strip::createFake(),
										Strip::createFake(),
									})),
					tailBus));

	for (int32_t i = 0; i < wormSegments.size(); ++i) {
		wormSegments.setColorRGB(i, 0, 0, 0);
	}

	std::list<Runner> runners;
	runners.emplace_back(Runner::createRandom());
	while (true) {
		for (std::list<Runner>::iterator it = runners.begin();
				it != runners.end();) {
			if (!it->progressOne(wormSegments)) {
				runners.erase(it++);
			} else if (it++->end() == 0) {
				runners.emplace_front(Runner::createRandom());
			}
		}
		wormSegments.flush();
		wait(0.1);
	}
}

