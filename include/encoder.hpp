/**
 * ------------------------------------------------------------------------------------------------
 * File:   SCodecEncoder.h
 * Author: Luis Monteiro
 *
 * Created on November 30, 2017, 5:48 PM
 * ------------------------------------------------------------------------------------------------
 */
#ifndef SCODECENCODER_H
#define SCODECENCODER_H
/**
 * space
 */
#include "SContainer.h"
/**
 * share
 */
#include "SEngineCodec.h"
/**
 * ------------------------------------------------------------------------------------------------
 *  Codec Encoder
 * ------------------------------------------------------------------------------------------------
 */
namespace Codec {
/**
 */
class SEncoder : private SEngine {
  public:
    using SEngine::HeaderSize;
    /**
     * ------------------------------------------------------------------------
     * Constructors & Destructor
     * ------------------------------------------------------------------------
     * default constructor
     * ----------------------------------------------------
     */
    SEncoder()
      : __capacity(0)
      , __length(0)
      , __stamp(SStamp::Get(SStamp::FULL))
      , __data()
      , __rand() {}
    /**
     * ----------------------------------------------------
     * constructor
     * @param capacity
     * @param stamp
     * ----------------------------------------------------
     */
    SEncoder(uint32_t capacity, pStamp stamp = SStamp::Get(SStamp::FULL))
      : __capacity(capacity)
      , __length(capacity)
      , __stamp(stamp)
      , __data()
      , __rand() {
        /**
         * checkup
         */
        if (__stamp->size() < UINT8_MAX) {
            throw nullptr;
        }
        /**
         * update
         */
        __data.reserve(__capacity);
    }
    /**
     * ----------------------------------------------------
     * constructor
     * @param init
     * @param stamp
     * ----------------------------------------------------
     */
    SEncoder(Container&& init, pStamp stamp = SStamp::Get(SStamp::FULL))
      : __capacity(init.size())
      , __length(init.size())
      , __stamp(stamp)
      , __data(move(init))
      , __rand() {
        /**
         * checkup
         */
        if (__stamp->size() < UINT8_MAX) {
            throw nullptr;
        }
    }
    /**
     * ----------------------------------------------------
     *  Move Constructor
     * ----------------------------------------------------
     */
    SEncoder(SEncoder&& codec)
      : __capacity(0)
      , __length(0)
      , __stamp(SStamp::Get(SStamp::FULL))
      , __data()
      , __rand() {
        *this = std::move(codec);
    }
    /**
     * ------------------------------------------------------------------------
     * Operators
     * ------------------------------------------------------------------------
     * move
     * ----------------------------------------------------
     */
    inline SEncoder& operator=(SEncoder&& en) {
        __capacity = std::move(en.__capacity);
        __length   = std::move(en.__length);
        __stamp    = std::move(en.__stamp);
        __data     = std::move(en.__data);
        return *this;
    }
    /**
     * ------------------------------------------------------------------------
     * Data
     * ------------------------------------------------------------------------
     * push data
     * ----------------------------------------------------
     */
    inline SEncoder& push(Frame&& data) {
        __data.emplace_back(std::move(data));
        return *this;
    }
    inline SEncoder& push(Container&& data) {
        for (auto& d : data) {
            __data.emplace_back(std::move(d));
        }
        return *this;
    }
    /**
     * ----------------------------------------------------
     * pop data
     * ----------------------------------------------------
     */
    inline Container pop(uint32_t highDensity = 0) {
        Container out;
        Encode(out, __length, __data, __rand, highDensity, *__stamp);
        return out;
    }
    /**
     * ----------------------------------------------------
     * clear data
     * ----------------------------------------------------
     */
    inline SEncoder& clear() {
        __data.clear();
        return *this;
    }
    /**
     * ------------------------------------------------------------------------
     * iterators
     * ------------------------------------------------------------------------
     */
    using const_iterator = Container::const_iterator;
    /**
     * forward
     */
    const_iterator begin() const { return __data.begin(); }
    const_iterator end() const { return __data.end(); }
    /**
     * ------------------------------------------------------------------------
     * Quantity
     * ------------------------------------------------------------------------
     * standard
     * ----------------------------------------------------
     */
    inline bool full() { return (__data.size() >= __capacity); }
    inline uint32_t size() { return __data.size(); }
    inline uint32_t capacity() { return __capacity; }
    /**
     * ----------------------------------------------------
     * get properties
     * ----------------------------------------------------
     */
    inline uint32_t frame_count() { return __length; }
    inline uint32_t frame_size() { return __data.at(0).size() + HeaderSize(); }
    /**
     * ----------------------------------------------------
     * set properties
     * ----------------------------------------------------
     */
    inline SEncoder& frame_count(uint32_t len) {
        __length = len;
        return *this;
    }

  private:
    /**
     * ------------------------------------------------------------------------
     * Variables
     * ------------------------------------------------------------------------
     **
     * context
     */
    uint32_t __capacity;
    uint32_t __length;
    /**
     * data
     */
    pStamp __stamp;
    Container __data;
    /**
     * seed generator
     */
    Random __rand;
};
/**
 * ------------------------------------------------------------------------------------------------
 * End
 * ------------------------------------------------------------------------------------------------
 */
} // namespace Codec
#endif /* SCODECENCODER_H */
