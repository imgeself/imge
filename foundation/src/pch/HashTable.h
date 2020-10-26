#pragma once

#define HASH_TABLE_UNUSED -1

template<typename K, typename V>
struct TableNode
{
    i8 metadata; // Robin hood hashing
    Hash64 hash;
    V value;
};

template<typename K, typename V>
struct HashTable
{
    u32 length;
    u32 capacity;
    TableNode<K,V>* data;
    ILinearAllocator* allocator;

    // Sets value, replace if there is a value with assocatied key
    void Set(K key, V value);
    // Return true if table contains the key
    bool Get(K key, V* outValue);
    void Remove(K key);

private:
    template<typename T = K>
    inline Hash64 GetHash64(T data)
    {
        return (Hash64) XXH64(&data, sizeof(T), 0);
    }

    template<typename T = K>
    inline Hash64 GetHash64(const char* key)
    {
        u64 len = strlen(key);
        return XXH64(key, len, 0);
    }

};

template<typename K, typename V>
inline static HashTable<K, V> CreateHashTable(AllocatorAPI* allocatorAPI, u32 initialCount = 4096)
{
    u64 allocationSize = initialCount * sizeof(TableNode<K,V>);
    u64 commitSize = allocationSize + (allocationSize % VM_PAGE_SIZE);
    HashTable<K, V> result = {};
    result.allocator = allocatorAPI->CreateLinearAllocator(Gigabyte(1), commitSize);
    result.capacity = initialCount;
    result.length = 0;
    result.data = (TableNode<K,V>*) result.allocator->Alloc(result.allocator->instance, allocationSize);
    for (u32 i = 0; i < result.capacity; ++i)
    {
        TableNode<K,V>* node = result.data + i;
        node->metadata = HASH_TABLE_UNUSED;
    }
    return result;
}

template<typename K, typename V>
inline static void CreateHashTable(HashTable<K, V>* table, AllocatorAPI* allocatorAPI)
{
    allocatorAPI->DestroyLinearAllocator(table->allocator);
    table->allocator = 0;
    table->data = 0;
    table->length = 0;
    table->capacity = 0;
}

template<typename K, typename V>
inline void HashTable<K,V>::Set(K key, V value)
{ 
    Hash64 hash = GetHash64(key);
    u64 index = hash % this->capacity;

    TableNode<K,V> insertNode = {};
    insertNode.metadata = 0;
    insertNode.hash = hash;
    insertNode.value = value;

    while (this->data[index].metadata != HASH_TABLE_UNUSED)
    {
        TableNode<K,V>* node = this->data + index;
        if (node->hash == hash) {
            // Value override
            node->value = value;
            return;
        }

        if (node->metadata < insertNode.metadata)
        {
            SwapMemory(node, &insertNode, sizeof(TableNode<K,V>));
        }

        index = (index + 1) % this->capacity;
        insertNode.metadata++;
    }

    TableNode<K,V>* node = this->data + index;
    *node = insertNode;
    this->length++;

    float loadFactor = (float) this->length / (float) this->capacity;
    if (loadFactor > 0.75)
    {
        // Table doubling
        u64 newCapacity = this->capacity * 2;
        u64 allocationSize = newCapacity * sizeof(TableNode<K,V>);
        this->allocator->Alloc(this->allocator->instance, allocationSize);

        TableNode<K,V>* oldTable = this->data + newCapacity;
        TableNode<K,V>* newTable = this->data;
        memcpy(oldTable, this->data, (allocationSize / 2));

        // Clear new table
        for (u64 i = 0; i < newCapacity; ++i)
        {
            TableNode<K,V>* node = newTable + i;
            node->metadata = HASH_TABLE_UNUSED;
        }

        // Insert existing nodes from old table to new table
        for (u64 i = 0; i < this->capacity; ++i)
        {
            TableNode<K,V>* node = oldTable + i;
            if (node->metadata != HASH_TABLE_UNUSED)
            {
                Hash64 hash = node->hash;
                u64 index = hash % newCapacity;

                TableNode<K,V> insertNode = {};
                insertNode.metadata = 0;
                insertNode.hash = hash;
                insertNode.value = node->value;

                while (this->data[index].metadata != HASH_TABLE_UNUSED)
                {
                    TableNode<K,V>* node = this->data + index;
                    if (node->hash == hash) {
                        // Value override
                        node->value = value;
                        return;
                    }

                    if (node->metadata < insertNode.metadata)
                    {
                        SwapMemory(node, &insertNode, sizeof(TableNode<K,V>));
                    }

                    index = (index + 1) % newCapacity;
                    insertNode.metadata++;
                }

                TableNode<K,V>* node = this->data + index;
                *node = insertNode;
            }
        }

        this->capacity *= 2;
        this->allocator->Free(this->allocator->instance, allocationSize / 2);
    }
}

template<typename K, typename V>
inline bool HashTable<K,V>::Get(K key, V* outValue)
{
    Hash64 hash = GetHash64(key);
    u64 index = hash % this->capacity;

    do {
        TableNode<K,V>* node = this->data + index;
        if (node->hash == hash)
        {
            *outValue = node->value;
            return true;
        }
        index = (index + 1) % this->capacity;
    } while (this->data[index].metadata != HASH_TABLE_UNUSED);

    return false;
}

template<typename K, typename V>
inline void HashTable<K,V>::Remove(K key)
{
    Hash64 hash = GetHash64(key);
    u64 index = hash % this->capacity;

    do {
        TableNode<K,V>* node = this->data + index;
        if (node->hash == hash)
        {
            node->metadata = HASH_TABLE_UNUSED;
            node->hash = 0;
            break;
        }
        index = (index + 1) % this->capacity;
    } while (this->data[index].metadata != HASH_TABLE_UNUSED);
    // TODO: Shrink the table

    do {
        TableNode<K,V>* nextNode = this->data + ((index + 1) % this->capacity);
        if (nextNode->metadata > 0)
        {
            nextNode->metadata--;
            SwapMemory(nextNode, (this->data + index), sizeof(TableNode<K,V>));
        }
        else
        {
            break;
        }

        index = (index + 1) % this->capacity;
    } while (this->data[index + 1].metadata != HASH_TABLE_UNUSED);
}