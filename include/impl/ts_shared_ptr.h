#ifndef THREADSAFESMARTPOINTERS_TS_SHARED_PTR_H
#define THREADSAFESMARTPOINTERS_TS_SHARED_PTR_H

/**
 * @file        ts_shared_ptr.h
 * @author      Argishti Ayvazyan (ayvazyan.argishti@gmail.com)
 * @brief       Declaration and implementations of thread-safe shared_ptr.
 * @date        7/21/2021.
 * @copyright   Copyright (c) 2021
 */


#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace ts {
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace impl {
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief           Checks exists a constructor in std::shared_ptr for given arguments list.
 *
 * @tparam T        The object type.
 * @tparam TArgs    The arguments pack types.
 */
template <typename T, typename... TArgs>
concept is_std_shared_init_list = requires(TArgs&&... args)
{
    std::shared_ptr<T> { std::forward<TArgs>(args)... };
};

/**
 * @brief       Checks the given type can be used as shared_mutex.
 *
 * @tparam T    The mutex type.
 */
template <typename T>
concept is_shared_lockable = requires(T mtx)
{
    mtx.lock_shared();
    mtx.unlock_shared();
    { mtx.try_lock_shared() } -> std::same_as<bool>;
};

/**
 * @brief       Gets std::shared_lock if T is shared_mutex, otherwise std::unique_lock.
 *
 * @tparam T    The mutex type.
 */
template <typename T>
using t_read_lock = std::conditional_t<is_shared_lockable<T>
        , std::shared_lock<T>
        , std::unique_lock<T>>;

/**
 * @brief       Define write lock.
 *
 * @tparam T    The mutex type.
 */
template <typename T>
using t_write_lock = std::unique_lock<T>;

////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace impl
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief           ts::shared_ptr is a smart pointer that retains shared thread-safe ownership of
 *                  an object through a pointer. Several shared_ptr objects may own the same object.
 *                  The object is destroyed and its memory deallocated when either of the following
 *                  happens:
 *                      * the last remaining shared_ptr owning the object is destroyed.
 *                      * the last remaining shared_ptr owning the object is assigned
 *                        another pointer via operator= or reset().
 *
 * @details         The ts::shared_ptr implemented using Execute Around Pointer Idiom.
 *                  The all methods called using structure dereference or subscript
 *                  operators will be executed under the protection of a mutex.
 * @example         // Below shown 2 examples for simple use.
 *                  ts::shared_ptr<std::vector<int>> p_vec { new std::vector<int>{} };
 *                  p_vec->push_back(13);
 * @example         ts::shared_ptr<std::vector<int>> p_vec = ts::make_shared<std::vector<int>>();
 *                  p_vec->push_back(13);
 * @example         // Below shown the ts::shared_ptr use example with shared_mutex,
 *                  // using the read-write lock.
 *                  ts::shared_ptr<std::vector<int>, std::shared_mutex> mutable_ptr {
 *                          new std::vector<int> };
 *                  mutable_ptr->push_back(13); // On mutable ptr working unique_lock.
 *                  ts::shared_ptr<const std::vector<int>, std::shared_mutex> const_ptr{mutable_ptr};
 *                  const_ptr->size(); // On const ptr working shared_lock.
 * @example         // Below shown use example for array.
 *                  auto ptr = ts::make_shared<int32_t[]>(100);
 *                  (*ptr)[1] = 12;
 *                  auto val = (*ptr)[2];
 * @warning         Do not take any reference using subscript operator.
 *                  All saved references are not thread-safe.
 * @example         // Below shown example for use with custom mutex.
 *                  ts::shared_ptr<std::vector<int>, boost::sync::spin_mutex>
 *                      p_vec { new std::vector<int>{} };
 *                  p_vec->push_back(13);
 * @example         Below shown example for use with custom deleter.
 *                  ts::shared_ptr<std::vector<int>, std::mutex
 *                      , std::function<void(std::vector<int>*)>>
 *                      p_vec { new std::vector<int>{}, [](auto* vec) { delete vec; } };
 *                  p_vec->push_back(13);
 * @warning         Structure dereference or subscript operators cannot protect object from
 *                  API races. To safe object from API races, you can use lock, unlock and get
 *                  API's. The examples below.
 * @example         auto queue = ts::make_shared<std::queue<int32_t>>();
 *                  // do something
 *                  {
 *                      queue.lock();
 *                      if (queue.get()->empty())
 *                      {
 *                          queue.get()->pop();
 *                      }
 *                      queue->unlock();
 *                  }
 * @example         auto queue = ts::make_shared<std::queue<int32_t>>();
 *                  // do something
 *                  {
 *                      std::lock_guard lock{queue};
 *                      if (queue.get()->empty())
 *                      {
 *                          queue.get()->pop();
 *                      }
 *                  }
 * @tparam T        The type of element or array of elements.
 * @tparam TMutex   The type of mutex (optional by default std::mutex)
 */
template <typename T, typename TMutex = std::mutex>
class shared_ptr
{
    template <typename TAnyValue, typename TAnyMutex>
    friend class shared_ptr;

    /**
     * Shows the object is read-only and possible to use shared_lock.
     */
    static constexpr bool is_read_only = std::is_const_v<T>;

    using t_mutex = TMutex;
    using t_mutex_ref = t_mutex&;
    using t_mutex_ptr = std::shared_ptr<t_mutex>;
    using t_data_ptr = std::shared_ptr<T>;


public:
    /**
     * T, the type of the object managed by this shared_ptr.
     */
    using element_type = typename std::shared_ptr<T>::element_type;

    /**
     * The mutex type.
     */
    using mutex_type = t_mutex;

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @internal
     *
     * @class           proxy_locker
     * @brief           The class proxy_locker is a proxy object and wrapper for mutex and pointer
     *                  that provides a convenient RAII-style mechanism for owning a mutex for the
     *                  duration of an object lifetime.
     *
     * @details         When a proxy_locker object is created, it attempts to take ownership of the
     *                  mutex it is given. When control leaves the scope in which the proxy_locker
     *                  object was created, the proxy_locker is destructed and the mutex is
     *                  released.
     * @tparam TLock    The lock_guard_type.
     */
    template <typename TLock>
    class proxy_locker
    {
        using t_lock_guard = TLock;

    public:
        /**
         * @brief       Construct object from mutex and object pointer.
         *              During the construction proxy_locker object locked the given mutex until
         *              object destruction.
         *
         * @param mtx   The mutex reference for locking.
         * @param ptr   The object pointer for giving to a user.
         */
        proxy_locker(t_mutex& mtx, T* ptr) noexcept
            : m_lock(mtx)
            , m_ptr(ptr)
        {
        }

        proxy_locker(proxy_locker&& o) noexcept = default;
        ~proxy_locker() = default;

        proxy_locker() = delete;
        proxy_locker(const proxy_locker&) = delete;
        proxy_locker& operator=(proxy_locker&&) = delete;
        proxy_locker& operator=(const proxy_locker&) = delete;

        element_type* operator->() noexcept(!impl::config::s_enable_exceptions)
        {
            if constexpr(impl::config::s_enable_exceptions)
            {
                if(nullptr == m_ptr)
                {
                    throw null_ptr_exception {
                        "Trying to dereference null pointer using -> operator." };
                }
            }
            return m_ptr;
        }

        const element_type* operator->() const noexcept(!impl::config::s_enable_exceptions)
        {
            if constexpr(impl::config::s_enable_exceptions)
            {
                if(nullptr == m_ptr)
                {
                    throw null_ptr_exception {
                        "Trying to dereference null pointer using -> operator." };
                }
            }
            return m_ptr;
        }

    private:
        t_lock_guard m_lock;
        element_type* m_ptr = nullptr;
    }; // class proxy_locker

    using t_proxy_locker = proxy_locker<impl::t_write_lock<t_mutex>>;
    using t_const_proxy_locker = const proxy_locker<impl::t_read_lock<t_mutex>>;

    using t_proxy_locker_ret = std::conditional_t<is_read_only
            , t_const_proxy_locker
            , t_proxy_locker>;
    ////////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////
    /**
     * @internal
     *
     * @class           proxy_locker_for_subscript
     * @brief           The class proxy_locker_for_subscript is a proxy object and wrapper for mutex
     *                  and pointer that provides a convenient RAII-style mechanism for owning
     *                  a mutex for the duration of an object lifetime.
     *
     * @details         When a proxy_locker_for_subscript object is created, it attempts to take
     *                  ownership of the mutex it is given. When control leaves the scope in which
     *                  the proxy_locker_for_subscript object was created, the
     *                  proxy_locker_for_subscript is destructed and the mutex is released.
     * @tparam TLock    The lock_guard_type.
     */
    template <typename TLock>
    class proxy_locker_for_subscript
    {
        using t_lock_guard = TLock;

    public:
        /**
         * @brief       Construct object from mutex and object pointer.
         *              During the construction proxy_locker_for_subscript object locked the
         *              given mutex until object destruction.
         *
         * @param mtx   The mutex reference for locking.
         * @param ptr   The object pointer for giving to a user.
         */
        proxy_locker_for_subscript(t_mutex& mtx, element_type* ptr) noexcept
            : m_lock(mtx)
            , m_ptr(ptr)
        {
        }

        proxy_locker_for_subscript(proxy_locker_for_subscript&& o) noexcept = default;
        ~proxy_locker_for_subscript() = default;

        proxy_locker_for_subscript() = delete;
        proxy_locker_for_subscript(const proxy_locker_for_subscript&) = delete;
        proxy_locker_for_subscript& operator=(proxy_locker_for_subscript&&) = delete;
        proxy_locker_for_subscript& operator=(const proxy_locker_for_subscript&) = delete;

    public:
        /**
         * @brief           The subscript operator for working with arrays.
         *
         * @param index     The array index.
         * @return          The reference to the object.
         */
        element_type& operator[](std::size_t index) noexcept (!impl::config::s_enable_exceptions)
        {
            return const_cast<element_type&>(std::as_const(*this)[index]);
        }

        /**
         * @brief           The subscript operator for working with arrays.
         *
         * @param index     The array index.
         * @return          The const reference to the object.
         */
        const element_type& operator[](std::size_t index) const
                            noexcept (!impl::config::s_enable_exceptions)
        {
            if constexpr (impl::config::s_enable_exceptions)
            {
                if (nullptr == m_ptr)
                {
                    throw null_ptr_exception {
                        "Trying to dereference null pointer using [] operator." };
                }
            }
            return m_ptr[index];
        }

    private:
        t_lock_guard m_lock;
        element_type* m_ptr = nullptr;
    }; // class proxy_locker_for_subscript

    using t_proxy_locker_for_subscript = proxy_locker_for_subscript<impl::t_write_lock<t_mutex>>;
    using t_const_proxy_locker_for_subscript
            = const proxy_locker_for_subscript<impl::t_read_lock<t_mutex>>;;

    using t_proxy_locker_for_subscript_ret = std::conditional_t<is_read_only
            , t_const_proxy_locker_for_subscript
            , t_proxy_locker_for_subscript>;

    ////////////////////////////////////////////////////////////////////////////////////////////////


public:
    /**
     * @brief           The wrapper constructor for use all constructors of std::shared_ptr.
     *
     * @tparam TArgs    The types pack for constructor arguments.
     * @param args      constructor arguments
     */
    template <typename... TArgs>
    shared_ptr(TArgs&&... args) requires(impl::is_std_shared_init_list<T, TArgs...>)
        : m_data { std::forward<TArgs>(args)... }
    {
    }

    /**
     * @brief       The thread-safe move constructor for ts::shared_ptr.
     *
     * @details     Locks resources from the original object then moves all resources
     *              to the new object (*this).
     * @warning     After the move action, the original object state will be undefined.
     *              To resume it, call the reset() method.
     * @warning     If the original object is invalid, the constructor behavior is undefined.
     * @param other The reference to the original object.
     */
    shared_ptr(shared_ptr&& other) noexcept
        : m_mtx {}
        , m_data {}
    {
        std::lock_guard lock { *(other.m_mtx.get()) };
        m_mtx = std::move(other.m_mtx);
        m_data = std::move(other.m_data);
    }

    /**
     * @brief           Contracts the readonly ts::shared_ptr<const T> from ts::shared_ptr<T>.
     *
     * @details         Locks resources from the original object then moves all resources
     *                  to the new object (*this).
     * @note            If the shared_ptr is read only and the mutex type is shared_mutex the
     *                  shared_ptr provides only const actions and uses the read_lock.
     * @warning         After the move action, the original object state will be undefined.
     *                  To resume it, call the reset() method.
     * @warning         If the original object is invalid, the constructor behavior is undefined.
     * @tparam TOrig    The object type of original object.
     * @param other     The reference to the original object.
     */
    template <typename TOrig> requires(!std::is_const_v<TOrig> && is_read_only)
    shared_ptr(shared_ptr<TOrig, t_mutex>&& other) noexcept
        : m_mtx {}
        , m_data {}
    {
        std::lock_guard lock { *(other.m_mtx.get()) };
        m_mtx = std::move(other.m_mtx);
        m_data = std::move(other.m_data);
    }

    /**
     * @brief       The thread-safe copy constructor for ts::shared_ptr.
     *
     * @details     Locks resources from the original object then copy all resources
     *              to the new object (*this).
     * @warning     If the original object is invalid, the constructor behavior is undefined.
     * @param other The reference to the original object.
     */
    shared_ptr(const shared_ptr<T, TMutex>& other)
        : m_mtx {}
        , m_data {}
    {
        std::lock_guard lock { *(other.m_mtx.get()) };
        m_mtx = other.m_mtx;
        m_data = other.m_data;
    }

    /**
     * @brief           Contracts the readonly ts::shared_ptr<const T> from ts::shared_ptr<T>.
     *
     * @details         Locks resources from the original object then copy all resources
     *                  to the new object (*this).
     * @note            If the shared_ptr is read only and the mutex type is shared_mutex the
     *                  shared_ptr provides only const actions and uses the read_lock.
     * @warning         If the original object is invalid, the constructor behavior is undefined.
     * @tparam TOrig    The object type of original object.
     * @param other     The reference to the original object.
     */
    template <typename TOrig> requires(!std::is_const_v<TOrig> && is_read_only)
    shared_ptr(const shared_ptr<TOrig, t_mutex>& other)
        : m_mtx {}
        , m_data {}
    {
        std::lock_guard lock { *(other.m_mtx.get()) };
        m_mtx = other.m_mtx;
        m_data = other.m_data;
    }


    /**
     * @brief       The thread-safe move assignment operator for ts::shared_ptr.
     *
     * @details     Locks resources from the original object then moves all resources
     *              to the new object (*this).
     * @warning     After the move action, the original object state will be undefined.
     *              To resume it, call the reset() method.
     * @warning     If the original object is invalid, the constructor behavior is undefined.
     * @param other The reference to the original object.
     * @return      The reference to the this object.
     */
    shared_ptr& operator=(shared_ptr&& other) noexcept
    {
        if(this == std::addressof(other))
        {
            return *this;
        }

        auto tmp_ref_to_mtx { this->m_mtx };
        std::scoped_lock lock { *(tmp_ref_to_mtx.get()), *(other.m_mtx.get()) };
        m_mtx = std::move(other.m_mtx);
        m_data = std::move(other.m_data);
        return *this;
    }


    /**
     * @brief       The thread-safe move assignment operator for ts::shared_ptr.
     *
     * @details     Locks resources from the original object then moves all resources
     *              to the new object (*this).
     * @warning     After the move action, the original object state will be undefined.
     *              To resume it, call the reset() method.
     * @warning     If the original object is invalid, the constructor behavior is undefined.
     * @param other The reference to the original object.
     * @return      The reference to the this object.
     */
    shared_ptr& operator=(const shared_ptr& other) noexcept
    {
        if(this == std::addressof(other))
        {
            return *this;
        }

        auto tmp_ref_to_mtx { this->m_mtx };
        std::scoped_lock lock { *(tmp_ref_to_mtx.get()), other };
        m_mtx = other.m_mtx;
        m_data = other.m_data;
        return *this;
    }


    /**
     * @brief   Gets raw pointer to object.
     *
     * @warning This API is not thread-safe. Use it only then shared_ptr
     *          locked (\refitem ts::shared_ptr::lock).
     *
     * @return  The raw pointer.
     */
    [[nodiscard]] element_type* get() const noexcept
    {
        return m_data.get();
    }

    /**
     * @brief   Checks whether *this owns an object, i.e. whether get() != nullptr.
     *
     * @return  true if *this owns an object, false otherwise.
     */
    explicit operator bool() const noexcept
    {
        impl::t_read_lock<t_mutex> lock { mutex_ref() };
        return static_cast<bool>(m_data);
    }

    /**
     * @brief   Replaces the managed object.
     */
    void reset() noexcept
    {
        auto new_mutex = std::make_shared<t_mutex>();
        std::lock_guard lock_new_mutex { *(new_mutex.get()) };
        if(nullptr == m_mtx)
        {
            m_mtx = std::move(new_mutex);
            m_data.reset();
            return;
        }
        auto tmp_ref_to_mtx { this->m_mtx };
        std::lock_guard lock_old_mutex { *(tmp_ref_to_mtx.get()) };
        m_mtx = std::move(new_mutex);
        m_data.reset();
    }


    /**
     * @brief               Replaces the managed object and sets new if that is given.
     *
     * @tparam TArgs        The type of argument, using for automatic overload all methods.
     * @param new_pointer   Pointer to a new object to manage
     */
    template <typename... TArgs>
    void reset(TArgs&&... new_pointer) noexcept
    {
        auto new_mutex = std::make_shared<t_mutex>();
        auto tmp_ref_to_mtx { this->m_mtx };
        std::scoped_lock lock { *(tmp_ref_to_mtx.get()), *(new_mutex.get()) };
        m_mtx = std::move(new_mutex);
        m_data.reset(new_pointer...);
    }

public:
    /**
     * @brief   Returns the reference to the object.
     *
     * @details This API working on Execute Around Pointer Idiom.
     *          Before giving the object reference to user locks the mutex,
     *          the mutex still locked until reached ";".
     * @throws  ts::null_ptr_exception if this pointer hasn't owned any object.
     *          The condition *this == nullptr is true.
     * @example ts::shared_ptr<std::vector<int>> p_vec = ts::make_shared<std::vector<int>>();
     *          p_vec->push_back(13);
     * @return  Returns a pointer to the object owned by *this.
     */
    t_proxy_locker_ret operator->() const
    {
        return t_proxy_locker_ret(mutex_ref(), m_data.get());
    }

    /**
     * @brief   Returns the object with subscript operator for working with arrays.
     *
     * @details This API working on Execute Around Pointer Idiom.
     *          Before giving the object reference to user locks the mutex,
     *          the mutex still locked until reached ";".
     * @throws  ts::null_ptr_exception if this pointer hasn't owned any object.
     *          The condition *this == nullptr is true.
     * @return  Returns a pointer to the object owned by *this.
     */
    t_proxy_locker_for_subscript_ret operator*() const
    {
        return t_proxy_locker_for_subscript_ret(mutex_ref(), m_data.get());
    }


public:
    /**
     * @brief   Locks the mutex, blocks if the mutex is not available.
     *          Using for solve API races.
     *
     * @example std::lock_guard lock{queue};
     *          if (!queue.get()->empty())
     *          {
     *              (void) queue.get()->pop();
     *          }
     */
    void lock() const
    {
        mutex_ref().lock();
    }

    /**
     * @brief   Unlocks the mutex.
     *          Using for solve API races.
     */
    void unlock() const
    {
        mutex_ref().unlock();
    }

    /**
     * @brief   Tries to lock the mutex. Returns immediately.
     *          On successful lock acquisition returns true, otherwise returns false.
     *
     * @return  true if the lock was acquired successfully, otherwise false.
     */
    bool try_lock() const
    {
        return mutex_ref().try_lock();
    }

    /**
     * @brief   Locks the mutex for shared ownership, blocks if the mutex is not available.
     *
     * @example std::shared_lock lock{queue};
     *          if (!queue.get()->empty())
     *          {
     *              (void) queue.get()->front();
     *          }
     */
    void lock_shared() const requires(is_read_only && impl::is_shared_lockable<t_mutex>)
    {
        mutex_ref().lock_shared();
    }

    /**
     * @brief   Unlocks the mutex (shared ownership).
     */
    void unlock_shared() const requires(is_read_only && impl::is_shared_lockable<t_mutex>)
    {
        mutex_ref().unlock_shared();
    }

    /**
     * @brief   Tries to lock the mutex for shared ownership, returns if the mutex is not available.
     *
     * @return  true if the lock was acquired successfully, otherwise false.
     */
    bool try_lock_shared() const requires(is_read_only && impl::is_shared_lockable<t_mutex>)
    {
        mutex_ref().try_lock_shared();
    }

private:

    /**
     * @internal
     * @brief   Gets reference to the mutex.
     *
     * @return  The reference to the mutex.
     */
    t_mutex_ref mutex_ref() const
    {
        return *(m_mtx.get());
    }

    /**
     * The pointer to the mutex for providing object thread-safety.
     */
    t_mutex_ptr m_mtx { std::make_shared<t_mutex>() };

    /**
     * The non-thread-safe shared pointer for manage object lifetime.
     */
    t_data_ptr m_data {};
};


/**
 * @brief           Constructs an object of type T and wraps it in a
 *                  ts::shared_ptr. (Specialization for a single object.)
 *
 * @example         ts::shared_ptr<std::vector<int>> p_vec = ts::make_shared<std::vector<int>>();
 *                  p_vec->push_back(13);
 *
 * @tparam T        The type of element.
 * @tparam TArgs    The types of list of arguments with which an instance of
 *                  T will be constructed.
 * @param args      List of arguments with which an instance of T will be constructed.
 * @return          ts::shared_ptr of an instance of type T.
 */
template <class T, class... TArgs>
std::enable_if_t<!std::is_array<T>::value, shared_ptr<T>> make_shared(TArgs&&... args)
{
    return shared_ptr<T>(new T(std::forward<TArgs>(args)...));
}

/**
 * @brief           Constructs an object of type T array and wraps it in a
 *                  ts::shared_ptr. (Specialization for a objects array.)
 *
 * @example         auto arr_ptr = ts::make_shared<int32_t[]>(element_count);
 *                  for (int32_t i = 0; i < element_count; ++i)
 *                  {
 *                      (*arr_ptr)[i] = 0;
 *                  }
 * @tparam T        The type of elements array.
 * @param n         The length of the array to construct.
 * @return          ts::shared_ptr of an instance of type T.
 */
template <class T>
std::enable_if_t<std::is_array<T>::value, shared_ptr<T>> make_shared(std::size_t n)
{
    using t_element_type = typename std::remove_extent_t<T>;
    return shared_ptr<T>(new t_element_type[n]);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ts
////////////////////////////////////////////////////////////////////////////////////////////////////


#endif // THREADSAFESMARTPOINTERS_TS_SHARED_PTR_H
