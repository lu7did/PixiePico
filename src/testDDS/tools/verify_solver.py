#!/usr/bin/env python3
"""Host-side reference for the exact search implemented in src/dds.c."""

from fractions import Fraction

XOSC = 12_000_000
VCO_MIN = 750_000_000
VCO_MAX = 1_600_000_000
SYS_MIN = 125_000_000
SYS_MAX = 250_000_000
L = 1024
N_MIN = 256
N_MAX = 0xFFFFFF


def clocks():
    seen = set()
    for fbdiv in range(16, 321):
        vco = XOSC * fbdiv
        if not VCO_MIN <= vco <= VCO_MAX:
            continue
        for post1 in range(1, 8):
            for post2 in range(1, post1 + 1):
                product = post1 * post2
                if vco % product:
                    continue
                sys_hz = vco // product
                if SYS_MIN <= sys_hz <= SYS_MAX and sys_hz not in seen:
                    seen.add(sys_hz)
                    yield sys_hz, vco, fbdiv, post1, post2


def solve(target):
    best = None
    for sys_hz, vco, fbdiv, post1, post2 in clocks():
        numerator = 128 * sys_hz * L
        for ones in range(L + 1):
            half_sum = 2 * L + ones
            ideal_den = target * half_sum
            nearest = (numerator + ideal_den // 2) // ideal_den
            for n in (nearest - 1, nearest, nearest + 1):
                if not N_MIN <= n <= N_MAX:
                    continue
                achieved = Fraction(numerator, n * half_sum)
                error = achieved - target
                candidate = (
                    abs(error), sys_hz, min(ones, L - ones), n,
                    error, achieved, vco, fbdiv, post1, post2, ones,
                )
                if best is None or candidate[:4] < best[:4]:
                    best = candidate
    return best


if __name__ == "__main__":
    for target in (7_074_000, 14_074_000, 28_074_000):
        result = solve(target)
        _, sys_hz, _, n, error, achieved, vco, fb, p1, p2, ones = result
        print(
            f"{target} Hz: PLL={sys_hz} Hz "
            f"(VCO={vco}, FB={fb}, P={p1}/{p2}), "
            f"CLKDIV={n // 256}+{n % 256}/256, "
            f"ones={ones}/{L}, achieved={float(achieved):.9f} Hz, "
            f"error={float(error):+.9f} Hz"
        )
