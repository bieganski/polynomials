
#ifndef __POLY_CALC_H__
#define __POLY_CALC_H__

#include "poly.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct PolyStackEl {
    Poly p;
    struct PolyStackEl *next;
} PolyStackEl;

typedef struct PolyStack {
    PolyStackEl *first;
} PolyStack;

/**
* @brief inicjuje stos
* @return stworzony stos
*/
PolyStack *Init();

/**
* @brief sprawdza pustość stosu
* @param stos
* @return czyPusty
*/
bool IsEmpty(PolyStack *s);

/**
* @brief usuwa górny element
* @param stos
* @return usunięty element
*/
Poly Pop(PolyStack *s);

/**
* @brief wrzuca na stos zadany element
* @param[in] p : wrzucany wielomian
* @param[in] s : stos
*/
void Push(Poly p, PolyStack *s);

/**
* @brief zwraca kopię górnego elementu
* @param[in] s : stos
* @return kopia
*/
Poly Top(PolyStack *s);

/**
* @brief Czyta wiersz i odpowiednio go interpretuje
* @param[in] s stos
*/
void Read(PolyStack *s);

/**
* @brief czyści zadany stos
* @param[in] s stos
*/
void CleanStack(PolyStack *s);

#endif /*__POLY_CALC_H__*/
