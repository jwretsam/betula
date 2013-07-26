
#ifndef __BETULA_H
#define __BETULA_H

#include <sys/types.h>

#include <string>
#include <vector>
#include <iostream>

#ifndef __BETULA_CONFIG_H
#include "betula_config.h"
#define __BETULA_CONFIG_H
#endif


#ifdef HAVE_BYTESWAP_H
#  include <byteswap.h>
#else
#  ifdef HAVE_SYS_BYTESWAP_H
#    include <sys/byteswap.h>
#  else
#    warning "Not found 'byteswap.h'. "
#define bswap_16(x) ((unsigned short int) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))
#define bswap_32(x)  ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#define bswap_64(x)  ((((x) & 0xff00000000000000ull) >> 56) \
      | (((x) & 0x00ff000000000000ull) >> 40) \
      | (((x) & 0x0000ff0000000000ull) >> 24) \
      | (((x) & 0x000000ff00000000ull) >> 8) \
      | (((x) & 0x00000000ff000000ull) << 8) \
      | (((x) & 0x0000000000ff0000ull) << 24) \
      | (((x) & 0x000000000000ff00ull) << 40) \
      | (((x) & 0x00000000000000ffull) << 56))
#  endif
#endif

#ifdef WORDS_BIGENDIAN
#define BETULA_NETWORK_ORDER 1
#endif


namespace betula {


// ----------------------

typedef u_int8_t uint8;
typedef int8_t sint8;
typedef int8_t int8;

typedef u_int16_t uint16;
typedef int16_t sint16;
typedef int16_t int16;

typedef u_int32_t uint32;
typedef int32_t sint32;
typedef int32_t int32;

typedef u_int64_t uint64;
typedef int64_t sint64;
typedef int64_t int64;

// ----------------------

inline uint16 i_bswap16(uint16 x) { return bswap_16(x); };
inline uint32 i_bswap32(uint32 x) { return bswap_32(x); };
inline uint64 i_bswap64(uint64 x) { return bswap_64(x); };


inline uint8 hton_uint8(uint8 x) { return x; };
inline sint8 hton_sint8(sint8 x) { return x; };
inline int8 hton_int8(int8 x) { return x; };

inline uint8 ntoh_uint8(uint8 x) { return x; };
inline sint8 ntoh_sint8(sint8 x) { return x; };
inline int8 ntoh_int8(int8 x) { return x; };


#ifdef BETULA_NETWORK_ORDER

inline uint16 hton_uint16(uint16 x) { return x; };
inline sint16 hton_sint16(sint16 x) { return x; };
inline int16 hton_int16(int16 x) { return x; };

inline uint16 ntoh_uint16(uint16 x) { return x; };
inline sint16 ntoh_sint16(sint16 x) { return x; };
inline int16 ntoh_int16(int16 x) { return x; };


inline uint32 hton_uint32(uint32 x) { return x; };
inline sint32 hton_sint32(sint32 x) { return x; };
inline int32 hton_int32(int32 x) { return x; };

inline uint32 ntoh_uint32(uint32 x) { return x; };
inline sint32 ntoh_sint32(sint32 x) { return x; };
inline int32 ntoh_int32(int32 x) { return x; };


inline uint64 hton_uint64(uint32 x) { return x; };
inline sint64 hton_sint64(sint32 x) { return x; };
inline int64 hton_int64(int32 x) { return x; };

inline uint64 ntoh_uint64(uint64 x) { return x; };
inline sint64 ntoh_sint64(sint64 x) { return x; };
inline int64 ntoh_int64(int64 x) { return x; };

#else

inline uint16 hton_uint16(uint16 x) { return i_bswap16(x); };
inline sint16 hton_sint16(sint16 x) { return i_bswap16(x); };
inline int16 hton_int16(int16 x) { return i_bswap16(x); };

inline uint16 ntoh_uint16(uint16 x) { return i_bswap16(x); };
inline sint16 ntoh_sint16(sint16 x) { return i_bswap16(x); };
inline int16 ntoh_int16(int16 x) { return i_bswap16(x); };


inline uint32 hton_uint32(uint32 x) { return i_bswap32(x); };
inline sint32 hton_sint32(sint32 x) { return i_bswap32(x); };
inline int32 hton_int32(int32 x) { return i_bswap32(x); };

inline uint32 ntoh_uint32(uint32 x) { return i_bswap32(x); };
inline sint32 ntoh_sint32(sint32 x) { return i_bswap32(x); };
inline int32 ntoh_int32(int32 x) { return i_bswap32(x); };


inline uint64 hton_uint64(uint32 x) { return i_bswap32(x); };
inline sint64 hton_sint64(sint32 x) { return i_bswap32(x); };
inline int64 hton_int64(int32 x) { return i_bswap32(x); };

inline uint64 ntoh_uint64(uint64 x) { return i_bswap64(x); };
inline sint64 ntoh_sint64(sint64 x) { return i_bswap64(x); };
inline int64 ntoh_int64(int64 x) { return i_bswap64(x); };

#endif


// ----------------------

union u_std_types
{
    uint8 uint8_f;
    sint8 sint8_f;
    int8 int8_f;

    uint16 uint16_f;
    sint16 sint16_f;
    int16 int16_f;

    uint32 uint32_f;
    sint32 sint32_f;
    int32 int32_f;

    uint64 uint64_f;
    sint64 sint64_f;
    int64 int64_f;
    char c[4];
};

// ----------------------

class message_base
{
public:

    message_base() { };
    virtual ~message_base() { };

    virtual void serialize(std::string &__out_buf) const = 0;
    virtual bool unserialize(const char* &__buffer, long &__buffer_len) = 0;
    virtual void debug_print(std::ostream &__dest, int __level=0) const = 0;

    bool unserialize_str(const std::string &buffer)
    {
	const char *ptr = buffer.c_str();
	long buffer_len = (long) buffer.length();
	
	return unserialize(ptr, buffer_len);
    };

};

inline std::ostream& operator <<(std::ostream &s, const message_base &x)
{
    x.debug_print(s);
    return s;
};


// ----------------------
// Misc
std::ostream& print_tab(std::ostream &out, int level);
std::ostream& print_fname(std::ostream &out, std::string field_name);



// ----------------------
// ----------------------
// ----------------------


}; // namespace betula

#endif // __BETULA_H
