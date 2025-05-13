#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c1294ncpdt.h"
#include "IEEE_CAN.h"

//--------------------------------------------------------------------
//%%%%%%%%%%%    ESCRITURA DE MENSAJE A MEMORIA CAN    %%%%%%%%%%%%%%%
// Dato: Mensaje de 8 Bytes a escribir en localidad NoObj
// NoObj: Localidad de memoria donde se guaradarán el mensaje
//--------------------------------------------------------------------
//ARB bits, Data lenght code, data bytes
void CAN_Memoria_Dato(uint64_t Dato, uint8_t NoObj){
    CAN0_IF1CMSK_R=0x83;                    //WRNRD: Tx datos de CANIF->CAN message object (MNUM)
                                            //DATAA: Tx bytes 0-3 en el mensaje objeto->CANIFNDAn
                                            //DATAB: Tx bytes 4-7 en el mensaje objeto->CANIFNDAn
    //-------------------Escritura de datos---------------------//
    CAN0_IF1DA1_R = Dato & 0x000000000000FFFF;          //Byte 0-1
    CAN0_IF1DA2_R = (Dato & 0x00000000FFFF0000)>>16;    //Byte 2-3
    CAN0_IF1DB1_R = (Dato & 0x0000FFFF00000000)>>32;    //Byte 4-5
    CAN0_IF1DB2_R = (Dato & 0xFFFF000000000000)>>48;    //Byte 6-7
    CAN0_IF1CRQ_R = NoObj;                  //No. del identificador para indicar la prioridad
    while(CAN0_IF1CRQ_R & 0x8000);          //Espera a que se termine una acción de escritura al espaciod e memoria NoObj
}

//--------------------------------------------------------------------
//%%%%%%%    ESCRITURA DE DATOS DE ARBITRAJE A MEMORIA CAN    %%%%%%%%
// ID: Identificador del mensaje 11 bitsCT
// TxRx: (True) Transmisión de datos (False) Recepción de datos
// NoObj: Localidad de memoria donde se guaradarán los datos
//--------------------------------------------------------------------
void CAN_Memoria_Arb(uint16_t ID, bool TxRx, uint8_t ObjNo){
    CAN0_IF1CMSK_R=0xA0;                    //WRNRD: Tx datos de CANIF->CAN message object (MNUM)
                                            //ARB: Tx ID+DIR+XTD+MSGVAL del message object a los registros de interfaz
    CAN0_IF1ARB2_R=0x8000|(ID<<2);          //MSGVAL hab, XTD: 11 bit Standard, DIR: Rx y ID
    if(TxRx){
        CAN0_IF1ARB2_R|=0x2000;             //DIR para Tx
    }
    CAN0_IF1CRQ_R = ObjNo;                  //No. del identificador para indicar la prioridad
    while(CAN0_IF1CRQ_R & 0x8000);          //Espera a que se termine una acción de escritura al espaciod e memoria NoObj
}

//------------------------------------------------------------------------------------
//%%%%%%%    ESCRITURA DE DATOS DE CONTROL Y ENMASCARAMIENTO A MEMORIA CAN    %%%%%%%%
// Mask: Enmascaramiento (Filtro) del mensaje
// DatoL: Longitud del mensaje a enviar/recibir en número de bytes
// TXIE: (True) Interrupción al transmitir dato de este NoObj (False) Sin interrupción
// RXIE: (True) Interrupción al recibir dato en este NoObj (False) Sin interrupción
// RMTEN: (True) Transmisión de trama remota (False) Transmisión de trama de datos
// NoObj: Localidad de memoria donde se guaradarán los datos
//------------------------------------------------------------------------------------
void CAN_Memoria_CtrlMsk(uint16_t Mask, uint8_t DatoL, bool TXIE, bool RXIE, bool RMTEN, uint8_t ObjNo){
    CAN0_IF1CMSK_R=0xD0;                    //WRNRD: Tx datos de CANIF->CAN message object (MNUM)
                                            //MASK: Tx IDMASK+DIR+MXTD del mensaje objeto->registros de interfaz
                                            //CONTROL: Datos de control IFnMCTL->registros de interfaz
    CAN0_IF1MCTL_R=(0x80|DatoL);            //DLC: Num bytes en el data frame, EOB (end of Buffer): Mensaje único
    if(RMTEN){
        CAN0_IF1MCTL_R|=0x200;            //RMTEN: Hab de trama remota
    }
    if(RXIE){
        CAN0_IF1MCTL_R|=0x400;            //RXIE: Hab de interrupción al recibir un mensaje en este NoObj
    }
    if(TXIE){
        CAN0_IF1MCTL_R|=0x800;            //TXIE:  Hab de interrupción al transmitir el mensaje en este NoObj
    }
    if(Mask){
      CAN0_IF1MSK1_R=0x0;
      CAN0_IF1MSK2_R=Mask<<2;                //Aplicación del enmascaramiento
      CAN0_IF1MCTL_R|=0x1000;                //UMASK: Hab de enmsacaramiento
    }
    CAN0_IF1CRQ_R = ObjNo;                  //No. del identificador para indicar la prioridad
    while(CAN0_IF1CRQ_R & 0x8000);          //Espera a que se termine una acción de escritura al espaciod e memoria NoObj
}

//------------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%    TRNSMISIÓN DE DATOS CAN    %%%%%%%%%%%%%%%%%%%%%%
// NoObj: Localidad de memoria de donde se tomará el mensaje a transmitir
//------------------------------------------------------------------------
void CAN_Tx(uint64_t NoObj){
    CAN0_IF1CMSK_R = 0x84;                  //WRNRD: Tx datos de CANIF->CAN message object (MNUM)
                                            //Hab NEWDAT/TXRQST: Como WRNRD=1; una Tx es solicitada
    CAN0_IF1CRQ_R = NoObj;                  //No. del identificador para indicar la prioridad
    while(CAN0_IF1CRQ_R & 0x8000);          //Espera a que se termine una acción de escritura a memoria
}

//------------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%    RECEPCIÓN DE DATOS CAN    %%%%%%%%%%%%%%%%%%%%%%
// NoObj: Localidad de memoria de donde se tomará el mensaje a transmitir
// return: Dato de 8 bytes alojado en la memoria del controlador CAN o error
//------------------------------------------------------------------------
uint64_t CAN_Rx(uint8_t ObjNo){
    uint64_t Dato=0;
    CAN0_IF1CMSK_R = 0x13;                 //Hab CONTROL, DATAA, DATAB
    CAN0_IF1CRQ_R = ObjNo;                 //No. de identificador para alojar el objeto mensaje
    while(CAN0_IF1CRQ_R & 0x8000);         //Espera a que se termine una acción de lectura/escritura
    if(CAN0_IF1MCTL_R & 0x8000){           //Si NEWDAT bit está hab, hay un nuevo dato en los registros de datos
        //------------Obtención de los nuevos datos-----------------//
            Dato|=(0x00000000000000FF & CAN0_IF1DA1_R);         //Byte 0
        if((CAN0_IF1MCTL_R & 0xF)>= 2){
            Dato|=(0x000000000000FF00 & CAN0_IF1DA1_R);         //Byte 1
        }
        if((CAN0_IF1MCTL_R & 0xF)>= 3){
            Dato|=(0x0000000000FF0000 & (CAN0_IF1DA2_R<<16));   //Byte 2
        }
        if((CAN0_IF1MCTL_R & 0xF)>= 4){
            Dato|=(0x00000000FF000000 & (CAN0_IF1DA2_R<<16));   //Byte 3
        }
        if((CAN0_IF1MCTL_R & 0xF)>= 5){
            Dato|=(0x000000FF00000000 & ((uint64_t)(CAN0_IF1DB1_R)<<32));   //Byte 4
        }
        if((CAN0_IF1MCTL_R & 0xF)>= 6){
            Dato|=(0x0000FF0000000000 & ((uint64_t)(CAN0_IF1DB1_R)<<32));   //Byte 5
        }
        if((CAN0_IF1MCTL_R & 0xF)>= 7){
            Dato|=(0x00FF000000000000 & ((uint64_t)(CAN0_IF1DB2_R)<<48));   //Byte 6
        }
        if((CAN0_IF1MCTL_R & 0xF)>= 8){
            Dato|=(0xFF00000000000000 & ((uint64_t)(CAN0_IF1DB2_R)<<48));   //Byte 7
        }
        CAN0_IF1CMSK_R |= 0x4;             //Hab NEWDAT para limpiar el NEWDAT de IFnMCTL
    }
    if(CAN0_IF1MCTL_R & 0x4000){           //Si MSGLST bit está hab, hubo un mensaje perdido
        CAN0_IF1MCTL_R &= ~0x4000;         //Limpieza del MSGLST bit
        return 0xFFFFFF;                   //Indicador de un error
    }
    CAN0_IF1CMSK_R |= 0x8;                 //Hab CLRINTPND para limpiar el INTPND de IFnMCTL
    CAN0_IF1CRQ_R =ObjNo;                  //No. de identificador para ejecutar las acciones anteriores
    return Dato;
}

////------------------------------------------------------------------------
////%%%%%%%%%%%%%%%%%%%%%%%    INICIALIZACIÓN I2C0    %%%%%%%%%%%%%%%%%%%%%%
//// NoObj: Localidad de memoria de donde se tomará el mensaje a transmitir
////------------------------------------------------------------------------
//void CAN_TxRemota(void){
//    CAN0_IF1ARB2_R&=~0x2000;          //MSGVAL hab, XTD: 11 bit Standard, DIR para Rx y ID
//}
//
////------------------------------------------------------------------------
////%%%%%%%%%%%%%%%%%%%%%%%    INICIALIZACIÓN I2C0    %%%%%%%%%%%%%%%%%%%%%%
//// NoObj: Localidad de memoria de donde se tomará el mensaje a transmitir
////------------------------------------------------------------------------
//void CAN_RxRemota(void){
//    CAN0_IF2ARB2_R|=0x2000;            //DIR para Tx
//    CAN0_IF2MCTL_R|=0x200;             //RMTEN: Solicitud de Tx Remota
//}


