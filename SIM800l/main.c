/**
 * School:      Universidad Nacional Aut�noma de M�xico | Facultad de Ingenier�a
 * Department:  Ingenier�a Electr�nica
 * Subject:     Microprocesadores y Microcontroladores
 * Code:        C�digo 24 | Universal Asynchronous Receiver/Transmitter (UART) - Rx
 * Description: Uso del UART_0 para recibir un valor num�rico (00 al 15) desde un terminal, con el cual
 *              se encender�n los LED de la tarjeta de desarrollo, empleando interrupci�n.
 * Author:      Edgar Rojas Guadarrama
 * Date:        29 de octubre de 2024
 */

/**
 * Archivos de cabecera
 */
#include <stdint.h>                                                                 /*  Definici�n de los tama�os de tipo entero */

/**
 * Definici�n de los apuntadores para acceso a memoria
 */
    /*  Nested Vectored Interrupt Controller Registers (NVIC) */                    /*  pp146   REGISTER MAP */
#define NVIC_EN0_R              (*((volatile uint32_t *)0xE000E100))                /*  pp154   Interrupt 31-0 Set Enable */
#define NVIC_EN1_R              (*((volatile uint32_t *)0xE000E104))                /*  pp154   Interrupt 31-0 Set Enable */
    /*  System Control Registers (SYSCTL) */                                        /*  pp247   REGISTER MAP */
#define SYSCTL_RCGCGPIO_R       (*((volatile uint32_t *)0x400FE608))                /*  pp382   GPIO Run Mode Clock Gating Control */
#define SYSCTL_RCGCUART_R       (*((volatile uint32_t *)0x400FE618))                /*  pp388   UART Run Mode Clock Gating Control */
#define SYSCTL_PRGPIO_R         (*((volatile uint32_t *)0x400FEA08))                /*  pp499   GPIO Peripheral Ready */
#define SYSCTL_PRUART_R         (*((volatile uint32_t *)0x400FEA18))                /*  pp505   UART Peripheral Ready */

    /*  UART Registers (UART_0) */                                                  /*  pp1173  REGISTER MAP */
#define UART0_DR_R              (*((volatile uint32_t *)0x4000C000))                /*  pp1175  UART Data */
#define UART0_IBRD_R            (*((volatile uint32_t *)0x4000C024))                /*  pp1184  UART Integer Baud-Rate Divisor */
#define UART0_FBRD_R            (*((volatile uint32_t *)0x4000C028))                /*  pp1185  UART Fractional Baud-Rate Divisor */
#define UART0_LCRH_R            (*((volatile uint32_t *)0x4000C02C))                /*  pp1186  UART Line Control */
#define UART0_CTL_R             (*((volatile uint32_t *)0x4000C030))                /*  pp1188  UART Control */
#define UART0_IFLS_R            (*((volatile uint32_t *)0x4000C034))                /*  pp1192  UART Interrupt FIFO Level Select */
#define UART0_IM_R              (*((volatile uint32_t *)0x4000C038))                /*  pp1194  UART Interrupt Mask */
#define UART0_ICR_R             (*((volatile uint32_t *)0x4000C044))                /*  pp1206  UART Interrupt Clear */
#define UART0_CC_R              (*((volatile uint32_t *)0x4000CFC8))                /*  pp1213  UART Clock Configuration */
#define UART0_TRIS_R            (*((volatile uint32_t *)0x4000C03C))

    /*  UART Registers (UART_7) */                                                  /*  pp1173  REGISTER MAP */
#define UART4_DR_R              (*((volatile uint32_t *)0x40010000))                /*  pp1175  UART Data */
#define UART4_IBRD_R            (*((volatile uint32_t *)0x40010024))                /*  pp1184  UART Integer Baud-Rate Divisor */
#define UART4_FBRD_R            (*((volatile uint32_t *)0x40010028))                /*  pp1185  UART Fractional Baud-Rate Divisor */
#define UART4_LCRH_R            (*((volatile uint32_t *)0x4001002C))                /*  pp1186  UART Line Control */
#define UART4_CTL_R             (*((volatile uint32_t *)0x40010030))                /*  pp1188  UART Control */
#define UART4_IFLS_R            (*((volatile uint32_t *)0x40010034))                /*  pp1192  UART Interrupt FIFO Level Select */
#define UART4_IM_R              (*((volatile uint32_t *)0x40010038))                /*  pp1194  UART Interrupt Mask */
#define UART4_ICR_R             (*((volatile uint32_t *)0x40010044))                /*  pp1206  UART Interrupt Clear */
#define UART4_CC_R              (*((volatile uint32_t *)0x40010FC8))                /*  pp1213  UART Clock Configuration */
#define UART4_TRIS_R            (*((volatile uint32_t *)0x4001003C))

    /*  GPIO Registers (PortA) */                                                   /*  pp757   REGISTER MAP */
#define GPIO_PORTA_AHB_AFSEL_R  (*((volatile uint32_t *)0x40058420))                /*  pp770   GPIO Alternate Function Select */
#define GPIO_PORTA_AHB_DEN_R    (*((volatile uint32_t *)0x4005851C))                /*  pp781   GPIO Digital Enable */
#define GPIO_PORTA_AHB_PCTL_R   (*((volatile uint32_t *)0x4005852C))                /*  pp787   GPIO Port Control */

    /*  GPIO Registers (PortK) */                                                   /*  pp757   REGISTER MAP */
#define GPIO_PORTK_AHB_AFSEL_R  (*((volatile uint32_t *)0x40061420))                /*  pp770   GPIO Alternate Function Select */
#define GPIO_PORTK_AHB_DEN_R    (*((volatile uint32_t *)0x4006151C))                /*  pp781   GPIO Digital Enable */
#define GPIO_PORTK_AHB_PCTL_R   (*((volatile uint32_t *)0x4006152C))                /*  pp787   GPIO Port Control */

    /*  GPIO Registers (PortF) */                                                   /*  pp757   REGISTER MAP  */
#define GPIO_PORTF_AHB_DATA_R   (*((volatile uint32_t *)(0x4005D000 | 0x11 << 2)))  /*  pp759   GPIO Data */
#define GPIO_PORTF_AHB_DIR_R    (*((volatile uint32_t *)0x4005D400))                /*  pp760   GPIO Direction */
#define GPIO_PORTF_AHB_DEN_R    (*((volatile uint32_t *)0x4005D51C))                /*  pp781   GPIO Digital Enable */

    /*  GPIO Registers (PortN) */                                                   /*  pp757   REGISTER MAP  */
#define GPIO_PORTN_DATA_R       (*((volatile uint32_t *)(0x40064000 | 0x03 << 2)))  /*  pp759   GPIO Data */
#define GPIO_PORTN_DIR_R        (*((volatile uint32_t *)0x40064400))                /*  pp760   GPIO Direction */
#define GPIO_PORTN_DEN_R        (*((volatile uint32_t *)0x4006451C))                /*  pp781   GPIO Digital Enable */

/**
 * Definiciones
 */
#define NVIC_ST_CTRL_COUNT      0x00010000                                          /*  Bandera COUNT del registro STCTRL del SysTick */
#define NVIC_ST_CTRL_ENABLE     0x00000001                                          /*  SysTick => STCTRL register -> Enable */
#define SYSCTL_RCGCGPIO_R0      0x00000001                                          /*  GPIO PortA => Run Mode Clock */
#define SYSCTL_RCGCGPIO_R9      0x00000200
#define SYSCTL_RCGCGPIO_R5      0x00000020                                          /*  GPIO PortF => Run Mode Clock */
#define SYSCTL_RCGCGPIO_R12     0x00001000                                          /*  GPIO PortN => Run Mode Clock */
#define SYSCTL_RCGCUART_R0      0x00000001                                          /*  UART Module 0 Run Mode Clock */
#define SYSCTL_RCGCUART_R4      0x00000010
#define UART_FR_TXFE            0x00000080                                          /*  UART Transmit FIFO Empty */
#define UART_FR_RXFF            0x00000040                                          /*  UART Receive FIFO Full */
#define UART_FR_TXFF            0x00000020                                          /*  UART Transmit FIFO Full */
#define UART_FR_RXFE            0x00000010                                          /*  UART Receive FIFO Empty */
#define UART_FR_BUSY            0x00000008                                          /*  UART Busy */
#define TIMER_RIS_T             0x00000010
#define TIMER_RIS_R             0x00000020
/**
 * Variables globales
 */
int valor, valor1;
/**
 * Funciones
 */

void UART0_Init(void){
    // * * * * * HABILITACI�N DEL M�DULO UART * * * * * * * * * * * * * * * * * *
    // 1. Habilitar el reloj del m�dulo UART (RCGCUART).
    //    Esperar a que se estabilice el reloj (PRUART).
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;                                        /*  Habilitaci�n del reloj del UART_0 */
    while ((SYSCTL_PRUART_R & SYSCTL_RCGCUART_R0) == 0) {};                         /*  Espera a que se estabilice el reloj del UART_0 */

    // 2. Habilitar el reloj del m�dulo GPIO (RCGCGPIO).
    //    Esperar a que se estabilice el reloj (PRGPIO).
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;                                        /*  Habilitaci�n del reloj del PortA */
    while ((SYSCTL_PRGPIO_R & SYSCTL_RCGCGPIO_R0) == 0) {};                         /*  Espera a que se estabilice el reloj del PortA */

    // 3. Habilitar la funci�n alterna de hardware de los pines GPIO (GPIOAFSEL).
    GPIO_PORTA_AHB_AFSEL_R |= 0x03;                                                 /*  PortA[0] => Alternate hardware function -> Enable */

    // 4. Habilitar las funciones digitales de los pines GPIO (GPIODEN).
    GPIO_PORTA_AHB_DEN_R |= 0x03;                                                   /*  PortA[0] => Digital functions -> Enabled */

    // 5. (OPCIONAL) Configurar el nivel de corriente del GPIO (GPIODR2D) y/o el slew rate, de acuerdo con el modo
    //    seleccionado (GPIOSLR).

    // 6. Seleccionar la funci�n alterna de hardware requerida en los pines GPIO con el campo PMCn (GPIOPCTL).
    GPIO_PORTA_AHB_PCTL_R = (GPIO_PORTA_AHB_PCTL_R & ~0x0000000F) | (1 << 0);       /*  PortA[0] => GPIO controlled by the function U0Rx */
    GPIO_PORTA_AHB_PCTL_R = (GPIO_PORTA_AHB_PCTL_R & ~0x000000F0) | (1 << 4);
    // * * * * * CONFIGURACI�N DEL BAUD RATE * * * * * * * * * * * * * * * * * *
    // 1. Deshabilitar el UART con el campo UARTEN (UARTCTL).
    UART0_CTL_R &= ~0x03;                                                           /*  UART_0 => UARTEN -> UART disabled */

    // 2. Cargar la parte entera del Baud-Rate Divisor (BRD) (UARTIBRD).
    UART0_IBRD_R =8; //104;

    // 3. Cargar la parte decimal del Baud-Rate Divisor (BRD) (UARTFBRD).
    UART0_FBRD_R =44; //11;

    // 4. Cargar los par�metros seriales deseados (UARTLCRH).
    UART0_LCRH_R = (UART0_LCRH_R & ~0x60) | (0x3 << 5);                             /*  UART_0 => WLEN -> 8 bits Word Length */
    UART0_LCRH_R = (UART0_LCRH_R & ~0x10) | (0x1 << 4);                             /*  UART_0 => FEN -> Transmit and receive FIFO buffers are enabled */

    // 5. Configurar la fuente de reloj del UART (UARTCC).
    UART0_CC_R = 0x5;                                                               /*  UART_0 => CS -> Alternate clock source (defined by ALTCLKCFG [pp280]) -> PIOSC */

    // 6. (OPCIONAL) configurar el canal uDMA y habilitar las opciones del DMA (UARTDMACTL).

    // 7. Si se requiere utilizar interrupciones:
        // 7.1 Configurar el nivel de la FIFO de recepci�n (campo RXIFLSEL) y/o el nivel de la FIFO de
        //     transmisi�n (campo TXIFLSEL), ambos en UARTIFLS.
    UART0_IFLS_R = (UART0_IFLS_R & ~0x3F) | (0x0 << 3);                             /*  UART_0 => RXIFLSEL -> Receive Interrupt FIFO -> 1/8 full */
    UART0_IFLS_R = (UART0_IFLS_R & ~0x3F) | (0x0 << 0);                             /*  UART_0 => RXIFLSEL -> Receive Interrupt FIFO -> 1/8 full */

        // 7.2 Desenmascarar la interrupci�n de recepci�n (campo RXIM) y/o la interrupci�n de transmisi�n
        //     (campo TXIM), ambos en UARTIM.
        //     * Se recomienda limpiar previamente la bandera de interrupci�n correspondiente en UARTICR.
    UART0_ICR_R |= 0x0030;                                                          /*  UART_0 => RXIC -> Receive Interrupt Clear -> Interrupt clear */
    UART0_IM_R |= 0x0030;                                                           /*  UART_0 => RXIM -> Receive Interrupt Mask -> Unmasked */

        // 7.3 Asignar el nivel de prioridad de la interrupci�n (PRIn).

        // 7.4 Habilitar la interrupci�n (ENn).
    NVIC_EN0_R = 1 << (5-0);                                                        /*  Interrupt 5 (UART_0) => Enabled */

    // 8. Habilitar el UART con el campo UARTEN (UARTCTL).
    UART0_CTL_R |= 0x01;                                                            /*  UART_0 => UARTEN -> UART enabled */
}
void UART4_Init(void){
    // * * * * * HABILITACI�N DEL M�DULO UART * * * * * * * * * * * * * * * * * *
    // 1. Habilitar el reloj del m�dulo UART (RCGCUART).
    //    Esperar a que se estabilice el reloj (PRUART).
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R4;                                        /*  Habilitaci�n del reloj del UART_0 */
    while ((SYSCTL_PRUART_R & SYSCTL_RCGCUART_R4) == 0) {};                         /*  Espera a que se estabilice el reloj del UART_0 */

    // 2. Habilitar el reloj del m�dulo GPIO (RCGCGPIO).
    //    Esperar a que se estabilice el reloj (PRGPIO).
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R9;                                        /*  Habilitaci�n del reloj del PortA */
    while ((SYSCTL_PRGPIO_R & SYSCTL_RCGCGPIO_R9) == 0) {};                         /*  Espera a que se estabilice el reloj del PortA */

    // 3. Habilitar la funci�n alterna de hardware de los pines GPIO (GPIOAFSEL).
    GPIO_PORTK_AHB_AFSEL_R |= 0x03;                                                 /*  PortK[0,1] => Alternate hardware function -> Enable */

    // 4. Habilitar las funciones digitales de los pines GPIO (GPIODEN).
    GPIO_PORTK_AHB_DEN_R |= 0x03;                                                   /*  PortK[0,1] => Digital functions -> Enabled */

    // 5. (OPCIONAL) Configurar el nivel de corriente del GPIO (GPIODR2D) y/o el slew rate, de acuerdo con el modo
    //    seleccionado (GPIOSLR).

    // 6. Seleccionar la funci�n alterna de hardware requerida en los pines GPIO con el campo PMCn (GPIOPCTL).
    GPIO_PORTK_AHB_PCTL_R = (GPIO_PORTK_AHB_PCTL_R & ~0x0000000F) | (1 << 0);       /*  PortA[0] => GPIO controlled by the function U0Rx */
    GPIO_PORTK_AHB_PCTL_R = (GPIO_PORTK_AHB_PCTL_R & ~0x000000F0) | (1 << 4);
    // * * * * * CONFIGURACI�N DEL BAUD RATE * * * * * * * * * * * * * * * * * *
    // 1. Deshabilitar el UART con el campo UARTEN (UARTCTL).
    UART4_CTL_R &= ~0x03;                                                           /*  UART_4 => UARTEN -> UART disabled */

    // 2. Cargar la parte entera del Baud-Rate Divisor (BRD) (UARTIBRD).
    UART4_IBRD_R =8; //104;

    // 3. Cargar la parte decimal del Baud-Rate Divisor (BRD) (UARTFBRD).
    UART4_FBRD_R =44; //11;

    // 4. Cargar los par�metros seriales deseados (UARTLCRH).
    UART4_LCRH_R = (UART4_LCRH_R & ~0x60) | (0x3 << 5);                             /*  UART_4 => WLEN -> 8 bits Word Length */
    UART4_LCRH_R = (UART4_LCRH_R & ~0x10) | (0x1 << 4);                             /*  UART_4 => FEN -> Transmit and receive FIFO buffers are enabled */

    // 5. Configurar la fuente de reloj del UART (UARTCC).
    UART4_CC_R = 0x5;                                                               /*  UART_4 => CS -> Alternate clock source (defined by ALTCLKCFG [pp280]) -> PIOSC */

    // 6. (OPCIONAL) configurar el canal uDMA y habilitar las opciones del DMA (UARTDMACTL).

    // 7. Si se requiere utilizar interrupciones:
        // 7.1 Configurar el nivel de la FIFO de recepci�n (campo RXIFLSEL) y/o el nivel de la FIFO de
        //     transmisi�n (campo TXIFLSEL), ambos en UARTIFLS.
    UART4_IFLS_R = (UART4_IFLS_R & ~0x3F) | (0x0 << 3);                             /*  UART_4 => RXIFLSEL -> Receive Interrupt FIFO -> 1/8 full */
    UART4_IFLS_R = (UART4_IFLS_R & ~0x3F) | (0x0 << 0);                             /*  UART_4 => RXIFLSEL -> Receive Interrupt FIFO -> 1/8 full */

        // 7.2 Desenmascarar la interrupci�n de recepci�n (campo RXIM) y/o la interrupci�n de transmisi�n
        //     (campo TXIM), ambos en UARTIM.
        //     * Se recomienda limpiar previamente la bandera de interrupci�n correspondiente en UARTICR.
    UART4_ICR_R |= 0x0030;                                                          /*  UART_4 => RXIC -> Receive Interrupt Clear -> Interrupt clear */
    UART4_IM_R |= 0x0030;                                                           /*  UART_4 => RXIM -> Receive Interrupt Mask -> Unmasked */

        // 7.3 Asignar el nivel de prioridad de la interrupci�n (PRIn).

        // 7.4 Habilitar la interrupci�n (ENn).
    NVIC_EN1_R = 1 << (57-32);                                                        /*  Interrupt 5 (UART_0) => Enabled */

    // 8. Habilitar el UART con el campo UARTEN (UARTCTL).
    UART4_CTL_R |= 0x01;                                                            /*  UART_4 => UARTEN -> UART enabled */
}

void UART0_Handler(void){
    UART4_DR_R = UART0_DR_R;                                                           /*  UART_0 => Lectura de la FIFO de recepci�n */
    UART4_DR_R = UART0_DR_R;                                                           /*  UART_0 => Lectura de la FIFO de recepci�n */
//    UART0_DR_R = Trac0;                                                        /*  Carga de un dato a la FIFO de transmisi�n */
//    UART0_DR_R = Trac1;
    if(UART0_TRIS_R==TIMER_RIS_T){           /*  UART_0 => TXIC -> Receive Interrupt Clear -> Interrupt clear */
        UART0_ICR_R |= 0x0010;
        }                                    /*  UART_0 => RXIC -> Receive Interrupt Clear -> Interrupt clear */
        if(UART0_TRIS_R==TIMER_RIS_R){
            UART0_ICR_R |= 0x0020;
            }                                                               /*  UART_0 => RXIC -> Receive Interrupt Clear -> Interrupt clear */
}

void UART4_Handler(void){
    UART0_DR_R= UART4_DR_R;                                                           /*  UART_4 => Lectura de la FIFO de recepci�n */
    UART0_DR_R= UART4_DR_R;                                                           /*  UART_4 => Lectura de la FIFO de recepci�n */
  //  UART4_DR_R = Rrac0;                                                        /*  Carga de un dato a la FIFO de transmisi�n */
   // UART4_DR_R = Rrac1;
    if(UART4_TRIS_R==TIMER_RIS_T){       /*  UART_4 => TXIC -> Receive Interrupt Clear -> Interrupt clear */
    UART4_ICR_R |= 0x0010;
    }                                    /*  UART_4 => RXIC -> Receive Interrupt Clear -> Interrupt clear */
    if(UART4_TRIS_R==TIMER_RIS_R){
        UART4_ICR_R |= 0x0020;
        }
}

/**
 * Funci�n principal
 */
int main(void){

                                                                 /*  Inicializaci�n del GPIO PortN */
    UART0_Init();
    UART4_Init();                                                                     /*  Inicializaci�n del UART_0 */

    while(1) {

    }
}
