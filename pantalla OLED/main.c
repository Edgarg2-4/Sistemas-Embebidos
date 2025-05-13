///*** PROGRAMA QUE CONFIGURA EL PERIFERICO I2C ***
//*** EN EL TIVA TM4C1294 COMO MAESTRO Y ESCLAVO Pantalla OLED***

//Librerias
#include<stdint.h>
#include<stdbool.h>

//REGISTROS DE RELOJ
#define SYSCTL_RCGCGPIO_R       (*((volatile uint32_t *)0x400FE608)) //Reloj del puerto
#define SYSCTL_RCGCI2C_R        (*((volatile uint32_t *)0x400FE620)) //Reloj de I2C
#define SYSCTL_PRGPIO_R        (*((volatile uint32_t *)0x400FEA08)) //Bandera de "Peripherial Ready"

//REGISTROS DEL PUERTO B
#define GPIO_PORTB_DATA_R   (*((volatile uint32_t *)0x400593FC)) //Para los datos del puerto
#define GPIO_PORTB_DIR_R    (*((volatile uint32_t *)0x40059400)) //Para seleccionar funci√≥n
#define GPIO_PORTB_AFSEL_R  (*((volatile uint32_t *)0x40059420)) //Para seleccionar funci√≥n alterna
#define GPIO_PORTB_ODR_R    (*((volatile uint32_t *)0x4005950C)) //Para activar el Open Drain
#define GPIO_PORTB_DEN_R    (*((volatile uint32_t *)0x4005951C)) //Para activar funci√≥n digital
#define GPIO_PORTB_PCTL_R   (*((volatile uint32_t *)0x4005952C)) //Para el control del puerto

//REGISTROS DEL M√ìUDLO I2C
#define I2C0_MSA_R              (*((volatile uint32_t *)0x40020000)) //I2C Master Slave Adress
#define I2C0_MCS_R              (*((volatile uint32_t *)0x40020004)) //I2C Master Control Status
#define I2C0_MDR_R              (*((volatile uint32_t *)0x40020008)) //I2C Master Data Register
#define I2C0_MTPR_R             (*((volatile uint32_t *)0x4002000C)) //I2C Master Time Period
#define I2C0_MCR_R              (*((volatile uint32_t *)0x40020020)) //I2C Master Configuration Register
#define I2C0_MICR_R              (*((volatile uint32_t *)0x4002001C)) //I2C Master Configuration Register
#define I2C0_MIMR_R              (*((volatile uint32_t *)0x40020010))
#define I2C0_EN0_R              (*((volatile uint32_t *)0xE000E100))
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

//Define de comandos
//DEFINICIONES DE SSD1306
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_SETCONTRAST 0x81         ///< See datasheet
#define SSD1306_CHARGEPUMP 0x8D          ///< See datasheet
#define SSD1306_SEGREMAP 0xA0            ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_NORMALDISPLAY 0xA6       ///< See datasheet
#define SSD1306_SETMULTIPLEX 0xA8        ///< See datasheet
#define SSD1306_DISPLAYOFF 0xAE          ///< See datasheet
#define SSD1306_DISPLAYON 0xAF           ///< See datasheet
#define SSD1306_COMSCANINC 0xC0          ///< Not currently used
#define SSD1306_COMSCANDEC 0xC8          ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET 0xD3    ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5  ///< See datasheet
#define SSD1306_SETCOMPINS 0xDA          ///< See datasheet
#define SSD1306_SETSTARTLINE 0x40  ///< See datasheet
#define SSD1306_SETPAGE0 0XB0
#define SSD1306_SETPAGE1 0XB1
#define SSD1306_SETPAGE2 0XB2
#define SSD1306_SETPAGE3 0XB3
#define SSD1306_SCROLLINGL_ 0X27
#define SSD1306_SCROLLINGR_ 0X26


//**Direcciones del SSD1306
int AdreSSD1306 =0x1E;///Direcci√≥n del Pantalla Oled SSD1306
int AdreCOM= 0x00;
int AdreDATA=0x01;

/*El c√°lculo del Time Period Register (TPR) se especifica en la p√°gina 1284
 Asumiendo un reloj de 16 MHz y un modo de operaci√≥n est√°ndar (100 kbps):
*/
int TPR = 7;

// Variables para manejar los valores del RTC
uint8_t error;
int i;
int cont=1;

void I2C_Init(void){
    //CONFIGURACI”N DE LOS RELOJ
    SYSCTL_RCGCI2C_R |= 0x0001; // Activamos el reloj de I2C0 [I2C9 I2C8 I2C7 ... I2C0]<--Mapa de RCGCI2C
    SYSCTL_RCGCGPIO_R |= 0x0002; // Activamos el reloj GPIO_PORTB mientras se activa el reloj de I2C0
    while((SYSCTL_PRGPIO_R&0x0002) == 0){};//Espero a que se active el reloj del puerto B

    /* Acorde con la tabla "Signals by function" de la p. 1808:
     el PIN 2 del puerto B (PB2) es el I2C0SCL del I2C0, y
     el PIN 3 del puerto B (PB3) es el I2C0SDA del I2C0 */

    //CONFIGURACI”N DE LOS GPIOS
    GPIO_PORTB_AFSEL_R |= 0x0C; // Activo la funciÛn alterna del PB2 y PB3
    GPIO_PORTB_ODR_R |= 0x08;   // Activo el OPEN DRAIN para el PB3, ya que el PB2 ya tiene uno por preconfig.
    GPIO_PORTB_DIR_R |= 0x0C;   //Activo al PB2 y al PB3 como OUTPUT
    GPIO_PORTB_DEN_R |= 0x0C;   //Activo la funciÛn digital de PB3 y PB2

   // GPIO_PORTB_PUR_R |= 0x0C;        //     habilita weak pull-up on PB3 Y PB2
    /*AsÌ como el registro AFSEL indica que se ejecutar· una funciÛn externa, en el registro PCTL
    debemos indicar QU… funciÛn alterna se realizar· acorde con la tabla 26-5 de la p.1808 e indicarlo
     en el correspondiente PCMn (uno por cada bit del puerto) del registro PCTL*/

    GPIO_PORTB_PCTL_R|=0x00002200;

    //CONFIGURACI”N DEL MODULO I2C0
    I2C0_MCR_R = 0x00000010; // Habilitar funciÛn MASTER para el I2C0
    I2C0_MTPR_R = TPR; // Se establece una velocidad est·ndar de 100kbps
}




// ** Funci√≥n esperar **
int esperar(){
    while(I2C0_MCS_R&0x00000001){}; //Espero a que la transmisi√≥n acabe
      if(I2C0_MCS_R&0x00000002==1){ //¬øHubo error?
          error=1;
          return error;
      };
      return 0;

}

I2C_comando(int comando){
    while(I2C0_MCS_R&0x00000001){};//i2C RADY?
    I2C0_MSA_R=(AdreSSD1306<<1)&0xFE; //Cargo la direcci√≥n del DS1307 e indico "SEND", es decir, el Slave va a recibir
    I2C0_MDR_R=AdreCOM&0x0FF; //Envio la Subdirecci√≥n( direcci√≥n del registro interno "segundos") al DS1307
    I2C0_MCS_R=(I2C_MCS_RUN|I2C_MCS_START); // Condici√≥n de START y corro
for(i=0;i<150; i++){}
esperar();    
    I2C0_MDR_R=comando; //Envio el valor de "segundos"
    I2C0_MCS_R=(I2C_MCS_STOP|I2C_MCS_RUN);
    for(i=0;i<300;i++){} //Delay
            esperar();
}

I2C_Data(int data){
    while(I2C0_MCS_R&0x00000001){};//i2C RADY?
    I2C0_MSA_R=(AdreSSD1306<<1)&0xFE; //Cargo la direcci√≥n del DS1307 e indico "SEND", es decir, el Slave va a recibir
    I2C0_MDR_R=AdreDATA&0x0FF; //Envio la Subdirecci√≥n( direcci√≥n del registro interno "segundos") al DS1307
    I2C0_MCS_R=(I2C_MCS_RUN|I2C_MCS_START); // Condici√≥n de START y corro
    for(i=0;i<150; i++){}
esperar();
    I2C0_MDR_R=data; //Envio el valor de "segundos"
    I2C0_MCS_R=(I2C_MCS_STOP|I2C_MCS_RUN);
    for(i=0;i<300;i++){} //Delay
            esperar();
}
void SSD1306_Init(void){

    for(i=0;i<400;i++){}
    I2C_comando(SSD1306_DISPLAYOFF);
    I2C_comando(SSD1306_SETMULTIPLEX);
    I2C_comando(0x1F);
    I2C_comando(SSD1306_SETDISPLAYOFFSET);
    I2C_comando(0x00);
    I2C_comando(SSD1306_SETSTARTLINE);
    I2C_comando(SSD1306_SEGREMAP|0x1);
    I2C_comando(SSD1306_COMSCANDEC);
    I2C_comando(SSD1306_SETCOMPINS);
    I2C_comando(0x02);
    I2C_comando(SSD1306_SETCONTRAST);
    I2C_comando(0x8F);
    I2C_comando(SSD1306_DISPLAYALLON_RESUME);
    I2C_comando(SSD1306_NORMALDISPLAY);
    I2C_comando(SSD1306_SETDISPLAYCLOCKDIV);
    I2C_comando(0x80);
    I2C_comando(SSD1306_CHARGEPUMP);
    I2C_comando(0x14);
    I2C_comando(SSD1306_DISPLAYON);

}
void SSD1306_Clear(void){
    int c;
    I2C_comando(SSD1306_SETPAGE0);
for(c=0;c<128;c++){
    I2C_Data(0x00);
}
I2C_comando(SSD1306_SETPAGE1);
for(c=0;c<128;c++){
    I2C_Data(0x00);
}
I2C_comando(SSD1306_SETPAGE2);
for(c=0;c<128;c++){
    I2C_Data(0x00);
}
I2C_comando(SSD1306_SETPAGE3);
for(c=0;c<128;c++){
    I2C_Data(0x00);}
    I2C_comando(SSD1306_SETPAGE0);
}

void SSD1306_SCROLLINGR(void){
    I2C_comando(SSD1306_SCROLLINGR_);
    I2C_comando(0x00);
    I2C_comando(0x00);
    I2C_comando(0x00);
    I2C_comando(0x03);
    I2C_comando(0x00);
    I2C_comando(0xFF);
    I2C_comando(0x2F);
    }

void Letra_A(void){
    I2C_Data(0xFF);
    I2C_Data(0xFF);
    I2C_Data(0x33);
    I2C_Data(0x33);
    I2C_Data(0xFF);
    I2C_Data(0xFF);
    I2C_Data(0x00);
}



/**
 * main.c
 */
int main(void)
{
    I2C_Init(); //Funci√≥n que inicializa los relojes, el GPIO y el I2C0

    //Inicializo Slave
    while(I2C0_MCS_R&0x00000001){}; // espera que el I2C est√© listo
    SSD1306_Init();
    SSD1306_Clear();

    I2C_comando(SSD1306_SETPAGE0);

    //Para transmitir
   
    Letra_A();

    while (1){
       I2C_comando(SSD1306_DISPLAYON);
    }

}
