#pragma once

#include <ARM7TDMI/CpuTypes.hpp>
#include <ARM7TDMI/Registers.hpp>

namespace CPU
{
class ARM7TDMI
{
public:
    ARM7TDMI();

private:
    Registers registers_;
};

}  // namespace CPU
