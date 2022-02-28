//STD Includes
#include "RvStdDefs.h"

#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>

// Generates a lookup table for the checksums of all 8-bit values.
__inline array<uint32_t, 256> generate_crc_lookup_table() noexcept
{
	auto const reversed_polynomial = uint32_t{ 0xEDB88320uL };

	// This is a function object that calculates the checksum for a value,
	// then increments the value, starting from zero.
	struct byte_checksum
	{
		uint32_t operator()() noexcept
		{
			auto checksum = static_cast<uint32_t>(n++);

			for (auto i = 0; i < 8; ++i)
				checksum = (checksum >> 1) ^ ((checksum & 0x1u) ? reversed_polynomial : 0);

			return checksum;
		}

		unsigned n = 0;
	};

	auto table = array<uint32_t, 256>{};
	eastl::generate(table.begin(), table.end(), byte_checksum{});

	return table;
}

__inline uint32_t checkSum(uint32_t checksum, char value, const array<uint32_t, 256>& table)
{
	return table[(checksum ^ value) & 0xFFu] ^ (checksum >> 8);
}

// Calculates the CRC for any sequence of values. (You could use type traits and a
// static assert to ensure the values can be converted to 8 bits.)
__inline uint32_t crc(char* first, size_t size, uint32_t initialVal = ~uint32_t{ 0 } &uint32_t{ 0xFFFFFFFFuL })
{
	// Generate lookup table only on first use then cache it - this is thread-safe.
	static const array<uint32_t, 256> table = generate_crc_lookup_table();

	// Calculate the checksum - make sure to clip to 32 bits, for systems that don't
	// have a true (fast) 32-bit type.
	uint32_t checkSumVal = initialVal;
	for (size_t i = 0; i < size; i++)
	{
		checkSumVal = checkSum(checkSumVal, first[i], table);
	}
	return uint32_t{ 0xFFFFFFFFuL } & checkSumVal;
}