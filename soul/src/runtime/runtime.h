#pragma once

#include "runtime/data.h"
#include "runtime/system.h"

namespace Soul {
    namespace Runtime {

        inline void Init(const Config& config) {
            System::Get().init(config);
        }

        inline void PushAllocator(Memory::Allocator* allocator) {
            System::Get().pushAllocator(allocator);
        }

        inline void PopAllocator() {
            System::Get().popAllocator();
        }

        inline Memory::Allocator* GetContextAllocator() {
            return System::Get().getContextAllocator();
        }

        inline TempAllocator* GetTempAllocator() {
            return System::Get().getTempAllocator();
        }

        inline void* Allocate(uint32 size, uint32 alignment) {
            return System::Get().allocate(size, alignment);
        }

        inline void Deallocate(void* addr, uint32 size) {
            return System::Get().deallocate(addr, size);
        }

        struct AllocatorInitializer {
            AllocatorInitializer() = delete;
            explicit AllocatorInitializer(Memory::Allocator* allocator) {
                PushAllocator(allocator);
            }

            void end() {
                PopAllocator();
            }
        };

        struct AllocatorZone{

            AllocatorZone() = delete;

            explicit AllocatorZone(Memory::Allocator* allocator) {
                PushAllocator(allocator);
            }

            ~AllocatorZone() {
                PopAllocator();
            }
        };

        #define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
        #define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
        #define SOUL_MEMORY_ALLOCATOR_ZONE(allocator) \
                Soul::Runtime::AllocatorZone STRING_JOIN2(allocatorZone, __LINE__)(allocator)
    }
}