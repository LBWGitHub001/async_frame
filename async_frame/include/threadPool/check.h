//
// Created by lbw on 25-2-19.
//

#ifndef CHECK_H
#define CHECK_H

template <class T>
class CheckType
{
private:
    template <typename U, typename = decltype(U(std::declval<U>()))>
    static std::true_type check(int);

    template <typename U>
    static std::false_type check(...);

public:
    static const bool value = decltype(check<T>(0))::value;
};


#endif //CHECK_H
