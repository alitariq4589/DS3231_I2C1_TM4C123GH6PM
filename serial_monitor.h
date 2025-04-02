
// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include <stdint.h>
//Register definitions for ClockEnable
#define         SYSCTLRCGCUARTR_serial         (*((volatile unsigned long*)0x400FE618))
#define         SYSCTLRCGCGPIOR_serial         (*((volatile unsigned long*)0x400FE608))

//Register definitions for UART0 module
#define     UART0CTLR                       (*((volatile unsigned long*)0x4000C030))
#define     UART0IBRDR                  (*((volatile unsigned long*)0x4000C024))
#define     UART0FBRDR                  (*((volatile unsigned long*)0x4000C028))
#define     UART0LCRHR                  (*((volatile unsigned long*)0x4000C02C))
#define     UART0CCR                        (*((volatile unsigned long*)0x4000CFC8))
#define     UART0FRR                        (*((volatile unsigned long*)0x4000C018))
#define     UART0DRR                        (*((volatile unsigned long*)0x4000C000))

//Register definitions for GPIOPortD
#define     GPIOPORTAAFSELR_serial         (*((volatile unsigned long*)0x40004420))
#define     GPIOPORTAPCTLR_serial          (*((volatile unsigned long*)0x4000452C))
#define     GPIOPORTADENR_serial               (*((volatile unsigned long*)0x4000451C))
#define     GPIOPORTADIRR_serial               (*((volatile unsigned long*)0x40004400))
#define     GPIOPORTALOCKR_serial          (*((volatile unsigned long*)0x40004520))
#define     GPIOPORTACRR_serial                (*((volatile unsigned long*)0x40004524))
#define     GPIOPORTAAMSELR_serial         (*((volatile unsigned long*)0x40004528))


#define UART_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART_CTL_UARTEN         0x00000001  // UART Enable
#define CR                                          13
#define BS                                          8
#define LF                                          10

//------------UART_Init------------
// Initialize the UART for 115,200 baud rate (assuming 50 MHz UART clock),
// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
// Input: none
// Output: none
void UART_Init(void){
  SYSCTLRCGCUARTR_serial |= 0x01;                  // activate UART0
  SYSCTLRCGCGPIOR_serial |= 0x01;                  // activate port A

  UART0CTLR &= ~UART_CTL_UARTEN;            // disable UART
  UART0IBRDR = 8;                           // IBRD = int(16,000,000 / (16 * 115,200)) = int(8.6805)
  UART0FBRDR = 44;                      // FBRD = int(0.6805 * 64 + 0.5) = 44
                                                                                    // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART0LCRHR = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
  UART0CTLR |= UART_CTL_UARTEN;             // enable UART
  GPIOPORTAAFSELR_serial |= 0x03;                      // enable alt funct on PA1-0
  GPIOPORTADENR_serial |= 0x03;                        // enable digital I/O on PA1-0
                                                                                    // configure PA1-0 as UART
  GPIOPORTAPCTLR_serial = (GPIOPORTAPCTLR_serial & 0xFFFFFF00)+0x00000011;
  GPIOPORTAAMSELR_serial &= ~0x03;          // disable analog functionality on PA
}

void UART_OutChar(char data){
  while((UART0FRR & UART_FR_TXFF) != 0);
  UART0DRR = data;
}

void UART_printf(char* pt){
  while(*pt){
    UART_OutChar(*pt);
    pt++;
  }
}
int number;
char number_char;
void UART_printf_int(int num){

  int temp,factor=1;
  temp=num;
  while(temp){
      temp=temp/10;
      factor = factor*10;
  }
  while(factor>1){
      factor = factor/10;
      number = num/factor;
      number_char = number + '0';
      UART_OutChar(number_char);
      num = num%factor;

  }
}

void UART_printf_float(float num){
    int int_part_num = (int) num;
    int float_part_num = (num - int_part_num)*100;

    UART_printf_int(int_part_num);
    UART_printf(".");
    UART_printf_int(float_part_num);

}




