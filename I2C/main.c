///*** PROGRAMA QUE CONFIGURA EL PERIFERICO I2C ***
//*** EN EL TIVA TM4C1294 COMO MAESTRO Y ESCLAVO EL RTC DS1307***

//Librerias
#include<stdint.h>
#include<stdbool.h>

//REGISTROS DE RELOJ
#define SYSCTL_RCGCGPIO_R       (*((volatile uint32_t *)0x400FE608)) //Reloj del puerto
#define SYSCTL_RCGCI2C_R        (*((volatile uint32_t *)0x400FE620)) //Reloj de I2C
#define SYSCTL_PRGPIO_R        (*((volatile uint32_t *)0x400FEA08)) //Bandera de "Peripherial Ready"

//REGISTROS DEL PUERTO B
#define GPIO_PORTB_DATA_R   (*((volatile uint32_t *)0x400593FC)) //Para los datos del puerto
#define GPIO_PORTB_DIR_R    (*((volatile uint32_t *)0x40059400)) //Para seleccionar función
#define GPIO_PORTB_AFSEL_R  (*((volatile uint32_t *)0x40059420)) //Para seleccionar función alterna
#define GPIO_PORTB_ODR_R    (*((volatile uint32_t *)0x4005950C)) //Para activar el Open Drain
#define GPIO_PORTB_DEN_R    (*((volatile uint32_t *)0x4005951C)) //Para activar función digital
#define GPIO_PORTB_PCTL_R   (*((volatile uint32_t *)0x4005952C)) //Para el control del puerto

//REGISTROS DEL MÓUDLO I2C
#define I2C0_MSA_R              (*((volatile uint32_t *)0x40020000)) //I2C Master Slave Adress
#define I2C0_MCS_R              (*((volatile uint32_t *)0x40020004)) //I2C Master Control Status
#define I2C0_MDR_R              (*((volatile uint32_t *)0x40020008)) //I2C Master Data Register
#define I2C0_MTPR_R             (*((volatile uint32_t *)0x4002000C)) //I2C Master Time Period
#define I2C0_MCR_R              (*((volatile uint32_t *)0x40020020)) //I2C Master Configuration Register

/*
El registro I2C Master Control/Status (I2C_MCS_R) tiene:
-Modo READ-ONLY DATUS: los 7 bits menos significativos son:
    7:Clock Time Out Error  6:BUS BUSY      5:IDLE
    4:Arbitration Lost      3:DataAck       2:AdrAck
    1:Error                 0:CONTROLLER BUSY

-Modo WRITE-ONLY CONTROL_ Los 6 bits menos significativos son:
    6:BURST    5:QuickCommand  4:High Speed Enable
    3:ACK      2:STOP          1:START
    0:RUN
*/
#define I2C_MCS_ACK 0x00000008 //Transmmitter Acknowledge Enable
#define I2C_MCS_DATACK 0x00000008 // Data Acknowledge Enable
#define I2C_MCS_ADRACK 0x00000004 // Acknowledge Address
#define I2C_MCS_STOP 0x00000004 // Generate STOP
#define I2C_MCS_START 0x00000002 // Generate START
#define I2C_MCS_ERROR 0x00000002 // Error
#define I2C_MCS_RUN 0x00000001 // I2C Master Enable

//**Direcciones del DS1307
int AdreDS1307 =0x068;///Dirección del RTC DS1307
int AdreSec= 0x00;
int AdreMin=0x01;

/*El cálculo del Time Period Register (TPR) se especifica en la página 1284
 Asumiendo un reloj de 16 MHz y un modo de operación estándar (100 kbps):
*/
int TPR = 7;

// Variables para manejar los valores del RTC
uint8_t segundos, minutos, horas, dia, fecha, mes, anio;
uint8_t error;
uint32_t i;

//*** Función que inicializa los relojes, el GPIO y el I2C0 ***
void I2C_Init(void){
    //CONFIGURACIÓN DE LOS RELOJ
    SYSCTL_RCGCI2C_R |= 0x0001; // Activamos el reloj de I2C0 [I2C9 I2C8 I2C7 ... I2C0]<--Mapa de RCGCI2C
    SYSCTL_RCGCGPIO_R |= 0x0002; // Activamos el reloj GPIO_PORTB mientras se activa el reloj de I2C0
    while((SYSCTL_PRGPIO_R&0x0002) == 0){};//Espero a que se active el reloj del puerto B

    //CONFIGURACIÓN DE LOS GPIOS
    /*Acorde con la tabla "Signals by function" de la p. 1808:
     el PIN 2 del puerto B (PB2) es el I2C0SCL del I2C0, y
     el PIN 3 del puerto B (PB3) es el I2C0SDA del I2C0
    */
    GPIO_PORTB_AFSEL_R |= 0x0C; // Activo la función alterna del PB2 y PB3
    GPIO_PORTB_ODR_R |= 0x08;   // Activo el OPEN DRAIN para el PB3, ya que el PB2 ya tiene uno por preconfig.
    GPIO_PORTB_DIR_R |= 0x0C;   //Activo al PB2 y al PB3 como OUTPUT
    GPIO_PORTB_DEN_R |= 0x0C;   //Activo la función digital de PB3 y PB2
    /*
    Así como el registro AFSEL indica que se ejecutará una función externa, en el registro PCTL
    debemos indicar qué función alterna se realizará acorde con la tabla 26-5 de la p.1808 e indicarlo
     en el correspondiente PCMn (uno por cada bit del puerto) del registro PCTL
     */
    GPIO_PORTB_PCTL_R|=0x00002200;

    //CONFIGURACIÓN DEL MODULO I2C0
    I2C0_MCR_R = 0x00000010; // Habilitar función MASTER para el I2C0
    I2C0_MTPR_R = TPR; // Se establece una velocidad estándar de 100kbps
}


// ** Función esperar **
int esperar(){
    while(I2C0_MCS_R&0x00000001){}; //Espero a que la transmisión acabe
      if(I2C0_MCS_R&0x00000002==1){ //¿Hubo error?
          error=1;
          return error;
      };
      return 0;

}


//** Función para configurar al esclavo RTC DS1307 **

void CargarFecha(){
    /*
     Programar: Jueves 31 de octubre del 2019, a las 9:40:00 pm

    El mapa de memoria del DS1207 es el siguiente:
    DIRECCIÓN  FUNCIÓN    BIT7   BIT6  BIT5  BIT4  BIT3  BIT2  BIT1   BIT0
        00h    Segundos     0     0     0      0    0      0    0       0
        01h     Minutos     0     0     0      0    0      0    0       0
        02h     Horas       0     0     0      1    0      0    0       0
        03h     Día         0     0     1      1    0      0    0       1
        04h     Fecha       0     0     0      0    0      1    0       0
        05h     Mes         0     0     0      1    0      0    0       0
        06h     Año         0     0     0      1    1      0    0       1
        07h     Control     0     0     0      0    0      0    1       1
     08h-3Fh    RAM 56x8
     */
    //Por lo tanto

    int segundos=0x00, minutos=0x59, horas=0x23, dia=0x07, fecha=0x31, mes=0x12, anio=0x2025;
    while(I2C0_MCS_R&0x00000001){}; // wait for I2C ready
    //Para transmitir
    I2C0_MSA_R=(AdreDS1307<<1)&0xFE; //Cargo la dirección del DS1307 e indico "SEND", es decir, el Slave va a recibir
    I2C0_MDR_R=AdreSec&0x0FF; //Envio la Subdirección( dirección del registro interno "segundos") al DS1307
    I2C0_MCS_R=(I2C_MCS_RUN|I2C_MCS_START); // Condición de START y corro
    for(i=0;i<150;i++){} //Delay
    esperar();


    I2C0_MDR_R=segundos; //Envio el valor de "segundos"
    I2C0_MCS_R=(I2C_MCS_RUN);
    for(i=0;i<150;i++){} //Delay
    esperar();


    I2C0_MDR_R=minutos; //Envio el valor de "minutos"
    I2C0_MCS_R=(I2C_MCS_RUN); //Inicio la transmisión 1
    for(i=0;i<150;i++){} //Delay
    esperar();


    I2C0_MDR_R=horas; //Envio el valor de "horas"
    I2C0_MCS_R=(I2C_MCS_RUN); //Inicio la transmisión 2
    for(i=0;i<150;i++){} //Delay
    esperar();


    I2C0_MDR_R=dia; //Envio el valor de "dia"
    I2C0_MCS_R=(I2C_MCS_RUN); //Inicio la transmisión 4
    for(i=0;i<150;i++){} //Delay
    esperar();


    I2C0_MDR_R=fecha; //Envio el valor de "fecha"
    I2C0_MCS_R=(I2C_MCS_RUN); //Inicio la transmisión 5
    for(i=0;i<150;i++){} //Delay
    esperar();


    I2C0_MDR_R=mes; //Envio el valor de "mes"
    I2C0_MCS_R=(I2C_MCS_RUN); //Inicio la transmisión 6
    for(i=0;i<150;i++){} //Delay
    esperar();


    I2C0_MDR_R= anio; //Envio el valor de "año"
    I2C0_MCS_R=(I2C_MCS_STOP|I2C_MCS_RUN); //Inicio la ultima transmisión y STOP
    for(i=0;i<150;i++){} //Delay
    esperar();

}

void leerFecha(){
        while(I2C0_MCS_R&0x00000001){}; // wait for I2C ready

        //Para actualizar registro para iniciar la lectura
        I2C0_MSA_R=(AdreDS1307<<1)&0xFE; //Cargo la dirección del DS1207 e indico "SEND", es decir, el Slave va a recibir
        I2C0_MDR_R=AdreSec&0x0FF; //Envio la Subdirección( dirección del registro interno "segundos") al DS1307
        I2C0_MCS_R=(I2C_MCS_START|I2C_MCS_RUN);// Condición de START, y corro
        for(i=0;i<150;i++){} //Delay
        esperar();


        //Para recibir información
        I2C0_MSA_R=(AdreDS1307<<1)&0xFE; //La dirección del DS1307 en el Master Slave Adress
        I2C0_MSA_R|=0x01; //Indico "RECIEVE", es decir, el Slave va a transmitir
        I2C0_MCS_R=(I2C_MCS_START|I2C_MCS_RUN|I2C_MCS_ACK); // Condición de RESTART, corro, ack
        for(i=0;i<150;i++){} //Delay
        esperar();
        segundos=(I2C0_MDR_R&0xFF); //El Master lee lo que envia el DS1307

        I2C0_MCS_R=(I2C_MCS_RUN|I2C_MCS_ACK); // corro, ack
        for(i=0;i<150;i++){} //Delay
        esperar();
        minutos=(I2C0_MDR_R&0xFF); //El Master lee lo que envía el DS1307

         I2C0_MCS_R=(I2C_MCS_RUN|I2C_MCS_ACK); //corro, ack
         for(i=0;i<150;i++){} //Delay
         esperar();
         horas=(I2C0_MDR_R&0xFF); //El Master lee lo que envía el DS1307

         I2C0_MCS_R=(I2C_MCS_RUN|I2C_MCS_ACK); // corro, ack
         for(i=0;i<150;i++){} //Delay
         esperar();
         dia=(I2C0_MDR_R&0xFF); //El Master lee lo que envía el DS1307

         I2C0_MCS_R=(I2C_MCS_RUN|I2C_MCS_ACK); // corro, ack
         for(i=0;i<150;i++){} //Delay
         esperar();
         fecha=(I2C0_MDR_R&0xFF); //El Master lee lo que envía el DS1307

         I2C0_MCS_R=(I2C_MCS_RUN|I2C_MCS_ACK); // corro, ack
         for(i=0;i<150;i++){} //Delay
         esperar();
         mes=(I2C0_MDR_R&0xFF); //El Master lee lo que envía el DS1307

         I2C0_MCS_R=(I2C_MCS_RUN|I2C_MCS_ACK); // corro, ack
         for(i=0;i<150;i++){} //Delay
         esperar();
         anio=(I2C0_MDR_R&0xFF); //El Master lee lo que envía el DS1307

         I2C0_MCS_R=(I2C_MCS_STOP|I2C_MCS_RUN); //corro, alto
}

//*** PROGRAMA PRINCIPAL ****

void main(){

    I2C_Init(); //Función que inicializa los relojes, el GPIO y el I2C0

    //Inicializo Slave
    while(I2C0_MCS_R&0x00000001){}; // espera que el I2C esté listo


    //Para transmitir

    CargarFecha(); // Función para configurar al esclavo RTC DS1307


    do{
    leerFecha();
    }while(error!=1);



}
