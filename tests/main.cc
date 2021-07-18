/**
 * @file        main.cc
 * @author      Argishti Ayvazyan (ayvazyan.argishti@gmail.com)
 * @brief       Unit tests implementations.
 * @date        7/14/2021
 * @copyright   Copyright (c) 2021
 */


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

    int32_t m_value {0};
    static inline int32_t ms_object_count { 0 };
};

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


int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

