#ifndef THREADSAFESMARTPOINTERS_NULLPTREXCEPTION_H
#define THREADSAFESMARTPOINTERS_NULLPTREXCEPTION_H

/**
 * @file        ts_null_ptr_exception.h
 * @author      Argishti Ayvazyan (ayvazyan.argishti@gmail.com)
 * @brief       Declaration and implementations of thread-safe smart pointers.
 * @date        7/20/2021.
 * @copyright   Copyright (c) 2021
 */

#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace ts {
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @class null_ptr_exception
 *
 * @brief The exception class for handling null pointer dereferencing.
 */
class null_ptr_exception : std::exception
{
public:
    null_ptr_exception() = default;
    ~null_ptr_exception() override = default;
    null_ptr_exception(null_ptr_exception&&) = default;
    null_ptr_exception(const null_ptr_exception&) = default;
    null_ptr_exception& operator=(null_ptr_exception&&) = default;
    null_ptr_exception& operator=(const null_ptr_exception&) = default;

    explicit null_ptr_exception(const char* massage) noexcept
        : std::exception(massage)
    { }
}; // class null_ptr_exception


////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ts
////////////////////////////////////////////////////////////////////////////////////////////////////


#endif //THREADSAFESMARTPOINTERS_NULLPTREXCEPTION_H
