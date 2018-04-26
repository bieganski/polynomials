/** @file
   Interfejs klasy wielomianów

   @author Mateusz Biegańsk
   @copyright Mateusz Biegański
   @date 2017-06-15
*/

#ifndef __POLY_H__
#define __POLY_H__

#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

/** Typ współczynników wielomianu */
typedef long poly_coeff_t;

/** Typ wykładników wielomianu */
typedef int poly_exp_t;

struct Mono;

/**
 * Struktura przechowująca wielomian
 * 'coeff' jest używany jeśli wielomian jest współczynnikiem,
 * jeśli nie jest to 'first' != NULL
 */
typedef struct Poly {
    poly_coeff_t coeff; ///< współczynnik
    struct Mono *first; ///< najmniejszy względem wykładnika jednomian
} Poly;

/**
  * Struktura przechowująca jednomian
  * Jednomian ma postać `p * x^e`.
  * Współczynnik `p` może też być wielomianem.
  * Będzie on traktowany jako wielomian nad kolejną zmienną (nie nad x).
  * Implementacja jest oparta na liście jednomianów, stąd
  * wskaźnik 'next' na kolejny jednomian składowy danego wielomianu
  */
typedef struct Mono {
    Poly p; ///< współczynnik
    poly_exp_t exp; ///< wykładnik
    struct Mono *next; ///< kolejny jednomian
} Mono;

/**
 * Tworzy wielomian, który jest współczynnikiem.
 * @param[in] c : wartość współczynnika
 * @return wielomian
 */
static inline Poly PolyFromCoeff(poly_coeff_t c) {
    return (Poly) {.coeff = c, .first = NULL};
}

/**
 * Tworzy wielomian tożsamościowo równy zeru.
 * @return wielomian
 */
static inline Poly PolyZero() {
    return PolyFromCoeff(0);
}

/**
 * Tworzy jednomian `p * x^e`.
 * Przejmuje na własność zawartość struktury wskazywanej przez @p p.
 * @param[in] p : wielomian - współczynnik jednomianu
 * @param[in] e : wykładnik
 * @return jednomian `p * x^e`
 */
static inline Mono MonoFromPoly(const Poly *p, poly_exp_t e) {
    return (Mono) {.p = *p, .exp = e, .next = NULL};
}

/**
 * Sprawdza, czy wielomian jest współczynnikiem.
 * @param[in] p : wielomian
 * @return Czy wielomian jest współczynnikiem?
 */
static inline bool PolyIsCoeff(const Poly *p) {
    assert(p != NULL);
    return p->first == NULL;
}

/**
 * Sprawdza, czy wielomian jest tożsamościowo równy zeru.
 * @param[in] p : wielomian
 * @return Czy wielomian jest równy zero?
 */
static inline bool PolyIsZero(const Poly *p) {
    return PolyIsCoeff(p) && p->coeff == 0;
}

/**
 * Usuwa wielomian z pamięci.
 * @param[in] p : wielomian
 */
void PolyDestroy(Poly *p);

/**
 * Usuwa jednomian z pamięci.
 * @param[in] m : jednomian
 */
static inline void MonoDestroy(Mono *m) {
    PolyDestroy(&m->p);
}

/**
 * Robi pełną, głęboką kopię wielomianu.
 * @param[in] p : wielomian
 * @return skopiowany wielomian
 */
Poly PolyClone(const Poly *p);

/**
 * Robi pełną, głęboką kopię jednomianu.
 * @param[in] m : jednomian
 * @param[in] next : wskaźnik na następny jednomian
 * @return skopiowany jednomian
 */
static inline Mono MonoClone(const Mono *m) {
    return (Mono) {.p = PolyClone(&m->p), .exp = m->exp, .next = NULL};
}

/**
 * Dodaje dwa wielomiany.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p + q`
 */
Poly PolyAdd(const Poly *p, const Poly *q);

/**
 * Sumuje listę jednomianów i tworzy z nich wielomian.
 * Przejmuje na własność zawartość tablicy @p monos.
 * @param[in] count : liczba jednomianów
 * @param[in] monos : tablica jednomianów
 * @return wielomian będący sumą jednomianów
 */
Poly PolyAddMonos(unsigned count, const Mono monos[]);

/**
 * Mnoży dwa wielomiany.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p * q`
 */
Poly PolyMul(const Poly *p, const Poly *q);

/**
 * Zwraca przeciwny wielomian.
 * @param[in] p : wielomian
 * @return `-p`
 */
Poly PolyNeg(const Poly *p);

/**
 * Odejmuje wielomian od wielomianu.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p - q`
 */
Poly PolySub(const Poly *p, const Poly *q);

/**
 * Zwraca stopień wielomianu ze względu na zadaną zmienną (-1 dla wielomianu
 * tożsamościowo równego zeru).
 * Zmienne indeksowane są od 0.
 * Zmienna o indeksie 0 oznacza zmienną główną tego wielomianu.
 * Większe indeksy oznaczają zmienne wielomianów znajdujących się
 * we współczynnikach.
 * @param[in] p : wielomian
 * @param[in] var_idx : indeks zmiennej
 * @return stopień wielomianu @p p z względu na zmienną o indeksie @p var_idx
 */
poly_exp_t PolyDegBy(const Poly *p, unsigned var_idx);

/**
 * Zwraca stopień wielomianu (-1 dla wielomianu tożsamościowo równego zeru).
 * @param[in] p : wielomian
 * @return stopień wielomianu @p p
 */
poly_exp_t PolyDeg(const Poly *p);

/**
 * Sprawdza równość dwóch wielomianów.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p = q`
 */
bool PolyIsEq(const Poly *p, const Poly *q);

/**
 * Wylicza wartość wielomianu w punkcie @p x.
 * Wstawia pod pierwszą zmienną wielomianu wartość @p x.
 * W wyniku może powstać wielomian, jeśli współczynniki są wielomianem
 * i zmniejszane są indeksy zmiennych w takim wielomianie o jeden.
 * Formalnie dla wielomianu @f$p(x_0, x_1, x_2, \ldots)@f$ wynikiem jest
 * wielomian @f$p(x, x_0, x_1, \ldots)@f$.
 * @param[in] p : zadany wielomian
 * @param[in] x : zmienna którą podstawiamy
 * @return @f$p(x, x_0, x_1, \ldots)@f$
 */
Poly PolyAt(const Poly *p, poly_coeff_t x);

/**
 * Pod i-tą zmienną wielomianu p wstawia wielomian x[i]
 * lub 0 jeśli i >= count.
 * @param[in] p : zadany wielomian
 * @param[in] count : liczba jednomianów w tablicy
 * @param[in] x : tablica jednomianów
 */
Poly PolyCompose(const Poly *p, unsigned count, const Poly x[]);

#endif /* __POLY_H__ */
