/********************************** (C) COPYRIGHT ******************************
* File Name          : DNS_Client.c
* Author             : 
* Version            : 
* Date               : 
* Description        : CH57xNET
*                      (1)��CH579 Examples by KEIL;
*                      (2)������0��������Ϣ,115200bps;
*                      (3)��������������ʾUDP �ͻ���ͨѶ����Ƭ���յ����ݺ󣬻ش���Ŀ��IP��λ��
*******************************************************************************/



/******************************************************************************/
/* ͷ�ļ�����*/
#include <stdio.h>
#include <string.h>
#include "CH57x_common.h"
#include "core_cm0.h"
#include "CH57xNET.H"
#include "DNS.H"

#define KEEPLIVE_ENABLE                      1                                  /* ����KEEPLIVE���� */

/* ����Ļ�������ȫ�ֱ�������Ҫ���壬���е��� */
__align(16)UINT8    CH57xMACRxDesBuf[(RX_QUEUE_ENTRIES )*16];                   /* MAC������������������16�ֽڶ��� */
__align(4) UINT8    CH57xMACRxBuf[RX_QUEUE_ENTRIES*RX_BUF_SIZE];                /* MAC���ջ�������4�ֽڶ��� */
__align(4) SOCK_INF SocketInf[CH57xNET_MAX_SOCKET_NUM];                         /* Socket��Ϣ����4�ֽڶ��� */

const UINT16 MemNum[8] = {CH57xNET_NUM_IPRAW,
                         CH57xNET_NUM_UDP,
                         CH57xNET_NUM_TCP,
                         CH57xNET_NUM_TCP_LISTEN,
                         CH57xNET_NUM_TCP_SEG,
                         CH57xNET_NUM_IP_REASSDATA,
                         CH57xNET_NUM_PBUF,
                         CH57xNET_NUM_POOL_BUF
                         };
const UINT16 MemSize[8] = {CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_IPRAW_PCB),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_UDP_PCB),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_TCP_PCB),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_TCP_PCB_LISTEN),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_TCP_SEG),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_IP_REASSDATA),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_PBUF) + CH57xNET_MEM_ALIGN_SIZE(0),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_PBUF) + CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_POOL_BUF)
                         };
__align(4)UINT8 Memp_Memory[CH57xNET_MEMP_SIZE];
__align(4)UINT8 Mem_Heap_Memory[CH57xNET_RAM_HEAP_SIZE];
__align(4)UINT8 Mem_ArpTable[CH57xNET_RAM_ARP_TABLE_SIZE];
/******************************************************************************/
/* ����ʾ�������غ� */
#define RECE_BUF_LEN                          536                               /* ���ջ������Ĵ�С */
/* CH57xNET��TCP��MSS����Ϊ536�ֽڣ���һ��TCP��������ݲ����Ϊ536�ֽ� */
/* TCPЭ��ջ���û������ڽ������أ��������ֵΪsocket�Ľ��ջ��������ȡ����趨 */
/* RX_QUEUE_ENTRIESʱҪ����MSS�ʹ���֮��Ĺ�ϵ�����細��ֵΪ4*MSS����Զ��һ�λᷢ�� */
/* 4��TCP�������RX_QUEUE_ENTRIESС��4�����Ȼ�ᵼ�����ݰ���ʧ���Ӷ�����ͨѶЧ�ʽ��� */
/* ����RX_QUEUE_ENTRIESҪ����( ����/MSS ),������socketͬʱ���д������������ݣ��� */ 
/* ����RX_QUEUE_ENTRIESҪ����(( ����/MSS )*socket����) �ڶ��socketͬʱ���д��������շ�ʱ */
/* Ϊ�˽�ԼRAM���뽫���ջ������ĳ�������ΪMSS */

/* CH579��ض��� */
UINT8 MACAddr[6] = {0x84,0xc2,0xe4,0x02,0x03,0x04};                             /* CH579MAC��ַ */
UINT8 IPAddr[4] = {192,168,1,200};                                              /* CH579IP��ַ */
UINT8 GWIPAddr[4] = {192,168,1,1};                                              /* CH579���� */
UINT8 IPMask[4] = {255,255,255,0};                                              /* CH579�������� */

UINT8 SocketId;                                                                 /* ����socket���������Բ��ö��� */
UINT8 SocketRecvBuf[RECE_BUF_LEN];                                              /* socket���ջ����� */
UINT8 MyBuf[RECE_BUF_LEN];                                                      /* ����һ����ʱ������ */

extern UINT8 dns_buf[512];

UINT8 status;
#define   MAX_URL_SIZE 128

UINT8  url_dn1[MAX_URL_SIZE] = "www.baidu.com";        
UINT8  url_dn2[MAX_URL_SIZE] = "www.tencent.com";        
UINT8  url_dn3[MAX_URL_SIZE] = "www.wch.cn";        


//extern const UINT16 *memp_num;

/*******************************************************************************
* Function Name  : IRQ_Handler
* Description    : IRQ�жϷ�����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ETH_IRQHandler( void )							/* ��̫���ж� */
{
	CH57xNET_ETHIsr();								/* ��̫���ж��жϷ����� */
}

void TMR0_IRQHandler( void ) 						/* ��ʱ���ж� */
{
	CH57xNET_TimeIsr(CH57xNETTIMEPERIOD);           /* ��ʱ���жϷ����� */
	R8_TMR0_INT_FLAG |= 0xff;						/* �����ʱ���жϱ�־ */
}



/*******************************************************************************
* Function Name  : mStopIfError
* Description    : ����ʹ�ã���ʾ�������
* Input          : iError �������
* Output         : None
* Return         : None
*******************************************************************************/
void mStopIfError(UINT8 iError)
{
    if (iError == CH57xNET_ERR_SUCCESS) return;                                 /* �����ɹ� */
    PRINT("mStopIfError: %02X\r\n", (UINT16)iError);                           /* ��ʾ���� */
}

/*******************************************************************************
* Function Name  : net_initkeeplive
* Description    : keeplive��ʼ��
* Input          : None      
* Output         : None
* Return         : None
*******************************************************************************/
#ifdef  KEEPLIVE_ENABLE
void net_initkeeplive(void)
{
    struct _KEEP_CFG  klcfg;

    klcfg.KLIdle = 20000;                                                       /* ���� */
    klcfg.KLIntvl = 10000;                                                      /* ��� */
    klcfg.KLCount = 5;                                                          /* ���� */
    CH57xNET_ConfigKeepLive(&klcfg);
}
#endif

/*******************************************************************************
* Function Name  : CH57xNET_LibInit
* Description    : ���ʼ������
* Input          : ip      ip��ַָ��
*                ��gwip    ����ip��ַָ��
*                : mask    ����ָ��
*                : macaddr MAC��ַָ�� 
* Output         : None
* Return         : ִ��״̬
*******************************************************************************/
UINT8 CH57xNET_LibInit(/*const*/ UINT8 *ip,/*const*/ UINT8 *gwip,/*const*/ UINT8 *mask,/*const*/ UINT8 *macaddr)
{
    UINT8 i;
    struct _CH57x_CFG cfg;

    if(CH57xNET_GetVer() != CH57xNET_LIB_VER)return 0xfc;                       /* ��ȡ��İ汾�ţ�����Ƿ��ͷ�ļ�һ�� */
    CH57xNETConfig = LIB_CFG_VALUE;                                             /* ��������Ϣ���ݸ�������ñ��� */
    cfg.RxBufSize = RX_BUF_SIZE; 
    cfg.TCPMss   = CH57xNET_TCP_MSS;
    cfg.HeapSize = CH57x_MEM_HEAP_SIZE;
    cfg.ARPTableNum = CH57xNET_NUM_ARP_TABLE;
    cfg.MiscConfig0 = CH57xNET_MISC_CONFIG0;
    CH57xNET_ConfigLIB(&cfg);
    i = CH57xNET_Init(ip,gwip,mask,macaddr);
#ifdef  KEEPLIVE_ENABLE
    net_initkeeplive( );
#endif
    return (i);                      /* ���ʼ�� */
}

/*******************************************************************************
* Function Name  : CH57xNET_HandleSockInt
* Description    : Socket�жϴ�������
* Input          : sockeid  socket����
*                ��initstat �ж�״̬
* Output         : None
* Return         : None
*******************************************************************************/
void CH57xNET_HandleSockInt(UINT8 sockeid,UINT8 initstat)
{
    UINT32 len;
    UINT8 i;

    if(initstat & SINT_STAT_RECV)                                               /* �����ж� */
    {
        len = CH57xNET_SocketRecvLen(sockeid,NULL);                             /* �Ὣ��ǰ����ָ�봫�ݸ�precv*/
        PRINT("Receive Len = %02x\n",len);                           
            if (len > 0)
            {    
                if((sockeid==0)&&(status ==3))
                {
			    PRINT("enter rece int!STATUS=3\r\n");
                CH57xNET_SocketRecv(sockeid,dns_buf,&len);
                status = 4;
                i = CH57xNET_SocketClose( sockeid,TCP_CLOSE_NORMAL );
                mStopIfError(i);
                PRINT("status = 4!\n");
                }
                else
                  CH57xNET_SocketRecv(sockeid,MyBuf,&len);                      /* �����ջ����������ݶ���MyBuf��*/
            }
    }
    if(initstat & SINT_STAT_CONNECT)                                            /* TCP�����ж� */
    {                                                               /* �������жϱ�ʾTCP�Ѿ����ӣ����Խ����շ����� */
        PRINT("TCP Connect Success\n");                           
    }
    if(initstat & SINT_STAT_DISCONNECT)                                         /* TCP�Ͽ��ж� */
    {                                                              /* �������жϣ�CH579���ڲ��Ὣ��socket�������Ϊ�ر�*/
        PRINT("TCP Disconnect\n");                                             /* Ӧ������������´������� */

    }
    if(initstat & SINT_STAT_TIM_OUT)                                            /* TCP��ʱ�ж� */
    {                                                              /* �������жϣ�CH579���ڲ��Ὣ��socket�������Ϊ�ر�*/
        PRINT("TCP Timout\n");                                                 /* Ӧ������������´������� */

    }
}


/*******************************************************************************
* Function Name  : CH57xNET_HandleGloableInt
* Description    : ȫ���жϴ�������
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CH57xNET_HandleGlobalInt(void)
{
    UINT8 initstat;
    UINT8 i;
    UINT8 socketinit;
    initstat = CH57xNET_GetGlobalInt();                                         /* ��ȫ���ж�״̬����� */
    if(initstat & GINT_STAT_UNREACH)                                            /* ���ɴ��ж� */
    {
        PRINT("UnreachCode ��%d\n",CH57xInf.UnreachCode);                      /* �鿴���ɴ���� */
        PRINT("UnreachProto ��%d\n",CH57xInf.UnreachProto);                    /* �鿴���ɴ�Э������ */
        PRINT("UnreachPort ��%d\n",CH57xInf.UnreachPort);                      /* ��ѯ���ɴ�˿� */     
    }
   if(initstat & GINT_STAT_IP_CONFLI)                                           /* IP��ͻ�ж� */
   {
   
   }
   if(initstat & GINT_STAT_PHY_CHANGE)                                          /* PHY�ı��ж� */
   {
       i = CH57xNET_GetPHYStatus();                                             /* ��ȡPHY״̬ */
       if(i>=0x02) status = 1;
       PRINT("status = 1!\n");
       PRINT("GINT_STAT_PHY_CHANGE %02x\n",i);
   }
   if(initstat & GINT_STAT_SOCKET)                                              /* Socket�ж� */
   {
       for(i = 0; i < CH57xNET_MAX_SOCKET_NUM; i ++)                     
       {
           socketinit = CH57xNET_GetSocketInt(i);                               /* ��socket�жϲ����� */
           if(socketinit)CH57xNET_HandleSockInt(i,socketinit);                  /* ������ж������� */
       }    
   }
}

/*******************************************************************************
* Function Name  : UDPSocketParamInit
* Description    : ����UDP socket
* Input          : socket����ֵ��Դ�˿ڣ�Ŀ�Ķ˿ڼ�Ŀ��IP
* Output         : None
* Return         : None
*******************************************************************************/
void UDPSocketParamInit(UINT8 S,UINT8 *addr,UINT16 SourPort,UINT16 DesPort )
{
   UINT8 i;                                                             
   SOCK_INF TmpSocketInf;                                                       /* ������ʱsocket���� */

   memset((void *)&TmpSocketInf,0,sizeof(SOCK_INF));                            /* ���ڲ��Ὣ�˱������ƣ�������ý���ʱ������ȫ������ */
   memcpy((void *)TmpSocketInf.IPAddr,addr,4);                                  /* ����Ŀ��IP��ַ */
 
   TmpSocketInf.DesPort = DesPort;                                              /* ����Ŀ�Ķ˿� */
   TmpSocketInf.SourPort = SourPort;                                            /* ����Դ�˿� */
   TmpSocketInf.ProtoType = PROTO_TYPE_UDP;                                     /* ����socekt���� */
   TmpSocketInf.RecvStartPoint = (UINT32)SocketRecvBuf;                         /* ���ý��ջ������Ľ��ջ����� */
   TmpSocketInf.RecvBufLen = RECE_BUF_LEN ;                                     /* ���ý��ջ������Ľ��ճ��� */
   i = CH57xNET_SocketCreat(&S,&TmpSocketInf);                           /* ����socket�������ص�socket����������SocketId�� */
   mStopIfError(i);                                                             /* ������ */
}


/*******************************************************************************
* Function Name  :  Timer0Init
* Description    : ��ʱ��1��ʼ��
* Input          : time ��ʱʱ��
* Output         : None
* Return         : None
*******************************************************************************/
void Timer0Init(UINT32 time)
{
	R8_TMR0_CTRL_MOD = RB_TMR_ALL_CLEAR;		                               /* ������м���ֵ */
	R8_TMR0_CTRL_MOD = 0;						                               /* ���ö�ʱ��ģʽ */
	R32_TMR0_CNT_END = FREQ_SYS/1000000*time;	                               /* ���ö�ʱʱ�� */
	R8_TMR0_INT_FLAG = R8_TMR0_INT_FLAG;		                               /* �����־ */
	R8_TMR0_INTER_EN = RB_TMR_IE_CYC_END;	                                   /* ��ʱ�ж� */
	R8_TMR0_CTRL_MOD |= RB_TMR_COUNT_EN;
	NVIC_EnableIRQ(TMR0_IRQn);	
}

/*******************************************************************************
* Function Name  : SystemClock_UART1_init
* Description    : ϵͳʱ�Ӻʹ���1��ʼ��������1���ڴ�ӡ���
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SystemClock_UART1_init(void)
{
    PWR_UnitModCfg(ENABLE, UNIT_SYS_PLL);                                      /* PLL�ϵ� */
    DelsyMs(3); 
    SetSysClock(CLK_SOURCE_HSE_32MHz);                                          /* �ⲿ���� PLL ���32MHz */
    GPIOA_SetBits( GPIO_Pin_9 );
    GPIOA_ModeCfg( GPIO_Pin_9, GPIO_ModeOut_PP_5mA );                           /* ����1��IO������ */
	UART1_DefInit( );                                                            /* ����1��ʼ�� */
}

/*******************************************************************************
* Function Name  : main
* Description    : ������
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int main(void) 
{
    UINT8 ip[4],i = 0;
    UINT8 dns_flag = 0;
		
	SystemClock_UART1_init();	
    i = CH57xNET_LibInit(IPAddr,GWIPAddr,IPMask,MACAddr);                       /* ���ʼ�� */
    mStopIfError(i);                                                            /* ������ */
    PRINT("CH57xNETLibInit Success\r\n");  
	Timer0Init( 10000 );		                                                /*��ʼ����ʱ��:10ms*/
	NVIC_EnableIRQ(ETH_IRQn);
	while ( CH57xInf.PHYStat < 2 ) {
		DelsyMs(50);
	}
    PRINT("CH579 DNS client create��\r\n"); 
	
    while(1)
    {   	
        CH57xNET_MainTask();                                                    /* CH57xNET��������������Ҫ����ѭ���в��ϵ��� */
        if(CH57xNET_QueryGlobalInt())CH57xNET_HandleGlobalInt();                /* ��ѯ�жϣ�������жϣ������ȫ���жϴ������� */
        if( dns_flag == 0 ){
            i = DnsQuery(SocketId,url_dn3,ip);                                         /* ��ѯ */
             if(i){ 
                dns_flag = 1;
                if( i == 1 ){
                    PRINT("Domain name:%s \n",url_dn3);
                    PRINT(" HTTPs_IP=%d.%d.%d.%d\n\n",ip[0],ip[1],ip[2],ip[3]);
                }  
            }
        }     
	}
}

/*********************************** endfile **********************************/