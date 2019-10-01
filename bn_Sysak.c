#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bn.h"

enum bn_codes {
    BN_OK, BN_NULL_OBJECT, BN_NO_MEMORY, BN_DIVIDE_BY_ZERO
};

struct bn_s {
    int *body;
    int  bodysize;
    int  sign;
};



void bn_print(bn *t) {
    if (t == NULL || t->body == NULL) {
        printf("NULL POINTER");
        return;
    }
    if (t->sign == -1) {
        printf("- ");
    } else if (t->sign == +1) {
        printf("+ ");
    } else {
        printf("  ");
    }
    int i;
    for (i = t->bodysize - 1; i>=0; i--) {
        printf("%d ", t->body[i]);
        //if (i % 3 == 0) printf(" ");
    }
    printf("(size: %d)\n", t->bodysize);
    return;
}

int bn_first_zeros(bn *t) {
    int i = t->bodysize - 1;
    while (i >= 0 && t->body[i] == 0) {
        i--;
    }
    if (i == t->bodysize - 1) return BN_OK;
    if (i < 0) {
        free(t->body);
        t->sign = 0;
        t->bodysize = 1;
        t->body = (int *)malloc(t->bodysize * sizeof(int));
        if (t->body == NULL) return BN_NO_MEMORY;
        t->body[0] = 0;
        return BN_OK;
    }
    t->bodysize = i + 1;
    int *r = (int *)malloc(t->bodysize * sizeof(int));
    if (r == NULL) return BN_NO_MEMORY;
    int j;
    for (j = 0; j < t->bodysize; j++) {
        r[j] = t->body[j];
    }
    free(t->body);
    t->body = r;
    return BN_OK;
}

bn *bn_add_same_sign(bn const *left, bn const *right) {
    bn *ret = (bn *)malloc(sizeof(bn));
    if (ret == NULL) return NULL;
    int last = 0;
    if (left->bodysize > right->bodysize) {
        ret->bodysize = left->bodysize;
        if (left->body[left->bodysize - 1] == 9)
            last = 1;
    } else if (left->bodysize < right->bodysize) {
        ret->bodysize = right->bodysize;
        if (right->body[right->bodysize - 1] == 9)
            last = 1;
    } else {
        ret->bodysize = left->bodysize;
        if (right->body[right->bodysize - 1] + left->body[left->bodysize - 1] >= 9)
            last = 1;
    }
    if (last)
        ret->bodysize++;
    ret->sign = left->sign;
    ret->body = (int *)malloc(ret->bodysize * sizeof(int));
    if (ret->body == NULL) {
        free(ret);
        return NULL;
    }
    int i, next = 0;
    for (i = 0; i < ret->bodysize - last; i++) {
        if (i < left->bodysize)
            next += left->body[i];
        if (i < right->bodysize)
            next += right->body[i];
        ret->body[i] = next % 10;
        next /= 10;
    }
    if (last) {
        ret->body[ret->bodysize - 1] = next;
        if (last && (next == 0)) {
            if (bn_first_zeros(ret)) {
                bn_delete(ret);
                return NULL;
            }
        }
    }
    return ret;
}

bn *bn_add_diff_sign(bn const *left, bn const *right) {
    bn *ret = (bn *)malloc(sizeof(bn));
    if (ret == NULL) return NULL;
    int i, left_bigger = 0;
    if (left->bodysize > right->bodysize) {
        ret->bodysize = left->bodysize;
        left_bigger = 1;
    } else if (left->bodysize < right->bodysize) {
        ret->bodysize = right->bodysize;
        left_bigger = 0;
    } else {
        ret->bodysize = left->bodysize;
        for (i = left->bodysize - 1; i >= 0; i--) {
            if (left->body[i] > right->body[i]) {
                left_bigger = 1;
                break;
            }
            if (left->body[i] < right->body[i]) {
                left_bigger = 0;
                break;
            }
        }
        if (i == -1) {
            ret->sign = 0;
            ret->bodysize = 1;
            ret->body = (int *)malloc(sizeof(int));
            if (ret->body == NULL) {
                free(ret);
                return NULL;
            }
            ret->body[0] = 0;
            return ret;
        }
    }
    bn const *big = left_bigger?left:right;
    bn const *small = left_bigger?right:left;
    ret->sign = big->sign;
    ret->body = (int *)calloc(ret->bodysize, sizeof(int));
    if (ret->body == NULL) {
        free(ret);
        return NULL;
    }
    for (i = 0; i < ret->bodysize; i++) {
        ret->body[i] += big->body[i];
        if (i < small->bodysize)
            ret->body[i] -= small->body[i];
        if (ret->body[i] < 0) {
            ret->body[i] += 10;
            ret->body[i + 1]--;
        }
    }
    if (ret->body[ret->bodysize - 1] == 0) {
        if (bn_first_zeros(ret)) {
            bn_delete(ret);
            return NULL;
        }
    }
    return ret;
}

bn *bn_new() {
    bn *r = (bn *) malloc (sizeof(bn));
    if (r == NULL) return NULL;
    r->bodysize = 1;
    r->sign = 0;
    r->body = (int *)malloc(r->bodysize * sizeof(int));
    if (r->body == NULL) {
        free(r);
        return NULL;
    }
    r->body[0] = 0;
    return r;
}

bn *bn_init(bn const *orig) {
    if (orig == NULL) return NULL;
    bn *r = (bn *) malloc (sizeof(bn));
    if (r == NULL) return NULL;
    r->bodysize = orig->bodysize;
    r->sign = orig->sign;
    r->body = (int *)malloc(r->bodysize * sizeof(int));
    if (r->body == NULL) {
        free(r);
        return NULL;
    }
    int i;
    for (i = 0; i < r->bodysize; i++) {
        r->body[i] = orig->body[i];
    }
    return r;
}

int bn_init_string(bn *t, const char *init_string) {
    if (t == NULL || init_string == NULL) return BN_NULL_OBJECT;
    if (t->body != NULL) {
        free(t->body);
        t->body = NULL;
    }
    int start;
    if (init_string[0] == '-') {
        t->sign = -1;
        start = 1;
    } else {
        t->sign = 1;
        start = 0;
    }
    while (start < strlen(init_string) && init_string[start] == '0') {
        start++;
    }
    if (start == strlen(init_string)) {
        t->bodysize = 1;
        t->sign = 0;
        t->body = (int *)malloc(t->bodysize * sizeof(int));
        if (t->body == NULL) return BN_NO_MEMORY;
        t->body[0] = 0;
        return BN_OK;
    }
    t->bodysize = strlen(init_string) - start;
    t->body = (int *)malloc(t->bodysize * sizeof(int));
    if (t->body == NULL) return BN_NO_MEMORY;
    int i, j = 0;
    for (i = strlen(init_string) - 1; i >= start; i--) {
        t->body[j] = init_string[i] - '0';
        j++;
    }
    return BN_OK;
}

int bn_init_string_radix(bn *t, const char *init_string, int radix) {
    if (t == NULL || init_string == NULL) return BN_NULL_OBJECT;
    if (t->body != NULL) {
        free(t->body);
        t->body = NULL;
    }
    int start;
    if (init_string[0] == '-') {
        start = 1;
    } else {
        start = 0;
    }
    while (start < strlen(init_string) && init_string[start] == '0') {
        start++;
    }
    t->bodysize = 1;
    t->sign = 0;
    t->body = (int *)calloc(t->bodysize, sizeof(int));
    if (t->body == NULL) return BN_NO_MEMORY;
    t->body[0] = 0;
    if (start == strlen(init_string)) {
        return BN_OK;
    }
    bn *rad = (bn *)malloc(sizeof(bn));
    if (rad == NULL) {
        return BN_NO_MEMORY;
    }
    rad->sign = 1;
    rad->bodysize = 1 + (radix >= 10);
    rad->body = (int *)malloc(rad->bodysize * sizeof(int));
    if (rad->body == NULL) {
        bn_delete(rad);
        return BN_NO_MEMORY;
    }
    if (radix >= 10) {
        rad->body[0] = radix % 10;
        rad->body[1] = radix / 10;
    } else {
        rad->body[0] = radix;
    }
    int i, digit;
    for (i = start; i < strlen(init_string); i++) {
        int code = bn_mul_to(t, rad);
        if (code) {
            bn_delete(rad);
            return BN_NO_MEMORY;
        };
        if (init_string[i] >= 'A' && init_string[i] <= 'Z') {
            digit = init_string[i] - 'A' + 10;
            bn *addup = bn_new();
            if (addup == NULL) {
                bn_delete(rad);
                return BN_NO_MEMORY;
            }
            if (bn_init_int(addup, digit) || bn_add_to(t, addup)) {
                bn_delete(addup);
                bn_delete(rad);
                return BN_NO_MEMORY;
            }
            bn_delete(addup);
        } else {
            digit =  init_string[i] - '0';
            bn *addup = bn_new();
            if (addup == NULL) {
                bn_delete(rad);
                return BN_NO_MEMORY;
            }
            if (bn_init_int(addup, digit) || bn_add_to(t, addup)) {
                bn_delete(addup);
                bn_delete(rad);
                return BN_NO_MEMORY;
            }
            bn_delete(addup);
        }
    }
    if (init_string[0] == '-') {
        t->sign = -1;
    }
    bn_delete(rad);
    return BN_OK;
}

int bn_init_int(bn *t, int init_int) {
    if (t == NULL) return BN_NULL_OBJECT;
    if (t->body != NULL) {
        free(t->body);
        t->body = NULL;
    }
    if (init_int == 0) {
        t->bodysize = 1;
        t->sign = 0;
        t->body = (int *)malloc(t->bodysize * sizeof(int));
        if (t->body == NULL) return BN_NO_MEMORY;
        t->body[0] = 0;
        return BN_OK;
    }
    if (init_int < 0) {
        t->sign = -1;
        init_int = 0 - init_int;
    } else {
        t->sign = 1;
    }
    int degree = 1, s = 0;
    while (degree <= init_int) {
        degree *= 10;
        s++;
    }
    t->bodysize = s;
    t->body = (int *)malloc(t->bodysize * sizeof(int));
    if (t->body == NULL) return BN_NO_MEMORY;
    int i = 0;
    while (init_int > 0) {
        t->body[i] = init_int % 10;
        init_int /= 10;
        i++;
    }
    return BN_OK;
}

int bn_delete(bn *t) {
    if (t == NULL) return;
    free(t->body);
    free(t);
    return BN_OK;
}

int bn_add_to(bn *t, bn const *right) {
    if (t == NULL || t->body == NULL || right == NULL || right->body == NULL) return BN_NULL_OBJECT;
    bn *ret;
    ret = bn_add(t, right);
    if (ret == NULL) {
        return BN_NO_MEMORY;
    }
    free(t->body);
    t->sign = ret->sign;
    t->bodysize = ret->bodysize;
    t->body = (int *)malloc(t->bodysize * sizeof(int));
    if (t->body == NULL) {
        bn_delete(ret);
        return BN_NO_MEMORY;
    }
    int i;
    for (i = 0; i < t->bodysize; i++) {
        t->body[i] = ret->body[i];
    }
    bn_delete(ret);
    return BN_OK;
}

int bn_sub_to(bn *t, bn const *right) {
    if (t == NULL || t->body == NULL || right == NULL || right->body == NULL) return BN_NULL_OBJECT;
    bn *ret;
    ret = bn_sub(t, right);
    if (ret == NULL) {
        return BN_NO_MEMORY;
    }
    free(t->body);
    t->sign = ret->sign;
    t->bodysize = ret->bodysize;
    t->body = (int *)malloc(t->bodysize * sizeof(int));
    if (t->body == NULL) {
        bn_delete(ret);
        return BN_NO_MEMORY;
    }
    int i;
    for (i = 0; i < t->bodysize; i++) {
        t->body[i] = ret->body[i];
    }
    bn_delete(ret);
    return BN_OK;
}

int bn_mul_to(bn *t, bn const *right) {
    if (t == NULL || t->body == NULL || right == NULL || right->body == NULL) return BN_NULL_OBJECT;
    bn *ret;
    ret = bn_mul(t, right);
    if (ret == NULL) {
        return BN_NO_MEMORY;
    }
    free(t->body);
    t->sign = ret->sign;
    t->bodysize = ret->bodysize;
    t->body = (int *)malloc(t->bodysize * sizeof(int));
    if (t->body == NULL) {
        bn_delete(ret);
        return BN_NO_MEMORY;
    }
    int i;
    for (i = 0; i < t->bodysize; i++) {
        t->body[i] = ret->body[i];
    }
    bn_delete(ret);
    return BN_OK;
}

int bn_div_to(bn *t, bn const *right) {
    if (t == NULL || t->body == NULL || right == NULL || right->body == NULL) return BN_NULL_OBJECT;
    if (right->sign == 0) return BN_DIVIDE_BY_ZERO;
    bn *ret;
    ret = bn_div(t, right);
    if (ret == NULL) {
        return BN_NO_MEMORY;
    }
    free(t->body);
    t->sign = ret->sign;
    t->bodysize = ret->bodysize;
    t->body = (int *)malloc(t->bodysize * sizeof(int));
    if (t->body == NULL) {
        bn_delete(ret);
        return BN_NO_MEMORY;
    }
    int i;
    for (i = 0; i < t->bodysize; i++) {
        t->body[i] = ret->body[i];
    }
    bn_delete(ret);
    return BN_OK;
}

int bn_mod_to(bn *t, bn const *right) {
    if (t == NULL || t->body == NULL || right == NULL || right->body == NULL) return BN_NULL_OBJECT;
    if (right->sign == 0) return BN_DIVIDE_BY_ZERO;
    bn *ret;
    ret = bn_mod(t, right);
    if (ret == NULL) {
        return BN_NO_MEMORY;
    }
    free(t->body);
    t->sign = ret->sign;
    t->bodysize = ret->bodysize;
    t->body = (int *)malloc(t->bodysize * sizeof(int));
    if (t->body == NULL) {
        bn_delete(ret);
        return BN_NO_MEMORY;
    }
    int i;
    for (i = 0; i < t->bodysize; i++) {
        t->body[i] = ret->body[i];
    }
    bn_delete(ret);
    return BN_OK;
}

int bn_pow_to(bn *t, int degr) {
    if (t == NULL || t->body == NULL) return BN_NULL_OBJECT;
    int degree = degr;
    if (degree == 0) {
        t->bodysize = 1;
        t->sign = 1;
        free(t->body);
        t->body = (int *)malloc(t->bodysize * sizeof(int));
        if (t->body == NULL) {
            return BN_NO_MEMORY;
        }
        t->body[0] = 1;
        return BN_OK;
    }
    bn *orig = bn_init(t);
    if (orig == NULL) {
        return BN_NO_MEMORY;
    }
    int amount = 0, k = 1;
    while (k < degree) {
        k *= 2;
        amount++;
    }
    amount *= 2;
    int *action = (int *)calloc(amount, sizeof(int));
    if (action == NULL) {
        bn_delete(orig);
        return BN_NO_MEMORY;
    }
    int i = 0;
    while (degree > 1) {
        if (degree % 2 == 0) {
            action[i] = 1;
            degree /= 2;
        } else {
            degree--;
        }
        i++;
    }
    int j;
    k = 0;
    for (j = i - 1; j >= 0; j--) {
        if (action[j]) {
            k = bn_mul_to(t, t);
        } else {
            k = bn_mul_to(t, orig);
        }
        if (k) {
            bn_delete(orig);
            return BN_NO_MEMORY;
        }
    }
    free(action);
    bn_delete(orig);
    return BN_OK;
}

int bn_root_to(bn *t, int reciprocal) {
    if (t == NULL || t->body == NULL) return BN_NULL_OBJECT;
    bn *ret;
    ret = (bn *)malloc(sizeof(bn));
    if (ret == NULL) {
        return BN_NO_MEMORY;
    }
    ret->bodysize = (t->bodysize - 1) / reciprocal + 1;
    ret->sign = 1;
    ret->body = (int *)calloc(ret->bodysize, sizeof(int));
    if (ret->body == NULL) {
        bn_delete(ret);
        return BN_NO_MEMORY;
    }
    int i;
    for (i = ret->bodysize - 1; i >= 0; i--) {
        int l = 0, r = 9;
        int digit = 0;
        while (l <= r) {
            int m = (l + r) / 2;
            ret->body[i] = m;
            bn *retret = bn_init(ret);
            if (retret == NULL || bn_pow_to(retret, reciprocal)) {
                bn_delete(ret);
                bn_delete(retret);
                return BN_NO_MEMORY;
            }
            if (bn_cmp(t, retret) >= 0) {
                digit = m;
                l = m + 1;
            } else {
                r = m - 1;
            }
            bn_delete(retret);
        }
        ret->body[i] = digit;
    }
    free(t->body);
    t->sign = ret->sign;
    t->bodysize = ret->bodysize;
    t->body = (int *)malloc(t->bodysize * sizeof(int));
    if (t->body == NULL) {
        bn_delete(ret);
        return BN_NO_MEMORY;
    }
    for (i = 0; i < t->bodysize; i++) {
        t->body[i] = ret->body[i];
    }
    bn_delete(ret);
    return BN_OK;
}

bn* bn_add(bn const *left, bn const *right) {
    if (left == NULL || left->body == NULL || right == NULL || right->body == NULL) return NULL;
    bn *ret;
    if (left->sign == right->sign) {
        ret = bn_add_same_sign(left, right);
    } else {
        ret = bn_add_diff_sign(left, right);
    }
    return ret;
}

bn* bn_sub(bn const *left, bn const *right) {
    if (left == NULL || left->body == NULL || right == NULL || right->body == NULL) return NULL;
    bn *ret;
    bn *minusright = bn_init(right);
    if (minusright == NULL) {
        return NULL;
    }
    minusright->sign *= -1;
    ret = bn_add(left, minusright);
    bn_delete(minusright);
    return ret;
}

bn* bn_mul(bn const *left, bn const *right) {
    if (left == NULL || left->body == NULL || right == NULL || right->body == NULL) return NULL;
    bn *ret = (bn *)malloc(sizeof(bn));
    ret->sign = left->sign * right->sign;
    if (ret->sign == 0) {
        ret->sign = 0;
        ret->bodysize = 1;
        ret->body = (int *)malloc(ret->bodysize * sizeof(int));
        if (ret->body == NULL) {
            free(ret);
            return NULL;
        }
        ret->body[0] = 0;
        return ret;
    }
    ret->bodysize = left->bodysize + right->bodysize;
    ret->body = (int *)calloc(ret->bodysize, sizeof(int));
    int i, j;
    for (i = 0; i < left->bodysize; i++) {
        for (j = 0; j < right->bodysize; j++) {
            ret->body[i+j] += left->body[i] * right->body[j];
            if (ret->body[i + j] > 9) {
                ret->body[i + j + 1] += ret->body[i + j] / 10;
                ret->body[i + j] %= 10;
            }
        }
    }
    if (ret->body[ret->bodysize - 1] == 0 && bn_first_zeros(ret)) {
        bn_delete(ret);
        return NULL;
    }
    return ret;
}

bn* bn_div(bn const *l, bn const *r) {
    if (l == NULL || l->body == NULL || r == NULL || r->body == NULL) return NULL;
    if (r->sign == 0) return NULL;
    bn *ret = (bn *)malloc(sizeof(bn));
    if (ret == NULL) {
        return NULL;
    }
    ret->bodysize = l->bodysize >= r->bodysize ? l->bodysize + 1 - r->bodysize : 1;
    ret->body = (int *)calloc(ret->bodysize, sizeof(int));
    if (ret->body == NULL) {
        bn_delete(ret);
        return NULL;
    }
    bn *left = bn_init(l);
    if (left == NULL) {
        bn_delete(ret);
        return NULL;
    }
    bn_abs(left);
    bn *right = bn_new();
    if (right == NULL) {
        bn_delete(ret);
        bn_delete(right);
        return NULL;
    }
    right->sign = 1;
    int i, j;
    for (i = ret->bodysize - 1; i >= 0; i--) {
        free(right->body);
        right->bodysize = i + r->bodysize;
        right->body = (int *)calloc(right->bodysize, sizeof(int));
        if (right->body == NULL) {
            bn_delete(ret);
            bn_delete(left);
            bn_delete(right);
            return NULL;
        }
        for (j = 0; j < r->bodysize; j++) {
            right->body[i + j] = r->body[j];
        }
        while (bn_cmp(left, right) >= 0) {
            ret->body[i]++;
            if (bn_sub_to(left, right)) {
                bn_delete(ret);
                bn_delete(left);
                bn_delete(right);
                return NULL;
            };
        }
    }
    ret->sign = 1;
    if ((left->sign != 0) && (l->sign * r->sign < 0)) {
        bn *one = bn_new();
        if (one == NULL) {
            bn_delete(ret);
            bn_delete(left);
            bn_delete(right);
            return NULL;
        }
        one->body[0] = 1;
        one->sign = 1;
        if (bn_add_to(ret, one)) {
            bn_delete(one);
            bn_delete(ret);
            bn_delete(left);
            bn_delete(right);
            return NULL;
        };
        bn_delete(one);
    }
    ret->sign = l->sign * r->sign;
    bn_delete(right);
    bn_delete(left);
    if (ret->body[ret->bodysize - 1] == 0 && bn_first_zeros(ret)) {
        bn_delete(ret);
        return NULL;
    }
    return ret;
}

bn* bn_mod(bn const *l, bn const *r) {
    if (l == NULL || l->body == NULL || r == NULL || r->body == NULL) return NULL;
    if (r->sign == 0) return NULL;
    bn *ret = (bn *)malloc(sizeof(bn));
    if (ret == NULL) {
        return NULL;
    }
    ret->bodysize = l->bodysize >= r->bodysize ? l->bodysize + 1 - r->bodysize : 1;
    ret->body = (int *)calloc(ret->bodysize, sizeof(int));
    if (ret->body == NULL) {
        bn_delete(ret);
        return NULL;
    }
    bn *left = bn_init(l);
    if (left == NULL) {
        bn_delete(ret);
        return NULL;
    }
    bn_abs(left);
    bn *right = bn_new();
    if (right == NULL) {
        bn_delete(ret);
        bn_delete(right);
        return NULL;
    }
    right->sign = 1;
    int i, j;
    for (i = ret->bodysize - 1; i >= 0; i--) {
        free(right->body);
        right->bodysize = i + r->bodysize;
        right->body = (int *)calloc(right->bodysize, sizeof(int));
        if (right->body == NULL) {
            bn_delete(ret);
            bn_delete(left);
            bn_delete(right);
            return NULL;
        }
        for (j = 0; j < r->bodysize; j++) {
            right->body[i + j] = r->body[j];
        }
        while (bn_cmp(left, right) >= 0) {
            ret->body[i]++;
            if (bn_sub_to(left, right)) {
                bn_delete(ret);
                bn_delete(left);
                bn_delete(right);
                return NULL;
            };
        }
    }
    if (left->sign == 0) {
    } else if (l->sign * r->sign >= 0) {
        left->sign = r->sign;
    } else {
        if (bn_sub_to(left, right)) {
            bn_delete(ret);
            bn_delete(left);
            bn_delete(right);
            return NULL;
        };
        left->sign = r->sign;
    }
    bn_delete(right);
    bn_delete(ret);
    if (left->body[left->bodysize - 1] == 0 && bn_first_zeros(left)) {
        bn_delete(left);
        return NULL;
    }
    return left;
}

bn* bn_mod_divto(bn *l, bn *r) {
    bn *ret = (bn *)malloc(sizeof(bn));
    if (ret == NULL) {
        return NULL;
    }
    ret->bodysize = l->bodysize >= r->bodysize ? l->bodysize + 1 - r->bodysize : 1;
    ret->body = (int *)calloc(ret->bodysize, sizeof(int));
    if (ret->body == NULL) {
        bn_delete(ret);
        return NULL;
    }
    bn *left = bn_init(l);
    if (left == NULL) {
        bn_delete(ret);
        return NULL;
    }
    bn_abs(left);
    bn *right = bn_new();
    if (right == NULL) {
        bn_delete(ret);
        bn_delete(right);
        return NULL;
    }
    right->sign = 1;
    int i, j;
    for (i = ret->bodysize - 1; i >= 0; i--) {
        free(right->body);
        right->bodysize = i + r->bodysize;
        right->body = (int *)calloc(right->bodysize, sizeof(int));
        if (right->body == NULL) {
            bn_delete(ret);
            bn_delete(left);
            bn_delete(right);
            return NULL;
        }
        for (j = 0; j < r->bodysize; j++) {
            right->body[i + j] = r->body[j];
        }
        while (bn_cmp(left, right) >= 0) {
            ret->body[i]++;
            if (bn_sub_to(left, right)) {
                bn_delete(ret);
                bn_delete(left);
                bn_delete(right);
                return NULL;
            };
        }
    }
    if (left->sign == 0) {
    } else if (l->sign * r->sign >= 0) {
        left->sign = r->sign;
    } else {
        if (bn_sub_to(left, right)) {
            bn_delete(ret);
            bn_delete(left);
            bn_delete(right);
            return NULL;
        };
        left->sign = r->sign;
    }
    if (left->body[left->bodysize - 1] == 0 && bn_first_zeros(left)) {
        bn_delete(left);
        bn_delete(ret);
        return NULL;
    }
    bn_first_zeros(ret);
    free(l->body);
    l->bodysize = ret->bodysize;
    l->body = (int *)malloc(l->bodysize * sizeof(int));
    for (i = 0; i < l->bodysize; i++) {
        l->body[i] = ret->body[i];
    }
    if (l->bodysize == 1 && l->body[0] == 0) {
        l->sign = 0;
    } else {
        l->sign = 1;
    }
    bn_delete(right);
    bn_delete(ret);
    return left;
}

char *bn_to_string_ten(bn const *t) {
    int start = 0;
    if (t->sign == -1) start++;
    char *ret = (char *)calloc(t->bodysize + 1 + start, sizeof(char));
    if (ret == NULL) return NULL;
    if (start) ret[0] = '-';
    int i;
    for (i = start; i < t->bodysize + start ; i++) {
        ret[i] = '0' + t->body[t->bodysize - 1 + start - i];
    }
    return ret;
}

char *bn_to_string_radix(bn const *t, int radix) {
    if (t->sign == 0) {
        char *ret = (char *)calloc(2, sizeof(char));
        ret[0] = '0';
        return ret;
    }
    int size = t->bodysize;
    if (radix >= 10) {
        size = t->bodysize;
    } else if (radix >= 4) {
        size = t->bodysize * 2;
    } else if (radix == 3) {
        size = t->bodysize * 3;
    } else if (radix == 2) {
        size = t->bodysize * 4;
    }
    char *ret = (char *)calloc(size + 2, sizeof(char));
    int i;
    for (i = 0; i < size + 1; i++) {
        ret[i] = '0';
    }
    bn *rad = (bn *)malloc(sizeof(bn));
    if (rad == NULL) {
        return NULL;
    }
    rad->sign = 1;
    rad->bodysize = 1 + (radix >= 10);
    rad->body = (int *)malloc(rad->bodysize * sizeof(int));
    if (rad->body == NULL) {
        bn_delete(rad);
        return NULL;
    }
    if (radix >= 10) {
        rad->body[0] = radix % 10;
        rad->body[1] = radix / 10;
    } else {
        rad->body[0] = radix;
    }
    bn *orig = bn_init(t);
    orig->sign = 1;
    i = strlen(ret) - 1;
    while (orig->sign > 0) {
        bn *mod = bn_mod_divto(orig, rad);
        if (mod->bodysize > 1) {
            ret[i] = 'A' - 10 + 10 * mod->body[1] + mod->body[0];
        } else {
            ret[i] = '0' + mod->body[0];
        }
        bn_delete(mod);
        i--;
    }
    bn_delete(orig);
    bn_delete(rad);
    for (i = 0; i < strlen(ret); i++) {
        if (ret[i] != '0') break;
    }
    size = strlen(ret) - i + (t->sign == -1);
    char *r = (char *)calloc(size + 1, sizeof(char));
    int j;
    for (j = i; j < strlen(ret); j++) {
        r[j - i + (t->sign == -1)] = ret[j];
    }
    if (t->sign == -1) {
        r[0] = '-';
    }
    free(ret);
    return r;
}

const char *bn_to_string(bn const *t, int radix) {
    if (t == NULL || t->body == NULL) return NULL;
    char *ret;
    if (radix == 10) {
        ret = bn_to_string_ten(t);
    } else {
        ret = bn_to_string_radix(t, radix);
    }
    return ret;
}

int bn_cmp(bn const *left, bn const *right) {
    if (left->sign < right->sign) return -1;
    if (left->sign > right->sign) return 1;
    if (left->bodysize > right->bodysize) return left->sign;
    if (left->bodysize < right->bodysize) return -left->sign;
    int i;
    for (i = left->bodysize - 1; i >= 0; i--) {
        if (left->body[i] > right->body[i]) return left->sign;
        if (left->body[i] < right->body[i]) return -left->sign;
    }
    return 0;
}

int bn_neg(bn *t) {
    if (t == NULL) return BN_NULL_OBJECT;
    t->sign *= -1;
    return BN_OK;
}

int bn_abs(bn *t) {
    if (t == NULL) return BN_NULL_OBJECT;
    t->sign = t->sign * t->sign;
    return BN_OK;
}

int bn_sign(bn const *t) {
    if (t == NULL) return BN_NULL_OBJECT;
    return t->sign;
}

bn* bn_factorial(int orig) {
    if (orig < 1) return NULL;
    bn *one = bn_new();
    one->body[0] = 1;
    one->sign = 1;
    bn *ret = bn_init(one);
    bn *multiplicator = bn_init(one);
    int i;
    for (i = 0; i < orig; i++) {
        bn_mul_to(ret, multiplicator);
        bn_add_to(multiplicator, one);
    }
    bn_delete(one);
    bn_delete(multiplicator);
    return ret;
}
