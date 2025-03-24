#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    explicit RawMemory(size_t capacity);

    ~RawMemory();

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept;
    RawMemory& operator=(RawMemory&& rhs) noexcept;

    T* operator+(size_t offset) noexcept;
    const T* operator+(size_t offset) const noexcept;

    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;

    void Swap(RawMemory& other) noexcept;

    const T* GetAddress() const noexcept;
    T* GetAddress() noexcept;

    size_t Capacity() const;

private:
    T* buffer_ = nullptr;
    size_t capacity_ = 0;

    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n);

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept;
};

template <typename T>
class Vector {
public:
    Vector() = default;
    ~Vector();

    explicit Vector(size_t size);

    Vector(const Vector& other); 
    Vector(Vector&& other);

    Vector& operator=(const Vector& other);
    Vector& operator=(Vector&& other);

    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;

    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    size_t Capacity() const noexcept;
    size_t Size() const noexcept;

    template<typename Value>
    void PushBack(Value&& value);

    template <typename... Args>
    T& EmplaceBack(Args&&... args);

    template <typename... Args>
    iterator Emplace (const_iterator pos, Args&&... args);

    iterator Insert(const_iterator pos, const T& value);
    iterator Insert(const_iterator pos, T&& value);
    
    void PopBack() noexcept;
    
    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>);

    void Reserve(size_t new_capacity);
    void Resize(size_t new_size);

    void Swap(Vector& other);

    bool Empty();

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    void ReinicializationDataIn(RawMemory<T>& new_data);

    //Шаблонный метод не осуществляет освобождение памяти
    template<typename InIter, typename OutIter>
    void ReinicializationDataIn(InIter first, InIter last, OutIter result);
};

//-------------------------RAW_MEMORY---------------------
template <typename T>
inline  RawMemory<T>::RawMemory(size_t capacity) : buffer_(Allocate(capacity)), 
                                                   capacity_(capacity) {
}

template <typename T>
inline RawMemory<T>::~RawMemory() {
    Deallocate(buffer_);
}

template <typename T>
inline RawMemory<T>::RawMemory(RawMemory&& other) noexcept {
    this->Swap(other);
}

template <typename T>
inline RawMemory<T>& RawMemory<T>::operator=(RawMemory&& rhs) noexcept {
    if(this != &rhs) {
        Swap(rhs);
    }

    return *this;     
}

template <typename T>
inline T* RawMemory<T>::operator+(size_t offset) noexcept {
    // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
    assert(offset <= capacity_);
    return buffer_ + offset;
}

template <typename T>
inline const T* RawMemory<T>::operator+(size_t offset) const noexcept {
    return const_cast<RawMemory&>(*this) + offset;
}

template <typename T>
inline const T& RawMemory<T>::operator[](size_t index) const noexcept {
    return const_cast<RawMemory&>(*this)[index];
}

template <typename T>
inline T& RawMemory<T>::operator[](size_t index) noexcept {
    assert(index < capacity_);
    return buffer_[index];
}

template <typename T>
inline void RawMemory<T>::Swap(RawMemory& other) noexcept {
    std::swap(buffer_, other.buffer_);
    std::swap(capacity_, other.capacity_);
}

template <typename T>
inline const T* RawMemory<T>::GetAddress() const noexcept {
    return buffer_;
}

template <typename T>
inline T* RawMemory<T>::GetAddress() noexcept {
    return buffer_;
}

template <typename T>
inline size_t RawMemory<T>::Capacity() const {
    return capacity_;
}

template <typename T>
inline T* RawMemory<T>::Allocate(size_t n) {
    return n != 0 ? static_cast<T*>(operator new(n* sizeof(T))) : nullptr;
}

template <typename T>
inline void RawMemory<T>::Deallocate(T* buf) noexcept {
     operator delete(buf);
}

//-------------------------VECTOR---------------------
template <typename T>
inline Vector<T>::~Vector() {
    std::destroy_n(data_.GetAddress(), size_);
}

template <typename T>
inline Vector<T>::Vector(size_t size) : data_(size),
                                        size_(size) {
    std::uninitialized_value_construct_n(data_.GetAddress(), size);  
}

template <typename T>
inline Vector<T>::Vector(const Vector& other) : data_(other.size_),
                                                size_(other.size_) {
    std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());   
}

template <typename T>
inline Vector<T>::Vector(Vector&& other) {
    Swap(other);
}

template <typename T>
inline Vector<T>& Vector<T>::operator=(const Vector& other) {
    if (this != &other) {

        if (other.size_ > data_.Capacity()) {
            Vector new_data(other);
            Swap(new_data);
        } else {
            std::copy_n(other.data_.GetAddress(), std::min(other.size_, size_), data_.GetAddress());
            
            if(other.size_ < size_) {    
                std::destroy_n(data_+ other.size_, size_ - other.size_);
            } else {
                std::uninitialized_copy_n(other.data_ + size_, other.size_ - size_, data_ + size_);
            }
            size_ = other.size_;
        }
    }

    return *this;
}

template <typename T>
inline Vector<T>& Vector<T>::operator=(Vector&& other) {
    if(this != &other) {
        Swap(other);
    }

    return *this;
}

template <typename T>
inline const T& Vector<T>::operator[](size_t index) const noexcept {
    return const_cast<Vector&>(*this)[index];
}

template <typename T>
inline T& Vector<T>::operator[](size_t index) noexcept {
    assert(index < size_);
    return data_[index];
}

template <typename T>
inline typename Vector<T>::iterator Vector<T>::begin() noexcept {
    return data_.GetAddress();
}

template <typename T>
inline typename Vector<T>::iterator Vector<T>::end() noexcept {
    return data_ + size_;
}

template <typename T>
inline typename Vector<T>::const_iterator Vector<T>::begin() const noexcept {
    return data_.GetAddress();
}

template <typename T>
inline typename Vector<T>::const_iterator Vector<T>::end() const noexcept {
    return data_ + size_;
}

template <typename T>
inline typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
    return begin();
}

template <typename T>
inline typename Vector<T>::const_iterator Vector<T>::cend() const noexcept {
    return end();
}

template <typename T>
inline size_t Vector<T>::Capacity() const noexcept {
    return data_.Capacity();
}

template <typename T>
inline size_t Vector<T>::Size() const noexcept {
    return size_;
}

template <typename T>
template <typename Value>
inline void Vector<T>::PushBack(Value&& value) {
    EmplaceBack(std::forward<Value>(value));
}

template <typename T>
template <typename... Args>
inline T& Vector<T>::EmplaceBack(Args&&... args) {
    return *Emplace(data_ + size_, std::forward<Args>(args)...);
}

template <typename T>
template <typename... Args>
inline typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args&&... args) {
    assert(pos >= begin() && pos <= end());

    size_t range = pos - begin();
    if(size_ < data_.Capacity()) {

        if(pos == end()) {
            new(end()) T(std::forward<Args>(args)...);
        } else {
            T temp_value(std::forward<Args>(args)...);
            new (end()) T(std::forward<T>(data_[size_ - 1]));
            std::move_backward(iterator(pos), end() - 1, end());
            data_[range] = std::move(temp_value);
        }
    } else {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        new(new_data + range) T(std::forward<Args>(args)...);

        ReinicializationDataIn(begin(), iterator(pos), new_data.GetAddress());
        ReinicializationDataIn(iterator(pos), end(), new_data + range + 1);

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    ++size_;

    return data_ + range;
}

template <typename T>
inline typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, const T &value) {
    assert(pos >= begin() && pos <= end() );
    return Emplace(pos, value);
}

template <typename T>
inline typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, T&& value) {
    assert(pos >= begin() && pos <= end() );
    return Emplace(pos, std::move(value));
}

template <typename T>
inline void Vector<T>::PopBack() noexcept {
    assert(!Empty());

    std::destroy_at(data_ + (size_ - 1));
    --size_;
}

template <typename T>
inline typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
    assert(pos >= begin() && pos < end());

    std::move(iterator(pos) + 1, end(), iterator(pos));
    PopBack();

    return iterator(pos);
}

template <typename T>
inline void Vector<T>::Reserve(size_t new_capacity) {
    if (new_capacity <= data_.Capacity()) {
        return;
    }

    RawMemory<T> new_data(new_capacity);
    ReinicializationDataIn(new_data);     
}

template <typename T>
inline void Vector<T>::Resize(size_t new_size) {
    if(new_size <= size_) {
         std::destroy_n(data_ + new_size, size_ - new_size); 
    } else {
        Reserve(new_size);
        std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
    }

    size_ = new_size;
}

template <typename T>
inline void Vector<T>::Swap(Vector& other) {
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
}

template <typename T>
inline bool Vector<T>::Empty() {
    return size_ == 0;
}

template <typename T>
inline void Vector<T>::ReinicializationDataIn(RawMemory<T>& new_data) {
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
    } else {
        std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
    }

    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}

template <typename T>
template <typename InIter, typename OutIter>
inline void Vector<T>::ReinicializationDataIn(InIter first, InIter last, OutIter result) {
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move(first, last, result);
    } else {
        std::uninitialized_copy(first, last, result);
    }
}
