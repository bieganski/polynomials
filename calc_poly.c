#include <assert.h>
#include <limits.h>
#include <string.h>

#include "poly.h"
#include "calc_poly.h"
#include "utils.h"

#define MONOS_ARR_INIT_SIZE 10
#define MAX_COMM_LEN 10
#define POLY_COEFF_MAX LONG_MAX
#define POLY_COEFF_MIN LONG_MIN
#define MAX_STACK_SIZE 100000

PolyStack *Init() {
    PolyStack *new = malloc(sizeof(PolyStack));
    assert(new != NULL);
    new->first = NULL;
    return new;
}

bool IsEmpty(PolyStack *s) {
    return s->first == NULL;
}

Poly Pop(PolyStack *s) {
    assert(!IsEmpty(s));
    PolyStackEl *tmp = s->first;
    s->first = tmp->next;
    Poly p = tmp->p;
    free(tmp);
    return p;
}

void Push(Poly p, PolyStack *s) {
    PolyStackEl *new = malloc(sizeof(PolyStackEl));
    assert(new != NULL);
    new->p = p;
    new->next = s->first;
    s->first = new;
}

Poly Top(PolyStack *s) {
    assert(!IsEmpty(s));
    return s->first->p;
}

static inline bool ProperLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool ProperDigit(char c) {
    return c >= '0' && c <= '9';
}

static int DigitNumber(long long int n) {
    int count = 1;
    while (n / 10) {
        n = n / 10;
        count++;
    }
    return count;
}

/**
 * Zwraca informację, czy dany znak 'c' dodany do 'testedNumber'
 * nie sprawi, że utworzona w ten sposób liczba wyjdzie poza zadany zakres.
 */
static bool WillBeProper(long long int lowerBound, long long int upperBound,
                         long long int testedNumber, char c_digitToAdd, int sign) {
    assert(ProperDigit(c_digitToAdd));
    int digitToAdd = c_digitToAdd - '0';
    int digs = DigitNumber(testedNumber);
    if (digs > 1)
        if (digs == DigitNumber(lowerBound) || digs == DigitNumber(upperBound))
            return false;
    if (testedNumber >= LLONG_MAX/10)
        return sign * digitToAdd <= LLONG_MAX % 10;
    if (testedNumber <= LLONG_MIN/10)
        return sign * digitToAdd >= LLONG_MIN % 10;
    long long int res = testedNumber * 10 + (sign * digitToAdd);
    return res <= upperBound && res >= lowerBound;
}

/**
 * Czyta liczbę aż do momentu wczytania niepoprawnego znaku
 * (przekraczający zakres bądź niebędący cyfrą).
 * errOccured jeśli poza zadanym zakresem.
 * Docelowo zwraca wynik w postaci poly_coeff_t.
 */
static poly_coeff_t ReadNumber(long long int lowerBound, long long int upperBound,
                       int *col, bool *errOccured) {
    poly_coeff_t res = 0;
    bool readAnything = false;
    int sign = 1;
    char c = getchar();
    ++*col;
    if (c == '-') {
        sign = -1;
        if (lowerBound >= 0) {
            *errOccured = true;
            return -1;
        }
        c = getchar();
        ++*col;
    }
    while (ProperDigit(c) && !*errOccured) {
        if (WillBeProper(lowerBound, upperBound, res, c, sign)) {
            readAnything = true;
            res = res * 10 + sign * (c - '0');
            c = getchar();
            ++*col;
        }
        else {
            *errOccured = true;
        }
    }
    ungetc(c, stdin);
    --*col;
    if (!readAnything)
        *errOccured = true;
    return res;
}

static void PolyPrint(const Poly *p);

static void MonoPrint(const Mono *m) {
    printf("(");
    PolyPrint(&m->p);
    printf(",%d)", m->exp);
}

static void PolyPrint(const Poly *p) {
    Mono *tmp = p->first;
    if (PolyIsCoeff(p))
        printf("%ld", p->coeff);
    else
        while (tmp != NULL) {
            MonoPrint(tmp);
            if (tmp->next != NULL)
                putchar('+');
            tmp = tmp->next;
        }
}

static void ReadTillNewLine() {
    char c = getchar();
    while (c != '\n')
        c = getchar();
}

static void InterpreteCommand(PolyStack *s, int r, char *command) {
    Poly p1, p2;
    char c;
    bool errOccured = false;
    int mockCol = 0;

    if (strcmp(command, "ZERO") == 0) {
        Push(PolyZero(), s);
    }
    else if (strcmp(command, "IS_COEFF") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Top(s);
            printf("%d\n", PolyIsCoeff(&p1));
        }
    }
    else if (strcmp(command, "IS_ZERO") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Top(s);
            printf("%d\n", PolyIsZero(&p1));
        }
    }
    else if (strcmp(command, "CLONE") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            Poly p = Top(s);
            Push(PolyClone(&p), s);
        }
    }
    else if (strcmp(command, "ADD") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Pop(s);
            if (IsEmpty(s)) {
                Push(p1, s);
                fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
            }
            else {
                p2 = Pop(s);
                Push(PolyAdd(&p1, &p2), s);
                PolyDestroy(&p1);
                PolyDestroy(&p2);
            }
        }
    }
    else if (strcmp(command, "MUL") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Pop(s);
            if (IsEmpty(s)) {
                Push(p1, s);
                fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
            }
            else {
                p2 = Pop(s);
                Push(PolyMul(&p1, &p2), s);
                PolyDestroy(&p1);
                PolyDestroy(&p2);
            }
        }
    }
    else if (strcmp(command, "NEG") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Pop(s);
            Push(PolyNeg(&p1), s);
            PolyDestroy(&p1);
        }
    }
    else if (strcmp(command, "SUB") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Pop(s);
            if (IsEmpty(s)) {
                Push(p1, s);
                fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
            }
            else {
                p2 = Pop(s);
                Push(PolySub(&p1, &p2), s);
                PolyDestroy(&p1);
                PolyDestroy(&p2);
            }
        }

    }
    else if (strcmp(command, "IS_EQ") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Pop(s);
            if (IsEmpty(s)) {
                Push(p1, s);
                fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
            }
            else {
                p2 = Top(s);
                printf("%d\n", PolyIsEq(&p1, &p2));
                Push(p1, s);
            }
        }
    }
    else if (strcmp(command, "DEG") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Top(s);
            printf("%d\n", PolyDeg(&p1));
        }
    }
    else if (strcmp(command, "DEG_BY") == 0) {
        poly_coeff_t cf = ReadNumber(0, UINT_MAX, &mockCol, &errOccured);
        c = getchar();
        if (errOccured || c != '\n') {
            fprintf(stderr, "ERROR %d WRONG VARIABLE\n", r);
            if (c != '\n')
                ReadTillNewLine();
        }
        else {
            if (IsEmpty(s)) {
                fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
            }
            else {
                p1 = Top(s);
                printf("%d\n", PolyDegBy(&p1, (poly_exp_t) cf));
            }
        }
    }
    else if (strcmp(command, "AT") == 0) {
        poly_coeff_t cf = ReadNumber(POLY_COEFF_MIN, POLY_COEFF_MAX,
            &mockCol, &errOccured);
        c = getchar();
        if (errOccured || c != '\n') {
            fprintf(stderr, "ERROR %d WRONG VALUE\n", r);
            if (c != '\n')
                ReadTillNewLine();
        }
        else {
            if (IsEmpty(s)) {
                fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
            }
            else {
                p1 = Pop(s);
                p2 = PolyAt(&p1, cf);
                Push(p2, s);
                PolyDestroy(&p1);
            }
        }
    }
    else if (strcmp(command, "PRINT") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Top(s);
            PolyPrint(&p1);
            printf("\n");
        }
    }
    else if (strcmp(command, "POP") == 0) {
        if (IsEmpty(s)) {
            fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
        }
        else {
            p1 = Pop(s);
            PolyDestroy(&p1);
        }
    }
    else if (strcmp(command, "COMPOSE") == 0) {
        unsigned count = ReadNumber(0, UINT_MAX, &mockCol, &errOccured);
        Poly res;
        c = getchar();
        if (errOccured || c != '\n') {
            fprintf(stderr, "ERROR %d WRONG COUNT\n", r);
            if (c != '\n')
                ReadTillNewLine();
        }
        else {
            if (count > MAX_STACK_SIZE) {
                fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
                return ;
            }
            Poly arr[count];
            if (!IsEmpty(s)) {
                p1 = Pop(s);
            }
            else {
                fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
                return ;
            }
            for (unsigned i = 0; i < count; i++) {
                if (!IsEmpty(s)) {
                    arr[i] = Pop(s);
                }
                else {
                    fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", r);
                    for (long j = i - 1; j >= 0; j--)
                        Push(arr[j], s);
                    Push(p1, s);
                    free(arr);
                    return ;
                }
            }
            res = PolyCompose(&p1, count, arr);
            for(unsigned j = 0; j < count; j++)
                PolyDestroy(&arr[j]);
            free(arr);
            PolyDestroy(&p1);
            Push(res, s);
        }
    } /* COMPOSE */
    else {
        fprintf(stderr, "ERROR %d WRONG COMMAND\n", r);
    }
}

static void ReadCommand(PolyStack *s, int r) {
    char c, command[MAX_COMM_LEN];
    bool err;
    int it = 0;
    do {
        c = getchar();
        if (ProperLetter(c) || c == '_')
            command[it++] = c;
    } while (it < MAX_COMM_LEN - 1 && (ProperLetter(c) || c == '_'));
    command[it] = '\0';
    bool cmdWithArg = strcmp(command, "AT") == 0
                    || strcmp(command, "DEG_BY") == 0
                    || strcmp(command, "COMPOSE") == 0;
    err = cmdWithArg ? c != ' ' : c != '\n';
    if (err) {
        if (c != '\n')
            ReadTillNewLine();
        fprintf(stderr, cmdWithArg ? "ERROR %d WRONG COUNT\n"
                    : "ERROR %d WRONG COMMAND\n", r);
        return ;
    }
    InterpreteCommand(s, r, command);
}

static inline bool MonoIsZero(Mono m) {
    return PolyIsZero(&(m.p));
}

static Poly ReadPoly(int r, int *col, bool *errOccured);

static Mono ReadMono(int r, int *col, bool *errOccured) {
    Mono m = (Mono) {.p = PolyZero(), .exp = -1};
    char c = getchar();
    ++*col;
    if (c != '(') {
        if (!*errOccured)
            fprintf(stderr, "ERROR %d %d\n", r, *col);
        ungetc(c, stdin);
        --*col;
        *errOccured = true;
        return m;
    }
    m.p = ReadPoly(r, col, errOccured);
    if (*errOccured)
        return m;
    c = getchar();
    ++*col;
    if (c != ',') {
        if (!*errOccured)
            fprintf(stderr, "ERROR %d %d\n", r, *col);
        ungetc(c, stdin);
        --*col;
        *errOccured = true;
        return m;
    }
    m.exp = (poly_exp_t) ReadNumber(0, INT_MAX, col, errOccured);
    if (*errOccured) {
        fprintf(stderr, "ERROR %d %d\n", r, *col + 1);
        return m;
    }
    c = getchar();
    ++*col;
    if (c != ')') {
        if (!*errOccured)
            fprintf(stderr, "ERROR %d %d\n", r, *col);
        ungetc(c, stdin);
        --*col;
        *errOccured = true;
    }
    return m;
}

static Poly ReadPoly(int r, int *col, bool *errOccured) {
    Poly p;
    Mono mTmp;
    unsigned size = MONOS_ARR_INIT_SIZE, count = 0;
    Mono *monos = malloc(size * sizeof(Mono));
    assert(monos != NULL);
    char c = getchar();
    ungetc(c, stdin);
    if (ProperDigit(c) || c == '-') {
        poly_coeff_t cf = ReadNumber(POLY_COEFF_MIN, POLY_COEFF_MAX,
                                     col, errOccured);
        if (*errOccured) //poza zakresem
            fprintf(stderr, "ERROR %d %d\n", r, *col + 1);
        p = PolyFromCoeff(cf);
        free(monos);
        return p;
    }
    else {
        mTmp = ReadMono(r, col, errOccured);
        if (*errOccured) {
            free(monos);
            PolyDestroy(&mTmp.p);
            return PolyZero();
        }
        monos[count++] = mTmp;
        while (!*errOccured) {
            c = getchar();
            ++*col;
            if (c != '+')
                break;
            monos[count++] = ReadMono(r, col, errOccured);
            if (*errOccured) {
                for(unsigned i = 0; i < count; i++)
                    PolyDestroy(&monos[i].p);
                free(monos);
                return PolyZero();
            }
            if (count == size - 1) {
                size *= 2;
                monos = realloc(monos, size * sizeof(Mono));
                assert(monos != NULL);
            }
        } /* while */
        if (!*errOccured) {
            assert(c != '+');
            ungetc(c, stdin);
            --*col;
        }
    }
    monos = realloc(monos, count * sizeof(Mono));
    if (count == 0) {
        assert(monos == NULL);
        return PolyZero();
    }
    p = PolyAddMonos(count, monos);
    free(monos);
    return p;
}

static void ReadPolyLine(PolyStack *s, int r) {
    Poly p;
    bool errOccured = false;
    int col = 0;
    char c;
    p = ReadPoly(r, &col, &errOccured);
    if (errOccured) {
        PolyDestroy(&p);
        ReadTillNewLine();
        return ;
    }
    else {
        c = getchar();
        col++;
        if (c == '\n') {
            Push(p, s);
        }
        else {
            PolyDestroy(&p);
            fprintf(stderr, "ERROR %d %d\n", r, col);
            ReadTillNewLine();
        }
    }
}

void Read(PolyStack *s) {
    char c;
    int r = 1;
    while (1) {
        c = getchar();
        if (c == EOF)
            return ;
        ungetc(c, stdin);
        if (ProperLetter(c))
            ReadCommand(s, r);
        else
            ReadPolyLine(s, r);
        r++;
    }
}

void CleanStack(PolyStack *s) {
    Poly p;
    while (!IsEmpty(s)) {
        p = Pop(s);
        PolyDestroy(&p);
    }
    free(s);
}

int main() {
    PolyStack *s = Init();
    Read(s);
    CleanStack(s);
    return 0;
}
