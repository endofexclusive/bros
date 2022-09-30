/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022 Martin Åberg */

#include <priv.h>

struct stm32_usart {
  unsigned sr;
  unsigned dr;
  unsigned bbr;
  unsigned cr1;
  unsigned cr2;
  unsigned cr3;
  unsigned gtpr;
};

#define STM32_USART_SR_RXNE (1U <<  5)
#define STM32_USART_SR_TXE  (1U <<  7)
#define STM32_USART_CR1_UE  (1U << 13)
#define STM32_USART_CR1_TE  (1U <<  3)
#define STM32_USART_CR1_RE  (1U <<  2)

/* usart{1,2,3} @ {40013800,40004400,40004800} */
static volatile struct stm32_usart *const usart =
 (void *) 0x40013800;

void iRawIOInit(Lib *lib) {
  usart->cr1 = 0;
  usart->cr2 = 0;
  usart->cr3 = 0;
  usart->cr1 = (0
    | STM32_USART_CR1_UE
    | STM32_USART_CR1_TE
    | STM32_USART_CR1_RE
  );
}

void iRawPutChar(Lib *lib, int c) {
  if (c == '\n') {
    iRawPutChar(lib, '\r');
  }
  while (!(usart->sr & STM32_USART_SR_TXE)) {
    for (int i = 0; i < 12; i++) {
      ;
    }
  }
  usart->dr = (unsigned) c & 0xff;
}

int iRawMayGetChar(Lib *lib) {
  if ((usart->sr & STM32_USART_SR_RXNE) == 0) {
    return -1;
  }
  return (unsigned char) usart->dr & 0xff;
}

