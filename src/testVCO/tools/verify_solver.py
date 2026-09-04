#!/usr/bin/env python3
"""Referencia y pruebas del solver estático implementado en ddsvco.c."""

from fractions import Fraction

XOSC = 12_000_000
REF_MIN = 5_000_000
VCO_MIN = 750_000_000
VCO_MAX = 1_600_000_000
SYS_MIN = 125_000_000
SYS_MAX = 250_000_000
N_MIN = 256
N_MAX = 0xFFFFFF


def solve(target_hz):
    best = None
    for refdiv in range(1, 64):
        ref = Fraction(XOSC, refdiv)
        if ref < REF_MIN or XOSC % refdiv:
            continue
        for fbdiv in range(16, 321):
            vco = ref * fbdiv
            if not VCO_MIN <= vco <= VCO_MAX:
                continue
            for postdiv1 in range(1, 8):
                for postdiv2 in range(1, postdiv1 + 1):
                    sys = vco / (postdiv1 * postdiv2)
                    if not SYS_MIN <= sys <= SYS_MAX:
                        continue
                    ideal = 128 * sys / target_hz
                    floor_n = ideal.numerator // ideal.denominator
                    for n in {floor_n, floor_n + 1}:
                        if not N_MIN <= n <= N_MAX:
                            continue
                        achieved = 128 * sys / n
                        error = achieved - target_hz
                        candidate = (
                            abs(error), -vco, sys, n,
                            refdiv, fbdiv, postdiv1, postdiv2,
                            achieved, error,
                        )
                        if best is None or candidate[:4] < best[:4]:
                            best = candidate
    return best


EXPECTED = {
    7_074_000: (2, 263, 7, 1, 4079, 2.7317619864812803),
    14_074_000: (2, 211, 3, 2, 1919, -3.1266284523189163),
    28_074_000: (2, 261, 4, 3, 595, -50.42016806722689),
}


if __name__ == "__main__":
    for target, expected in EXPECTED.items():
        result = solve(target)
        _, _, sys, n, refdiv, fbdiv, p1, p2, achieved, error = result
        actual_tuple = (refdiv, fbdiv, p1, p2, n)
        assert actual_tuple == expected[:5], (target, actual_tuple, expected)
        assert abs(float(error) - expected[5]) < 1e-9
        print(
            f"{target} Hz: REFDIV={refdiv}, FBDIV={fbdiv}, "
            f"POSTDIV={p1}/{p2}, PLL_SYS={float(sys):.9f} Hz, "
            f"CLKDIV={n // 256}+{n % 256}/256, "
            f"salida={float(achieved):.9f} Hz, "
            f"error={float(error):+.9f} Hz"
        )
