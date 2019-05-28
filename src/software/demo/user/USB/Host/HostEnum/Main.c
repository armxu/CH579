/********************************** (C) COPYRIGHT *******************************
* File Name          : Main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/12/15
* Description 		 : USB�豸ö��
*******************************************************************************/

#define USER_ALLOCAT

#include "CH57x_common.h"

__align(4) UINT8  RxBuffer[ MAX_PACKET_SIZE ] ;      // IN, must even address
__align(4) UINT8  TxBuffer[ MAX_PACKET_SIZE ] ;      // OUT, must even address
__align(4) UINT8  Com_Buffer[ 128 ];                 // �����û���ʱ������,ö��ʱ���ڴ���������,ö�ٽ���Ҳ����������ͨ��ʱ������

int main()
{	
	UINT8	s;
    
	SetSysClock( CLK_SOURCE_HSE_32MHz );
    PWR_UnitModCfg( ENABLE, UNIT_SYS_PLL );		// ��PLL
    DelsyMs(5);
    
	GPIOA_SetBits(GPIO_Pin_9);
	GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
	UART1_DefInit();
	PRINT( "Start @ChipID=%02X\n", R8_CHIP_ID );
	
    pHOST_RX_RAM_Addr = RxBuffer;
    pHOST_TX_RAM_Addr = TxBuffer;
	USB_HostInit();
	PRINT( "Wait Device In\n" );
	while(1)
	{
		s = ERR_SUCCESS;
		if ( R8_USB_INT_FG & RB_UIF_DETECT ) {  // �����USB��������ж�����
			R8_USB_INT_FG = RB_UIF_DETECT ; 
			s = AnalyzeRootHub( );   
			if ( s == ERR_USB_CONNECT ) 		FoundNewDev = 1;
		}
		
		if ( FoundNewDev || s == ERR_USB_CONNECT ) {  // ���µ�USB�豸����
			FoundNewDev = 0;
			mDelaymS( 200 );  // ����USB�豸�ղ�����δ�ȶ�,�ʵȴ�USB�豸���ٺ���,������ζ���
			s = InitRootDevice( Com_Buffer );  // ��ʼ��USB�豸
			//if ( s != ERR_SUCCESS ) 	return( s );
		}
	}
}
