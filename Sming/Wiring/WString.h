/* $Id: WString.h 1156 2011-06-07 04:01:16Z bhagman $
||
|| @author         Paul Stoffregen <paul@pjrc.com>
|| @url            http://wiring.org.co/
|| @contribution   Hernando Barragan <b@wiring.org.co>
|| @contribution   Brett Hagman <bhagman@wiring.org.co>
|| @contribution   Alexander Brevig <abrevig@wiring.org.co>
||
|| @description
|| | String class.
|| |
|| | Wiring Common API
|| #
||
|| @license Please see cores/Common/License.txt.
||
*/

/*
 * @author: 2 Oct 2018 - mikee47 <mike@sillyhouse.net>
 *
 * The standard String object default constructor creates an empty string, which requires a heap allocation of 1 byte.
 * I changed this behaviour to default to a null string (invalid) to avoid this (usually) un-necessary allocation.
 * If the value of the string hasn't actually been assigned yet then an 'invalid' (or null) string is the more logical choice.
 * Additional changes ensure that the content of such a string are equivalent to an empty string "".
 *
 * Background
 *
 * The intent of the Wiring authors seems to be that an expression producing a String object will fail and produce
 * an 'invalid' String (that evaluates to False) if any of the allocations within that expression fail. This could
 * be due to heap fragmentation, low memory or a String which is just too big.
 *
 * By example:
 *
 * 	String tmp = String("A") + String("B");
 *
 * If a heap allocation fails on either "A" or "B" the the result should be a null string. However, this is not actually
 * the case. In practice, if "A" fails but "B" does not then the result will be "B", while if "A" succeeds but "B" fails
 * then the result will be 'invalid'. This would appear to be an oversight in the Wiring library (last updated July 2016).
 *
 * I made a decision with these changes that heap allocation errors are a rare enough occurrence that attempting to deal with
 * them in such a manner causes more problems than it solves.
 *
 * These changes have a knock-on effect in that if any of the allocations in an expression fail, then the result, tmp,
 * will be unpredictable.
 *
 * @author Nov 2019 mikee47 <mike@sillyhouse.net>
 *
 * Small String Optimisation (SSO). Based on the Arduino ESP8266 core implementation.
 * An empty String object now consumes 12 bytes (from 8) but provides an SSO capacity of 11 characters.
 * Capacity and length types changed to size_t, thus String is no longer restricted to 64K.
 *
 */

#pragma once

#ifdef __cplusplus

#include "WConstants.h"
#include <cstddef>
#include <string.h>
#include <sming_attr.h>

#include <FlashString/String.hpp>

/**
 * @brief Read-only String class stored in flash memory
 */
using FlashString = FSTR::String;

// When compiling programs with this class, the following gcc parameters
// dramatically increase performance and memory (RAM) efficiency, typically
// with little or no increase in code size.
//     -felide-constructors
//     -std=c++0x

// An inherited class for holding the result of a concatenation.  These
// result objects are assumed to be writable by subsequent concatenations.
class StringSumHelper;

/**
 * @ingroup pgmspace
 * @{
 */

// Arduino-style flash strings
class __FlashStringHelper; // Never actually defined
/**
 * @brief Provides a strongly-typed pointer to allow safe implicit
 * operation using String class methods.
 */
using flash_string_t = const __FlashStringHelper*;

/**
 * @brief Cast a PGM_P (flash memory) pointer to a flash string pointer
 * @param pstr_pointer
 */
#define FPSTR(pstr_pointer) reinterpret_cast<flash_string_t>(pstr_pointer)

/**
 * @brief Wrap a string literal stored in flash and access it using a String object
 * @param string_literal The string literal value, e.g. "this is a string"
 *
 * The string data is stored in flash and only read into RAM when executed.
 * For example: Serial.print(F("This is a test string\n"));
 *
 * Unlike string pointers, content is length counted so may include NUL characters.
 */
#define F(string_literal) String(FPSTR(PSTR_COUNTED(string_literal)), sizeof(string_literal) - 1)

/** @} */

/**
 * @defgroup wiring Wiring Framework
 * @{
 */

/**
 * @brief The String class
 *
 * Note that a string object's default constructor creates an empty string.
 * This is not the same as a null string.
 * A null string evaluates to false, but an empty string evaluates to true.
 *
 * Small String Optimisation means that heap is only used for strings longer than
 * 10 characters, not including the NUL terminator. This is simply making use
 * of existing storage within the String object.
 *
 * This length can be increased using STRING_OBJECT_SIZE, but note the additional
 * space remains unused when switching to heap storage for longer Strings.
 */
class String
{
protected:
	// use a function pointer to allow for "if (s)" without the
	// complications of an operator bool(). for more information, see:
	// http://www.artima.com/cppsource/safebool.html
	using StringIfHelperType = void (String::*)() const;
	void StringIfHelper() const
	{
	}

public:
	// Use these for const references, e.g. in function return values
	static const String nullstr; ///< A null string evaluates to false
	static const String empty;   ///< An empty string evaluates to true

	/**
     * @brief Default constructor
     * @note Creates a null String which evaluates to false.
     */
	String() : ptr{nullptr, 0, 0}
	{
	}

	/**
     * @name Copy constructors
     *
     * If the initial value is null or invalid, or if memory allocation
     * fails, the string will be marked as invalid (i.e. "if (s)" will be false).
     *
     * @{
    */
	String(const char* cstr);
	String(const char* cstr, size_t length) : String()
	{
		if(cstr)
			copy(cstr, length);
	}
	String(const String& str) : String()
	{
		*this = str;
	}
	explicit String(flash_string_t pstr, size_t length) : String()
	{
		setString(pstr, length);
	}
	explicit String(flash_string_t pstr) : String()
	{
		setString(pstr);
	}

	String(String&& rval) noexcept : String()
	{
		move(rval);
	}
	String(StringSumHelper&& rval) noexcept;
	explicit String(char c);
	explicit String(unsigned char, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	explicit String(int num, unsigned char base = DEC, unsigned char width = 0, char pad = '0')
		: String(long(num), base, width, pad)
	{
	}
	explicit String(unsigned int num, unsigned char base = DEC, unsigned char width = 0, char pad = '0')
		: String((unsigned long)(num), base, width, pad)
	{
	}
	explicit String(long, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	explicit String(long long, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	explicit String(unsigned long, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	explicit String(unsigned long long, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	explicit String(float, unsigned char decimalPlaces = 2);
	explicit String(double, unsigned char decimalPlaces = 2);
	/** @} */

	~String(void)
	{
		invalidate();
	}

	void setString(const char* cstr);
	void setString(const char* cstr, size_t length);
	void setString(flash_string_t pstr);
	void setString(flash_string_t pstr, size_t length);

	// memory management

	/**
     * @brief Pre-allocate String memory
     * @param size
     * @retval bool true on success, false on failure
     *
     * On failure, the String is left unchanged.
     * reserve(0), if successful, will validate an invalid string (i.e., "if (s)" will be true afterwards)
     */
	bool reserve(size_t size);

	/** @brief set the string length accordingly, expanding if necessary
     *  @param length required for string (nul terminator additional)
     *  @retval true on success, false on failure
     *  @note extra characters are undefined
     */
	bool setLength(size_t length);

	/**
     * @brief Obtain the String length in characters, excluding NUL terminator
     */
	inline size_t length(void) const
	{
		return sso.set ? sso.len : ptr.len;
	}

	/**
     * @brief Used with setBuffer and getBuffer methods
     */
	struct Buffer {
		char* data;	///< Allocated using malloc
		size_t size;   ///< Size of memory allocation
		size_t length; ///< Length of content, MUST be < size
	};

	/**
     * @brief Set String content using move semantics from external memory buffer
     * @param buffer We'll take ownership of this buffer
     * @retval bool true on success; on failure, ownership of buffer is not transferred
     * @note length MUST be < `size` - A NUL character is written at this location
     */
	bool setBuffer(const Buffer& buffer);

	/**
     * @brief Get String content using move semantics
     * @retval Buffer
     * @note String is invalidated by this call. Caller is responsible for buffer memory.
     */
	Buffer getBuffer();

	/**
     * @name Copy operators
     *
     * If the value is null or invalid, or if the memory allocation fails,
     * the String will be marked as invalid ("if (s)" will be false).
     *
     * @{
     */
	String& operator=(const String& rhs);
	String& operator=(const char* cstr);
	/** @} */

	/**
     * @name Move operators
     *
     * Move content from one String to another without any heap allocation.
     *
     * Move operators are automatically selected by the compiler when it is able,
     * such as when returning temporary String objects from functions.
     *
     * In other situations, use `std::move`:
     *
     *		String original("A String");
     *		String copy("This is the content for the copy");
     *		copy = std::move(myString);
     *
     * `copy` will now contain "A String", whilst `original` will be invalidated.
     *
     * @{
     */
	String& operator=(String&& rval) noexcept
	{
		if(this != &rval)
			move(rval);
		return *this;
	}
	String& operator=(StringSumHelper&& rval) noexcept;
	/** @} */

	/**
     * @name Concatenation methods
     * @retval bool true on success, false on failure
     *
     * Works with built-in types.
     * On failure, the string is left unchanged.
     * If the argument is null or invalid, the concatenation is considered unsuccessful.
     *
     * @{
     */
	bool concat(const String& str)
	{
		return concat(str.cbuffer(), str.length());
	}
	bool concat(const FlashString& fstr);
	bool concat(const char* cstr);
	bool concat(const char* cstr, size_t length);
	bool concat(char c)
	{
		return concat(&c, 1);
	}
	bool concat(unsigned char num, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	bool concat(int num, unsigned char base = DEC, unsigned char width = 0, char pad = '0')
	{
		return concat(long(num), base, width, pad);
	}
	bool concat(unsigned int num, unsigned char base = DEC, unsigned char width = 0, char pad = '0')
	{
		return concat((unsigned long)(num), base, width, pad);
	}
	bool concat(long num, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	bool concat(long long num, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	bool concat(unsigned long num, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	bool concat(unsigned long long num, unsigned char base = DEC, unsigned char width = 0, char pad = '0');
	bool concat(float num);
	bool concat(double num);

	template <typename E>
	constexpr typename std::enable_if<std::is_enum<E>::value && !std::is_convertible<E, int>::value, bool>::type
	concat(E value)
	{
		extern String toString(E);
		return concat(toString(value));
	}
	/** @} */

	/**
     * @name Concatenation operators
     *
     * If there's not enough memory for the concatenated value, the string
     * will be left unchanged (but this isn't signalled in any way)
     *
     * @{
     */
	String& operator+=(const String& rhs)
	{
		concat(rhs);
		return (*this);
	}
	String& operator+=(const FlashString& rhs)
	{
		concat(rhs);
		return (*this);
	}
	String& operator+=(const char* cstr)
	{
		concat(cstr);
		return (*this);
	}
	template <typename T> String& operator+=(T value)
	{
		concat(value);
		return *this;
	}
	/** @} */

	friend StringSumHelper& operator+(const StringSumHelper& lhs, const String& rhs);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, const char* cstr);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, char c);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, unsigned char num);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, int num);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, unsigned int num);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, long num);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, unsigned long num);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, unsigned long long num);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, float num);
	friend StringSumHelper& operator+(const StringSumHelper& lhs, double num);

	/**
     * @brief Provides safe bool() operator
     *
     * Evaluates as false if String is null, otherwise evaluates as true
     */
	operator StringIfHelperType() const
	{
		return isNull() ? 0 : &String::StringIfHelper;
	}

	/**
     * @name Comparison methods
     * Works with String and 'c' string
     * @retval int Returns < 0 if String is lexically before the argument, > 0 if after or 0 if the same
     *
     * Comparisons are case-sensitive, binary comparison
     * null strings (including cstr == nullptr) are treated as empty.
     *
     * @{
     */
	int compareTo(const char* cstr, size_t length) const;
	int compareTo(const String& s) const
	{
		return compareTo(s.cbuffer(), s.length());
	}
	/** @} */

	/**
     * @name Test for equality
     * Compares content byte-for-byte using binary comparison
     * @retval bool Returns true if strings are identical
     *
     * null strings (including cstr == nullptr) are treated as empty.
     *
     * @{
     */
	bool equals(const String& s) const
	{
		return equals(s.cbuffer(), s.length());
	}
	bool equals(const char* cstr) const;
	bool equals(const char* cstr, size_t length) const;
	bool equals(const FlashString& fstr) const
	{
		return fstr.equals(*this);
	}
	/** @} */

	/**
     * @name Equality operator ==
     * @retval bool true if Strings are identical
     * @{
     */
	bool operator==(const String& rhs) const
	{
		return equals(rhs);
	}
	bool operator==(const char* cstr) const
	{
		return equals(cstr);
	}
	bool operator==(const FlashString& fstr) const
	{
		return equals(fstr);
	}
	/** @} */

	/**
     * @name In-equality operator !=
     * @retval bool Returns true if strings are not identical
     * @{
     */
	bool operator!=(const String& rhs) const
	{
		return !equals(rhs);
	}
	bool operator!=(const char* cstr) const
	{
		return !equals(cstr);
	}
	/** @} */

	/**
     * @name Comparison operators
     * @{
     */
	bool operator<(const String& rhs) const
	{
		return compareTo(rhs) < 0;
	}
	bool operator>(const String& rhs) const
	{
		return compareTo(rhs) > 0;
	}
	bool operator<=(const String& rhs) const
	{
		return compareTo(rhs) <= 0;
	}
	bool operator>=(const String& rhs) const
	{
		return compareTo(rhs) >= 0;
	}
	/** @} */

	/**
     * @name Test for equality, without case-sensitivity
     * @retval bool true if strings are considered the same
     *
     * null strings are treated as empty.
     *
     * @{
     */
	bool equalsIgnoreCase(const char* cstr) const;
	bool equalsIgnoreCase(const char* cstr, size_t length) const;
	bool equalsIgnoreCase(const String& s2) const
	{
		return equalsIgnoreCase(s2.cbuffer(), s2.length());
	}
	bool equalsIgnoreCase(const FlashString& fstr) const
	{
		return fstr.equalsIgnoreCase(*this);
	}
	/** @} */

	/**
     * @brief Compare the start of a String
     * Comparison is case-sensitive, must match exactly
     * @param prefix
     * @retval bool true on match
     */
	bool startsWith(const String& prefix) const
	{
		return startsWith(prefix, 0);
	}

	/**
     * @brief Compare a string portion
     * @param prefix
     * @param offset Index to start comparison at
     * @retval bool true on match
     * @note Comparison is case-sensitive, must match exactly
     *
     * mis-named as does not necessarily compare from start
     */
	bool startsWith(const String& prefix, size_t offset) const;

	/**
     * @brief Compare the end of a String
     * @param suffix
     * @retval bool true on match
     */
	bool endsWith(char suffix) const;

	/**
     * @brief Compare the end of a String
     * @param suffix
     * @retval bool true on match
     */
	bool endsWith(const String& suffix) const;

	// character access

	/**
     * @brief Obtain the character at the given index
     * @param index
     * @retval char
     * @note If index is invalid, returns NUL \0
     */
	char charAt(size_t index) const
	{
		return operator[](index);
	}

	/**
     * @brief Sets the character at a given index
     * @param index
     * @param c
     * @note If index is invalid, does nothing
     */
	void setCharAt(size_t index, char c);

	/**
     * @name Array operators
     *
     * If index is invalid, returns NUL \0
     *
     * @{
     */
	char operator[](size_t index) const;
	char& operator[](size_t index);
	/** @} */

	/** @brief Read contents of a String into a buffer
     *  @param buf buffer to write data
     *  @param bufsize size of buffer in bytes
     *  @param index offset to start
     *  @retval unsigned number of bytes copied, excluding nul terminator
     *  @note Returned data always nul terminated so buffer size needs to take this
     *  into account
     */
	size_t getBytes(unsigned char* buf, size_t bufsize, size_t index = 0) const;

	/**
     * @brief Read contents of String into a buffer
     * @see See `getBytes()`
     */
	void toCharArray(char* buf, size_t bufsize, size_t index = 0) const
	{
		getBytes(reinterpret_cast<unsigned char*>(buf), bufsize, index);
	}

	/**
     * @brief Get a constant (un-modifiable) pointer to String content
     * @retval const char* Always valid, even for a null string
     */
	const char* c_str() const
	{
		return cbuffer() ?: empty.cbuffer();
	}

	/**
     * @brief Get a modifiable pointer to String content
     * @note If String is NUL, returns nullptr.
     */
	char* begin()
	{
		return buffer();
	}

	/**
     * @brief Get a modifiable pointer to one-past the end of the String
     * @note Points to the terminating NUL character.
     * If String is NUL, returns nullptr.
     */
	char* end()
	{
		return buffer() + length();
	}
	const char* begin() const
	{
		return c_str();
	}
	const char* end() const
	{
		return c_str() + length();
	}

	// search

	/**
     * @name int indexOf(...)
     * Locate a character or String within another String.
     * @retval int Index if found, -1 if not found
     *
     * By default, searches from the beginning of the String, but can also start from a given index,
     * allowing for the locating of all instances of the character or String.
     *
     * @{
     */
	int indexOf(char ch, size_t fromIndex = 0) const;
	int indexOf(const char* s2_buf, size_t fromIndex, size_t s2_len) const;
	int indexOf(const char* s2_buf, size_t fromIndex = 0) const
	{
		return indexOf(s2_buf, fromIndex, strlen(s2_buf));
	}
	int indexOf(const String& s2, size_t fromIndex = 0) const
	{
		return indexOf(s2.cbuffer(), fromIndex, s2.length());
	}
	/** @} */

	/**
     * @name int lastIndexOf(...)
     * Locate a character or String within another String
     * @retval int Index if found, -1 if not found
     *
     * By default, searches from the end of the String, but can also work backwards from a given index,
     * allowing for the locating of all instances of the character or String.
     *
     * @{
     */
	int lastIndexOf(char ch) const;
	int lastIndexOf(char ch, size_t fromIndex) const;
	int lastIndexOf(const String& s2) const;
	int lastIndexOf(const String& s2, size_t fromIndex) const;
	int lastIndexOf(const char* s2_buf, size_t fromIndex, size_t s2_len) const;
	/** @} */

	/**
     * @name String substring(...)
     * Get a substring of a String.
     * @param from Index of first character to retrieve
     * @param to (optional) One-past the ending character to retrieve
     *
     * The starting index is inclusive (the corresponding character is included in the substring),
     * but the optional ending index is exclusive (the corresponding character is not included in the substring).
     *
     * If the ending index is omitted, the substring continues to the end of the String.
     *
     * If you don't need the original String, consider using remove() instead:
     *
     * 		String original("This is the original string.");
     * 		String sub = original.substring(0, 13);
     *
     * This produces the same result:
     *
     * 		original.remove(13);
     *
     * @{
     */
	String substring(size_t from, size_t to) const;
	String substring(size_t from) const
	{
		return substring(from, length());
	}
	/** @} */

	// modification

	/**
     * @name replace(...)
     * Replace all instances of a given character or substring with another character or substring.
     * @retval bool true on success, false on allocation failure
     *
     * Replacing a single character always succeeds as this is handled in-place.
     *
     * Where `replace` is longer than `find` the String may need to be re-allocated, which could
     * fail. If this happens the method returns false and the String is left unchanged.
     *
     * @{
     */
	void replace(char find, char replace);
	bool replace(const String& find, const String& replace);
	bool replace(const char* find_buf, size_t find_len, const char* replace_buf, size_t replace_len);
	/** @} */

	/**
     * @name remove()
     * Remove characters from a String.
     * @param index Index of the first character to remove
     * @param count Number of characters to remove
     * @note The String is modified in-situ without any reallocation
     *
     * If no count is provided then all characters from the given index to the end of the
     * String are removed.
     *
     * @{
     */
	void remove(size_t index)
	{
		remove(index, SIZE_MAX);
	}
	void remove(size_t index, size_t count);
	/** @} */

	/**
     * @brief Convert the entire String content to lower case
     */
	void toLowerCase(void);

	/**
     * @brief Convert the entire String content to upper case
     */
	void toUpperCase(void);

	/**
     * @brief Remove all leading and trailing characters from the String
	 * @param Set of characters to remove, defaults to whitespace set
     */
	void trim(const char* set = " \t\n\v\f\r");

	/**
	 * @name Pad string to a minimum length
	 *
	 * This is used, for example, when outputting tabular data.
	 * The string is modified in-situ to minimise memory reallocations.
	 *
	 * Methods may be chained like this::
	 *
	 * 		Serial << String(value).padLeft(10, '.') << endl;
	 *
	 * @{
	 */

	/**
	 * @brief Insert padding at start of string if length is less than given width
	 */
	String& padLeft(uint16_t minWidth, char c = ' ')
	{
		return pad(int16_t(-minWidth), c);
	}

	/**
	 * @brief Insert padding at end of string if length is less than given width
	 */
	String& padRight(uint16_t minWidth, char c = ' ')
	{
		return pad(int16_t(minWidth), c);
	}

	/**
	 * @brief Pad string if length is less than given width
	 * @param width Left-padded if < 0, right-padded if > 0.
	 */
	String& pad(int16_t minWidth, char c = ' ');

	/** @} */

	// parsing/conversion
	long toInt(void) const;
	float toFloat(void) const;

	/// Max chars. (excluding NUL terminator) we can store in SSO mode
	static constexpr size_t SSO_CAPACITY = STRING_OBJECT_SIZE - 2;

protected:
	/// Used when contents allocated on heap
	struct PtrBuf {
		char* buffer;	// the actual char array
		size_t len;		 // the String length (not counting the '\0')
		size_t capacity; // the array length minus one (for the '\0')
	};
	// For small strings we can store data directly without requiring the heap
	struct SsoBuf {
		char buffer[SSO_CAPACITY + 1];
		unsigned char len : 7;
		unsigned char set : 1; ///< true for SSO mode
	};
	union {
		PtrBuf ptr;
		SsoBuf sso;
	};

	static_assert(STRING_OBJECT_SIZE == sizeof(SsoBuf), "SSO Buffer alignment problem");
	static_assert(STRING_OBJECT_SIZE >= sizeof(PtrBuf), "STRING_OBJECT_SIZE too small");
	static_assert(STRING_OBJECT_SIZE <= 128, "STRING_OBJECT_SIZE too large (max. 128)");
	static_assert(STRING_OBJECT_SIZE % 4 == 0, "STRING_OBJECT_SIZE must be a multiple of 4");

protected:
	// Free any heap memory and set to non-SSO mode; isNull() will return true
	void invalidate(void);

	// String is Null (invalid) by default, i.e. non-SSO and null buffer
	__forceinline bool isNull() const
	{
		return !sso.set && (ptr.buffer == nullptr);
	}

	// Get writeable buffer pointer
	__forceinline char* buffer()
	{
		return sso.set ? sso.buffer : ptr.buffer;
	}

	// Get read-only buffer pointer
	__forceinline const char* cbuffer() const
	{
		return sso.set ? sso.buffer : ptr.buffer;
	}

	// Get currently assigned capacity for current mode
	__forceinline size_t capacity() const
	{
		return sso.set ? SSO_CAPACITY : ptr.capacity;
	}

	// Called whenever string length changes to ensure NUL terminator is set
	__forceinline void setlen(size_t len)
	{
		if(sso.set) {
			sso.len = len;
			sso.buffer[len] = '\0';
		} else {
			ptr.len = len;
			if(ptr.buffer != nullptr) {
				ptr.buffer[len] = '\0';
			}
		}
	}

	// copy and move
	String& copy(const char* cstr, size_t length);
	String& copy(flash_string_t pstr, size_t length);
	void move(String& rhs);
};

/** @} */

class StringSumHelper : public String
{
public:
	StringSumHelper(const String& s) : String(s)
	{
	}
	StringSumHelper(const char* p) : String(p)
	{
	}
	StringSumHelper(char c) : String(c)
	{
	}
	StringSumHelper(unsigned char num) : String(num)
	{
	}
	StringSumHelper(int num) : String(num)
	{
	}
	StringSumHelper(unsigned int num) : String(num)
	{
	}
	StringSumHelper(long num) : String(num)
	{
	}
	StringSumHelper(long long num) : String(num)
	{
	}
	StringSumHelper(unsigned long num) : String(num)
	{
	}
	StringSumHelper(unsigned long long num) : String(num)
	{
	}
	StringSumHelper(float num) : String(num)
	{
	}
	StringSumHelper(double num) : String(num)
	{
	}
};

#include "SplitString.h"

#endif // __cplusplus
