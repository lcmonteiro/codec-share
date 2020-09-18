/**
 * ------------------------------------------------------------------------------------------------
 * File:   SCodecStamp.cpp
 * Author: Luis Monteiro
 *
 * Created on November 30, 2015, 5:48 PM
 * ------------------------------------------------------------------------------------------------
 ** 
 * std
 */
#include <random>
/**
 * local
 */
#include "SStampCodec.h"
/**
 * namespaces
 */
using namespace std;
using namespace Codec;
/**
 * ------------------------------------------------------------------------
 * definitions
 * ------------------------------------------------------------------------
 */	
const map<const int, const pStamp> SStamp::__definitions {
	{SStamp::NONE, make_shared<const Stamp>(256, Density{  0,   0})},
	{SStamp::FULL, make_shared<const Stamp>(256, Density{255, 255})}
};
/**
 * ------------------------------------------------------------------------
 * Templates
 * ------------------------------------------------------------------------
 */
const map<const int, pair<const Density, const Density>> SStamp::__templates {
	{SStamp::NONE, {
		{0, 0}, {0, 0}
	}}, {SStamp::SPARSE, {
		{1, 4}, {255, 255}
	}}, {SStamp::STREAM, {
		{1, 8}, {3, 127}
	}}, {SStamp::MESSAGE, {
		{7, 16}, {255, 255}
	}}, {SStamp::FULL, {
		{255, 255}, {255, 255}
	}}
};
/**
 * ------------------------------------------------------------------------
 * Generate
 * ------------------------------------------------------------------------
 */
pStamp SStamp::Generate(
	const uint64_t seed, const Density& min, const Density& max
) {
	/**
	 * generator
	 */
	mt19937_64 c(seed);
	/**
	 * init stamp
	 */
	Stamp out(256);
	/**
	 * build stamp
	 */
	for (auto& v : out) {
		v.first = uint8_t(c());
		v.first |= min.first;
		v.first &= max.first;
		v.second = uint8_t(c());
		v.second |= min.second;
		v.second &= max.second;
	}
	/**
	 * return shared stamp
	 */
	return make_shared<const Stamp>(move(out));
}
/**
 * ------------------------------------------------------------------------------------------------
 * End
 * ------------------------------------------------------------------------------------------------
 */