/* Empty: every benchs/compcert/*.c program wired up so far also
 * includes <stdio.h>, which pulls in libc.h -- and libc.h already
 * declares floor/ceil/sqrt/pow/exp/log/fmod/sin/cos/tan/asin/acos/
 * atan/atan2/... under their familiar names (include/math/basic.h),
 * so there is nothing math.h itself needs to add. See stdio.h's own
 * comment.
 */
