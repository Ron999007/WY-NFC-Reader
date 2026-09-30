#ifndef DELAY_H
#define DELAY_H

#include "NuMicro.h"

//! Delay in millisecond(s)
//! \param MilliSec time of delay, in millisecond(s)
void Delay_ms(uint32_t MilliSec);

//! Delay in microsecond(s)
//! \param MicroSec time of delay, in microsecond(s)
void Delay_us(uint32_t MicroSec);

//! Delay in second(s)
//! \param Second time of delay, in Second(s)
void Delay_s(uint32_t Second);

#endif