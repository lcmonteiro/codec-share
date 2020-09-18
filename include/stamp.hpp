/**
 * ------------------------------------------------------------------------------------------------ 
 * File:   SCodecStamp.h
 * Author: Luis Monteiro
 *
 * Created on September 29, 2015, 5:48 PM
 * ------------------------------------------------------------------------------------------------
 */
#ifndef SCODECSTAMP_H
#define SCODECSTAMP_H
/**
 * std
 */
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
/**
 * ------------------------------------------------------------------------------------------------
 * Codec Stamp
 * ------------------------------------------------------------------------------------------------
 */
namespace Codec {
/**
 * ----------------------------------------------------------------------------
 * Definitions
 * ----------------------------------------------------------------------------
 * density:
 * 1º field size
 * 2º sparsity
 */
using Density = std::pair<uint8_t, uint8_t>;
/**
 * stamp: 
 * group of densities  
 */
using Stamp = std::vector<Density>;
/**
 * stamp pointer 
 */
using pStamp = std::shared_ptr<const Stamp>;
/**
 * ------------------------------------------------------------------------------------------------
 * Codec Stamp
 * ------------------------------------------------------------------------------------------------
 */
class SStamp {
public:
	/**
	 * ------------------------------------------------------------------------
	 * Types
	 * ------------------------------------------------------------------------
	 */
	typedef enum {
		NONE, SPARSE, STREAM, MESSAGE, FULL
	} Type;
	/** 
	 * ------------------------------------------------------------------------
	 * get stamp by type
	 * ------------------------------------------------------------------------
	 */
	static pStamp Get(const Type t) {
			return __definitions.at(t);  
	} 
	/** 
	 * ------------------------------------------------------------------------
	 * Generate stamp
	 * ------------------------------------------------------------------------
	 */
	static pStamp Generate(Type t, const uint64_t seed = 0) {
		return Generate(
			seed, __templates.at(t).first, __templates.at(t).second);
	}
	static pStamp Generate(
		const uint64_t seed = 0, 
		const Density& min  = {1, 1}, 
		const Density& max  = {255, 255}
	);  
private:
	/**
	 * ------------------------------------------------------------------------
	 * Variables
	 * ------------------------------------------------------------------------
	 **
	 * stamp definitions
	 */
	static const std::map<
		const int, 
		const pStamp
	> __definitions;
	/** 
	 * stamp templates
	 */
	static const std::map<
		const int, std::pair<
			const Density, // minimum  
			const Density  // maximum
		>
	> __templates;
};
/**
 * ------------------------------------------------------------------------------------------------
 * End
 * ------------------------------------------------------------------------------------------------
 */ 
}
#endif /* SCODECSTAMP_H */
