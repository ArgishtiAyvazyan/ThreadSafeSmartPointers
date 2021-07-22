/**
 * @file        main.cc
 * @author      Argishti Ayvazyan (ayvazyan.argishti@gmail.com)
 * @brief       Unit tests implementations.
 * @date        7/14/2021
 * @copyright   Copyright (c) 2021
 */

#include <iostream>
#include <map>
#include <thread>
#include <queue>
#include <atomic>

#include <gtest/gtest.h>

#include <ts_memory.h>

class dummy_object
{
public:
    dummy_object() { ++ms_object_count; }
    ~dummy_object() { --ms_object_count; }

    void inc() noexcept { ++m_value; }
    void dec() noexcept { --m_value; }

    bool is_const() noexcept { return false; }
    bool is_const() const noexcept { return true; }

    int32_t m_value {0};
    static inline int32_t ms_object_count { 0 };
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// ts::unique_ptr testing.
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// API testing.
////////////////////////////////////////////////////////////////////////////////

TEST(unique_ptr_api_testing, constructor_with_pointer)
{
    {
        ts::unique_ptr<dummy_object> ptr{new dummy_object{}};
        ts::unique_ptr<dummy_object> ptr1;
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(unique_ptr_api_testing, make_unique)
{
    {
        auto ptr = ts::make_unique<dummy_object>();
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(unique_ptr_api_testing, constructor_with_nullptr)
{
    {
        ts::unique_ptr<dummy_object> empty_ptr1 = nullptr;
        ASSERT_FALSE(empty_ptr1);
        ts::unique_ptr<dummy_object> empty_ptr2 { nullptr };
        ASSERT_FALSE(empty_ptr2);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(unique_ptr_api_testing, structure_dereference)
{
    {
        auto ptr = ts::make_unique<dummy_object>();
        ptr->inc();
        ptr->dec();
        ASSERT_EQ(ptr->m_value, 0);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(unique_ptr_api_testing, poiner_to_array)
{
    {
        auto ptr = ts::make_unique<dummy_object[]>(100);
        (*ptr)[1].inc();
        (*ptr)[2].dec();
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(unique_ptr_api_testing, move_test)
{
    {
        constexpr int32_t test_count = 100;
        std::vector<ts::unique_ptr<dummy_object>> arr_objects;
        arr_objects.reserve(2 * test_count);
        for (int32_t i = 0; i < test_count; ++i)
        {
            arr_objects.push_back(ts::make_unique<dummy_object>());
            arr_objects.emplace_back(ts::make_unique<dummy_object>());
        }
        for (int32_t i = 0; i < test_count; ++i)
        {
            arr_objects[i] = std::move(arr_objects[i + 1]);
        }
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(unique_ptr_api_testing, copy_test)
{
    static_assert(!std::is_copy_assignable_v<ts::unique_ptr<dummy_object>>);
    static_assert(!std::is_copy_constructible_v<ts::unique_ptr<dummy_object>>);
}

TEST(unique_ptr_api_testing, deleter)
{
    {
        auto deleter = [](dummy_object* p)
        {
            delete p;
        };
        using deleted_unique_ptr = ts::unique_ptr<dummy_object, std::mutex
                , decltype(deleter)>;

        constexpr int32_t test_count = 100;
        std::vector<deleted_unique_ptr> arr_objects;
        arr_objects.reserve(test_count);
        for (int32_t i = 0; i < test_count; ++i)
        {
            arr_objects.push_back(
                    deleted_unique_ptr{new dummy_object{}, deleter});
        }
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(unique_ptr_api_testing, deleter2)
{
    {
        auto deleter = [](dummy_object* p)
        {
            delete p;
        };
        using deleted_unique_ptr = ts::unique_ptr<dummy_object, std::mutex
                , std::function<void(dummy_object*)>>;

        constexpr int32_t test_count = 100;
        std::vector<deleted_unique_ptr> arr_objects;
        arr_objects.reserve(test_count);
        for (int32_t i = 0; i < test_count; ++i)
        {
            arr_objects.push_back(deleted_unique_ptr{new dummy_object{}, deleter});
        }
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

template <typename T>
void custom_deleter(T* ptr)
{
    delete ptr;
}

TEST(unique_ptr_api_testing, get_deleter_testing)
{
    using deleted_unique_ptr = ts::unique_ptr<int, std::mutex
            , void(*)(int*)>;
    deleted_unique_ptr unique_ptr { new int, &custom_deleter<int> };
    ASSERT_EQ(unique_ptr.get_deleter(), &custom_deleter<int>);
}

TEST(unique_ptr_api_testing, bool_operator)
{
    auto initialized_ptr = ts::make_unique<int>();
    ASSERT_TRUE(initialized_ptr);
    ts::unique_ptr<int> empty_ptr;
    ASSERT_FALSE(empty_ptr);
    empty_ptr = std::move(initialized_ptr);
    ASSERT_FALSE(initialized_ptr);
    ASSERT_TRUE(empty_ptr);
}

TEST(unique_ptr_api_testing, release_operator)
{
    ts::unique_ptr<int> empty_ptr;
    ASSERT_EQ(empty_ptr.release(), nullptr);
    auto initialized_ptr = ts::make_unique<int>();
    auto p_object = initialized_ptr.release();
    ASSERT_TRUE(nullptr != p_object);
    delete p_object;
    ASSERT_FALSE(initialized_ptr);
}

TEST(unique_ptr_api_testing, reset_operator)
{
    {
        auto initialized_ptr = ts::make_unique<dummy_object>();
        initialized_ptr.reset();
        ASSERT_EQ(0, dummy_object::ms_object_count);
        initialized_ptr.reset(new dummy_object);
        ASSERT_EQ(1, dummy_object::ms_object_count);
    }
    ASSERT_EQ(0, dummy_object::ms_object_count);
}


TEST(unique_ptr_api_testing, compare_operator)
{
    std::vector<ts::unique_ptr<int>> old_values;
    old_values.reserve(200);
    for (int32_t i = 0; i < 100; ++i)
    {
        auto ptr1 = ts::make_unique<int>();
        auto ptr2 = ts::make_unique<int>();
        ASSERT_EQ ((ptr1 == ptr2), (ptr1.get() == ptr2.get()));
        ASSERT_EQ ((ptr1 != ptr2), (ptr1.get() != ptr2.get()));
        ASSERT_EQ ((ptr1 < ptr2), (ptr1.get() < ptr2.get()));
        ASSERT_EQ ((ptr1 <= ptr2), (ptr1.get() <= ptr2.get()));
        ASSERT_EQ ((ptr1 > ptr2), (ptr1.get() > ptr2.get()));
        ASSERT_EQ ((ptr1 >= ptr2), (ptr1.get() >= ptr2.get()));

        old_values.push_back(std::move(ptr1));
        old_values.push_back(std::move(ptr2));
    }
}

TEST(unique_ptr_api_testing, self_compare)
{
    auto ptr = ts::make_unique<int>();
    ASSERT_TRUE(ptr == ptr);
    ASSERT_FALSE(ptr != ptr);
    ASSERT_FALSE(ptr < ptr);
    ASSERT_FALSE(ptr > ptr);
    ASSERT_TRUE(ptr <= ptr);
    ASSERT_TRUE(ptr >= ptr);
}

class class_with_three_way_comparison_operator
{
public:
    auto operator<=>(const class_with_three_way_comparison_operator&) const = default;

    ts::unique_ptr<int> m_ptr1;
    ts::unique_ptr<int> m_ptr2;
};

TEST(unique_ptr_api_testing, three_way_comparison_operator)
{
    class_with_three_way_comparison_operator a, b;
    ASSERT_TRUE(a == b);
    ASSERT_TRUE(a >= b);
    ASSERT_TRUE(a <= b);
    ASSERT_FALSE(a != b);
    ASSERT_FALSE(a < b);
    ASSERT_FALSE(a > b);
}

TEST(unique_ptr_api_testing, three_way_comparison_operator_2)
{
    ts::unique_ptr<int> x;
    ts::unique_ptr<int> y;
    ASSERT_EQ((std::compare_three_way{}(x.get(), y.get()))
              , (std::compare_three_way{}(x, y)));
}

TEST(unique_ptr_api_testing, compares_a_empty_unique_ptr_and_nullptr)
{
    ts::unique_ptr<int> empty_ptr;
    ASSERT_TRUE(empty_ptr == nullptr);
    ASSERT_TRUE(nullptr == empty_ptr);
    ASSERT_FALSE(empty_ptr != nullptr);
    ASSERT_FALSE(nullptr != empty_ptr);

    // operator ==
    ASSERT_EQ((empty_ptr == nullptr), (empty_ptr.get() == nullptr));
    ASSERT_EQ((nullptr == empty_ptr), (nullptr == empty_ptr.get()));

    // operator !=
    ASSERT_EQ((empty_ptr != nullptr), (empty_ptr.get() != nullptr));
    ASSERT_EQ((nullptr != empty_ptr), (nullptr != empty_ptr.get()));

    // operator <
    ASSERT_EQ((empty_ptr < nullptr), (empty_ptr.get() < nullptr));
    ASSERT_EQ((nullptr < empty_ptr), (nullptr < empty_ptr.get()));

    // operator >
    ASSERT_EQ((empty_ptr > nullptr), (empty_ptr.get() > nullptr));
    ASSERT_EQ((nullptr > empty_ptr), (nullptr > empty_ptr.get()));

    // operator <=
    ASSERT_EQ((empty_ptr <= nullptr), (empty_ptr.get() <= nullptr));
    ASSERT_EQ((nullptr <= empty_ptr), (nullptr <= empty_ptr.get()));

    // operator >=
    ASSERT_EQ((empty_ptr >= nullptr), (empty_ptr.get() >= nullptr));
    ASSERT_EQ((nullptr >= empty_ptr), (nullptr >= empty_ptr.get()));
}

TEST(unique_ptr_api_testing, compares_a_initialized_and_empty_unique_ptr)
{
    auto initialized_ptr = ts::make_unique<int>();
    ts::unique_ptr<int> empty_ptr;

    // operator ==
    ASSERT_EQ((initialized_ptr == nullptr), (initialized_ptr == empty_ptr));
    ASSERT_EQ((initialized_ptr == nullptr), (initialized_ptr == empty_ptr));
    ASSERT_EQ((nullptr == initialized_ptr), (empty_ptr == initialized_ptr));
    ASSERT_EQ((nullptr == initialized_ptr), (empty_ptr == initialized_ptr));

    // operator !=
    ASSERT_EQ((initialized_ptr != nullptr), (initialized_ptr != empty_ptr));
    ASSERT_EQ((initialized_ptr != nullptr), (initialized_ptr != empty_ptr));
    ASSERT_EQ((nullptr != initialized_ptr), (empty_ptr != initialized_ptr));
    ASSERT_EQ((nullptr != initialized_ptr), (empty_ptr != initialized_ptr));

    // operator <
    ASSERT_EQ((initialized_ptr < nullptr), (initialized_ptr < empty_ptr));
    ASSERT_EQ((initialized_ptr < nullptr), (initialized_ptr < empty_ptr));
    ASSERT_EQ((nullptr < initialized_ptr), (empty_ptr < initialized_ptr));
    ASSERT_EQ((nullptr < initialized_ptr), (empty_ptr < initialized_ptr));

    // operator >
    ASSERT_EQ((initialized_ptr > nullptr), (initialized_ptr > empty_ptr));
    ASSERT_EQ((initialized_ptr > nullptr), (initialized_ptr > empty_ptr));
    ASSERT_EQ((nullptr > initialized_ptr), (empty_ptr > initialized_ptr));
    ASSERT_EQ((nullptr > initialized_ptr), (empty_ptr > initialized_ptr));

    // operator <=
    ASSERT_EQ((initialized_ptr <= nullptr), (initialized_ptr <= empty_ptr));
    ASSERT_EQ((initialized_ptr <= nullptr), (initialized_ptr <= empty_ptr));
    ASSERT_EQ((nullptr <= initialized_ptr), (empty_ptr <= initialized_ptr));
    ASSERT_EQ((nullptr <= initialized_ptr), (empty_ptr <= initialized_ptr));

    // operator >=
    ASSERT_EQ((initialized_ptr >= nullptr), (initialized_ptr >= empty_ptr));
    ASSERT_EQ((initialized_ptr >= nullptr), (initialized_ptr >= empty_ptr));
    ASSERT_EQ((nullptr >= initialized_ptr), (empty_ptr >= initialized_ptr));
    ASSERT_EQ((nullptr >= initialized_ptr), (empty_ptr >= initialized_ptr));
}


TEST(unique_ptr_api_testing, compares_a_initialized_unique_ptr_and_nullptr)
{
    auto initialized_ptr = ts::make_unique<int>();
    ASSERT_TRUE(initialized_ptr != nullptr);
    ASSERT_FALSE(initialized_ptr == nullptr);
    ASSERT_TRUE(nullptr != initialized_ptr);
    ASSERT_FALSE(nullptr == initialized_ptr);


    // operator ==
    ASSERT_EQ((initialized_ptr == nullptr), (initialized_ptr.get() == nullptr));
    ASSERT_EQ((nullptr == initialized_ptr), (nullptr == initialized_ptr.get()));

    // operator !=
    ASSERT_EQ((initialized_ptr != nullptr), (initialized_ptr.get() != nullptr));
    ASSERT_EQ((nullptr != initialized_ptr), (nullptr != initialized_ptr.get()));

    // operator <
    ASSERT_EQ((initialized_ptr < nullptr), (initialized_ptr.get() < nullptr));
    ASSERT_EQ((nullptr < initialized_ptr), (nullptr < initialized_ptr.get()));

    // operator >
    ASSERT_EQ((initialized_ptr > nullptr), (initialized_ptr.get() > nullptr));
    ASSERT_EQ((nullptr > initialized_ptr), (nullptr > initialized_ptr.get()));

    // operator <=
    ASSERT_EQ((initialized_ptr <= nullptr), (initialized_ptr.get() <= nullptr));
    ASSERT_EQ((nullptr <= initialized_ptr), (nullptr <= initialized_ptr.get()));

    // operator >=
    ASSERT_EQ((initialized_ptr >= nullptr), (initialized_ptr.get() >= nullptr));
    ASSERT_EQ((nullptr >= initialized_ptr), (nullptr >= initialized_ptr.get()));
}

TEST(unique_ptr_api_testing, null_ptr_exception)
{
    if constexpr (ts::impl::config::s_enable_exceptions)
    {
        ts::unique_ptr<dummy_object> empty_ptr;
        EXPECT_THROW(empty_ptr->inc(), ts::null_ptr_exception);
        ts::unique_ptr<dummy_object[]> empty_arr;
        EXPECT_THROW((*empty_ptr)[1], ts::null_ptr_exception);
    }
}


////////////////////////////////////////////////////////////////////////////////
// Thread safety testing.
////////////////////////////////////////////////////////////////////////////////

TEST(unique_ptr_thread_safety_testing, concurrent_insert)
{
    const auto hardware_concurrency = std::thread::hardware_concurrency() != 0
            ? std::thread::hardware_concurrency()
            : 2;
    constexpr int32_t insert_pair_thread = 100;

    auto map_ptr = ts::make_unique<std::map<int, int>>();

    std::vector<std::thread> arr_threads;
    arr_threads.reserve(hardware_concurrency);
    for (int32_t i = 0; i < hardware_concurrency; ++i)
    {
        auto task = [&map_ptr, i]()
        {
            for (int32_t j = 0; j < insert_pair_thread; ++j)
            {
                const auto val = j + (i * insert_pair_thread);
                map_ptr->emplace(val, val);
            }
        };
        arr_threads.emplace_back(task);
    }

    std::ranges::for_each(arr_threads, std::mem_fn(&std::thread::join));

    const auto target_size = insert_pair_thread * hardware_concurrency;
    ASSERT_EQ(map_ptr->size(), target_size);
    for (int32_t i = 0; i < target_size; ++i)
    {
        ASSERT_EQ(i, map_ptr->at(i));
    }
}

TEST(unique_ptr_thread_safety_testing, concurrent_arr_read_write)
{
    const auto hardware_concurrency = std::thread::hardware_concurrency() != 0
        ? std::thread::hardware_concurrency()
        : 2;
    constexpr int32_t element_count = 10;

    auto arr_ptr = ts::make_unique<int32_t[]>(element_count);
    for (int32_t i = 0; i < element_count; ++i)
    {
        (*arr_ptr)[i] = 0;
    }
    std::vector<std::thread> arr_threads;
    arr_threads.reserve(hardware_concurrency);
    for (int32_t i = 0; i < hardware_concurrency; ++i)
    {
        auto task = [&arr_ptr, i]()
        {
            for (int32_t j = 0; j < element_count; ++j)
            {
                if (i & 1)
                {
                    ++(*arr_ptr)[j];
                }
                else
                {
                    --(*arr_ptr)[j];
                }
            }
        };
        arr_threads.emplace_back(task);
    }

    std::ranges::for_each(arr_threads, std::mem_fn(&std::thread::join));
    for (int32_t i = 0; i < element_count; ++i)
    {
        ASSERT_EQ((*arr_ptr)[i], 0);
    }
}

TEST(unique_ptr_thread_safety_testing, api_race)
{
    for (int32_t pass = 0; pass < 100; ++pass)
    {
        auto queue = ts::make_unique<std::queue<int32_t>>();
        constexpr int32_t element_count = 100;
        std::atomic_bool done{false};

        auto producer = [&queue, &done]()
        {
            for (int32_t i = 0; i < element_count; ++i)
            {
                queue->push(i);
            }
            done.store(true);
        };

        auto consumer = [&queue, &done]()
        {
            while (!done || !queue->empty())
            {
                std::lock_guard lock{queue};
                if (queue.get()->empty())
                {
                    continue;
                }
                queue.get()->pop();
            }
        };

        std::vector<std::thread> arr_threads;
        arr_threads.emplace_back(producer);
        for (int32_t i = 0; i < 4; ++i)
        {
            arr_threads.emplace_back(consumer);
        }

        std::ranges::for_each(arr_threads, std::mem_fn(&std::thread::join));
        ASSERT_EQ(0, queue->size());
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// ts::shared_ptr testing.
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST(shared_ptr_api_testing, constructor_with_pointer)
{
    {
        ts::shared_ptr<dummy_object> ptr{new dummy_object{}};
        ts::shared_ptr<dummy_object> ptr1;
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, constructor_with_nullptr)
{
    {
        ts::shared_ptr<dummy_object> empty_ptr1 = nullptr;
        ASSERT_FALSE(empty_ptr1);
        ts::shared_ptr<dummy_object> empty_ptr2 { nullptr };
        ASSERT_FALSE(empty_ptr2);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, deleter)
{
    {
        auto deleter = [](dummy_object* p)
        {
            delete p;
        };

        constexpr int32_t test_count = 100;
        std::vector<ts::shared_ptr<dummy_object>> arr_objects;
        arr_objects.reserve(test_count);
        for (int32_t i = 0; i < test_count; ++i)
        {
            arr_objects.push_back(ts::shared_ptr<dummy_object>{new dummy_object{}, deleter});
        }
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, copy)
{
    {
        ts::shared_ptr<dummy_object> obj1{new dummy_object};
        ts::shared_ptr<dummy_object> obj2{obj1};
        ASSERT_EQ (obj1.get(), obj2.get());
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, copy_assignment)
{
    {
        ts::shared_ptr<dummy_object> obj1{new dummy_object};
        ts::shared_ptr<dummy_object> obj2{new dummy_object};
        obj1 = obj2;
        ASSERT_EQ (obj1.get(), obj2.get());
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, copy2)
{
    {
        ts::shared_ptr<dummy_object> obj{new dummy_object};
        std::vector<ts::shared_ptr<dummy_object>> arr_objects;
        for (int32_t i = 0; i < 100; ++i)
        {
            arr_objects.push_back(obj);
        }
        ASSERT_EQ(dummy_object::ms_object_count, 1);
        arr_objects.clear();
        ASSERT_EQ(dummy_object::ms_object_count, 1);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, move)
{
    {
        auto* object = new dummy_object;
        ts::shared_ptr<dummy_object> obj1{object};
        ts::shared_ptr<dummy_object> obj2{std::move(obj1)};
        obj1.reset();
        ASSERT_FALSE(obj1);
        ASSERT_EQ (obj1.get(), nullptr);
        ASSERT_EQ (obj2.get(), object);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, move_assignment)
{
    {
        ts::shared_ptr<dummy_object> obj1{new dummy_object};
        ts::shared_ptr<dummy_object> obj2{new dummy_object};
        auto* object2 = obj2.get();
        obj1 = std::move(obj2);
        obj2.reset();
        ASSERT_FALSE(obj2);
        ASSERT_EQ (obj1.get(), object2);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, poiner_to_array)
{
    {
        ts::shared_ptr<dummy_object[]> arr { new dummy_object[100] };
        ts::shared_ptr<dummy_object[100]> arr1 { new dummy_object[100] };
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, make_shared)
{
    {
        std::vector<ts::shared_ptr<dummy_object>> arr_objects;
        for (int32_t i = 0; i < 100; ++i)
        {
            arr_objects.push_back(ts::make_shared<dummy_object>());
        }
        ASSERT_EQ(dummy_object::ms_object_count, 100);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, make_shared_array)
{
    {
        std::vector<ts::shared_ptr<dummy_object[]>> arr_objects;
        for (int32_t i = 0; i < 100; ++i)
        {
            arr_objects.push_back(ts::make_shared<dummy_object[]>(i));
        }
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, reset_operator)
{
    {
        auto initialized_ptr = ts::make_shared<dummy_object>();
        initialized_ptr.reset();
        ASSERT_EQ(0, dummy_object::ms_object_count);
        initialized_ptr.reset(new dummy_object);
        ASSERT_EQ(1, dummy_object::ms_object_count);
    }
    ASSERT_EQ(0, dummy_object::ms_object_count);
}


TEST(shared_ptr_api_testing, structure_dereference)
{
    {
        auto ptr = ts::make_shared<dummy_object>();
        ptr->inc();
        ptr->dec();
        ASSERT_EQ(ptr->m_value, 0);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}


TEST(shared_ptr_api_testing, null_ptr_exception)
{
    if constexpr (ts::impl::config::s_enable_exceptions)
    {
        ts::shared_ptr<dummy_object> empty_ptr;
        EXPECT_THROW(empty_ptr->inc(), ts::null_ptr_exception);
        ts::shared_ptr<dummy_object[]> empty_arr;
        EXPECT_THROW((*empty_ptr)[1], ts::null_ptr_exception);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, non_const_to_const_ptr)
{
    {
        ts::shared_ptr<dummy_object> mutable_ptr = ts::make_shared<dummy_object>();
        mutable_ptr->inc();
        ASSERT_FALSE(mutable_ptr->is_const());
        ts::shared_ptr<const dummy_object> const_ptr{mutable_ptr};
        ASSERT_TRUE(const_ptr->is_const());
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, move_non_const_to_const_ptr)
{
    {
        ts::shared_ptr<dummy_object> mutable_ptr = ts::make_shared<dummy_object>();
        mutable_ptr->inc();
        ASSERT_FALSE(mutable_ptr->is_const());
        ts::shared_ptr<const dummy_object> const_ptr{std::move(mutable_ptr)};
        ASSERT_TRUE(const_ptr->is_const());
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, non_const_to_const_ptr_with_shared_mutex)
{
    {
        ts::shared_ptr<dummy_object, std::shared_mutex> mutable_ptr { new dummy_object };
        mutable_ptr->inc();
        ASSERT_FALSE(mutable_ptr->is_const());
        ts::shared_ptr<const dummy_object, std::shared_mutex> const_ptr{mutable_ptr};
        ASSERT_TRUE(const_ptr->is_const());
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

struct dummy_shared_mutex
{
    void lock() { ++lock_coll_count; }
    void unlock() { ++unlock_coll_count; }
    bool try_lock() { ++try_lock_coll_count; }
    void lock_shared() { ++lock_shared_coll_count; }
    void unlock_shared() { ++unlock_shared_coll_count; }
    bool try_lock_shared() { ++try_lock_shared_coll_count; }


    static void reset()
    {
        lock_coll_count = 0;
        unlock_coll_count = 0;
        try_lock_coll_count = 0;
        lock_shared_coll_count = 0;
        unlock_shared_coll_count = 0;
        try_lock_shared_coll_count = 0;
    }

    static inline int32_t lock_coll_count { 0 };
    static inline int32_t unlock_coll_count { 0 };
    static inline int32_t try_lock_coll_count { 0 };
    static inline int32_t lock_shared_coll_count { 0 };
    static inline int32_t unlock_shared_coll_count { 0 };
    static inline int32_t try_lock_shared_coll_count { 0 };
};


TEST(shared_ptr_api_testing, non_const_to_const_ptr_with_shared_mutex2)
{
    dummy_shared_mutex::reset();
    {
        ts::shared_ptr<dummy_object, dummy_shared_mutex> mutable_ptr { new dummy_object };

        ASSERT_EQ (dummy_shared_mutex::lock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::unlock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::try_lock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::lock_shared_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::unlock_shared_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::try_lock_shared_coll_count, 0);

        ASSERT_FALSE(mutable_ptr->is_const());

        ASSERT_EQ (dummy_shared_mutex::lock_coll_count, 1);
        ASSERT_EQ (dummy_shared_mutex::unlock_coll_count, 1);
        ASSERT_EQ (dummy_shared_mutex::try_lock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::lock_shared_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::unlock_shared_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::try_lock_shared_coll_count, 0);

        ts::shared_ptr<const dummy_object, dummy_shared_mutex> const_ptr{mutable_ptr};
        ASSERT_TRUE(const_ptr->is_const());

        ASSERT_EQ (dummy_shared_mutex::lock_coll_count, 2);
        ASSERT_EQ (dummy_shared_mutex::unlock_coll_count, 2);
        ASSERT_EQ (dummy_shared_mutex::try_lock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::lock_shared_coll_count, 1);
        ASSERT_EQ (dummy_shared_mutex::unlock_shared_coll_count, 1);
        ASSERT_EQ (dummy_shared_mutex::try_lock_shared_coll_count, 0);

    }
    dummy_shared_mutex::reset();
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, locking_apis)
{
    static_assert(!ts::impl::is_shared_lockable<ts::shared_ptr<dummy_object>>);
    static_assert(!ts::impl::is_shared_lockable<ts::shared_ptr<const dummy_object>>);
    static_assert(!ts::impl::is_shared_lockable<ts::shared_ptr<dummy_object, std::shared_mutex>>);
    static_assert(ts::impl::is_shared_lockable<ts::shared_ptr<const dummy_object, std::shared_mutex>>);
}

TEST(shared_ptr_api_testing, poiner_to_array_subscript_ret_val)
{
    {
        ts::shared_ptr<dummy_object[], std::shared_mutex> mutable_ptr { new dummy_object[100] };
        ASSERT_FALSE((*mutable_ptr)[1].is_const());
        ts::shared_ptr<const dummy_object[], std::shared_mutex> const_ptr{mutable_ptr};
        ASSERT_TRUE((*const_ptr)[1].is_const());
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, non_const_to_const_ptr_with_shared_mutex_subscript)
{
    dummy_shared_mutex::reset();
    {
        ts::shared_ptr<dummy_object[], dummy_shared_mutex> mutable_ptr { new dummy_object[100] };

        ASSERT_EQ (dummy_shared_mutex::lock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::unlock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::try_lock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::lock_shared_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::unlock_shared_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::try_lock_shared_coll_count, 0);

        ASSERT_FALSE((*mutable_ptr)[0].is_const());

        ASSERT_EQ (dummy_shared_mutex::lock_coll_count, 1);
        ASSERT_EQ (dummy_shared_mutex::unlock_coll_count, 1);
        ASSERT_EQ (dummy_shared_mutex::try_lock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::lock_shared_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::unlock_shared_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::try_lock_shared_coll_count, 0);

        ts::shared_ptr<const dummy_object[], dummy_shared_mutex> const_ptr{mutable_ptr};

        ASSERT_TRUE((*const_ptr)[0].is_const());

        ASSERT_EQ (dummy_shared_mutex::lock_coll_count, 2);
        ASSERT_EQ (dummy_shared_mutex::unlock_coll_count, 2);
        ASSERT_EQ (dummy_shared_mutex::try_lock_coll_count, 0);
        ASSERT_EQ (dummy_shared_mutex::lock_shared_coll_count, 1);
        ASSERT_EQ (dummy_shared_mutex::unlock_shared_coll_count, 1);
        ASSERT_EQ (dummy_shared_mutex::try_lock_shared_coll_count, 0);

    }
    dummy_shared_mutex::reset();
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, poiner_to_array_subscript)
{
    {
        ts::shared_ptr<dummy_object[]> ptr { new dummy_object[100] };
        (*ptr)[0].inc();
        ASSERT_EQ (((*ptr)[0].m_value), 1);
        (*ptr)[0].dec();
        ASSERT_EQ (((*ptr)[0].m_value), 0);
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Thread safety testing.
////////////////////////////////////////////////////////////////////////////////

TEST(shared_ptr_thread_safety_testing, concurrent_insert)
{
    const auto hardware_concurrency = std::thread::hardware_concurrency() != 0
            ? std::thread::hardware_concurrency()
            : 2;
    constexpr int32_t insert_pair_thread = 100;

    auto map_ptr = ts::make_shared<std::map<int, int>>();

    std::vector<std::thread> arr_threads;
    arr_threads.reserve(hardware_concurrency);
    for (int32_t i = 0; i < hardware_concurrency; ++i)
    {
        auto task = [&map_ptr, i]()
        {
            for (int32_t j = 0; j < insert_pair_thread; ++j)
            {
                const auto val = j + (i * insert_pair_thread);
                map_ptr->emplace(val, val);
            }
        };
        arr_threads.emplace_back(task);
    }

    std::ranges::for_each(arr_threads, std::mem_fn(&std::thread::join));

    const auto target_size = insert_pair_thread * hardware_concurrency;
    ASSERT_EQ(map_ptr->size(), target_size);
    for (int32_t i = 0; i < target_size; ++i)
    {
        ASSERT_EQ(i, map_ptr->at(i));
    }
}

TEST(shared_ptr_thread_safety_testing, read_write_lock)
{
    const auto hardware_concurrency = std::thread::hardware_concurrency() != 0
            ? std::thread::hardware_concurrency()
            : 2;
    constexpr int32_t insert_count = 1000;

    using ts_ptr = ts::shared_ptr<std::map<int, int>, std::shared_mutex>;
    using const_ts_ptr = ts::shared_ptr<const std::map<int, int>, std::shared_mutex>;
    ts_ptr map_ptr { new std::map<int, int>{} };

    auto insert_task = [&map_ptr]()
    {
        for (int32_t i = 0; i < insert_count; ++i)
        {
            map_ptr->emplace(i, i);
        }
    };

    auto erase_task = [&map_ptr]()
    {
        for (int32_t i = 0; i < insert_count; ++i)
        {
            while (!map_ptr->contains(i))
            {
                std::this_thread::yield();
            }
            map_ptr->erase(i);
        }
    };


    auto check_task = [const_map = const_ts_ptr {map_ptr}]()
    {
        int32_t size = 0;
        std::shared_lock lock { const_map };
        for (const auto[key, value] : *(const_map.get()))
        {
            ++size;
        }

        ASSERT_EQ (size, const_map.get()->size());
    };


    std::vector<std::thread> arr_threads;
    for (int32_t i = 0; i < 10; ++i)
    {
        arr_threads.emplace_back(check_task);
    }
    arr_threads.emplace_back(insert_task);
    arr_threads.emplace_back(erase_task);

    std::ranges::for_each(arr_threads, std::mem_fn(&std::thread::join));

    ASSERT_EQ(map_ptr->size(), 0);
}

TEST(shared_ptr_thread_safety_testing, concurrent_arr_read_write)
{
    const auto hardware_concurrency = std::thread::hardware_concurrency() != 0
        ? std::thread::hardware_concurrency()
        : 2;
    constexpr int32_t element_count = 10;

    auto arr_ptr = ts::make_shared<int32_t[]>(element_count);
    for (int32_t i = 0; i < element_count; ++i)
    {
        (*arr_ptr)[i] = 0;
    }
    std::vector<std::thread> arr_threads;
    arr_threads.reserve(hardware_concurrency);
    for (int32_t i = 0; i < hardware_concurrency; ++i)
    {
        auto task = [&arr_ptr, i]()
        {
            for (int32_t j = 0; j < element_count; ++j)
            {
                if (i & 1)
                {
                    ++(*arr_ptr)[j];
                }
                else
                {
                    --(*arr_ptr)[j];
                }
            }
        };
        arr_threads.emplace_back(task);
    }

    std::ranges::for_each(arr_threads, std::mem_fn(&std::thread::join));
    for (int32_t i = 0; i < element_count; ++i)
    {
        ASSERT_EQ((*arr_ptr)[i], 0);
    }
}

TEST(shared_ptr_thread_safety_testing, api_race)
{
    for (int32_t pass = 0; pass < 100; ++pass)
    {
        auto queue = ts::make_shared<std::queue<int32_t>>();
        constexpr int32_t element_count = 100;
        std::atomic_bool done{false};

        auto producer = [&queue, &done]()
        {
            for (int32_t i = 0; i < element_count; ++i)
            {
                queue->push(i);
            }
            done.store(true);
        };

        auto consumer = [&queue, &done]()
        {
            while (!done || !queue->empty())
            {
                std::lock_guard lock{queue};
                if (queue.get()->empty())
                {
                    continue;
                }
                queue.get()->pop();
            }
        };

        std::vector<std::thread> arr_threads;
        arr_threads.emplace_back(producer);
        for (int32_t i = 0; i < 4; ++i)
        {
            arr_threads.emplace_back(consumer);
        }

        std::ranges::for_each(arr_threads, std::mem_fn(&std::thread::join));
        ASSERT_EQ(0, queue->size());
    }
}

TEST(shared_ptr_thread_safety_testing, concurrent_copy_move_delete)
{
    {
        auto ts_ptr = ts::make_shared<dummy_object>();
        constexpr int32_t test_count = 100;

        auto copy_move_delete_task = [ts_ptr]()
        {
            std::vector<ts::shared_ptr<dummy_object>> ts_ptrs;
            for (int32_t i = 0; i < test_count; ++i)
            {
                ts_ptrs.push_back(ts_ptr);
            }

            for (int32_t i = 0; i < test_count; ++i)
            {
                auto tmp { std::move (ts_ptrs[i]) };
                ts_ptrs[i].reset();
                ts_ptrs[i] = std::move(ts_ptrs[std::size(ts_ptrs) - i - 1]);
                ts_ptrs[std::size(ts_ptrs) - i - 1].reset();
                ts_ptrs[std::size(ts_ptrs) - i - 1] = std::move (tmp);
            }

            for (int32_t i = 0; i < test_count; ++i)
            {
                // TODO need to revisit.
//                ts_ptrs.erase(ts_ptrs.begin());
                ts_ptrs.pop_back();
            }

        };

        std::vector<std::jthread> arr_threads;
        for (int32_t i = 0; i < 10; ++i)
        {
            arr_threads.emplace_back(copy_move_delete_task);
        }
    }
    ASSERT_EQ(dummy_object::ms_object_count, 0);
}

TEST(shared_ptr_api_testing, compare_operator)
{
    std::vector<ts::shared_ptr<int>> old_values;
    old_values.reserve(200);
    for (int32_t i = 0; i < 100; ++i)
    {
        auto ptr1 = ts::make_shared<int>();
        auto ptr2 = ts::make_shared<int>();
        ASSERT_EQ ((ptr1 == ptr2), (ptr1.get() == ptr2.get()));
        ASSERT_EQ ((ptr1 != ptr2), (ptr1.get() != ptr2.get()));
        ASSERT_EQ ((ptr1 < ptr2), (ptr1.get() < ptr2.get()));
        ASSERT_EQ ((ptr1 <= ptr2), (ptr1.get() <= ptr2.get()));
        ASSERT_EQ ((ptr1 > ptr2), (ptr1.get() > ptr2.get()));
        ASSERT_EQ ((ptr1 >= ptr2), (ptr1.get() >= ptr2.get()));

        old_values.push_back(std::move(ptr1));
        old_values.push_back(std::move(ptr2));
    }
}

TEST(shared_ptr_api_testing, self_compare)
{
    auto ptr = ts::make_shared<int>();
    ASSERT_TRUE(ptr == ptr);
    ASSERT_FALSE(ptr != ptr);
    ASSERT_FALSE(ptr < ptr);
    ASSERT_FALSE(ptr > ptr);
    ASSERT_TRUE(ptr <= ptr);
    ASSERT_TRUE(ptr >= ptr);
}

TEST(shared_ptr_api_testing, compares_a_empty_shared_ptr_and_nullptr)
{
    ts::shared_ptr<int> empty_ptr;
    ASSERT_TRUE(empty_ptr == nullptr);
    ASSERT_TRUE(nullptr == empty_ptr);
    ASSERT_FALSE(empty_ptr != nullptr);
    ASSERT_FALSE(nullptr != empty_ptr);

    // operator ==
    ASSERT_EQ((empty_ptr == nullptr), (empty_ptr.get() == nullptr));
    ASSERT_EQ((nullptr == empty_ptr), (nullptr == empty_ptr.get()));

    // operator !=
    ASSERT_EQ((empty_ptr != nullptr), (empty_ptr.get() != nullptr));
    ASSERT_EQ((nullptr != empty_ptr), (nullptr != empty_ptr.get()));

    // operator <
    ASSERT_EQ((empty_ptr < nullptr), (empty_ptr.get() < nullptr));
    ASSERT_EQ((nullptr < empty_ptr), (nullptr < empty_ptr.get()));

    // operator >
    ASSERT_EQ((empty_ptr > nullptr), (empty_ptr.get() > nullptr));
    ASSERT_EQ((nullptr > empty_ptr), (nullptr > empty_ptr.get()));

    // operator <=
    ASSERT_EQ((empty_ptr <= nullptr), (empty_ptr.get() <= nullptr));
    ASSERT_EQ((nullptr <= empty_ptr), (nullptr <= empty_ptr.get()));

    // operator >=
    ASSERT_EQ((empty_ptr >= nullptr), (empty_ptr.get() >= nullptr));
    ASSERT_EQ((nullptr >= empty_ptr), (nullptr >= empty_ptr.get()));
}

TEST(shared_ptr_api_testing, compares_a_initialized_and_empty_shared_ptr)
{
    auto initialized_ptr = ts::make_shared<int>();
    ts::shared_ptr<int> empty_ptr;

    // operator ==
    ASSERT_EQ((initialized_ptr == nullptr), (initialized_ptr == empty_ptr));
    ASSERT_EQ((initialized_ptr == nullptr), (initialized_ptr == empty_ptr));
    ASSERT_EQ((nullptr == initialized_ptr), (empty_ptr == initialized_ptr));
    ASSERT_EQ((nullptr == initialized_ptr), (empty_ptr == initialized_ptr));

    // operator !=
    ASSERT_EQ((initialized_ptr != nullptr), (initialized_ptr != empty_ptr));
    ASSERT_EQ((initialized_ptr != nullptr), (initialized_ptr != empty_ptr));
    ASSERT_EQ((nullptr != initialized_ptr), (empty_ptr != initialized_ptr));
    ASSERT_EQ((nullptr != initialized_ptr), (empty_ptr != initialized_ptr));

    // operator <
    ASSERT_EQ((initialized_ptr < nullptr), (initialized_ptr < empty_ptr));
    ASSERT_EQ((initialized_ptr < nullptr), (initialized_ptr < empty_ptr));
    ASSERT_EQ((nullptr < initialized_ptr), (empty_ptr < initialized_ptr));
    ASSERT_EQ((nullptr < initialized_ptr), (empty_ptr < initialized_ptr));

    // operator >
    ASSERT_EQ((initialized_ptr > nullptr), (initialized_ptr > empty_ptr));
    ASSERT_EQ((initialized_ptr > nullptr), (initialized_ptr > empty_ptr));
    ASSERT_EQ((nullptr > initialized_ptr), (empty_ptr > initialized_ptr));
    ASSERT_EQ((nullptr > initialized_ptr), (empty_ptr > initialized_ptr));

    // operator <=
    ASSERT_EQ((initialized_ptr <= nullptr), (initialized_ptr <= empty_ptr));
    ASSERT_EQ((initialized_ptr <= nullptr), (initialized_ptr <= empty_ptr));
    ASSERT_EQ((nullptr <= initialized_ptr), (empty_ptr <= initialized_ptr));
    ASSERT_EQ((nullptr <= initialized_ptr), (empty_ptr <= initialized_ptr));

    // operator >=
    ASSERT_EQ((initialized_ptr >= nullptr), (initialized_ptr >= empty_ptr));
    ASSERT_EQ((initialized_ptr >= nullptr), (initialized_ptr >= empty_ptr));
    ASSERT_EQ((nullptr >= initialized_ptr), (empty_ptr >= initialized_ptr));
    ASSERT_EQ((nullptr >= initialized_ptr), (empty_ptr >= initialized_ptr));
}

TEST(shared_ptr_api_testing, compares_a_initialized_shared_ptr_and_nullptr)
{
    auto initialized_ptr = ts::make_shared<int>();
    ASSERT_TRUE(initialized_ptr != nullptr);
    ASSERT_FALSE(initialized_ptr == nullptr);
    ASSERT_TRUE(nullptr != initialized_ptr);
    ASSERT_FALSE(nullptr == initialized_ptr);


    // operator ==
    ASSERT_EQ((initialized_ptr == nullptr), (initialized_ptr.get() == nullptr));
    ASSERT_EQ((nullptr == initialized_ptr), (nullptr == initialized_ptr.get()));

    // operator !=
    ASSERT_EQ((initialized_ptr != nullptr), (initialized_ptr.get() != nullptr));
    ASSERT_EQ((nullptr != initialized_ptr), (nullptr != initialized_ptr.get()));

    // operator <
    ASSERT_EQ((initialized_ptr < nullptr), (initialized_ptr.get() < nullptr));
    ASSERT_EQ((nullptr < initialized_ptr), (nullptr < initialized_ptr.get()));

    // operator >
    ASSERT_EQ((initialized_ptr > nullptr), (initialized_ptr.get() > nullptr));
    ASSERT_EQ((nullptr > initialized_ptr), (nullptr > initialized_ptr.get()));

    // operator <=
    ASSERT_EQ((initialized_ptr <= nullptr), (initialized_ptr.get() <= nullptr));
    ASSERT_EQ((nullptr <= initialized_ptr), (nullptr <= initialized_ptr.get()));

    // operator >=
    ASSERT_EQ((initialized_ptr >= nullptr), (initialized_ptr.get() >= nullptr));
    ASSERT_EQ((nullptr >= initialized_ptr), (nullptr >= initialized_ptr.get()));
}


int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

