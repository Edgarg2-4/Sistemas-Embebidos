/**
 * MCP41010
 * Single/Dual Digital Potentiometer with SPI™ Interface
 * main.c
 *
 * Genera una forma de onda de diente de sierra.
 *
 * SSI0 (SPI)
 * PA2: SCLK
 * PA3: FSS
 * PA4: MOSI
 * PA5: MISO (no se usa)
 *
 * MCP41010
 * Vcc: 3.3 V
 * PB0: GND
 * PA0: 3.3 v
 */

#include <stdint.h>
#include <math.h>
#include "inc/tm4c1294ncpdt.h"

void SSI0_init (void);
void SSI0_sendData (uint16_t dat);
void pot_setVal (uint8_t slider);
uint8_t x = 0x00; // Valor inicial.
int cont = 1;
int cont1;
int UpDown;
int frecuencia = 26300;
uint8_t SW1 = 0x01;                                                              /*  GPIO PortJ[1] */
uint8_t SW2 = 0x02;     /*  GPIO PortJ[0] */
float punto;
int i=0;
float punto;
float sen=0;
uint8_t puntos[512];
int N=512;
float pi=3.141592;

#define TIMER_ICR_TATOCINT  0x00000001;

int main(void)
{
    for(i=0;i<N;i++){
        punto = (127*(sin(2*pi*1*i/N))) + 127;
        puntos[i]=(uint8_t)(punto);

    }


    SSI0_init();   // Función que habilita el SPI
    GPIO_Init();
    GPTM1A_Init();
    pot_setVal(x++);
    while (1) {

    }
}

void GPIO_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_PPGPIO_P8;                                          /*  Habilitaci�n del reloj del PortJ */
       while ((SYSCTL_PRGPIO_R & SYSCTL_PPGPIO_P8) == 0) {};                           /*  Espera a que se estabilice el reloj del PortJ */

       GPIO_PORTJ_AHB_DIR_R &= ~0x03;                                                  /*  PortJ[1:0] => Input */
       GPIO_PORTJ_AHB_PUR_R |= 0x03;                                                   /*  PortJ[1:0] => Pull-Up resistors -> Enabled */
       GPIO_PORTJ_AHB_DEN_R |= 0x03;                                                   /*  PortJ[1:0] => Digital functions -> Enabled */
       GPIO_PORTJ_AHB_IS_R &= ~0x03;                                                   /*  PortJ[1:0] => Interrupt sense -> Edge-sensitive */
       GPIO_PORTJ_AHB_IBE_R &= ~0x03;                                                  /*  PortJ[1:0] => Interrupt both edges -> Controlled by the GPIO_IEV register */
       GPIO_PORTJ_AHB_IEV_R &= ~0x03;                                                  /*  PortJ[1:0] => Interrupt event -> Falling edge triggers an interrupt */
       GPIO_PORTJ_AHB_ICR_R = 0x03;                                                    /*  PortJ[1,0] => Interrupt is Cleared */
       GPIO_PORTJ_AHB_IM_R |= 0x03;                                                    /*  PortJ[1:0] => Interrupt mask -> Unmasked */
       NVIC_PRI12_R = (NVIC_PRI12_R & 0x00FFFFFF) | (0b001 << 29);                     /*  PortJ => Interrupt priority -> 0 */
       NVIC_EN1_R = 1 << (51-32);

}

void SSI0_init (void) {
    SYSCTL_RCGCSSI_R = SYSCTL_RCGCSSI_R0; // Activa reloj al SSI0
    while ((SYSCTL_PRSSI_R & SYSCTL_PRSSI_R0) == 0); // Espera a que este listo
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0; // Activa reloj del GPIO A
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R0) == 0); // Espera a que este listo

    GPIO_PORTA_AHB_AFSEL_R |= 0x3C; // Seleciona la función alterna de PA[2:5].
    GPIO_PORTA_AHB_PCTL_R = (GPIO_PORTA_AHB_PCTL_R & 0XFF0000FF) | 0x00FFFF00; // Configura las terminales de PA a su función de SSI0.
    GPIO_PORTA_AHB_AMSEL_R = 0x00; // Deshabilita la función analogica
    GPIO_PORTA_AHB_DIR_R = (GPIO_PORTA_AHB_DIR_R & ~0x3C) | 0x1C; // Configura al puerto como salida
    GPIO_PORTA_AHB_DEN_R |= 0x3C; // Habilita la función digital del puerto

    SSI0_CR1_R = 0x00; // Selecciona modo maestro/deshabilita SSI0. (p. 1247)
    SSI0_CPSR_R = 0x02; // preescalador (CPSDVSR) del reloj SSI (p. 1252)
    // configura para Freescale SPI; 16bit; 4 Mbps; SPO = 0; SPH = 0 (p. 1245)
    SSI0_CR0_R = (0x0100 | SSI_CR0_FRF_MOTO | SSI_CR0_DSS_16) & ~(SSI_CR0_SPO | SSI_CR0_SPH);

//    SSI0_ICR_R =0x40; // clear
//    SSI0_IM_R = 0x40; // habilitacion
//    NVIC_EN0_R = 0x80;
    SSI0_CR1_R |= SSI_CR1_SSE; // Habilita SSI0.
}

void GPTM1A_Init(void){


        // TIMER - 0. Habilitar el reloj del módulo GPTM en el registro RCGCTIMER y esperar a que se estabilice el reloj
        //            en el registro PRTIMER.
        SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;                                      /*  Habilitación del GPTM_2 */
            while ((SYSCTL_PRTIMER_R & SYSCTL_RCGCTIMER_R1) == 0) {};


        // TIMER - 1. Asegurarse de que el TIMER está deshabilitado con el campo TnEN del registro GPTMCTL.

            TIMER1_CTL_R &= ~0x01;

        // TIMER - 2. Configurar el TIMER para que funcione de forma individual (16 bits) en el registro GPTMCFG.
            TIMER1_CFG_R = 0x0;

        // TIMER - 3. Configurar los siguientes campos en el registro GPTMTnMR.

//            TIMER1_TAMR_R = (TIMER0_TAMR_R & ~0x02) | 0x2;


        // TIMER - 6. Cargar el valor de inicio del TIMER en el registro GPTMTnILR.
            TIMER1_TAILR_R=26300;

        // TIMER - 7. Si se requiere interrupción:
            // Limpiar la bandera de interrupción en el campo CnECINT del registro GPTMICR.
            // Desenmascarar la interrupción en el campo CnEIM del registro GPTMIMR.
            // Definir el nivel de prioridad en los registros PRIn.
            // Habilitar la interrupción en los registros ENn.
            TIMER1_ICR_R = 0x01;
                TIMER1_IMR_R = TIMER_ICR_TATOCINT;                                      /*habilitacion de interrup mask */
                NVIC_PRI5_R = (NVIC_PRI5_R & 0xFFF00FFF) | (0b000 << 13);                     /*  Timmer 1 prioridad*/
                    NVIC_EN0_R = 1 << (21-0);
        // TIMER - 8. Habilitar el TIMER con el campo TnEN del registro GPTMCTL.
                                                                       /*  TIMER_2 => Timer A -> TAEN -> Timer enabled */


        // TIMER - 9. Revisar por polling el campo CnERIS del registro GPTMRIS o esperar a que se genere el evento de
        //            interrupción (si se habilitó). En ambos casos, la bandera de estado se limpia al escribir un 1 en
        //            el campo CnECINT del registro GPTMICR.
        // NOTA: El tiempo en donde sucedió el evento puede ser obtenido al leer el registro GPTMTnR.
    }


void SSI0_sendData (uint16_t dat) {
    // Envia dato de 16-bit

    SSI0_DR_R = dat; // envia dato.
    x++;
    TIMER1_CTL_R |= 0x01;

}

void pot_setVal(uint8_t slider) {
    //Combine el valor del control deslizante con el código de comando de escritura.
    // Estructura del mensaje SPI: [comando (8-bits)][deslizador (8-bits)]
    SSI0_sendData(0x1100 | slider);
}
void GPIOJ_Handler(void){
    TIMER1_TAILR_R=frecuencia;
    GPIO_PORTJ_AHB_ICR_R = 0x03;
    if(GPIO_PORTJ_AHB_DATA_R == SW1){
                 frecuencia -= 1000;
                 if(frecuencia == 300){
                            frecuencia = 26300;
                        }
            }
            if(GPIO_PORTJ_AHB_DATA_R == SW2){
                        cont++;
                        if(cont == 5){
                                cont = 1;
                                }
                   }

}
void Timer1_Handler(void){

    TIMER1_ICR_R = 0x01;
    switch(cont){
    case 1:
        if (UpDown == 0) {
                x++;
                SSI0_DR_R = 0x1100 | x; // envia dato.                                                          /*  Envío de una trama de datos por SSI_0 */
                if (x == 255) {
                    UpDown = 1;
                }
            } else {
                x--;
                SSI0_DR_R = 0x1100 | x; // envia dato.                                                    /*  Envío de una trama de datos por SSI_0 */
                if (x == 0) {
                    UpDown = 0;
                }
            }

    break;

    case 2:
        x++;
        SSI0_DR_R = 0x1100 | x;
        break;

    case 3:
        if (UpDown == 0) {
                       x=255;
                       UpDown = 1;
                       SSI0_DR_R = 0x1100 | x; // envia dato.
        }
                       else{
                           x=0;
                           UpDown = 0;


            SSI0_DR_R = 0x1100 | x;}

            break;

    case 4:
        if (i==512){
                        i=0;
                    }

            SSI0_DR_R = 0x1100 | puntos[i];
            i++;

            break;
    }

}

