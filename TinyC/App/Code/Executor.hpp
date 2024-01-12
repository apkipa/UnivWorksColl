#pragma once

#include "Logger.hpp"
#include <vector>
#include <atomic>

namespace CTinyC {
    enum ByteCodeType : uint8_t {
        DebugInterrupt = 0,
        PushDword,
        PopDword,
        DuplicateDword,
        //PushVarRef,             // Can be used to obtain frame pointer
        PushStackRef,
        AdjustStackRefConst,
        ReadRefDword,
        WriteRefDword,
        Call,
        CallIndirect,
        Ret,
        RetDword,
        Jump,
        JumpCond,
        Add,
        Sub,
        Mul,
        Div,
        CmpG,
        CmpGe,
        CmpE,
        CmpNe,
        CmpL,
        CmpLe,

        // Requires a packed dword describing callee ABI
        // 0: EOF, 1: u32, 2: u64, 3: uintptr_t, 4: f64
        // At most 32 / 4 = 8 arguments
        FfiCall,
        // Requires a dword specifying syscall ID
        // 0 => halt, 1 => putchar, 2 => getchar, 3 => input, 4 => output
        SysCall,
    };

    // NOTE: Stack type is full-descending
    struct Executor {
        Executor(Logger* logger) : m_logger(logger), m_halted(true) {}

        void load(void* bytecode, size_t len, size_t memory_size, size_t start_offset);
        void set_ip(size_t ip) {
            m_ip = checked_get_vm_mem_ptr(ip);
        }
        // Returns whether VM can continue running (i.e. not halted)
        bool execute(size_t max_count, std::atomic_bool const& interrupt_flag);

    private:
        size_t checked_get_vm_mem_ptr(size_t ptr, int32_t offset = 0) {
            if (offset > 0 && static_cast<size_t>(offset) > size(m_memory)) {
                throw std::runtime_error("VM memory pointer out of bounds");
            }
            if (offset < 0 && static_cast<size_t>(-static_cast<int64_t>(offset)) > size(m_memory)) {
                throw std::runtime_error("VM memory pointer out of bounds");
            }
            if (ptr > size(m_memory)) {
                throw std::runtime_error("VM memory pointer out of bounds");
            }
            ptr += static_cast<size_t>(offset);
            if (ptr > size(m_memory)) {
                throw std::runtime_error("VM memory pointer out of bounds");
            }
            return ptr;
        }
        uint32_t checked_read_vm_mem_dword(size_t ptr) {
            if (ptr >= size(m_memory) - 3) {
                throw std::runtime_error("VM memory read out of bounds");
            }
            uint32_t v{};
            v = (v << 8) + m_memory[ptr + 3];
            v = (v << 8) + m_memory[ptr + 2];
            v = (v << 8) + m_memory[ptr + 1];
            v = (v << 8) + m_memory[ptr + 0];
            return v;
        }
        void checked_write_vm_mem_dword(size_t ptr, uint32_t v) {
            if (ptr >= size(m_memory) - 3) {
                throw std::runtime_error("VM memory write out of bounds");
            }
            for (size_t i = 0; i < 4; i++) {
                m_memory[ptr] = v & 0xff;
                v >>= 8;
                ptr++;
            }
        }
        uint8_t checked_read_vm_mem_byte(size_t ptr) {
            if (ptr >= size(m_memory)) {
                throw std::runtime_error("VM memory read out of bounds");
            }
            return m_memory[ptr];
        }
        void checked_write_vm_mem_byte(size_t ptr, uint8_t v) {
            if (ptr >= size(m_memory)) {
                throw std::runtime_error("VM memory write out of bounds");
            }
            m_memory[ptr] = v;
        }

        void execute_syscall();

        Logger* m_logger;
        bool m_halted;
        size_t m_ip{}, m_sp{};
        std::vector<uint8_t> m_memory;
    };
}
