#include "serial_monitor.h"

#include<stdio.h>
#include<string.h>
#include<stdlib.h>


// Clock gating control related register definitions
# define RCGC_I2C_R     *(( volatile unsigned long *)0x400FE620 )
# define RCGC_GPIO_R    *(( volatile unsigned long *)0x400FE608 )
# define CLOCK_GPIOA    0x00000001 // Port A clock control
# define CLOCK_I2C1     0x00000002 // I2C1 clock control

// I2C base address and register definitions
# define I2C_BASE 0x40021000

# define I2CM_SLAVE_ADDR_R  *( volatile long *)( I2C_BASE + 0x000 )
# define I2CM_CONTROL_R     *( volatile long *)( I2C_BASE + 0x004 )
# define I2CM_STATUS_R      *( volatile long *)( I2C_BASE + 0x004 )
# define I2CM_DATA_R        *( volatile long *)( I2C_BASE + 0x008 )
# define I2CM_TIME_PERIOD_R *( volatile long *)( I2C_BASE + 0x00C )
# define I2CM_CONFIG_R      *( volatile long *)( I2C_BASE + 0x020 )
# define I2CM_BUS_MONITOR_R *( volatile long *)( I2C_BASE + 0x02C )
# define I2CM_CONFIG2_R     *( volatile long *)( I2C_BASE + 0x038 )

// I2C interrupt configuration register definitions
# define I2C_INT_MASK_R     *( volatile long *)( I2C_BASE + 0x038 )
# define I2C_RIS_R          *( volatile long *)( I2C_BASE + 0x03C )
# define I2C_MIS_R          *( volatile long *)( I2C_BASE + 0x040 )
# define I2C_INT_CLEAR_R    *( volatile long *)( I2C_BASE + 0x044 )

// GPIO Port D alternate function configuration
# define GPIO_PORTA_AFSEL_R     *(( volatile unsigned long *)0x40004420 )
# define GPIO_PORTA_PCTL_R      *(( volatile unsigned long *)0x4000452C )
# define GPIO_PORTA_DEN_R       *(( volatile unsigned long *)0x4000451C )
# define GPIO_PORTA_OD_R        *(( volatile unsigned long *)0x4000450C )

// I2C bit field definitions
# define I2C_ENABLE         0x01
# define BUSY_STATUS_FLAG   0x01

// I2C command definitions for control register
# define CMD_SINGLE_SEND        0x00000007
# define CMD_SINGLE_RECEIVE     0x00000007
# define CMD_BURST_SEND_START   0x00000003
# define CMD_BURST_SEND_CONT    0x00000001
# define CMD_BURST_SEND_FINISH  0x00000005

// Function prototypes
void I2C_Init ( void );
unsigned char I2C_Receive_Data ( unsigned char Slave_addr ,unsigned char Data_addr );
void I2C_Send_Data ( unsigned char Slave_addr ,unsigned char Data_addr , unsigned char Data );

// I2C intialization and GPIO alternate function configuration
void I2C_Init ( void ){
    RCGC_GPIO_R      |= CLOCK_GPIOA ; // Enable the clock for port D
    RCGC_I2C_R       |= CLOCK_I2C1 ; // Enable the clock for I2C 3
    GPIO_PORTA_DEN_R |= 0xC0; // Assert DEN for port D

    // Configure Port A pins 6 and 7 as I2C 3
    GPIO_PORTA_AFSEL_R |= 0xC0 ;
    GPIO_PORTA_PCTL_R  |= 0x33000000 ;
    GPIO_PORTA_OD_R    |= 0x80 ; // SDA (PD1 ) pin as open darin
    I2CM_CONFIG_R       = 0x0010 ; // Enable I2C 3 master function

/* Configure I2C3 clock frequency
(1 + TIME_PERIOD ) = SYS_CLK /(2*( SCL_LP + SCL_HP ) * I2C_CLK_Freq )
TIME_PERIOD = 16 ,000 ,000/(2(6+4) *100000) - 1 = 7 */

    I2CM_TIME_PERIOD_R = 0x07 ;
}

// Receive one byte of data from I2C slave device
unsigned char I2C_Receive_Data ( unsigned char Slave_addr , unsigned char Data_addr ){
    unsigned char value = 0;

    // Wait for the master to become idle
    while ( I2CM_STATUS_R & BUSY_STATUS_FLAG ) { };

    // Configure slave address for write operation , data and control
    I2CM_SLAVE_ADDR_R = (Slave_addr << 1);
    I2CM_DATA_R       = Data_addr ; // Data to be written
    I2CM_CONTROL_R    = CMD_SINGLE_SEND ; // Command to be performed

    // Wait until master is done
    while ( I2CM_STATUS_R & 0x01 ) { };

    // Configure the slave address for read operation
    I2CM_SLAVE_ADDR_R   = (( Slave_addr << 1) | 0x01 );
    I2CM_CONTROL_R      = CMD_SINGLE_RECEIVE ; // Command to be performed

    // Wait until master is done
    while ( I2CM_STATUS_R & BUSY_STATUS_FLAG ) { };
    value = ( unsigned char ) I2CM_DATA_R ; // Read the data received
    return value;
}

// This function writes one data byte to Data_addr of I2C slave
void I2C_Send_Data ( unsigned char Slave_addr ,unsigned char Data_addr , unsigned char Data ){
    // Wait for the master to become idle
    while ( I2CM_STATUS_R & BUSY_STATUS_FLAG ) { };

    // Configure slave address for write operation , data and control
    I2CM_SLAVE_ADDR_R   = ( Slave_addr << 1);
    I2CM_DATA_R         = Data_addr ; // Data to be written
    I2CM_CONTROL_R      = CMD_BURST_SEND_START ; // Command to be performed

    // Wait until master is done
    while ( I2CM_STATUS_R & BUSY_STATUS_FLAG ) { };

    // Configure data to be written and send command
    I2CM_DATA_R     = Data ;
    I2CM_CONTROL_R  = CMD_BURST_SEND_CONT ;

    // Wait until master is done
    while ( I2CM_STATUS_R & BUSY_STATUS_FLAG ) { };

    // Send the finsh command
    I2CM_CONTROL_R = CMD_BURST_SEND_FINISH ;

    // Wait until master is done
    while ( I2CM_STATUS_R & BUSY_STATUS_FLAG ){ };
}


//Convert decimal to bindary coded decimal (which is supported by ds3231 registers)
int dec_to_bcd(int Decimal)
{
    return (((Decimal/10)<<4) | (Decimal % 10));
}

//Convert binary coded decimal to decimal
int bcd_to_dec(unsigned char x) {
    return x - 6 * (x >> 4);
}



# define DEV_ADDRESS 0x68

// Global variable definitions
unsigned char reg_addr = 0x00 ;
unsigned char seconds , minutes , hours , date ;




int second(){
//    int subtraction_factor;
//    if ((I2C_Receive_Data ( DEV_ADDRESS , 0x00) & 0b01110000)== 0){
//        subtraction_factor=0;
//    }
//    else if((I2C_Receive_Data ( DEV_ADDRESS , 0x00) & 0b01110000)== 16){
//        subtraction_factor=6;
//    }
//    else if((I2C_Receive_Data ( DEV_ADDRESS , 0x00) & 0b01110000)== 32){
//        subtraction_factor=12;
//    }
//    else if((I2C_Receive_Data ( DEV_ADDRESS , 0x00) & 0b01110000)== 48){
//        subtraction_factor=18;
//    }
//    else if((I2C_Receive_Data ( DEV_ADDRESS , 0x00) & 0b01110000)== 64){
//        subtraction_factor=24;
//    }
//    else if((I2C_Receive_Data ( DEV_ADDRESS , 0x00) & 0b01110000)== 80){
//        subtraction_factor=30;
//    }
////    UART_printf("Subtraction Factor: ");
////    UART_printf_int(subtraction_factor);
////    UART_printf("\n");
//    return ((I2C_Receive_Data ( DEV_ADDRESS , 0x00))-subtraction_factor);

   int  dec_seconds = bcd_to_dec(I2C_Receive_Data ( DEV_ADDRESS , 0x00));
    return dec_seconds;
}

int minute(){

    int dec_minutes = bcd_to_dec(I2C_Receive_Data ( DEV_ADDRESS , 0x01));
    return dec_minutes;

}

int hour(){

    int dec_hour = bcd_to_dec(I2C_Receive_Data ( DEV_ADDRESS , 0x02));
    return dec_hour;

}

int Date(){

    int dec_date = bcd_to_dec(I2C_Receive_Data ( DEV_ADDRESS , 0x04));
    return dec_date;
}

int Month(){
    int dec_month = bcd_to_dec(I2C_Receive_Data ( DEV_ADDRESS , 0x05) & 0b11111);
    return dec_month;
}

int Year(){

    int dec_year = bcd_to_dec(I2C_Receive_Data ( DEV_ADDRESS , 0x06));
    return dec_year;
}

void RTC_set_time(int hours_in_24,int minutes, int seconds ){



    int bcd_hours   = dec_to_bcd(hours_in_24);
    int bcd_minutes = dec_to_bcd(minutes);
    int bcd_seconds   = dec_to_bcd(seconds);

    I2C_Send_Data ( DEV_ADDRESS ,0x02 , bcd_hours); //Set date
    I2C_Send_Data ( DEV_ADDRESS ,0x01 , bcd_minutes); //Set month
    I2C_Send_Data ( DEV_ADDRESS ,0x00 , bcd_seconds); //Set year
}

void RTC_set_date(int date, int month, int year){
    int bcd_date  = dec_to_bcd(date);
    int bcd_month = dec_to_bcd(month);
    int bcd_year  = dec_to_bcd(year);

    I2C_Send_Data ( DEV_ADDRESS ,0x04 , bcd_date); //Set date
    I2C_Send_Data ( DEV_ADDRESS ,0x05 , bcd_month); //Set month
    I2C_Send_Data ( DEV_ADDRESS ,0x06 , bcd_year); //Set year
}



void delay(int number){
    int i,j;
    for (i=0;i<number;i++)
        for (j=0;j<number;j++);
}





char day_of_week[][20] = {"MONDAY","TUESDAY","WEDNESDAY","THURSDAY","FRIDAY","SATURDAY","SUNDAY"};
int main ( void ){

    // Initialize I2C moude 3
    UART_Init();
    I2C_Init ();

    // Configure the RTC chip to turn on the clock
    //I2C_Send_Data ( DEV_ADDRESS , reg_addr , 0);
    //RTC_set_time(00,00, 00); //For resetting time, uncomment this line !
    //RTC_set_date(01,01,01);
    while (1) {
        UART_printf_int(hour());
        UART_OutChar(':');
        UART_printf_int(minute());
        UART_OutChar(':');
        UART_printf_int(second());
        UART_printf(" ");
        UART_printf(day_of_week[(I2C_Receive_Data ( DEV_ADDRESS , 0x03))]);
        UART_printf("\n");
        UART_printf_int(Date());
        UART_printf("-");
        UART_printf_int(Month());
        UART_printf("-");
        UART_printf_int(Year());
        UART_printf("\n");
    }
}
