#pragma once

static inline void setBit(uint8_t &value, uint8_t bit, bool enabled)
{
    if (enabled)
        value |= (1 << bit);
    else
        value &= ~(1 << bit);
}