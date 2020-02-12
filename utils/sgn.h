/*
*   return the sign of val
*   by R. Falque
*   07/02/2020
*/

#ifndef SIGN_H
#define SIGN_H

#include <iostream>

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
};

template <typename T> bool is_positive(T val) {
    return (T(0) < val);
};

#endif
