#ifndef THIRD_PARTY_STRNSTR_H
#define THIRD_PARTY_STRNSTR_H

#include <stddef.h>
#include <string.h>

/*
 * Toolchains modernas (incl. newlib en muchos SDKs) ya declaran strnstr()
 * en <string.h>. Volver a declararla dispara -Wredundant-decls (tratado como error).
 *
 * Si algún día compilas en un entorno que NO tenga strnstr(), la solución correcta
 * es compilar una implementación alternativa con OTRO nombre o definir HAVE_STRNSTR=0
 * en ese entorno, pero aquí no debemos redeclarar.
 */

#endif /* THIRD_PARTY_STRNSTR_H */


