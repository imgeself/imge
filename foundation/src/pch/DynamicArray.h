#pragma once

template<typename Type>
struct DynamicArray
{
    u32 length;
    Type* data;
    ILinearAllocator* allocator;

    Type& operator[](u32 index);

    inline Type PopBack();
    inline Type PopFirst();
    inline void Append(Type item);
    inline void Insert(Type item, u32 index);
    inline void RemoveAt(u32 index);
    inline void Clear();
};

template<typename Type>
Type& DynamicArray<Type>::operator[](u32 index)
{
    return this->data[index];
}

template<typename Type>
inline static DynamicArray<Type> CreateDynamicArray(AllocatorAPI* allocatorAPI)
{
    DynamicArray<Type> result = {};
    result.allocator = allocatorAPI->CreateLinearAllocator(Gigabyte(1), VM_PAGE_SIZE);
    result.length = 0;
    result.data = (Type*) result.allocator->instance->pointer;
    return result;
}

template<typename Type>
inline static void DestroyDynamicArray(DynamicArray<Type>* array, AllocatorAPI* allocatorAPI)
{
    allocatorAPI->DestroyLinearAllocator(array->allocator);
    array->allocator = 0;
    array->data = 0;
    array->length = 0;
}

template<typename Type>
inline Type DynamicArray<Type>::PopBack()
{
    ASSERT(this->length > 0, "Array is empty");

    Type item = this->data[this->length - 1];
    this->allocator->Free(this->allocator->instance, sizeof(Type));
    this->length--;
    return item;
}

template<typename Type>
inline Type DynamicArray<Type>::PopFirst()
{
    ASSERT(this->length > 0, "Array is empty");

    Type item = this->data[0];
    // Shift array
    memcpy(this->data, this->data + 1, sizeof(Type) * (this->length - 1));
    this->allocator->Free(this->allocator->instance, sizeof(Type));
    this->length--;
    return item;
}

// Adding operations
template<typename Type>
inline void DynamicArray<Type>::Append(Type item)
{
    *((Type*) this->allocator->Alloc(this->allocator->instance, sizeof(Type))) = item;
    this->length++;
}

template<typename Type>
inline void DynamicArray<Type>::Insert(Type item, u32 index)
{
    ASSERT(index < this->length, "Index out of bounds");
    this->allocator->Alloc(this->allocator->instance, sizeof(Type));
    memcpy(this->data + index + 1, this->data + index, sizeof(Type) * (this->length - index));
    this->data[index] = item;
    this->length++;
}

template<typename Type>
inline void DynamicArray<Type>::RemoveAt(u32 index)
{
    ASSERT(index < this->length, "Index out of bounds");
    memcpy(this->data + index, this->data + index + 1, sizeof(Type) * (this->length - index - 1));
    this->allocator->Free(this->allocator->instance, sizeof(Type));
}

template<typename Type>
inline void DynamicArray<Type>::Clear()
{
    this->length = 0;
    this->allocator->Free(this->allocator->instance, this->allocator->instance->commitedSize - VM_PAGE_SIZE);
    this->allocator->ClearMemory(this->allocator->instance);
}