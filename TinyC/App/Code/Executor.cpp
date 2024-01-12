#include "pch.h"

#include "Executor.hpp"

//#include <ffi.h>

namespace CTinyC {
    const size_t STACK_SIZE = 1024ull * 1024 * 4;

    void Executor::load(void* bytecode, size_t len, size_t memory_size, size_t start_offset) {
        if (len + start_offset + STACK_SIZE > memory_size) {
            throw std::invalid_argument("VM memory too small");
        }
        m_memory.clear();
        m_memory.resize(memory_size, 0x0);
        std::memcpy(m_memory.data() + start_offset, bytecode, len);
        m_halted = false;
        m_ip = start_offset;
        m_sp = size(m_memory) - 20;
        checked_write_vm_mem_dword(m_sp + 9, 0);
        checked_write_vm_mem_byte(m_sp + 8, ByteCodeType::SysCall);
        checked_write_vm_mem_dword(m_sp + 4, 0);
        checked_write_vm_mem_dword(m_sp, (uint32_t)(m_sp + 8));
    }
    bool Executor::execute(size_t max_count, std::atomic_bool const& interrupt_flag) try {
        if (m_halted) { return false; }
        size_t exec_inst_cnt{};
        while (exec_inst_cnt++ < max_count && !interrupt_flag.load(std::memory_order_relaxed)) {
            auto bytecode_type = static_cast<ByteCodeType>(checked_read_vm_mem_byte(m_ip));
            m_ip = checked_get_vm_mem_ptr(m_ip, 1);
            uint32_t tmp_dw1, tmp_dw2;

            m_logger->debug(std::format(L"Decoding instruction {} at {}(0x{:08x}), sp = {}",
                (uint32_t)bytecode_type, m_ip - 1, m_ip - 1, m_sp));

            switch (bytecode_type) {
            case ByteCodeType::DebugInterrupt:
                m_logger->debug(L"DebugInterrupt");
                goto end_loop;
            case ByteCodeType::PushDword:
                tmp_dw1 = checked_read_vm_mem_dword(m_ip);
                m_ip = checked_get_vm_mem_ptr(m_ip, 4);
                m_logger->debug(std::format(L"PushDword {}", tmp_dw1));
                m_sp = checked_get_vm_mem_ptr(m_sp, -4);
                checked_write_vm_mem_dword(m_sp, tmp_dw1);
                break;
            case ByteCodeType::PopDword:
                m_logger->debug(L"PopDword");
                m_sp = checked_get_vm_mem_ptr(m_sp, 4);
                break;
            case ByteCodeType::DuplicateDword:
                m_logger->debug(L"DuplicateDword");
                tmp_dw1 = checked_read_vm_mem_dword(m_sp);
                m_sp = checked_get_vm_mem_ptr(m_sp, -4);
                checked_write_vm_mem_dword(m_sp, tmp_dw1);
                break;
            case ByteCodeType::PushStackRef:
                m_logger->debug(L"PushStackRef");
                tmp_dw1 = m_sp;
                m_sp = checked_get_vm_mem_ptr(m_sp, -4);
                checked_write_vm_mem_dword(m_sp, tmp_dw1);
                break;
            case ByteCodeType::AdjustStackRefConst:
                tmp_dw1 = checked_read_vm_mem_dword(m_ip);
                m_ip = checked_get_vm_mem_ptr(m_ip, 4);
                m_logger->debug(std::format(L"AdjustStackRefConst {}", tmp_dw1));
                m_sp = checked_get_vm_mem_ptr(m_sp, tmp_dw1);
                break;
            case ByteCodeType::ReadRefDword:
                tmp_dw1 = checked_read_vm_mem_dword(m_sp);
                tmp_dw2 = checked_read_vm_mem_dword(tmp_dw1);
                m_logger->debug(std::format(L"ReadRefDword ({} -> {})", tmp_dw1, tmp_dw2));
                checked_write_vm_mem_dword(m_sp, tmp_dw2);
                break;
            case ByteCodeType::WriteRefDword:
                tmp_dw1 = checked_read_vm_mem_dword(m_sp);
                m_sp = checked_get_vm_mem_ptr(m_sp, 4);
                tmp_dw2 = checked_read_vm_mem_dword(m_sp);
                m_sp = checked_get_vm_mem_ptr(m_sp, 4);
                m_logger->debug(std::format(L"WriteRefDword ({} -> {})", tmp_dw1, tmp_dw2));
                checked_write_vm_mem_dword(tmp_dw2, tmp_dw1);
                break;

            case ByteCodeType::Call:
                tmp_dw1 = checked_read_vm_mem_dword(m_ip);  // 读取调用的指令地址
                m_logger->debug(std::format(L"Call {}", tmp_dw1));
                m_ip = checked_get_vm_mem_ptr(m_ip, 4);  // 增加指令指针
                m_sp = checked_get_vm_mem_ptr(m_sp, -4);  // 递减堆栈指针来存储调用后的位置
                checked_write_vm_mem_dword(m_sp, m_ip);  // 写回调用后的指令地址
                m_ip = tmp_dw1;  // 跳转到指令的位置
                break;
            case ByteCodeType::CallIndirect:
                tmp_dw1 = checked_read_vm_mem_dword(m_sp);
                m_logger->debug(std::format(L"CallIndirect ({})", tmp_dw1));
                checked_write_vm_mem_dword(m_sp, m_ip);  // 写回调用后的指令地址
                m_ip = tmp_dw1;  // 跳转到指令的位置
                break;
            case ByteCodeType::Ret:
                tmp_dw1 = checked_read_vm_mem_dword(m_sp);
                m_logger->debug(std::format(L"Ret ({})", tmp_dw1));
                m_sp = checked_get_vm_mem_ptr(m_sp, 4);
                m_ip = tmp_dw1;
                break;
            case ByteCodeType::RetDword:
                tmp_dw1 = checked_read_vm_mem_dword(m_sp);
                m_sp = checked_get_vm_mem_ptr(m_sp, 4);
                tmp_dw2 = checked_read_vm_mem_dword(m_sp);
                checked_write_vm_mem_dword(m_sp, tmp_dw1);
                m_logger->debug(std::format(L"RetDword (v={}, retaddr={})",
                    tmp_dw1, tmp_dw2));
                m_ip = tmp_dw2;
                break;

            case ByteCodeType::Jump:
                tmp_dw1 = checked_read_vm_mem_dword(m_ip);
                m_logger->debug(std::format(L"Jump {}", tmp_dw1));
                m_ip = checked_get_vm_mem_ptr(tmp_dw1);  // 无条件跳转到字节码指示的位置
                break;

            case ByteCodeType::JumpCond:
                tmp_dw1 = checked_read_vm_mem_dword(m_ip);  // 读取跳转地址
                m_logger->debug(std::format(L"JumpCond {}", tmp_dw1));
                m_ip = checked_get_vm_mem_ptr(m_ip, 4);  // 增加指令指针
                tmp_dw2 = checked_read_vm_mem_dword(m_sp);  // 读取条件
                m_sp = checked_get_vm_mem_ptr(m_sp, 4);  // 增加堆栈指针
                if (tmp_dw2 != 0) {  // 如果条件不为0，则跳转
                    m_ip = checked_get_vm_mem_ptr(tmp_dw1);
                }
                break;

            case ByteCodeType::Add:
            case ByteCodeType::Sub:
            case ByteCodeType::Mul:
            case ByteCodeType::Div:
            case ByteCodeType::CmpG:
            case ByteCodeType::CmpGe:
            case ByteCodeType::CmpE:
            case ByteCodeType::CmpNe:
            case ByteCodeType::CmpL:
            case ByteCodeType::CmpLe:
                tmp_dw2 = checked_read_vm_mem_dword(m_sp);  // 取出栈顶元素
                m_sp = checked_get_vm_mem_ptr(m_sp, 4);  // 增加堆栈指针
                tmp_dw1 = checked_read_vm_mem_dword(m_sp);  // 取出下一个栈顶元素
                switch (bytecode_type) {
                case ByteCodeType::Add:
                    m_logger->debug(L"Add");
                    tmp_dw1 += tmp_dw2;
                    break;
                case ByteCodeType::Sub:
                    m_logger->debug(L"Sub");
                    tmp_dw1 -= tmp_dw2;
                    break;
                case ByteCodeType::Mul:
                    m_logger->debug(L"Mul");
                    tmp_dw1 *= tmp_dw2;
                    break;
                case ByteCodeType::Div:
                    m_logger->debug(L"Div");
                    if (tmp_dw2 == 0) { throw std::runtime_error("division by zero"); }
                    tmp_dw1 /= tmp_dw2;
                    break;
                    // 接下来是比较运算指令的实现...
                    // ...
                case ByteCodeType::CmpG:
                    m_logger->debug(L"CmpG");
                    tmp_dw1 = (int32_t)tmp_dw1 > (int32_t)tmp_dw2 ? 1 : 0;  // 执行比较，存储结果
                    break;
                case ByteCodeType::CmpGe:
                    m_logger->debug(L"CmpGe");
                    tmp_dw1 = (int32_t)tmp_dw1 >= (int32_t)tmp_dw2 ? 1 : 0;
                    break;
                case ByteCodeType::CmpE:
                    m_logger->debug(L"CmpE");
                    tmp_dw1 = (int32_t)tmp_dw1 == (int32_t)tmp_dw2 ? 1 : 0;
                    break;
                case ByteCodeType::CmpNe:
                    m_logger->debug(L"CmpNe");
                    tmp_dw1 = (int32_t)tmp_dw1 != (int32_t)tmp_dw2 ? 1 : 0;
                    break;
                case ByteCodeType::CmpL:
                    m_logger->debug(L"CmpL");
                    tmp_dw1 = (int32_t)tmp_dw1 < (int32_t)tmp_dw2 ? 1 : 0;
                    break;
                case ByteCodeType::CmpLe:
                    m_logger->debug(L"CmpLe");
                    tmp_dw1 = (int32_t)tmp_dw1 <= (int32_t)tmp_dw2 ? 1 : 0;
                    break;
                }
                checked_write_vm_mem_dword(m_sp, tmp_dw1);  // 结果写回栈顶
                break;

            case ByteCodeType::FfiCall:
                m_logger->debug(L"FfiCall");
                throw std::runtime_error("FfiCall is not supported");
            case ByteCodeType::SysCall:
                execute_syscall();
                break;
            default:
                throw std::runtime_error("unrecognized bytecode");
            }

            if (m_halted) { break; }
        }
end_loop:
        return !m_halted;
    }
    catch (std::runtime_error const& e) {
        m_halted = true;
        m_logger->error(std::format(L"VM PANIC: {}", winrt::to_hstring(e.what())));
        throw;
    }
    catch (...) {
        m_halted = true;
        throw;
    }
    void Executor::execute_syscall() {
        auto call_num = checked_read_vm_mem_dword(m_ip);
        m_logger->debug(std::format(L"Syscall, id = {}", call_num));
        m_ip = checked_get_vm_mem_ptr(m_ip, 4);
        uint32_t tmp_dw1, tmp_dw2;

        switch (call_num) {
        case 0:
            m_halted = true;
            return;
        case 1:
            tmp_dw1 = checked_read_vm_mem_dword(m_sp);
            m_sp = checked_get_vm_mem_ptr(m_sp, 4);
            putchar(tmp_dw1);
            return;
        case 2:
            tmp_dw1 = getchar();
            m_sp = checked_get_vm_mem_ptr(m_sp, -4);
            checked_write_vm_mem_dword(m_sp, tmp_dw1);
            return;
        case 3:     // input
            scanf("%d", &tmp_dw1);
            m_sp = checked_get_vm_mem_ptr(m_sp, -4);
            checked_write_vm_mem_dword(m_sp, tmp_dw1);
            return;
        case 4:     // output
            tmp_dw1 = checked_read_vm_mem_dword(m_sp);
            m_sp = checked_get_vm_mem_ptr(m_sp, 4);
            printf("%d ", tmp_dw1);
            return;
        default:
            throw std::runtime_error("unrecognized syscall");
        }
    }
}
