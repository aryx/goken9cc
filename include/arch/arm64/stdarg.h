/* Plan9-style va_list for arm64, adapted from principia's
 * include/arch/{arm,mips}/u.h (4-byte-slot 32-bit convention) to this
 * arch's slot width.
 *
 * Verified empirically against 7c -S on throwaway probes: every named
 * *and* variadic argument occupies a full 8-byte-aligned stack slot on
 * this arch, regardless of its actual type width (e.g. an int32 parameter
 * still gets the same 8-byte stride as a following int64/pointer/double
 * one) -- so va_start/va_arg must round any type smaller than 8 bytes up
 * to 8 before doing the pointer arithmetic; types already >= 8 bytes
 * (long, double, pointers) match the compiler's own natural pointer
 * arithmetic and need no special-casing.
 */

typedef char* va_list;

#define va_start(list, start) \
	(list = (sizeof(start) < 8 ? (char*)((vlong*)&(start) + 1) : (char*)(&(start) + 1)))

#define va_end(list)

#define va_arg(list, mode) \
	((sizeof(mode) == 1) ? ((list += 8), (mode*)list)[-8] : \
	 (sizeof(mode) == 2) ? ((list += 8), (mode*)list)[-4] : \
	 (sizeof(mode) == 4) ? ((list += 8), (mode*)list)[-2] : \
	 ((list += sizeof(mode)), (mode*)list)[-1])
