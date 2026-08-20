// ft8_tones_from_text.c
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// -----------------------------------------------------------------------------
// INTEGRACIÓN CON FT8 LIB (kgoba/ft8_lib)
//
// Esta librería provee encoder FT8/FT4 en C para entornos embebidos. :contentReference[oaicite:2]{index=2}
//
// Como el repo puede cambiar nombres exactos de funciones/headers,
// la idea es:
//   1) incluir los .c/.h de ft8_lib dentro de tu proyecto Pico
//   2) ajustar estos includes + nombres de función a los que tengas en tu copia
// -----------------------------------------------------------------------------
// EJEMPLO (ajusta a tu árbol real):
// #include "ft8/pack.h"
// #include "ft8/encode.h"

// --- ADAPTA ESTOS PROTOTIPOS A TU FT8_LIB REAL ---
// Deben: (a) empaquetar el texto a 77 bits, (b) generar 79 tonos (0..7).

// pack77: toma texto FT8 (hasta 37 chars) y produce payload "packed" (9-10 bytes típicos).
// Retorna true si pudo empaquetar el tipo de mensaje soportado.
bool ft8_pack77(const char *msg, uint8_t packed[10]);

// encode: toma packed y produce 79 tonos (0..7).
void ft8_encode_79(const uint8_t packed[10], uint8_t tones[79]);

// -----------------------------------------------------------------------------
// API simple para tu aplicación
// -----------------------------------------------------------------------------
bool ft8_message_to_tones(const char *message, uint8_t tones_out[79])
{
    if (!message || !tones_out) return false;

    // FT8 suele aceptar hasta 37 caracteres en el campo msg (WSJT-X).
    // ft8_lib soporta el set básico (CQ call grid, etc.). :contentReference[oaicite:3]{index=3}
    if (strlen(message) > 37) return false;

    uint8_t packed[10];
    memset(packed, 0, sizeof(packed));
    memset(tones_out, 0, 79);

    if (!ft8_pack77(message, packed)) {
        return false; // mensaje no soportado / parse error
    }

    ft8_encode_79(packed, tones_out);

    // Sanitizar por si acaso:
    for (int i = 0; i < 79; i++) {
        tones_out[i] &= 0x07; // 0..7
    }

    return true;
}

