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
        using deleted_unique_ptr = ts::unique_ptr<dummy_object
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
        using deleted_unique_ptr = ts::unique_ptr<dummy_object
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

