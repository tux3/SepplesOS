#ifndef ALGORITHM_H
#define ALGORITHM_H

#define MIN(x, y) ({                \
    auto _min1 = (x);          \
    auto _min2 = (y);          \
    (void) (&_min1 == &_min2);      \
    _min1 < _min2 ? _min1 : _min2; })

#define MAX(x, y) ({                \
    auto _min1 = (x);          \
    auto _min2 = (y);          \
    (void) (&_min1 == &_min2);      \
    _min1 > _min2 ? _min1 : _min2; })

#endif // ALGORITHM_H
