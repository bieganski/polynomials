/** @file
   Implementacja klasy wielomianów

   @author Mateusz Biegański
   @copyright Mateusz Biegański
   @date 2017-06-15
*/

#include "poly.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/** Maksymalna wartość wykładnika wielomianu */
#define POLY_EXP_MAX INT_MAX

/**
 * Alokuje niepoprawny(!) jednomian i sprawdza poprawnosć alokacji.
 * @return stworzony jednomian
 */
static Mono* MonoAlloc(){
    Mono *new = malloc(sizeof(Mono));
    assert(new != NULL);
    new->p = PolyZero();
    new->next = NULL;
    new->exp = 0;
    return new;
}

/**
 * Zwraca liczbę jednomianów w wielomianie normalnym.
 * O(n), n - liczba jednomianów
 * @param[in] p : wielomian normalny
 * @return liczba jednomianów
 */
static unsigned PolyLen(const Poly *p){
    unsigned counter = 1;
    assert(!PolyIsCoeff(p));
    Mono *tmp = p->first;
    while(tmp->next != NULL) {
        counter++;
        tmp = tmp->next;
    }
    return counter;
}

/**
 * Działa jak MonoClone, jednak umożliwia ustawienie
 * wybranego wskaźnika na następny monomian.
 * @param[in] m : kopiowany jednomian
 * @param[in] next : wskaźnik na obiekt będący następnym jednomianem
 * @return skopiowany monomian
 */
static inline Mono MonoCloneNext(const Mono *m, Mono *next) {
    return (Mono) {.p = PolyClone(&m->p), .exp = m->exp, .next = next};
}

/**
 * Niszczy listę jednomianów należących do jakiegoś wielomianu.
 * (zaalokowanych dynamicznie)
 * @param[in] m : pierwszy usuwany jednomian
 */
static void MonoListDestroy(Mono *m){
    if (m == NULL)
        return ;
    MonoListDestroy(m->next);
    MonoDestroy(m);
    free(m);
}

void PolyDestroy(Poly *p){
    if (PolyIsCoeff(p))
        return ;
    MonoListDestroy(p->first);
}

/**
 * Kopiuje głęboko listę jednomianów.
 * @param[in] m wskaźnik na pierwszy element listy
 * @return wskaźnik na pierwszy element skopiowanej listy
 */
static Mono* MonoListClone(Mono *m){
    Mono *next, *new;
    if (m == NULL)
        return NULL;
    next = MonoListClone(m->next);
    new = MonoAlloc();
    *new = MonoCloneNext(m, next);
    return new;
}

Poly PolyClone(const Poly *p){
    if (PolyIsCoeff(p))
        return *p;
    return (Poly) {.first = MonoListClone(p->first)};
}

static Poly PolyAddCoeff(const Poly *p, poly_coeff_t c);

/**
 * Dodaje wielomian normalny i współczynnik.
 * @param[in] p : wielomian normalny
 * @param[in] c : współczynnik
 * @return 'p + c'
 */
static Poly PolyAddPolyCoeff(const Poly *p, poly_coeff_t c){
    Poly new;
    Mono *first;
    assert(p->first != NULL);
    if (c == 0)
        return PolyClone(p);
    else if (p->first->exp == 0) {
        Poly r0 = PolyAddCoeff(&p->first->p, c);
        if (PolyIsZero(&r0)) {
            PolyDestroy(&r0);
            return (Poly) {.first = MonoListClone(p->first->next)};
        }
        else if (PolyIsCoeff(&r0) && p->first->next == NULL) {
            return r0;
        }
        else {
            first = MonoAlloc();
            *first = MonoFromPoly(&r0, 0);
            first->next = MonoListClone(p->first->next);;
            return (Poly) {.first = first};
        }
    }
    else {
        new = PolyClone(p);
        Mono *tmp = new.first;
        Mono *tmpZeroCoeff = MonoAlloc();
        tmpZeroCoeff->p = PolyFromCoeff(c);
        tmpZeroCoeff->exp = 0;
        tmpZeroCoeff->next = tmp;
        new.first = tmpZeroCoeff;
        return new;
    }
    assert(false);
    return new;
}

/**
 * Dodaje do wielomianu współczynnik.
 * @param[in] p : wielomian
 * @param[in] c : współczynnik
 * @return `p + c`
 */
static Poly PolyAddCoeff(const Poly *p, poly_coeff_t c){
    if (PolyIsCoeff(p))
        return PolyFromCoeff(p->coeff + c);
    else
        return PolyAddPolyCoeff(p, c);
}

/**
 * Przyjmuje wartość true jeśli zadany wielomian ma
 * tylko jeden jednomian stopnia 0, którego współczynnik
 * jest stałą.
 * @param[in] p : sprawdzany wielomian
 * @return 'czy wielomian trzeba znormalizować'
 */
static inline bool OnlyZeroExpMonoWithConstCoeff(Poly *p) {
    if (PolyIsCoeff(p))
        return false;
    return p->first->next == NULL && p->first->exp == 0
           && PolyIsCoeff(&p->first->p);
}

/**
 * Dodaje dwa normalne wielomiany.
 * @param[in] p : wielomian normalny
 * @param[in] q : wielomian normalny
 * @return 'p + q'
 */
static Poly PolyAddPolyPoly(const Poly *p, const Poly *q){
    Mono *newTmp, *newFirst = NULL, *prevTmp = NULL;
    Mono *tmpp = p->first, *tmpq = q->first;
    assert(p->first != NULL);
    assert(q->first != NULL);
    while (tmpp != NULL || tmpq != NULL) {
        poly_exp_t pe = tmpp != NULL ? tmpp->exp : POLY_EXP_MAX;
        poly_exp_t qe = tmpq != NULL ? tmpq->exp : POLY_EXP_MAX;
        if (pe != qe) {
            bool qeGreater = pe < qe;
            newTmp = MonoAlloc();
            *newTmp = qeGreater ? MonoClone(tmpp) : MonoClone(tmpq);
            if (prevTmp == NULL)
                newFirst = newTmp;
            else
                prevTmp->next = newTmp;
            if (qeGreater)
                tmpp = tmpp->next;
            else
                tmpq = tmpq->next;
            prevTmp = newTmp;
        }
        else {
            Poly sum = PolyAdd(&tmpp->p, &tmpq->p);
            if (!PolyIsZero(&sum)) {
                newTmp = MonoAlloc();
                *newTmp = (Mono) {.p = sum, .exp = pe, .next = NULL};
                if (prevTmp == NULL)
                    newFirst = newTmp;
                else
                    prevTmp->next = newTmp;
                prevTmp = newTmp;
            }
            tmpp = tmpp->next;
            tmpq = tmpq->next;
        }
    } /* while */

    if (newFirst == NULL) {
        return PolyZero();
    }
    else {
        Poly res = (Poly) {.first = newFirst};
        if (OnlyZeroExpMonoWithConstCoeff(&res)) {
            Poly coeffPoly = res.first->p;
            assert(PolyIsCoeff(&coeffPoly));
            free(res.first);
            return coeffPoly;
        }
        return res;
    }
}

Poly PolyAdd(const Poly *p, const Poly *q){
    if (PolyIsCoeff(p))
        return PolyAddCoeff(q, p->coeff);
    else if (PolyIsCoeff(q))
        return PolyAddCoeff(p, q->coeff);
    else
        return PolyAddPolyPoly(p, q);
}

static poly_exp_t MonoCmp(const void *a, const void *b){
    return ((const Mono *) a)->exp - ((const Mono *) b)->exp;
}

static inline Mono MonoDummy(){
    return (Mono) {.p = PolyFromCoeff(1), .exp = -1, .next = NULL};
}

static inline bool MonoIsDummy(Mono *m){
    return m->exp == -1;
}

/**
 * Usuwa jednomiany z zerowymi współczynnikami.
 * @param[in] p : normalizowany wielomian
 * @return 'wynikowy wielomian'
 */
static Poly PolyFix(Poly *p){
    if (PolyIsCoeff(p))
        return *p;
    Mono *dummy, *act, *prev, *tmp;
    prev = dummy = MonoAlloc();
    *prev = MonoDummy();
    prev->next = act = p->first;
    while (act != NULL) {
        assert(act->exp > prev->exp);
        if (PolyIsZero(&act->p)) {
            tmp = act->next;
            free(act);
            prev->next = act = tmp;
        }
        else {
            prev = act;
            act = act->next;
        }
    }
    p->first = dummy->next;
    free(dummy);
    return p->first == NULL ? PolyZero() : *p;
}

/**
 * Tworzy wielomian z zadanej listy jednomianów, jednakże nie sprawdza,
 * czy współczynniki stworzonych jednomianów są zerami.
 * @param[in] count : liczba jednomianów
 * @param[in] monos : wskaźnik do tablicy jednomianów
 * @return wynikowy, niepoprawny wielomian
 */
static Poly PolyAddMonosZeroCoeff(unsigned count, const Mono monos[]){
    assert(count > 0);
    Mono *act, *tmp, *dummy, *prev;
    Poly res;
    prev = act = dummy = MonoAlloc();
    *dummy = MonoDummy();
    Mono *arr = calloc(count, sizeof(Mono));
    assert(arr != NULL);
    memcpy(arr, monos, count * sizeof(Mono));
    qsort(arr, count, sizeof(Mono), MonoCmp);
    for (unsigned i = 0; i < count; i++) {
        tmp = MonoAlloc();
        if (MonoIsDummy(act)) {
            *tmp = MonoFromPoly(&arr[i].p, arr[i].exp);
            act = prev->next = tmp;
        }
        else if (arr[i].exp != act->exp) {
            assert(arr[i].exp > act->exp);
            *tmp = MonoFromPoly(&arr[i].p, arr[i].exp);
            act->next = tmp;
            prev = act;
            act = tmp;
        }
        else {
            Poly sum = PolyAdd(&act->p, &arr[i].p);
            *tmp = MonoFromPoly(&sum, arr[i].exp);
            PolyDestroy(&arr[i].p);
            PolyDestroy(&act->p);
            free(act);
            prev->next = act = tmp;
        }
    }
    Mono *first = dummy->next;
    assert(first != NULL);
    if (first->next == NULL && first->exp == 0 && PolyIsCoeff(&first->p)) {
        res = first->p;
        free(first);
    }
    else {
        res = (Poly) {.first = first};
    }
    free(arr), free(dummy);
    return res;
}

Poly PolyAddMonos(unsigned count, const Mono monos[]){
    Poly res = PolyAddMonosZeroCoeff(count, monos);
    return PolyFix(&res);
}

/**
 * Mnoży dwa wielomiany normalne.
 * @param[in] p : wielomian normalny
 * @param[in] q : wielomian normalny
 * @return `p * q`
 */
static Poly PolyMulPolyPoly(const Poly *p, const Poly *q){
    unsigned count = PolyLen(p) * PolyLen(q);
    Mono  *arr = calloc(count, sizeof(struct Mono));
    unsigned k = 0;
    Mono *tmpp = p->first, *tmpq = q->first;
    while (tmpp != NULL) {
        tmpq = q->first;
        while (tmpq != NULL) {
            arr[k++] = (Mono) {.p = PolyMul(&tmpp->p, &tmpq->p),
                               .exp = tmpp->exp + tmpq->exp};
            tmpq = tmpq->next;
        }
        tmpp = tmpp->next;
    }
    Poly res = PolyAddMonos(count, arr);
    free(arr);
    return res;
}

static Poly PolyMulCoeff(const Poly *p, poly_coeff_t c);

/**
 * Mnoży wielomian normalny przez współczynnik.
 * @param[in] p : wielomian normalny
 * @param[in] c : współczynnik
 * @return `p * c`
 */
static Poly PolyMulPolyCoeff(const Poly *p, poly_coeff_t c){
    if (c == 0)
        return PolyZero();
    unsigned len = PolyLen(p), count = 0;
    Mono *tmp = p->first;
    Mono *arr = calloc(len, sizeof(Mono));
    while (tmp != NULL) {
        arr[count++] = (Mono) {.p = PolyMulCoeff(&tmp->p, c), .exp = tmp->exp};
        tmp = tmp->next;
    }
    assert(count == len);
    Poly res = PolyAddMonos(len, arr);
    free(arr);
    return res;
}

/**
 * Mnoży wielomian przez współczynnik.
 * @param[in] p : wielomian
 * @param[in] c : współczynnik
 * @return `p * c`
 */
static Poly PolyMulCoeff(const Poly *p, poly_coeff_t c){
    if (PolyIsCoeff(p))
        return PolyFromCoeff(p->coeff * c);
    else
        return PolyMulPolyCoeff(p, c);
}

Poly PolyMul(const Poly *p, const Poly *q){
    if (PolyIsCoeff(p))
        return PolyMulCoeff(q, p->coeff);
    else if (PolyIsCoeff(q))
        return PolyMulCoeff(p, q->coeff);
    else
        return PolyMulPolyPoly(p, q);
}

Poly PolyNeg(const Poly *p){
    return PolyMulCoeff(p, -1);
}

Poly PolySub(const Poly *p, const Poly *q){
    Poly neg_q = PolyNeg(q);
    Poly res = PolyAdd(p, &neg_q);
    PolyDestroy(&neg_q);
    return res;
}

poly_exp_t PolyDegBy(const Poly *p, unsigned var_idx){
    poly_exp_t deg = -1;
    if (PolyIsCoeff(p)) {
        return PolyIsZero(p) ? -1 : 0;
    }
    else if (var_idx == 0) {
        Mono *tmp = p->first;
        while (tmp->next != NULL)
            tmp = tmp->next;
        return tmp->exp;
    }
    else {
        Mono *tmp = p->first;
        while (tmp != NULL) {
            poly_exp_t d = PolyDegBy(&tmp->p, var_idx - 1);
            if (d > deg)
                deg = d;
            tmp = tmp->next;
        }
    }
    return deg;
}

poly_exp_t PolyDeg(const Poly *p){
    if (PolyIsCoeff(p)) {
        return PolyIsZero(p) ? -1 : 0;
    }
    else {
        poly_exp_t deg = -1;
        Mono *tmp = p->first;
        while (tmp != NULL) {
            poly_exp_t d = PolyDeg(&tmp->p) + tmp->exp;
            if (d > deg)
                deg = d;
            tmp = tmp->next;
        }
        return deg;
    }
}

bool PolyIsEq(const Poly *p, const Poly *q){
    if (PolyIsCoeff(p) != PolyIsCoeff(q)) {
        return false;
    }
    else if (PolyIsCoeff(p)) {
        return p->coeff == q->coeff;
    }
    else if (PolyLen(p) != PolyLen(q)) {
        return false;
    }
    else {
        Mono *tmpp = p->first, *tmpq = q->first;
        while (tmpp != NULL) {
            if (tmpp->exp != tmpq->exp || !PolyIsEq(&tmpp->p, &tmpq->p))
                return false;
            tmpp = tmpp->next;
            tmpq = tmpq->next;
        }
        return true;
    }
}

/**
 * Algorytm szybkiego potęgowania.
 * @param[in] a - podstawa
 * @param[in] n - wykładnik
 * @return 'a^n'
 */
static inline poly_coeff_t Pow(poly_coeff_t a, poly_exp_t n){
    if (n == 0)
        return 1;
    if (n % 2 == 0)
        return Pow(a * a, n / 2);
    else
        return a * Pow(a * a, n / 2);
}

Poly PolyAt(const Poly *p, poly_coeff_t x){
    if (PolyIsCoeff(p))
        return *p;
    Poly res = PolyZero();
    Mono *tmp = p->first;
    poly_exp_t actExp = 0;
    poly_coeff_t powRes = 1;
    while (tmp != NULL) {
        powRes *= Pow(x, tmp->exp - actExp);
        actExp = tmp->exp;
        Poly mulTmp = PolyMulCoeff(&tmp->p, powRes);
        Poly addTmp = PolyAdd(&res, &mulTmp);
        PolyDestroy(&res);
        PolyDestroy(&mulTmp);
        res = addTmp;
        tmp = tmp->next;
    }
    return res;
}

/**
 * Podnosi wielomian 'p' do nieujemnej potęgi 'exp'
 * @param[in] p - podstawa
 * @param[in] exp - wykładnik
 * @return 'p^exp'
 */
static Poly PolyPow(const Poly *p, unsigned exp) {
    assert(exp >= 0);
    if (exp == 0)
        return PolyFromCoeff(1);
    Poly tmp, res = PolyClone(p);
    while (--exp) {
        tmp = PolyMul(&res, p);
        PolyDestroy(&res);
        res = tmp;
    }
    return res;
}

static Poly PolyComposeRec(unsigned actDeg, const Poly *p,
        unsigned count, const Poly x[]);

/**
 * Za zmienną jednomianu 'm' wstawia wielomian 'p',
 * a zamiast współczynnika m->p bierze wynik PolyComposeRec
 * wołanej właśnie na tym współczynniku ze stopniem o jeden większym.
 * @param[in] m - podmieniany jednomian
 * @param[in] p - wielomian wstawiany w miejsce 'm'
 * @param[in] actDeg - głębokość rekursji
 * @return wygenerowany wielomian
 */
static Poly PolySubstitute(const Mono *m, const Poly *p, unsigned actDeg,
        unsigned count, const Poly x[]){
    Poly q = PolyPow(p, m->exp);
    Poly r = PolyComposeRec(actDeg + 1, &m->p, count, x);
    Poly res = PolyMul(&q, &r);
    PolyDestroy(&q);
    PolyDestroy(&r);
    return res;
}

Poly PolyCompose(const Poly *p, unsigned count, const Poly x[]){
    return PolyComposeRec(0, p, count, x);
}
/**
 * Tworzy wielomian poprzez wstawianie wielomianów
 * z zadanej tablicy na miejsce kolejnych jednomianów.
 * @param[in] actDeg - głębokość rekursji
 * @param[in] m - podmieniany jednomian
 * @param[in] p - wielomian wstawiany w miejsce 'm'
 * @return wygenerowany wielomian
 */
static Poly PolyComposeRec(unsigned actDeg, const Poly *p,
        unsigned count, const Poly x[]) {
    Poly substituted, tmpPoly;
    Mono *tmp = p->first;
    Poly res = tmp == NULL ? *p : PolyZero();
    Poly usedToSubst = actDeg >= count ? PolyZero() : x[actDeg];
    while (tmp != NULL) {
        substituted = PolySubstitute(tmp, &usedToSubst, actDeg, count, x);
        tmpPoly = res;
        res = PolyAdd(&res, &substituted);
        PolyDestroy(&tmpPoly);
        PolyDestroy(&substituted);
        tmp = tmp->next;
    }
    return res;
}
