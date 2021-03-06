# Thread-safe smart pointers for C++

The thread-safe smart pointers for concurrently using objects. Pointers are implemented using the Execute Around Pointer Idiom. The library provides ts::unique_ptr and ts::shared_ptr. Thats fully protected from data race and can help to solve API race problems.

## ts::shared_ptr

### ts::shared_ptr provides sharing the object between many threads.

ts::shared_ptr is a smart pointer that retains shared thread-safe ownership of an object through a pointer. Several shared_ptr objects may own the same object. The object is destroyed and its memory deallocated when either of the following happens:
* the last remaining shared_ptr owning the object is destroyed.
* the last remaining shared_ptr owning the object is assigned another pointer via operator = or reset().

The ts::shared_ptr has optimization for working with read-write locks.

The all methods called using structure dereference or subscript operators will be executed under the protection of a mutex.



## ts::shared_ptr documentation

ts::shared_ptr signature

```c++
template <typename T, typename TMutex = std::mutex>
class shared_ptr { ... };
```
template parameters:
* **T** - is the type of element or array of elements.
* **TMutex** - is the type of mutex (optional by default std::mutex).

### Member types
| **Member type** | **Definition** |
| --- | --- |
| element_type | std::remove_extent_t\<T\>, the type of the object managed by this shared_ptr. |
| mutex_type | TMutex, the type of mutex. |

### Member functions
| **Name** | **Description** |
| --- | --- |
| (constructor) | Constructs new shared_ptr |
| (destructor) | Destructs the owned object if no more shared_ptrs link to it |
| operator= | Assigns or move the shared_ptr |
| reset | Replaces the managed object and sets new if that is given. |
| get | Returns the stored pointer |
| operator-> | Dereferences the stored pointer |
| operator[] | provides indexed access to the stored array |
| operator bool | Checks if the stored pointer is not null |


### Non-member functions
| **Name** | **Description** |
| --- | --- |
| make_shared | Creates a shared pointer that manages a new object |
|operator==, operator<=> | Compares with another shared_ptr or with nullptr |

### make_shared
```c++
template <class T, class... TArgs>
ts::shared_ptr<T> make_shared(TArgs&&... args)
```
Constructs an object of type T and wraps it in a ts::shared_ptr. (Specialization for a single object.)

Template parameters:
* **T** - is the type of element.
* **TArgs** - is the types of list of arguments with which an instance of T will be constructed.

Parameters:
* **args** - is the list of arguments with which an instance of T will be constructed.

Return value:
* **ts::shared_ptr** of an instance of type T.

```c++
template <class T>
ts::shared_ptr<T> ts::make_shared(std::size_t n)
```
Constructs an object of type T array and wraps it in a ts::shared_ptr. (Specialization for a objects array.)

Template parameters:
* **T** - is the type of elements array.

Parameters:
* **n** - is the length of the array to construct.

Return value:
* **ts::shared_ptr** of an instance of type T.

  

## The use examples
The simple use examples:
```c++
ts::shared_ptr<std::vector<int>> p_vec { new std::vector<int>{} };
p_vec->push_back(13);
```
```c++
ts::shared_ptr<std::vector<int>> p_vec = ts::make_shared<std::vector<int>>();
p_vec->push_back(13);
```
The use example with shared_mutex:
```c++
ts::shared_ptr<std::vector<int>, std::shared_mutex> mutable_ptr { new std::vector<int> };
mutable_ptr->push_back(13); // On mutable ptr working unique_lock.

ts::shared_ptr<const std::vector<int>, std::shared_mutex> const_ptr{mutable_ptr};
const_ptr->size(); // On const ptr working shared_lock.
```

The use example with arrays:
```c++
auto ptr = ts::make_shared<int32_t[]>(100);
(*ptr)[1] = 12;
auto val = (*ptr)[2];
```
| :warning:  Do not take any reference using subscript operator. All saved references are not thread-safe.  |
|-----------------------------------------|

The use example custom mutex:
```c++
ts::shared_ptr<std::vector<int>, boost::sync::spin_mutex> p_vec { new std::vector<int>{} };
p_vec->push_back(13);
```

The use example with custom deleter:
```c++
ts::shared_ptr<std::vector<int>> p_vec { new std::vector<int>{}, [](auto* vec) { delete vec; } };
p_vec->push_back(13);
```

| :warning: Structure dereference or subscript operators cannot protect object from API races. To safe object from API races, you can use lock, unlock and get API's. The examples below:  |
|-----------------------------------------|

```c++
auto queue = ts::make_shared<std::queue<int32_t>>();
// do something
{
    queue.lock();
    if (queue.get()->empty())
    {
        queue.get()->pop();
    }
    queue->unlock();
}
```
or

```c++
auto queue = ts::make_shared<std::queue<int32_t>>();
// do something
{
    std::lock_guard lock{queue};
    if (queue.get()->empty())
    {
        queue.get()->pop();
    }
}
```
If an element or array type (T) is const, and ts::shared_ptr uses shared_mutex then it's possible to use std::shared_lock. It locks the mutex for shared ownership.
```c++
auto queue = ts::make_shared<std::vector<int32_t>>();
// do something
{
    std::lock_guard lock{queue};
    if (queue.get()->empty())
    {
        foo(queue.get()->top());
        // queue.get()->pop(); // is not available. 
    }
}
```

## ts::unique_ptr

### ts::unique_ptr provides the single object thread-safe usage.

ts::unique_ptr is a thread-safe smart pointer that owns and manages and provides thread safety of another object through a pointer and disposes of that object when the ts::unique_ptr goes out of scope.

The ts::unique_ptr implemented using Execute Around Pointer Idiom. The all methods called using structure dereference or subscript operators will be executed under the protection of a mutex.

## ts::unique_ptr documentation

ts::unique_ptr signature
```c++
template <typename T, typename TMutex = std::mutex, typename TDeleter = std::default_delete<T>>
class unique_ptr { ... };
```
template parameters:
* **T** - is the type of element or array of elements.
* **TMutex** - is the type of mutex (optional by default std::mutex).
* **TDeleter** - is the type of deleter (optional by default std::default_delete<T>)

### Member types
| **Member type** | **Definition** |
| --- | --- |
| pointer | std::remove_reference<Deleter>::type::pointer if that type exists, otherwise T*. Must satisfy NullablePointer |
| element_type | std::remove_extent_t\<T\>, the type of the object managed by this unique_ptr. |
| deleter_type | Deleter, the function object or lvalue reference to function or to function object, to be called from the destructor. |
| mutex_type | TMutex, the type of mutex. |

### Member functions
| **Name** | **Description** |
| --- | --- |
| (constructor) | Constructs new shared_ptr |
| (destructor) | Destructs the owned object if no more shared_ptrs link to it |
| operator= | Assigns or move the shared_ptr |
| release | Releases the ownership of the managed object and returns owned object pointer. |
| reset | Replaces the managed object and sets new if that is given. |
| get | Returns the stored pointer |
| get_deleter | Returns the deleter object which would be used for destruction of the managed object. |
| operator-> | Dereferences the stored pointer |
| operator[] | provides indexed access to the stored array |
| operator bool | Checks if the stored pointer is not null |


### Non-member functions
| **Name** | **Description** |
| --- | --- |
| make_unique | creates a unique pointer that manages a new object |
|operator==, operator<=> | Compares with another shared_ptr or with nullptr |

### make_unique

```c++
template <class T, class... TArgs>
ts::unique_ptr<T> ts::make_unique(TArgs&&... args)
```
Constructs an object of type T and wraps it in a ts::unique_ptr. (Specialization for a single object.)

Template parameters:
* **T** - is the type of element.
* **TArgs** - is the types of list of arguments with which an instance of T will be constructed.

Parameters:
* **args** - is the list of arguments with which an instance of T will be constructed.

Return value:
* **ts::unique_ptr** of an instance of type T.

```c++
template <class T>
ts::unique_ptr<T> ts::make_unique(std::size_t n)
```
Constructs an object of type T array and wraps it in a ts::unique_ptr. (Specialization for a objects array.)

Template parameters:
* **T** - is the type of elements array.

Parameters:
* **n** - is the length of the array to construct.

Return value:
* **ts::unique_ptr** of an instance of type T.

## The use examples
The simple use examples:

```c++
ts::unique_ptr<std::vector<int>> p_vec { new std::vector<int>{} };
p_vec->push_back(13);
```
```c++
auto p_vec = ts::make_unique<std::vector<int>>();
p_vec->push_back(13);
```

The use example with arrays:
```c++
auto ptr = ts::make_unique<int32_t[]>(100);
(*ptr)[1] = 12;
auto val = (*ptr)[2];
```

| :warning:  Do not take any reference using subscript operator. All saved references are not thread-safe.  |
|-----------------------------------------|

The use example custom mutex:
```c++
ts::unique_ptr<std::vector<int>, boost::sync::spin_mutex> p_vec { new std::vector<int>{} };
p_vec->push_back(13);
```

The use example with custom deleter:
```c++
ts::unique_ptr<std::vector<int>, std::mutex, std::function<void(std::vector<int>*)>> 
            p_vec { new std::vector<int>{}, [](auto* vec) { delete vec; } };
p_vec->push_back(13);
```


| :warning: Structure dereference or subscript operators cannot protect object from API races. To safe object from API races, you can use lock, unlock and get API's. The examples below:  |
|-----------------------------------------|

```c++
auto queue = ts::make_unique<std::queue<int32_t>>();
// do something
{
    queue.lock();
    if (queue.get()->empty())
    {
        queue.get()->pop();
    }
    queue->unlock();
}
```
or

```c++
auto queue = ts::make_unique<std::queue<int32_t>>();
// do something
{
    std::lock_guard lock{queue};
    if (queue.get()->empty())
    {
        queue.get()->pop();
    }
}
```

## Building:

### Release build:

```bash
mkdir build
cd ./build
cmake ..
make -j <job count>
```

### Build unit tests.
```bash
mkdir build
cd ./build
cmake -DBUILD_UNIT_TESTS=YES ..
make -j <job count>
```

