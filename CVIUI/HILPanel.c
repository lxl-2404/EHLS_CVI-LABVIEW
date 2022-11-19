#include "HPS_Controller.h"
#include <userint.h>
#include "HILPanel.h"
#include "ExtraVar.h"
#include <udpsupp.h>
#include <tcpsupp.h>
#include "Functions.h"
#include <formatio.h>
#include <utility.h>
#include <ansi_c.h>

#define ANY_CHART_TRACE         1
#define TORQUE_CHART_TRACE      5
#define PROPORTION_TRACE        4
#define VMIC_TEST_TRACE			9

extern char DftPathSign; //默认路径标志位
extern char pathpass[];  //路径存放数组
extern char originalkey[] ;
extern int keyerr = 1;
static int HILPanelHandle;

 /*
#define ANY_CHART_TRACE              1
#define FRE_SCAN_TEST     	         2
#define ANALOGY_FRE_SCAN_TEST     	 9   
  */
#define  READBUFFERLEN	  1024    

TESTDATA ReadDataMessage;
//TESTDATANEW  ReadDataMessageBuff[READBUFFERLEN];  //采用了新的发送数据方式，发送16位的字节数
//int BufferWriteIndex=0;
//int BufferReadIndex=0;
 
MessageHead sendMessageHead; 
MessageHead readMessageHead;  
CommandMessage sendCommandMessage; 
CommandMessage readCommandMessage; 

extern unsigned int gTCPConnection;
//static unsigned int 	gCommandConnection				= TCP_INVALID_CONNECTION;  
static int 				gLock							= 0;

static int CVICALLBACK TCPDataCallback (unsigned handle, int xType, int errCode, void *cbData);   
int transdatasizeH = 36;

static unsigned int		gUDPChannel			= 0;

#define UDP_PORT   20000
#define MAX_IP4_STRING_LENGTH		15
#define MAX_CLIENT_NAME_LENGTH		30

int CVICALLBACK	UDPChatCallback (unsigned channel, int eventType, int errCode, void *callbackData);   
typedef unsigned char IPAddr[MAX_IP4_STRING_LENGTH + 1]; 
typedef unsigned char ClientName[MAX_CLIENT_NAME_LENGTH + 1]; 

//油源串口命令字
extern uint16_t send_reg[32]; 
extern uint16_t tag_reg[32];   
extern unsigned short control_word;  
float source_p1; 
float source_p2;
float sys_temp;

extern int CVICALLBACK HPStimer(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);



typedef struct
{
	short command;
	short commandvalue1;
	short commandvalue2;   
	short reserve;
	short checksum;
}UDPRECMessage;

UDPRECMessage udpmsg;

typedef struct
{
	short command;
	short PressureStateValue1;
	short PressureStateValue2; 
	short TemperatureValue;
	short SourceState;
	short LoadState;
	short checksum;
}UDPSENDMessage;

UDPSENDMessage udpsendmsg; 
#define SEND_COMPUTER_ADDR		"192.168.1.100"	



//HIL面板初始化
int HILPanelInit(void)
{
	if ((HILPanelHandle = LoadPanel (0, "HILPanel.uir", HILPNL)) < 0)
	  return -1;
	
	if (ConnectToTCPServer (&gTCPConnection, HOST_PORT, HOST_ADDRESS, TCPDataCallback, 0, 2000) < 0)
	{
		//SetCtrlVal(runningpanel,RUNPANEL_MSGSTRING,"连接实时主机失败，请确认实时程序已经运行，且网络连接正常！");	
		MessagePopup ("Error", "连接实时主机失败，请确认实时程序已经运行，且网络连接正常！");    
		goto Error;
	}
	else
	{
		//SetCtrlVal(StaticPanelHandle,RUNPANEL_MSGSTRING,"实时主机连接正常！");	       
	}
	
	
		
	//HIL初始化
	InitHILCtrlPara();
	InitHILDisIndex();   
	DisplayPanel (HILPanelHandle);
	
	//UDP远程通信
	CreateUDPChannel(UDP_PORT, &gUDPChannel);
	SetUDPAttribute(gUDPChannel, ATTR_UDP_CALLBACK, UDPChatCallback);
	
	
	RunUserInterface ();
	

	DiscardPanel (HILPanelHandle);
	
	if (gUDPChannel)
		DisposeUDPChannel(gUDPChannel);



	return 1;
Error:
    /* error operation */
	return -1;
	

Done: 
	/* clean up */   
	if (gTCPConnection != TCP_INVALID_CONNECTION)
		DisconnectFromTCPServer (gTCPConnection);
}

void Hctrlparadown(void)
{
	double dTemp;
	GetPanelHandleFromTabPage (HILPanelHandle, HILPNL_TAB,1,&tabpanel1);
			
	//通道1的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,0,&tabpanel2);
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTP,&dTemp);
	pExMem->ctrlPara[1].Ktp = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTI,&dTemp);
	pExMem->ctrlPara[1].Kti = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTD,&dTemp);
	pExMem->ctrlPara[1].Ktd = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FT1,&dTemp);
	pExMem->ctrlPara[1].Ft1 = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FT2,&dTemp);
	pExMem->ctrlPara[1].Ft2 = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FTK,&dTemp);
	pExMem->ctrlPara[1].Ftk = dTemp;
	
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KEP,&dTemp);
	pExMem->ctrlPara[1].Kep = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KED,&dTemp);
	pExMem->ctrlPara[1].Ked = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_TED,&dTemp);
	pExMem->ctrlPara[1].Ted = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X1,&dTemp);
	pExMem->ctrlPara[1].X1 = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X2,&dTemp);
	pExMem->ctrlPara[1].X2 = dTemp;
	GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X3,&dTemp);
	pExMem->ctrlPara[1].X3 = dTemp;
	
	//通道2的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,1,&tabpanel2);
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTP,&dTemp);
	pExMem->ctrlPara[2].Ktp = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTI,&dTemp);
	pExMem->ctrlPara[2].Kti = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTD,&dTemp);
	pExMem->ctrlPara[2].Ktd = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FT1,&dTemp);
	pExMem->ctrlPara[2].Ft1 = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FT2,&dTemp);
	pExMem->ctrlPara[2].Ft2 = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FTK,&dTemp);
	pExMem->ctrlPara[2].Ftk = dTemp;
	
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KEP,&dTemp);
	pExMem->ctrlPara[2].Kep = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KED,&dTemp);
	pExMem->ctrlPara[2].Ked = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_TED,&dTemp);
	pExMem->ctrlPara[2].Ted = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X1,&dTemp);
	pExMem->ctrlPara[2].X1 = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X2,&dTemp);
	pExMem->ctrlPara[2].X2 = dTemp;
	GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X3,&dTemp);
	pExMem->ctrlPara[2].X3 = dTemp;
	
	//通道3的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,2,&tabpanel2);
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTP,&dTemp);
	pExMem->ctrlPara[3].Ktp = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTI,&dTemp);
	pExMem->ctrlPara[3].Kti = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTD,&dTemp);
	pExMem->ctrlPara[3].Ktd = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FT1,&dTemp);
	pExMem->ctrlPara[3].Ft1 = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FT2,&dTemp);
	pExMem->ctrlPara[3].Ft2 = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FTK,&dTemp);
	pExMem->ctrlPara[3].Ftk = dTemp;
	
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KEP,&dTemp);
	pExMem->ctrlPara[3].Kep = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KED,&dTemp);
	pExMem->ctrlPara[3].Ked = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_TED,&dTemp);
	pExMem->ctrlPara[3].Ted = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X1,&dTemp);
	pExMem->ctrlPara[3].X1 = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X2,&dTemp);
	pExMem->ctrlPara[3].X2 = dTemp;
	GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X3,&dTemp);
	pExMem->ctrlPara[3].X3 = dTemp;
	
	//通道4的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,3,&tabpanel2);
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTP,&dTemp);
	pExMem->ctrlPara[4].Ktp = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTI,&dTemp);
	pExMem->ctrlPara[4].Kti = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTD,&dTemp);
	pExMem->ctrlPara[4].Ktd = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FT1,&dTemp);
	pExMem->ctrlPara[4].Ft1 = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FT2,&dTemp);
	pExMem->ctrlPara[4].Ft2 = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FTK,&dTemp);
	pExMem->ctrlPara[4].Ftk = dTemp;
	
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KEP,&dTemp);
	pExMem->ctrlPara[4].Kep = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KED,&dTemp);
	pExMem->ctrlPara[4].Ked = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_TED,&dTemp);
	pExMem->ctrlPara[4].Ted = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X1,&dTemp);
	pExMem->ctrlPara[4].X1 = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X2,&dTemp);
	pExMem->ctrlPara[4].X2 = dTemp;
	GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X3,&dTemp);
	pExMem->ctrlPara[4].X3 = dTemp;
	
	
	//轴向通道1的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB, 4, &tabpanel2);
	GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_KTP,&dTemp);
	pExMem->ctrlPara[5].Ktp = dTemp;
	GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_KTI,&dTemp);
	pExMem->ctrlPara[5].Kti = dTemp;
	GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_KTD,&dTemp);
	pExMem->ctrlPara[5].Ktd = dTemp;
	GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_FT1,&dTemp);
	pExMem->ctrlPara[5].Ft1 = dTemp;
	GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_FT2,&dTemp);
	pExMem->ctrlPara[5].Ft2 = dTemp;
	GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_FTK,&dTemp);
	pExMem->ctrlPara[5].Ftk = dTemp;
	
	//轴向通道2的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB, 5, &tabpanel2);
	GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_KTP,&dTemp);
	pExMem->ctrlPara[6].Ktp = dTemp;
	GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_KTI,&dTemp);
	pExMem->ctrlPara[6].Kti = dTemp;
	GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_KTD,&dTemp);
	pExMem->ctrlPara[6].Ktd = dTemp;
	GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_FT1,&dTemp);
	pExMem->ctrlPara[6].Ft1 = dTemp;
	GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_FT2,&dTemp);
	pExMem->ctrlPara[6].Ft2 = dTemp;
	GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_FTK,&dTemp);
	pExMem->ctrlPara[6].Ftk = dTemp;
	
		//轴向通道3的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB, 6, &tabpanel2);
	GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_KTP,&dTemp);
	pExMem->ctrlPara[7].Ktp = dTemp;
	GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_KTI,&dTemp);
	pExMem->ctrlPara[7].Kti = dTemp;
	GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_KTD,&dTemp);
	pExMem->ctrlPara[7].Ktd = dTemp;
	GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_FT1,&dTemp);
	pExMem->ctrlPara[7].Ft1 = dTemp;
	GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_FT2,&dTemp);
	pExMem->ctrlPara[7].Ft2 = dTemp;
	GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_FTK,&dTemp);
	pExMem->ctrlPara[7].Ftk = dTemp;
	
		//轴向通道4的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB, 7, &tabpanel2);
	GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_KTP,&dTemp);
	pExMem->ctrlPara[8].Ktp = dTemp;
	GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_KTI,&dTemp);
	pExMem->ctrlPara[8].Kti = dTemp;
	GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_KTD,&dTemp);
	pExMem->ctrlPara[8].Ktd = dTemp;
	GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_FT1,&dTemp);
	pExMem->ctrlPara[8].Ft1 = dTemp;
	GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_FT2,&dTemp);
	pExMem->ctrlPara[8].Ft2 = dTemp;
	GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_FTK,&dTemp);
	pExMem->ctrlPara[8].Ftk = dTemp;
	
}

void InitHILCtrlSet(void)
{
	GetPanelHandleFromTabPage (HILPanelHandle, HILPNL_TAB,1,&tabpanel1);  
	
	//通道1的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,0,&tabpanel2);
	
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTP,pExMem->ctrlPara[1].Ktp);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTI,pExMem->ctrlPara[1].Kti);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTD,pExMem->ctrlPara[1].Ktd);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FT1,pExMem->ctrlPara[1].Ft1);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FT2,pExMem->ctrlPara[1].Ft2);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FTK,pExMem->ctrlPara[1].Ftk);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KEP,pExMem->ctrlPara[1].Kep);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KED,pExMem->ctrlPara[2].Ked);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_TED,pExMem->ctrlPara[1].Ted);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X1,pExMem->ctrlPara[1].X1);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X2,pExMem->ctrlPara[1].X2);
	SetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X3,pExMem->ctrlPara[1].X3);
	
	//通道2的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,1,&tabpanel2);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTP,pExMem->ctrlPara[2].Ktp);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTI,pExMem->ctrlPara[2].Kti);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTD,pExMem->ctrlPara[2].Ktd);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FT1,pExMem->ctrlPara[2].Ft1);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FT2,pExMem->ctrlPara[2].Ft2);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FTK,pExMem->ctrlPara[2].Ftk);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KEP,pExMem->ctrlPara[2].Kep);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KED,pExMem->ctrlPara[2].Ked);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_TED,pExMem->ctrlPara[2].Ted);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X1,pExMem->ctrlPara[2].X1);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X2,pExMem->ctrlPara[2].X2);
	SetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X3,pExMem->ctrlPara[2].X3);

	//通道3的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,2,&tabpanel2);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTP,pExMem->ctrlPara[3].Ktp);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTI,pExMem->ctrlPara[3].Kti);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTD,pExMem->ctrlPara[3].Ktd);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FT1,pExMem->ctrlPara[3].Ft1);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FT2,pExMem->ctrlPara[3].Ft2);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FTK,pExMem->ctrlPara[3].Ftk);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KEP,pExMem->ctrlPara[3].Kep);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KED,pExMem->ctrlPara[3].Ked);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_TED,pExMem->ctrlPara[3].Ted);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X1,pExMem->ctrlPara[3].X1);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X2,pExMem->ctrlPara[3].X2);
	SetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X3,pExMem->ctrlPara[3].X3);

	//通道4的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,3,&tabpanel2);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTP,pExMem->ctrlPara[4].Ktp);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTI,pExMem->ctrlPara[4].Kti);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTD,pExMem->ctrlPara[4].Ktd);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FT1,pExMem->ctrlPara[4].Ft1);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FT2,pExMem->ctrlPara[4].Ft2);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FTK,pExMem->ctrlPara[4].Ftk);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KEP,pExMem->ctrlPara[4].Kep);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KED,pExMem->ctrlPara[4].Ked);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_TED,pExMem->ctrlPara[4].Ted);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X1,pExMem->ctrlPara[4].X1);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X2,pExMem->ctrlPara[4].X2);
	SetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X3,pExMem->ctrlPara[4].X3);
	
	//通道4的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,4,&tabpanel2);
	SetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_KTP,pExMem->ctrlPara[5].Ktp);
	SetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_KTI,pExMem->ctrlPara[5].Kti);
	SetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_KTD,pExMem->ctrlPara[5].Ktd);
	SetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_FT1,pExMem->ctrlPara[5].Ft1);
	SetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_FT2,pExMem->ctrlPara[5].Ft2);
	SetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_FTK,pExMem->ctrlPara[5].Ftk);
	
	//通道4的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,5,&tabpanel2);
	SetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_KTP,pExMem->ctrlPara[6].Ktp);
	SetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_KTI,pExMem->ctrlPara[6].Kti);
	SetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_KTD,pExMem->ctrlPara[6].Ktd);
	SetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_FT1,pExMem->ctrlPara[6].Ft1);
	SetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_FT2,pExMem->ctrlPara[6].Ft2);
	SetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_FTK,pExMem->ctrlPara[6].Ftk);
	
	//通道4的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,6,&tabpanel2);
	SetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_KTP,pExMem->ctrlPara[7].Ktp);
	SetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_KTI,pExMem->ctrlPara[7].Kti);
	SetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_KTD,pExMem->ctrlPara[7].Ktd);
	SetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_FT1,pExMem->ctrlPara[7].Ft1);
	SetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_FT2,pExMem->ctrlPara[7].Ft2);
	SetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_FTK,pExMem->ctrlPara[7].Ftk);
	
	
	//通道4的控制参数
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,7,&tabpanel2);
	SetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_KTP,pExMem->ctrlPara[8].Ktp);
	SetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_KTI,pExMem->ctrlPara[8].Kti);
	SetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_KTD,pExMem->ctrlPara[8].Ktd);
	SetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_FT1,pExMem->ctrlPara[8].Ft1);
	SetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_FT2,pExMem->ctrlPara[8].Ft2);
	SetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_FTK,pExMem->ctrlPara[8].Ftk);
	
	
}

void InitHILTestPara(void)
{
	double dTemp;
	int tempInt;

	//通道的测试类型
	GetCtrlVal(HILPanelHandle, HILPNL_CH_TEST_TYPE, &tempInt);
	pExMem->testPara[1].iTestType = tempInt;
	pExMem->testPara[2].iTestType = tempInt;
	pExMem->testPara[3].iTestType = tempInt;
	pExMem->testPara[4].iTestType = tempInt;
	pExMem->testPara[5].iTestType = tempInt; 
	pExMem->testPara[6].iTestType = tempInt; 
	pExMem->testPara[7].iTestType = tempInt; 
	pExMem->testPara[8].iTestType = tempInt; 
	
	GetCtrlVal(HILPanelHandle, HILPNL_CH_TEST_TIME, &dTemp);    
	pExMem->testPara[1].dTestTime = dTemp;
	pExMem->testPara[2].dTestTime = dTemp;
	pExMem->testPara[3].dTestTime = dTemp;
	pExMem->testPara[4].dTestTime = dTemp;
	pExMem->testPara[5].dTestTime = dTemp; 
	pExMem->testPara[6].dTestTime = dTemp; 
	pExMem->testPara[7].dTestTime = dTemp; 
	pExMem->testPara[8].dTestTime = dTemp; 

	
	GetPanelHandleFromTabPage (HILPanelHandle, HILPNL_TAB, 0, &tabpanel1);

	//通道1的测试参数
	GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,0,&tabpanel2);
	GetCtrlVal(tabpanel2,CH1_TAB_CH_AMP,&dTemp);
	pExMem->testPara[1].dAmp = dTemp;
	GetCtrlVal(tabpanel2,CH1_TAB_CH_OFFSET,&dTemp);
	pExMem->testPara[1].dOffset = dTemp;
	GetCtrlVal(tabpanel2,CH1_TAB_CH_FREQ,&dTemp);    
	pExMem->testPara[1].dFreq = dTemp;  
	//GetCtrlVal(tabpanel2,CH1_TAB_CH_START_FREQ,&dTemp);    
	//pExMem->testPara[1].dFreq = dTemp; 
	GetCtrlVal(tabpanel2,CH1_TAB_CH_STOP_FREQ,&dTemp);    
	pExMem->testPara[1].dStopFreq = dTemp;

	GetCtrlVal(tabpanel2,CH1_TAB_CH_ON_OFF,&tempInt); 
	if(tempInt == 1)
	 {
		pExMem->testPara[1].iEnable = 1;
	 	pExMem->testPara[1].iStatus = 2;  		  //1表示位置通道，2表示加载通道
	 }
	 GetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,&tempInt); 
	pExMem->testPara[1].iWave = tempInt; 


	//通道2的测试参数 
	GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,1,&tabpanel2);
	GetCtrlVal(tabpanel2,CH2_TAB_CH_AMP,&dTemp);
	pExMem->testPara[2].dAmp = dTemp;
	GetCtrlVal(tabpanel2,CH2_TAB_CH_OFFSET,&dTemp);
	pExMem->testPara[2].dOffset = dTemp;
	GetCtrlVal(tabpanel2,CH2_TAB_CH_FREQ,&dTemp);    
	pExMem->testPara[2].dFreq = dTemp;  
	//GetCtrlVal(tabpanel2,CH2_TAB_CH_START_FREQ,&dTemp);    
	//pExMem->testPara[2].dStartFreq = dTemp; 
	GetCtrlVal(tabpanel2,CH2_TAB_CH_STOP_FREQ,&dTemp);    
	pExMem->testPara[2].dStopFreq = dTemp;
	GetCtrlVal(tabpanel2,CH2_TAB_CH_ON_OFF,&tempInt); 
	if(tempInt == 1)
	 {
		pExMem->testPara[2].iEnable = 1;  
	 	pExMem->testPara[2].iStatus = 2;  		  //1表示位置通道，2表示加载通道
	 }
	GetCtrlVal(tabpanel2,CH2_TAB_CH_WAVE,&tempInt); 
	pExMem->testPara[2].iWave = tempInt; 

	//通道3的测试参数 
	GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,2,&tabpanel2);
	GetCtrlVal(tabpanel2,CH3_TAB_CH_AMP,&dTemp);
	pExMem->testPara[3].dAmp = dTemp;
	GetCtrlVal(tabpanel2,CH3_TAB_CH_OFFSET,&dTemp);
	pExMem->testPara[3].dOffset = dTemp;
	GetCtrlVal(tabpanel2,CH3_TAB_CH_FREQ,&dTemp);    
	pExMem->testPara[3].dFreq = dTemp;  
	//GetCtrlVal(tabpanel2,CH3_TAB_CH_START_FREQ,&dTemp);    
	//pExMem->testPara[3].dStartFreq = dTemp; 
	GetCtrlVal(tabpanel2,CH3_TAB_CH_STOP_FREQ,&dTemp);    
	pExMem->testPara[3].dStopFreq = dTemp;
	GetCtrlVal(tabpanel2,CH3_TAB_CH_ON_OFF,&tempInt); 
	if(tempInt == 1)
	 {
		pExMem->testPara[3].iEnable = 1;  
	 	pExMem->testPara[3].iStatus = 2;  		  //1表示位置通道，2表示加载通道
	 }
	GetCtrlVal(tabpanel2,CH3_TAB_CH_WAVE,&tempInt); 
	pExMem->testPara[3].iWave = tempInt; 

	//通道4的测试参数 
	GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,3,&tabpanel2);
	GetCtrlVal(tabpanel2,CH4_TAB_CH_AMP,&dTemp);
	pExMem->testPara[4].dAmp = dTemp;
	GetCtrlVal(tabpanel2,CH4_TAB_CH_OFFSET,&dTemp);
	pExMem->testPara[4].dOffset = dTemp;
	GetCtrlVal(tabpanel2,CH4_TAB_CH_FREQ,&dTemp);    
	pExMem->testPara[4].dFreq = dTemp;  
	//GetCtrlVal(tabpanel2,CH4_TAB_CH_START_FREQ,&dTemp);    
	//pExMem->testPara[4].dStartFreq = dTemp; 
	GetCtrlVal(tabpanel2,CH4_TAB_CH_STOP_FREQ,&dTemp);    
	pExMem->testPara[4].dStopFreq = dTemp;
	GetCtrlVal(tabpanel2,CH4_TAB_CH_ON_OFF,&tempInt); 
	if(tempInt == 1)
	 {
		pExMem->testPara[4].iEnable = 1;  
	 	pExMem->testPara[4].iStatus = 2;  		  //1表示位置通道，2表示加载通道
	 }
	GetCtrlVal(tabpanel2,CH4_TAB_CH_WAVE,&tempInt); 
	pExMem->testPara[4].iWave = tempInt; 

	//轴向通道1的测试参数 
	GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,4,&tabpanel2);
	GetCtrlVal(tabpanel2,AXICH1_TAB_CH_AMP,&dTemp);
	pExMem->testPara[5].dAmp = dTemp;
	GetCtrlVal(tabpanel2,AXICH1_TAB_CH_OFFSET,&dTemp);
	pExMem->testPara[5].dOffset = dTemp;
	GetCtrlVal(tabpanel2,AXICH1_TAB_CH_FREQ,&dTemp);    
	pExMem->testPara[5].dFreq = dTemp;  
	//GetCtrlVal(tabpanel2,CH4_TAB_CH_START_FREQ,&dTemp);    
	//pExMem->testPara[4].dStartFreq = dTemp; 
	GetCtrlVal(tabpanel2,AXICH1_TAB_CH_STOP_FREQ,&dTemp);    
	pExMem->testPara[5].dStopFreq = dTemp;
	GetCtrlVal(tabpanel2,AXICH1_TAB_CH_ON_OFF,&tempInt); 
	if(tempInt == 1)
	 {
		pExMem->testPara[5].iEnable = 1;    
	 	pExMem->testPara[5].iStatus = 2;  		  //1表示位置通道，2表示加载通道
	 }
	GetCtrlVal(tabpanel2,AXICH1_TAB_CH_WAVE,&tempInt); 
	pExMem->testPara[5].iWave = tempInt; 

	//轴向通道2的测试参数 
	GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,5,&tabpanel2);
	GetCtrlVal(tabpanel2,AXICH2_TAB_CH_AMP,&dTemp);
	pExMem->testPara[6].dAmp = dTemp;
	GetCtrlVal(tabpanel2,AXICH2_TAB_CH_OFFSET,&dTemp);
	pExMem->testPara[6].dOffset = dTemp;
	GetCtrlVal(tabpanel2,AXICH2_TAB_CH_FREQ,&dTemp);    
	pExMem->testPara[6].dFreq = dTemp;  
	//GetCtrlVal(tabpanel2,CH4_TAB_CH_START_FREQ,&dTemp);    
	//pExMem->testPara[4].dStartFreq = dTemp; 
	GetCtrlVal(tabpanel2,AXICH2_TAB_CH_STOP_FREQ,&dTemp);    
	pExMem->testPara[6].dStopFreq = dTemp;
	GetCtrlVal(tabpanel2,AXICH2_TAB_CH_ON_OFF,&tempInt); 
	if(tempInt == 1)
	 {
		pExMem->testPara[6].iEnable = 1;    
	 	pExMem->testPara[6].iStatus = 2;  		  //1表示位置通道，2表示加载通道
	 }
	GetCtrlVal(tabpanel2,AXICH2_TAB_CH_WAVE,&tempInt); 
	pExMem->testPara[6].iWave = tempInt; 

	//轴向通道3的测试参数 
	GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,6,&tabpanel2);
	GetCtrlVal(tabpanel2,AXICH3_TAB_CH_AMP,&dTemp);
	pExMem->testPara[7].dAmp = dTemp;
	GetCtrlVal(tabpanel2,AXICH3_TAB_CH_OFFSET,&dTemp);
	pExMem->testPara[7].dOffset = dTemp;
	GetCtrlVal(tabpanel2,AXICH3_TAB_CH_FREQ,&dTemp);    
	pExMem->testPara[7].dFreq = dTemp;  
	//GetCtrlVal(tabpanel2,CH4_TAB_CH_START_FREQ,&dTemp);    
	//pExMem->testPara[4].dStartFreq = dTemp; 
	GetCtrlVal(tabpanel2,AXICH3_TAB_CH_STOP_FREQ,&dTemp);    
	pExMem->testPara[7].dStopFreq = dTemp;
	GetCtrlVal(tabpanel2,AXICH3_TAB_CH_ON_OFF,&tempInt); 
	if(tempInt == 1)
	 {
		 pExMem->testPara[7].iEnable = 1;  
	 	pExMem->testPara[7].iStatus = 2;  		  //1表示位置通道，2表示加载通道
	 }
	GetCtrlVal(tabpanel2,AXICH3_TAB_CH_WAVE,&tempInt); 
	pExMem->testPara[7].iWave = tempInt; 

	//轴向通道4的测试参数 
	GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,7,&tabpanel2);
	GetCtrlVal(tabpanel2,AXICH4_TAB_CH_AMP,&dTemp);
	pExMem->testPara[8].dAmp = dTemp;
	GetCtrlVal(tabpanel2,AXICH4_TAB_CH_OFFSET,&dTemp);
	pExMem->testPara[8].dOffset = dTemp;
	GetCtrlVal(tabpanel2,AXICH4_TAB_CH_FREQ,&dTemp);    
	pExMem->testPara[8].dFreq = dTemp;  
	//GetCtrlVal(tabpanel2,CH4_TAB_CH_START_FREQ,&dTemp);    
	//pExMem->testPara[4].dStartFreq = dTemp; 
	GetCtrlVal(tabpanel2,AXICH4_TAB_CH_STOP_FREQ,&dTemp);    
	pExMem->testPara[8].dStopFreq = dTemp;
	GetCtrlVal(tabpanel2,AXICH4_TAB_CH_ON_OFF,&tempInt); 
	if(tempInt == 1)
	 {
		 pExMem->testPara[8].iEnable = 1;  
	 	pExMem->testPara[8].iStatus = 2;  		  //1表示位置通道，2表示加载通道
	 }
	GetCtrlVal(tabpanel2,AXICH4_TAB_CH_WAVE,&tempInt); 
	pExMem->testPara[8].iWave = tempInt; 
}

void HILSetScreenCycNum(void)
{
	double dfreq;	   //预设频率
	double dring_val;  //采样周期
	long N;   //每屏显示点数
	int i,ringindex;
	
	for(i=1;i<9;i++)
	{
		if(pExMem->testPara[i].iStatus && (chart1_ch == i))
		{
			N = (long)(2000 / (pExMem->testPara[i].dFreq) * CYCLE_NUM_PER_SCREEN);  //屏幕显示点数计算公式
			if(N >= 3 && N <= 10000)
			{
				SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_1,ATTR_POINTS_PER_SCREEN,N);
				SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_2,ATTR_POINTS_PER_SCREEN,N);
				SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_3,ATTR_POINTS_PER_SCREEN,N);
				SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_4,ATTR_POINTS_PER_SCREEN,N);
			}
			 /*
			for(ringindex = 0;ringindex < 11;ringindex++)  //检测幅值所处范围并设置y坐标
			{
				//GetValueFromIndex(HILPanelHandle,HILPNL_CHART1_YRANGE_RING,ringindex,&dring_val);
				if(pExMem->testPara[i].dAmp < dring_val)
				{
					SetCtrlIndex(HILPanelHandle,HILPNL_CHART1_YRANGE_RING,ringindex);
					SetCtrlIndex(HILPanelHandle,HILPNL_CHART2_YRANGE_RING,ringindex);
					SetAxisRange(HILPanelHandle,HILPNL_STRIPCHART_1,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dring_val,dring_val);
					SetAxisRange(HILPanelHandle,HILPNL_STRIPCHART_2,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dring_val,dring_val);
					break;
				}
			}
			 */
		}
	}
}

void InitHILDisIndex(void)
{
	char tempLabel[64];
	//初始化显示索引
	//tempLabel = " ";
	
	GetCtrlVal(HILPanelHandle,HILPNL_CHART1_CH_RING,&chart1_ch); 
	/*
	GetCtrlVal(HILPanelHandle,HILPNL_CHART1_CONTENT_RING,&chart1_content);
	strcpy(tempLabel,""); 
	strcat(tempLabel,contentLabel[chart1_content]);
	SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_1,ATTR_LABEL_TEXT,tempLabel);
	SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_1,ATTR_LABEL_JUSTIFY,VAL_LEFT_JUSTIFIED); 
	*/
	
	GetCtrlVal(HILPanelHandle,HILPNL_CHART2_CH_RING,&chart2_ch);
	GetCtrlVal(HILPanelHandle,HILPNL_CHART2_CONTENT_RING,&chart2_content);
	strcpy(tempLabel,""); 
	strcat(tempLabel,contentLabel[chart2_content]);
	SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_2,ATTR_LABEL_TEXT,tempLabel);
	//SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_2,ATTR_LABEL_JUSTIFY,VAL_LEFT_JUSTIFIED); 

	GetCtrlVal(HILPanelHandle,HILPNL_CHART3_CH_RING,&chart3_ch); 
	/*
	GetCtrlVal(HILPanelHandle,HILPNL_CHART3_CONTENT_RING,&chart3_content);
	strcpy(tempLabel,""); 
	strcat(tempLabel,contentLabel[chart3_content]);
	SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_3,ATTR_LABEL_TEXT,tempLabel);
	SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_3,ATTR_LABEL_JUSTIFY,VAL_LEFT_JUSTIFIED); 
	*/

	GetCtrlVal(HILPanelHandle,HILPNL_CHART4_CH_RING,&chart4_ch);
	GetCtrlVal(HILPanelHandle,HILPNL_CHART4_CONTENT_RING,&chart4_content);
	strcpy(tempLabel,""); 
	strcat(tempLabel,contentLabel[chart4_content==5?chart4_content+1:chart4_content]);
	SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_4,ATTR_LABEL_TEXT,tempLabel);
	//SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_4,ATTR_LABEL_JUSTIFY,VAL_LEFT_JUSTIFIED); 
}

//初始化控制参数
void InitHILCtrlPara(void)
{
	double dTemp;
	//获取ini文件句柄
	inihandle = Ini_New(0);
	//读取ini文件
	//GetWindowsDirs (windowsDirectory, systemDirectory);
	//strcpy( pathname ,windowsDirectory);
	
	GetDir (currentDir);
	strcpy( pathname ,currentDir);
	strcat(pathname,"\\HILPara.ini");
	
	inierr = Ini_ReadFromFile(inihandle, pathname); 
	if( inierr < 0)
	{
		sprintf(msgBuff,"打开参数文件错误！err = %d",inierr);
		MessagePopup ("Err",msgBuff);  
		//打开文件失败
	}
	else
	{
		//通道1 ----KTP/KTI/KTD
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_Ktp", &dTemp)>0)
		{
			pExMem->ctrlPara[1].Ktp = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].Ktp = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_Kti", &dTemp)>0)
		{
			pExMem->ctrlPara[1].Kti = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].Kti = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_Ktd", &dTemp)>0)
		{
			pExMem->ctrlPara[1].Ktd = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].Ktd = 0.0;
		}
		//三个模糊参数
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_Ft1", &dTemp)>0)
		{
			pExMem->ctrlPara[1].Ft1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].Ft1 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_Ft2", &dTemp)>0)
		{
			pExMem->ctrlPara[1].Ft2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].Ft2 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_Ftk", &dTemp)>0)
		{
			pExMem->ctrlPara[1].Ftk = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].Ftk = 0.0;
		}
		//---------kep/ked/ted
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_Kep", &dTemp)>0)
		{
			pExMem->ctrlPara[1].Kep = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].Kep = 0.0;
		}		
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_Ked", &dTemp)>0)
		{
			pExMem->ctrlPara[1].Ked = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].Ked = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_Ted", &dTemp)>0)
		{
			pExMem->ctrlPara[1].Ted = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].Ted = 0.0;
		}
		//-------------x1/x2/x3
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_X1", &dTemp)>0)
		{
			pExMem->ctrlPara[1].X1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].X1 = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_X2", &dTemp)>0)
		{
			pExMem->ctrlPara[1].X2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].X2 = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH1_X3", &dTemp)>0)
		{
			pExMem->ctrlPara[1].X3 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[1].X3 = 0.0;
		}
        //通道2
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_Ktp", &dTemp)>0)
		{
			pExMem->ctrlPara[2].Ktp = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].Ktp = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_Kti", &dTemp)>0)
		{
			pExMem->ctrlPara[2].Kti = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].Kti = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_Ktd", &dTemp)>0)
		{
			pExMem->ctrlPara[2].Ktd = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].Ktd = 0.0;
		}
		//三个模糊参数
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_Ft1", &dTemp)>0)
		{
			pExMem->ctrlPara[2].Ft1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].Ft1 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_Ft2", &dTemp)>0)
		{
			pExMem->ctrlPara[2].Ft2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].Ft2 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_Ftk", &dTemp)>0)
		{
			pExMem->ctrlPara[2].Ftk = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].Ftk = 0.0;
		}
		//---------kep/ked/ted
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_Kep", &dTemp)>0)
		{
			pExMem->ctrlPara[2].Kep = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].Kep = 0.0;
		}		
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_Ked", &dTemp)>0)
		{
			pExMem->ctrlPara[2].Ked = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].Ked = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_Ted", &dTemp)>0)
		{
			pExMem->ctrlPara[2].Ted = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].Ted = 0.0;
		}
		//-------------x1/x2/x3
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_X1", &dTemp)>0)
		{
			pExMem->ctrlPara[2].X1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].X1 = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_X2", &dTemp)>0)
		{
			pExMem->ctrlPara[2].X2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].X2 = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH2_X3", &dTemp)>0)
		{
			pExMem->ctrlPara[2].X3 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[2].X3 = 0.0;
		}
		//通道3
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_Ktp", &dTemp)>0)
		{
			pExMem->ctrlPara[3].Ktp = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].Ktp = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_Kti", &dTemp)>0)
		{
			pExMem->ctrlPara[3].Kti = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].Kti = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_Ktd", &dTemp)>0)
		{
			pExMem->ctrlPara[3].Ktd = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].Ktd = 0.0;
		}
		//三个模糊参数
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_Ft1", &dTemp)>0)
		{
			pExMem->ctrlPara[3].Ft1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].Ft1 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_Ft2", &dTemp)>0)
		{
			pExMem->ctrlPara[3].Ft2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].Ft2 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_Ftk", &dTemp)>0)
		{
			pExMem->ctrlPara[3].Ftk = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].Ftk = 0.0;
		}
		//---------kep/ked/ted
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_Kep", &dTemp)>0)
		{
			pExMem->ctrlPara[3].Kep = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].Kep = 0.0;
		}		
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_Ked", &dTemp)>0)
		{
			pExMem->ctrlPara[3].Ked = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].Ked = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_Ted", &dTemp)>0)
		{
			pExMem->ctrlPara[3].Ted = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].Ted = 0.0;
		}
		//-------------x1/x2/x3
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_X1", &dTemp)>0)
		{
			pExMem->ctrlPara[3].X1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].X1 = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_X2", &dTemp)>0)
		{
			pExMem->ctrlPara[3].X2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].X2 = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH3_X3", &dTemp)>0)
		{
			pExMem->ctrlPara[3].X3 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[3].X3 = 0.0;
		}
		//通道4
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_Ktp", &dTemp)>0)
		{
			pExMem->ctrlPara[4].Ktp = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].Ktp = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_Kti", &dTemp)>0)
		{
			pExMem->ctrlPara[4].Kti = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].Kti = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_Ktd", &dTemp)>0)
		{
			pExMem->ctrlPara[4].Ktd = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].Ktd = 0.0;
		}
		//三个模糊参数
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_Ft1", &dTemp)>0)
		{
			pExMem->ctrlPara[4].Ft1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].Ft1 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_Ft2", &dTemp)>0)
		{
			pExMem->ctrlPara[4].Ft2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].Ft2 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_Ftk", &dTemp)>0)
		{
			pExMem->ctrlPara[4].Ftk = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].Ftk = 0.0;
		}
		//---------kep/ked/ted
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_Kep", &dTemp)>0)
		{
			pExMem->ctrlPara[4].Kep = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].Kep = 0.0;
		}		
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_Ked", &dTemp)>0)
		{
			pExMem->ctrlPara[4].Ked = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].Ked = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_Ted", &dTemp)>0)
		{
			pExMem->ctrlPara[4].Ted = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].Ted = 0.0;
		}
		//-------------x1/x2/x3
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_X1", &dTemp)>0)
		{
			pExMem->ctrlPara[4].X1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].X1 = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_X2", &dTemp)>0)
		{
			pExMem->ctrlPara[4].X2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].X2 = 0.0;
		}
		if(Ini_GetDouble (inihandle, "CtrlPara","CH4_X3", &dTemp)>0)
		{
			pExMem->ctrlPara[4].X3 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[4].X3 = 0.0;
		}
		
		//轴向通道1
		if(Ini_GetDouble (inihandle, "CtrlPara","CH5_Ktp", &dTemp)>0)
		{
			pExMem->ctrlPara[5].Ktp = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[5].Ktp = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH5_Kti", &dTemp)>0)
		{
			pExMem->ctrlPara[5].Kti = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[5].Kti = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH5_Ktd", &dTemp)>0)
		{
			pExMem->ctrlPara[5].Ktd = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[5].Ktd = 0.0;
		}
		//三个模糊参数
		if(Ini_GetDouble (inihandle, "CtrlPara","CH5_Ft1", &dTemp)>0)
		{
			pExMem->ctrlPara[5].Ft1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[5].Ft1 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH5_Ft2", &dTemp)>0)
		{
			pExMem->ctrlPara[5].Ft2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[5].Ft2 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH5_Ftk", &dTemp)>0)
		{
			pExMem->ctrlPara[5].Ftk = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[5].Ftk = 0.0;
		}
		
		//轴向通道2
		if(Ini_GetDouble (inihandle, "CtrlPara","CH6_Ktp", &dTemp)>0)
		{
			pExMem->ctrlPara[6].Ktp = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[6].Ktp = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH6_Kti", &dTemp)>0)
		{
			pExMem->ctrlPara[6].Kti = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[6].Kti = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH6_Ktd", &dTemp)>0)
		{
			pExMem->ctrlPara[6].Ktd = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[6].Ktd = 0.0;
		}
		//三个模糊参数
		if(Ini_GetDouble (inihandle, "CtrlPara","CH6_Ft1", &dTemp)>0)
		{
			pExMem->ctrlPara[6].Ft1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[6].Ft1 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH6_Ft2", &dTemp)>0)
		{
			pExMem->ctrlPara[6].Ft2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[6].Ft2 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH6_Ftk", &dTemp)>0)
		{
			pExMem->ctrlPara[6].Ftk = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[6].Ftk = 0.0;
		}
		
		//轴向通道3
		if(Ini_GetDouble (inihandle, "CtrlPara","CH7_Ktp", &dTemp)>0)
		{
			pExMem->ctrlPara[7].Ktp = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[7].Ktp = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH7_Kti", &dTemp)>0)
		{
			pExMem->ctrlPara[7].Kti = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[7].Kti = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH7_Ktd", &dTemp)>0)
		{
			pExMem->ctrlPara[7].Ktd = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[7].Ktd = 0.0;
		}
		//三个模糊参数
		if(Ini_GetDouble (inihandle, "CtrlPara","CH7_Ft1", &dTemp)>0)
		{
			pExMem->ctrlPara[7].Ft1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[7].Ft1 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH7_Ft2", &dTemp)>0)
		{
			pExMem->ctrlPara[7].Ft2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[7].Ft2 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH7_Ftk", &dTemp)>0)
		{
			pExMem->ctrlPara[7].Ftk = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[7].Ftk = 0.0;
		}
		
		//轴向通道4
		if(Ini_GetDouble (inihandle, "CtrlPara","CH8_Ktp", &dTemp)>0)
		{
			pExMem->ctrlPara[8].Ktp = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[8].Ktp = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH8_Kti", &dTemp)>0)
		{
			pExMem->ctrlPara[8].Kti = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[8].Kti = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH8_Ktd", &dTemp)>0)
		{
			pExMem->ctrlPara[8].Ktd = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[8].Ktd = 0.0;
		}
		//三个模糊参数
		if(Ini_GetDouble (inihandle, "CtrlPara","CH8_Ft1", &dTemp)>0)
		{
			pExMem->ctrlPara[8].Ft1 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[8].Ft1 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH8_Ft2", &dTemp)>0)
		{
			pExMem->ctrlPara[8].Ft2 = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[8].Ft2 = 0.0;
		}
	
		if(Ini_GetDouble (inihandle, "CtrlPara","CH8_Ftk", &dTemp)>0)
		{
			pExMem->ctrlPara[8].Ftk = dTemp;
		}
		else 
		{
			 pExMem->ctrlPara[8].Ftk = 0.0;
		}
		
		
	
	}
	
	InitHILCtrlSet();
}

//使能控制参数设置
void LightHILParaSet(void)
{
	GetPanelHandleFromTabPage (HILPanelHandle, HILPNL_TAB,1,&tabpanel1);
	
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,0,&tabpanel2);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_KTP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_KTI,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_KTD,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_FT1,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_FT2,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_FTK,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_KEP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_KED,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_TED,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_X1,ATTR_DIMMED,0);  
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_X2,ATTR_DIMMED,0);  
	SetCtrlAttribute(tabpanel2,CH1_P_TAB_NUM_HIL_X3,ATTR_DIMMED,0);
	
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,1,&tabpanel2);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_KTP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_KTI,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_KTD,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_FT1,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_FT2,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_FTK,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_KEP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_KED,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_TED,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_X1,ATTR_DIMMED,0);  
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_X2,ATTR_DIMMED,0);  
	SetCtrlAttribute(tabpanel2,CH2_P_TAB_NUM_HIL_X3,ATTR_DIMMED,0);
	
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,2,&tabpanel2);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_KTP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_KTI,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_KTD,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_FT1,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_FT2,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_FTK,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_KEP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_KED,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_TED,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_X1,ATTR_DIMMED,0);  
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_X2,ATTR_DIMMED,0);  
	SetCtrlAttribute(tabpanel2,CH3_P_TAB_NUM_HIL_X3,ATTR_DIMMED,0);
	
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,3,&tabpanel2);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_KTP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_KTI,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_KTD,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_FT1,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_FT2,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_FTK,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_KEP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_KED,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_TED,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_X1,ATTR_DIMMED,0);  
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_X2,ATTR_DIMMED,0);  
	SetCtrlAttribute(tabpanel2,CH4_P_TAB_NUM_HIL_X3,ATTR_DIMMED,0);
	
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,4,&tabpanel2);
	SetCtrlAttribute(tabpanel2,CH1_A_TAB_NUM_HIL_KTP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_A_TAB_NUM_HIL_KTI,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_A_TAB_NUM_HIL_KTD,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_A_TAB_NUM_HIL_FT1,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_A_TAB_NUM_HIL_FT2,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH1_A_TAB_NUM_HIL_FTK,ATTR_DIMMED,0);
	
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,5,&tabpanel2);
	SetCtrlAttribute(tabpanel2,CH2_A_TAB_NUM_HIL_KTP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_A_TAB_NUM_HIL_KTI,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_A_TAB_NUM_HIL_KTD,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_A_TAB_NUM_HIL_FT1,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_A_TAB_NUM_HIL_FT2,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH2_A_TAB_NUM_HIL_FTK,ATTR_DIMMED,0);
	
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,6,&tabpanel2);   
	SetCtrlAttribute(tabpanel2,CH3_A_TAB_NUM_HIL_KTP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_A_TAB_NUM_HIL_KTI,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_A_TAB_NUM_HIL_KTD,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_A_TAB_NUM_HIL_FT1,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_A_TAB_NUM_HIL_FT2,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH3_A_TAB_NUM_HIL_FTK,ATTR_DIMMED,0);
	
	GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,7,&tabpanel2);   
	SetCtrlAttribute(tabpanel2,CH4_A_TAB_NUM_HIL_KTP,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_A_TAB_NUM_HIL_KTI,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_A_TAB_NUM_HIL_KTD,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_A_TAB_NUM_HIL_FT1,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_A_TAB_NUM_HIL_FT2,ATTR_DIMMED,0);
	SetCtrlAttribute(tabpanel2,CH4_A_TAB_NUM_HIL_FTK,ATTR_DIMMED,0);
}

////////////////////////////////////////These are Callback Functions
int CVICALLBACK HILPanelCallback (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:

			break;
	}
	return 0;
}

int CVICALLBACK HILYRangeChangeCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double dTemp;
	switch (event)
	{
		case EVENT_COMMIT:
			
			switch(control)
			{
				case  HILPNL_CHART1_YRANGE_RING:
					GetCtrlVal(panel,control,&dTemp);
					SetAxisRange(panel,HILPNL_STRIPCHART_1,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dTemp,dTemp);
					break;
				case  HILPNL_CHART2_YRANGE_RING:
					GetCtrlVal(panel,control,&dTemp);
					SetAxisRange(panel,HILPNL_STRIPCHART_2,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dTemp,dTemp);
					break;
				case  HILPNL_CHART3_YRANGE_RING:
					GetCtrlVal(panel,control,&dTemp);
					SetAxisRange(panel,HILPNL_STRIPCHART_3,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dTemp,dTemp);
					break;
				case  HILPNL_CHART4_YRANGE_RING:
					GetCtrlVal(panel,control,&dTemp);
					SetAxisRange(panel,HILPNL_STRIPCHART_4,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dTemp,dTemp);
					break;
			} 
			
			break;
	}
	return 0;
}

int CVICALLBACK HILSaveCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
		strcpy(saveDir,"D:\\测试数据" );
		//err = MakeDir (saveDir);
		strcat(saveDir,"\\" );
		strcat(saveDir, DateStr());
		//err = MakeDir (saveDir);

		if(FileSelectPopup (saveDir, "*.dat", "*.dat", "数据保存", VAL_SAVE_BUTTON, 0, 1, 1, 0,savepath ) != 0)
		{
			savefilehandle = OpenFile (savepath , VAL_WRITE_ONLY, VAL_TRUNCATE, VAL_BINARY);
			WriteFile (savefilehandle, (char*)saveBuf, saveIndex * dataStructSize ); 
			CloseFile (savefilehandle);
		}

		break;
	}
	return 0;
}

int CVICALLBACK HILRecordCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			if(recording == 0)
			{
				recording = 1;
				saveIndex = 0;
				SetCtrlAttribute(panel,control,ATTR_LABEL_TEXT,"停止记录");
			}
			else
			{
				recording = 0;
				SetCtrlAttribute(panel,control,ATTR_LABEL_TEXT,"开始记录");  
			}
			break;
	}
	return 0;
}

int CVICALLBACK HILDisplayChangeCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			InitHILDisIndex();         
			break;
	}
	return 0;
}

int CVICALLBACK AHILDisplayChangeCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	long N;
	int iTemp,ringindex;
	double dring_val;
	
	switch (event)
	{
		case EVENT_COMMIT:
			
			GetCtrlVal(panel,control,&iTemp);								//设置四个strips显示通道相同
			SetCtrlVal(HILPanelHandle,HILPNL_CHART1_CH_RING,iTemp);
			SetCtrlVal(HILPanelHandle,HILPNL_CHART2_CH_RING,iTemp);
			SetCtrlVal(HILPanelHandle,HILPNL_CHART3_CH_RING,iTemp);
			SetCtrlVal(HILPanelHandle,HILPNL_CHART4_CH_RING,iTemp);
			
			InitHILDisIndex();
			
			if(pExMem->testPara[chart1_ch].iStatus) //如果所选通道的状态为开
			{
				N = (long)(2000 / (pExMem->testPara[chart1_ch].dFreq) * CYCLE_NUM_PER_SCREEN);  //屏幕显示点数计算公式
				if(N >= 3 && N <= 10000)
				{
					SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_1,ATTR_POINTS_PER_SCREEN,N);
					SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_2,ATTR_POINTS_PER_SCREEN,N);
					SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_3,ATTR_POINTS_PER_SCREEN,N);
					SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_4,ATTR_POINTS_PER_SCREEN,N);
				}
				
				/*
				for(ringindex = 0;ringindex < 11;ringindex++)  //检测幅值所处范围并设置y坐标
				{
					GetValueFromIndex(HILPanelHandle,HILPNL_CHART1_YRANGE_RING,ringindex,&dring_val);
					if(pExMem->testPara[chart1_ch].dAmp < dring_val)
					{
						SetCtrlIndex(HILPanelHandle,HILPNL_CHART1_YRANGE_RING,ringindex);
						SetCtrlIndex(HILPanelHandle,HILPNL_CHART2_YRANGE_RING,ringindex);
						SetAxisRange(HILPanelHandle,HILPNL_STRIPCHART_1,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dring_val,dring_val);
						SetAxisRange(HILPanelHandle,HILPNL_STRIPCHART_2,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dring_val,dring_val);
						break;
					}
				}
				*/
			}
			
			break;
	}
	return 0;
}

int CVICALLBACK HILExitCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			if(ConfirmPopup ("确定退出", "确定退出？")) 
			{
				//将指令信息发送给实时系统，消息头
				sendMessageHead.iCommand = PAUSE_PROCESS;
				sendMessageHead.iMessageLen = 0;
				if(gTCPConnection != 0)
					ClientTCPWrite(gTCPConnection, &sendMessageHead, MESSAGEHEADSIZE, 1000);         //发送命令头
				QuitUserInterface(0);  
				DisconnectFromTCPServer(gTCPConnection);    
			}
			break;
	}
	return 0;
}

//力矩复位
int CVICALLBACK HILResetCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			 sendMessageHead.iCommand = RESET_PROCESS;
			 sendMessageHead.iMessageLen = COMMANDSIZE;
			 ClientTCPWrite(gTCPConnection, &sendMessageHead, MESSAGEHEADSIZE, 1000);         //发送命令头
			 Sleep(10);  
			 
			 Hctrlparadown();     
			 InitHILTestPara();
			 ClientTCPWrite(gTCPConnection, pExMem, COMMANDSIZE, 5000);		    //发送命令   
			 SetCtrlAttribute(HILPanelHandle,HILPNL_PB_START,ATTR_LABEL_TEXT,"启动");
			break;
	}
	return 0;
}

//位置回零
/*int CVICALLBACK HILReturnZeroCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double dTemp;
	switch (event)
	{
		case EVENT_COMMIT:
			InitHILTestPara();  					  
			
			GetPanelHandleFromTabPage (HILPanelHandle, HILPNL_TAB,2,&tabpanel1);    
			
			GetCtrlVal(tabpanel1,POS_TAB_CH1_ZERO_NUMERIC,&dTemp);
			pExMem->testPara[1].dOffset = dTemp;
			
			GetCtrlVal(tabpanel1,POS_TAB_CH2_ZERO_NUMERIC,&dTemp);
			pExMem->testPara[2].dOffset = dTemp;
			
			GetCtrlVal(tabpanel1,POS_TAB_CH3_ZERO_NUMERIC,&dTemp);
			pExMem->testPara[3].dOffset = dTemp;
			
			GetCtrlVal(tabpanel1,POS_TAB_CH4_ZERO_NUMERIC,&dTemp);
			pExMem->testPara[4].dOffset = dTemp;
			break;
	}
	return 0;
}*/

int CVICALLBACK HILStartCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double dTemp;
	int tempInt;
	int SendBuffLen;
	switch (event)
	{
		case EVENT_COMMIT:
		
			HILSetScreenCycNum();  //设置屏幕显示点数  	
			
			//将指令信息发送给实时系统，消息头，包括指令以及后续具体指令包的长度
			
			sendMessageHead.iCommand = START_PROCESS;
			sendMessageHead.iMessageLen = COMMANDSIZE;
			ClientTCPWrite(gTCPConnection, &sendMessageHead, MESSAGEHEADSIZE, 1000);         //发送命令头
			
			Sleep(10);
			
			Hctrlparadown();     
			InitHILTestPara();	   //初始化测试参数 
			ClientTCPWrite(gTCPConnection, pExMem, COMMANDSIZE, 5000);		    //发送命令  
			SetCtrlAttribute(panel,control,ATTR_LABEL_TEXT,"已启");
			break;
	}
	return 0;
}

int CVICALLBACK HILChTesttypeChange (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int iTemp;
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panel,control,&iTemp);
			
			switch(iTemp)
			{
				case ANY_CHART_TRACE:
					GetPanelHandleFromTabPage (HILPanelHandle,HILPNL_TAB,0,&tabpanel1);
					//侧向通道1
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,0,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道2
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,1,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道3
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,2,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道4
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,3,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_STOP_FREQ,ATTR_DIMMED,1); 
					
					//轴向通道1
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,4,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道2
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,5,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道3
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,6,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道4
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,7,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_STOP_FREQ,ATTR_DIMMED,1); 
					  
					break;
					
				case TORQUE_CHART_TRACE:
					GetPanelHandleFromTabPage (HILPanelHandle,HILPNL_TAB,0,&tabpanel1);
					//侧向通道1
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,0,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道2
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,1,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道3
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,2,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道4
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,3,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_STOP_FREQ,ATTR_DIMMED,1); 
					
					//轴向通道1
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,4,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道2
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,5,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道3
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,6,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道4
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,7,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_STOP_FREQ,ATTR_DIMMED,1); 
					
					break;
					
				case PROPORTION_TRACE:
					GetPanelHandleFromTabPage (HILPanelHandle,HILPNL_TAB,0,&tabpanel1);
					//侧向通道1
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,0,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm/°"); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道2
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,1,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(tabpanel2,CH2_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm/°"); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道3
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,2,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(tabpanel2,CH3_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm/°"); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道4
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,3,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(tabpanel2,CH4_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm/°"); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_STOP_FREQ,ATTR_DIMMED,1); 
					
					//轴向通道1
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,4,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm/°"); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道2
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,5,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(tabpanel2,CH2_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm/°"); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道3
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,6,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(tabpanel2,CH3_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm/°"); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道4
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,7,&tabpanel2);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(tabpanel2,CH4_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm/°"); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_STOP_FREQ,ATTR_DIMMED,1); 
					
					break;
					
				case VMIC_TEST_TRACE:
					GetPanelHandleFromTabPage (HILPanelHandle,HILPNL_TAB,0,&tabpanel1);
					//侧向通道1
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,0,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,VMIC_TEST_TRACE);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_WAVE,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道2
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,1,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,VMIC_TEST_TRACE); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道3
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,2,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,VMIC_TEST_TRACE); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//侧向通道4
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,3,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,VMIC_TEST_TRACE); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"Nm");
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_STOP_FREQ,ATTR_DIMMED,1); 
					
					//轴向通道1
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,4,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,VMIC_TEST_TRACE); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);  
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道2
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,5,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,VMIC_TEST_TRACE); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH2_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道3
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,6,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,VMIC_TEST_TRACE); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH3_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					
					//轴向通道4
					GetPanelHandleFromTabPage(tabpanel1, SET_TAB_CH_TAB,7,&tabpanel2);
					SetCtrlVal(tabpanel2,CH1_TAB_CH_WAVE,VMIC_TEST_TRACE); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH1_TAB_CH_OFFSET,ATTR_DIMMED,0);   
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(tabpanel2,CH1_TAB_TEXTMSG_15,ATTR_CTRL_VAL,"T");
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(tabpanel2,CH4_TAB_CH_STOP_FREQ,ATTR_DIMMED,1); 
					
					break;
			}

			break;
	}
	return 0;
}
/*
int CVICALLBACK HILCh2TesttypeChange (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int iTemp; 
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panel,control,&iTemp);
			switch(iTemp)
			{
				case ANY_CHART_TRACE:
					SetCtrlAttribute(panel,CH2_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(panel,CH2_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH2_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(panel,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					break;
				case TORQUE_CHART_TRACE:
					SetCtrlAttribute(panel,CH2_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(panel,CH2_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH2_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(panel,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,0);
					break;
				case PROPORTION_TRACE:
					SetCtrlAttribute(panel,CH2_TAB_CH_WAVE,ATTR_DIMMED,1);
					
					SetCtrlAttribute(panel,CH2_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH2_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH2_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(panel,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					break;
					
			}
			break;
	}
	return 0;
}

int CVICALLBACK HILCh3TesttypeChange (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int iTemp; 
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panel,control,&iTemp);
			switch(iTemp)
			{
				case ANY_CHART_TRACE:
					SetCtrlAttribute(panel,CH3_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(panel,CH3_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH3_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(panel,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					break;
				case TORQUE_CHART_TRACE:
					SetCtrlAttribute(panel,CH3_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(panel,CH3_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH3_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(panel,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,0);
					break;
				case PROPORTION_TRACE:
					SetCtrlAttribute(panel,CH3_TAB_CH_WAVE,ATTR_DIMMED,1);
					
					SetCtrlAttribute(panel,CH3_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH3_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH3_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(panel,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					break;
					
			}
			break;
	}
	return 0;
}

int CVICALLBACK HILCh4TesttypeChange (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int iTemp;      
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panel,control,&iTemp);
			switch(iTemp)
			{
				case ANY_CHART_TRACE:
					SetCtrlAttribute(panel,CH4_TAB_CH_WAVE,ATTR_DIMMED,0);
					SetCtrlAttribute(panel,CH4_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH4_TAB_CH_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(panel,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					break;
				case TORQUE_CHART_TRACE:
					SetCtrlAttribute(panel,CH4_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(panel,CH4_TAB_CH_AMP,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH4_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"指令幅值");
					SetCtrlAttribute(panel,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,0);
					break;
				case PROPORTION_TRACE:
					SetCtrlAttribute(panel,CH4_TAB_CH_WAVE,ATTR_DIMMED,1);
					SetCtrlAttribute(panel,CH4_TAB_CH_AMP,ATTR_DIMMED,0); 
					SetCtrlAttribute(panel,CH4_TAB_CH_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH4_TAB_CH_AMP,ATTR_LABEL_TEXT,"加载梯度");
					SetCtrlAttribute(panel,CH1_TAB_CH_START_FREQ,ATTR_DIMMED,1); 
					SetCtrlAttribute(panel,CH1_TAB_CH_STOP_FREQ,ATTR_DIMMED,1);
					break;
					
			}
			break;
	}
	return 0;
}
*/
int CVICALLBACK HILSaveParaCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double dTemp; 
	switch (event)
	{
		case EVENT_COMMIT:
			if(ConfirmPopup("Confirm Popup","保存此组参数？它将替换原内存中参数！"))
			{
			GetPanelHandleFromTabPage (HILPanelHandle, HILPNL_TAB, 1, &tabpanel1);    
			
			//通道1的控制参数
			GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,0,&tabpanel2);
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_Ktp",dTemp);
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTI,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_Kti",dTemp); 
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTD,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_Ktd",dTemp);
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FT1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_Ft1",dTemp);
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FT2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_Ft2",dTemp); 
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FTK,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_Ftk",dTemp); 
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KEP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_Kep",dTemp);
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KED,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_Ked",dTemp); 
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_TED,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_Ted",dTemp);
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_X1",dTemp);
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_X2",dTemp); 
			GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X3,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH1_X3",dTemp);
			 Ini_WriteToFile (inihandle, pathname);  
			
			//通道2的控制参数
			GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,1,&tabpanel2);
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_Ktp",dTemp);
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTI,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_Kti",dTemp); 
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTD,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_Ktd",dTemp);
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FT1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_Ft1",dTemp);
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FT2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_Ft2",dTemp); 
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FTK,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_Ftk",dTemp); 
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KEP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_Kep",dTemp);
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KED,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_Ked",dTemp); 
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_TED,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_Ted",dTemp);
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_X1",dTemp);
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_X2",dTemp); 
			GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X3,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH2_X3",dTemp);
			 Ini_WriteToFile (inihandle, pathname);  
			
			//通道3的控制参数
			GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,2,&tabpanel2);
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_Ktp",dTemp);
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTI,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_Kti",dTemp); 
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTD,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_Ktd",dTemp);
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FT1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_Ft1",dTemp);
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FT2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_Ft2",dTemp); 
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FTK,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_Ftk",dTemp); 
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KEP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_Kep",dTemp);
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KED,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_Ked",dTemp); 
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_TED,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_Ted",dTemp);
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_X1",dTemp);
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_X2",dTemp); 
			GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X3,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH3_X3",dTemp);
			 Ini_WriteToFile (inihandle, pathname);  
			
			//通道4的控制参数
			GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,3,&tabpanel2);
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_Ktp",dTemp);
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTI,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_Kti",dTemp); 
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTD,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_Ktd",dTemp);
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FT1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_Ft1",dTemp);
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FT2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_Ft2",dTemp); 
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FTK,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_Ftk",dTemp); 
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KEP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_Kep",dTemp);
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KED,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_Ked",dTemp); 
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_TED,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_Ted",dTemp);
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_X1",dTemp);
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_X2",dTemp); 
			GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X3,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH4_X3",dTemp);
			 Ini_WriteToFile (inihandle, pathname);  
			 
			//轴向通道1的控制参数
			GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,4,&tabpanel2);
			GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_KTP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH5_Ktp",dTemp);
			GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_KTI,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH5_Kti",dTemp); 
			GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_KTD,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH5_Ktd",dTemp);
			GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_FT1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH5_Ft1",dTemp);
			GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_FT2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH5_Ft2",dTemp); 
			GetCtrlVal(tabpanel2,CH1_A_TAB_NUM_HIL_FTK,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH5_Ftk",dTemp); 
			
			//轴向通道3的控制参数
			GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,5,&tabpanel2);
			GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_KTP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH6_Ktp",dTemp);
			GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_KTI,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH6_Kti",dTemp); 
			GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_KTD,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH6_Ktd",dTemp);
			GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_FT1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH6_Ft1",dTemp);
			GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_FT2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH6_Ft2",dTemp); 
			GetCtrlVal(tabpanel2,CH2_A_TAB_NUM_HIL_FTK,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH6_Ftk",dTemp);
			
			//轴向通道3的控制参数
			GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,6,&tabpanel2);
			GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_KTP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH7_Ktp",dTemp);
			GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_KTI,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH7_Kti",dTemp); 
			GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_KTD,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH7_Ktd",dTemp);
			GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_FT1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH7_Ft1",dTemp);
			GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_FT2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH7_Ft2",dTemp); 
			GetCtrlVal(tabpanel2,CH3_A_TAB_NUM_HIL_FTK,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH7_Ftk",dTemp); 
			
			//轴向通道4的控制参数
			GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,7,&tabpanel2);
			GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_KTP,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH8_Ktp",dTemp);
			GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_KTI,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH8_Kti",dTemp); 
			GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_KTD,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH8_Ktd",dTemp);
			GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_FT1,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH8_Ft1",dTemp);
			GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_FT2,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH8_Ft2",dTemp); 
			GetCtrlVal(tabpanel2,CH4_A_TAB_NUM_HIL_FTK,&dTemp);
			Ini_PutDouble(inihandle,"CtrlPara","CH8_Ftk",dTemp); 
			
			Ini_WriteToFile (inihandle, pathname);   
			
			}
			else
			{}
			
			
			break;
	}
	return 0;
}

int CVICALLBACK HILSetParaCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double dTemp;
	int selbutton;
	char keyinputbuffer[65];
	
	switch (event)
	{
		case EVENT_COMMIT:
			if(keyerr)  //检查密码标志位
			{
				selbutton = GenericMessagePopup("Generic Message Popup","控制参数修改需要管理员权限，请输入密码：",
												"确定","取消",0,keyinputbuffer,64,0,VAL_GENERIC_POPUP_INPUT_STRING,
												VAL_GENERIC_POPUP_BTN1,VAL_GENERIC_POPUP_BTN2);
				switch (selbutton)
				{
					case VAL_GENERIC_POPUP_BTN1:
						keyerr = strcmp(keyinputbuffer,originalkey); //密码比较
						if(!keyerr)									 //如果吻合
						{
							//确认
							if(ConfirmPopup("Confirm Popup","密码正确！修改参数将影响系统控制性能，进行修改？"))
							{
								keyerr = 0;
								LightHILParaSet();	//light the paraset
							}
							else
							{
								MessagePopup("Message Popup","您已经取消了控制参数的修改操作！");
							}
						}
						else										 //如果不吻合
						{
							MessagePopup("Message Popup","输入密码错误！请核对密码后再进行操作！");
							keyerr = 1;
						}
						break;
					case VAL_GENERIC_POPUP_BTN2:
						break;
					default:
						break;
							
				}
				
			}
			else
			{
				LightHILParaSet();
				GetPanelHandleFromTabPage (HILPanelHandle, HILPNL_TAB,1,&tabpanel1); 
			
				//通道1的控制参数
				GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,0,&tabpanel2);
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTP,&dTemp);
				pExMem->ctrlPara[1].Ktp = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTI,&dTemp);
				pExMem->ctrlPara[1].Kti = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KTD,&dTemp);
				pExMem->ctrlPara[1].Ktd = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FT1,&dTemp);
				pExMem->ctrlPara[1].Ft1 = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FT2,&dTemp);
				pExMem->ctrlPara[1].Ft2 = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_FTK,&dTemp);
				pExMem->ctrlPara[1].Ftk = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KEP,&dTemp);
				pExMem->ctrlPara[1].Kep = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_KED,&dTemp);
				pExMem->ctrlPara[1].Ked = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_TED,&dTemp);
				pExMem->ctrlPara[1].Ted = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X1,&dTemp);
				pExMem->ctrlPara[1].X1 = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X2,&dTemp);
				pExMem->ctrlPara[1].X2 = dTemp;
				GetCtrlVal(tabpanel2,CH1_P_TAB_NUM_HIL_X3,&dTemp);
				pExMem->ctrlPara[1].X3 = dTemp;
			
				//通道2的参数设置
				GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,1,&tabpanel2);   
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTP,&dTemp);
				pExMem->ctrlPara[2].Ktp = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTI,&dTemp);
				pExMem->ctrlPara[2].Kti = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KTD,&dTemp);
				pExMem->ctrlPara[2].Ktd = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FT1,&dTemp);
				pExMem->ctrlPara[2].Ft1 = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FT2,&dTemp);
				pExMem->ctrlPara[2].Ft2 = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_FTK,&dTemp);
				pExMem->ctrlPara[2].Ftk = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KEP,&dTemp);
				pExMem->ctrlPara[2].Kep = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_KED,&dTemp);
				pExMem->ctrlPara[2].Ked = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_TED,&dTemp);
				pExMem->ctrlPara[2].Ted = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X1,&dTemp);
				pExMem->ctrlPara[2].X1 = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X2,&dTemp);
				pExMem->ctrlPara[2].X2 = dTemp;
				GetCtrlVal(tabpanel2,CH2_P_TAB_NUM_HIL_X3,&dTemp);
				pExMem->ctrlPara[2].X3 = dTemp;
			
				//通道三参数设置
				GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,2,&tabpanel2);
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTP,&dTemp);
				pExMem->ctrlPara[3].Ktp = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTI,&dTemp);
				pExMem->ctrlPara[3].Kti = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KTD,&dTemp);
				pExMem->ctrlPara[3].Ktd = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FT1,&dTemp);
				pExMem->ctrlPara[3].Ft1 = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FT2,&dTemp);
				pExMem->ctrlPara[3].Ft2 = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_FTK,&dTemp);
				pExMem->ctrlPara[3].Ftk = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KEP,&dTemp);
				pExMem->ctrlPara[3].Kep = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_KED,&dTemp);
				pExMem->ctrlPara[3].Ked = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_TED,&dTemp);
				pExMem->ctrlPara[3].Ted = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X1,&dTemp);
				pExMem->ctrlPara[3].X1 = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X2,&dTemp);
				pExMem->ctrlPara[3].X2 = dTemp;
				GetCtrlVal(tabpanel2,CH3_P_TAB_NUM_HIL_X3,&dTemp);
			
				//通道四参数设置
				GetPanelHandleFromTabPage(tabpanel1, PARA_TAB_CTRL_PARA_TAB,3,&tabpanel2); 
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTP,&dTemp);
				pExMem->ctrlPara[4].Ktp = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTI,&dTemp);
				pExMem->ctrlPara[4].Kti = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KTD,&dTemp);
				pExMem->ctrlPara[4].Ktd = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FT1,&dTemp);
				pExMem->ctrlPara[4].Ft1 = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FT2,&dTemp);
				pExMem->ctrlPara[4].Ft2 = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_FTK,&dTemp);
				pExMem->ctrlPara[4].Ftk = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KEP,&dTemp);
				pExMem->ctrlPara[4].Kep = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_KED,&dTemp);
				pExMem->ctrlPara[4].Ked = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_TED,&dTemp);
				pExMem->ctrlPara[4].Ted = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X1,&dTemp);
				pExMem->ctrlPara[4].X1 = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X2,&dTemp);
				pExMem->ctrlPara[4].X2 = dTemp;
				GetCtrlVal(tabpanel2,CH4_P_TAB_NUM_HIL_X3,&dTemp);
				
				/*
				pExMem->requestType = SETPARA_PROCESS; 
				pExMem->request  = IS_REQUEST; 
				*/
			}
			
			break;
	}
	return 0;
}

int CVICALLBACK HILOilCtrlCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			DisplayHPSPanel();             
			break;
	}
	return 0;
}

int CVICALLBACK HILDataReDisplay (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	
	int selectstatus = 0;
	static int confirm = 0;
	
	switch (event)
	{
		case EVENT_COMMIT:
			if(DftPathSign == 0)
			{
				selectstatus = FileSelectPopup("d:\\","*.exe",".exe;.dat;.dll","选择数据回放软件",
												VAL_SELECT_BUTTON,0,1,0,0,pathname);
				if(selectstatus > 0)
				{
					confirm = ConfirmPopup("Confirm Popup","将所选程序确定为默认的数据回放程序？");
					
					if(confirm)
					{
						DftPathSign = 1; 
						strcpy(pathpass,"");
						strcat(pathpass,pathname);
						LaunchExecutable (pathpass);
					} 
					else
						DftPathSign = 0;
				}
				
			}
			else
			{
				strcpy(pathpass,"");
				strcat(pathpass,pathname);
				LaunchExecutable (pathpass);	//加载外部数据回放软件
			}
			
			break;
	}
	return 0;
}

int CVICALLBACK HILTabChanelSelect (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	long N;
	int iTemp,ringindex;
	double dring_val;
	
	switch (event)
	{
		case EVENT_ACTIVE_TAB_CHANGE:
			
			if(eventData1 != eventData2)
			{
				eventData2 += 1;
				SetCtrlVal(HILPanelHandle,HILPNL_CHART1_CH_RING,eventData2);
				SetCtrlVal(HILPanelHandle,HILPNL_CHART2_CH_RING,eventData2);
				SetCtrlVal(HILPanelHandle,HILPNL_CHART3_CH_RING,eventData2);
				SetCtrlVal(HILPanelHandle,HILPNL_CHART4_CH_RING,eventData2);
				
				InitHILDisIndex();
				
				if(pExMem->testPara[chart1_ch].iStatus) //如果所选通道的状态为开
				{
					N = (long)(2000 / (pExMem->testPara[chart1_ch].dFreq) * CYCLE_NUM_PER_SCREEN);  //屏幕显示点数计算公式
					if(N >= 3 && N <= 10000)
					{
						SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_1,ATTR_POINTS_PER_SCREEN,N);
						SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_2,ATTR_POINTS_PER_SCREEN,N);
						SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_3,ATTR_POINTS_PER_SCREEN,N);
						SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_4,ATTR_POINTS_PER_SCREEN,N);
					}
					/*
					for(ringindex = 0;ringindex < 11;ringindex++)  //检测幅值所处范围并设置y坐标
					{
						//GetValueFromIndex(HILPanelHandle,HILPNL_CHART1_YRANGE_RING,ringindex,&dring_val);
						if(pExMem->testPara[chart1_ch].dAmp < dring_val)
						{
							SetCtrlIndex(HILPanelHandle,HILPNL_CHART1_YRANGE_RING,ringindex);
							SetCtrlIndex(HILPanelHandle,HILPNL_CHART2_YRANGE_RING,ringindex);
							SetAxisRange(HILPanelHandle,HILPNL_STRIPCHART_1,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dring_val,dring_val);
							SetAxisRange(HILPanelHandle,HILPNL_STRIPCHART_2,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dring_val,dring_val);
							break;
						}
					}
					*/
				}
			}
			break;
	}
	return 0;
}
int CVICALLBACK HILParaTabChanelSelect (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	long N;
	int iTemp,ringindex;
	double dring_val;
	
	switch (event)
	{
		case EVENT_ACTIVE_TAB_CHANGE:
			
			if(eventData1 != eventData2)
			{
				eventData2 += 1;
				SetCtrlVal(HILPanelHandle,HILPNL_CHART1_CH_RING,eventData2);
				SetCtrlVal(HILPanelHandle,HILPNL_CHART2_CH_RING,eventData2);
				SetCtrlVal(HILPanelHandle,HILPNL_CHART3_CH_RING,eventData2);
				SetCtrlVal(HILPanelHandle,HILPNL_CHART4_CH_RING,eventData2);
				
				InitHILDisIndex();
				
				if(pExMem->testPara[chart1_ch].iStatus) //如果所选通道的状态为开
				{
					N = (long)(2000 / (pExMem->testPara[chart1_ch].dFreq) * CYCLE_NUM_PER_SCREEN);  //屏幕显示点数计算公式
					if(N >= 3 && N <= 10000)
					{
						SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_1,ATTR_POINTS_PER_SCREEN,N);
						SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_2,ATTR_POINTS_PER_SCREEN,N);
						SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_3,ATTR_POINTS_PER_SCREEN,N);
						SetCtrlAttribute(HILPanelHandle,HILPNL_STRIPCHART_4,ATTR_POINTS_PER_SCREEN,N);
					}
					/*
					for(ringindex = 0;ringindex < 11;ringindex++)  //检测幅值所处范围并设置y坐标
					{
						//GetValueFromIndex(HILPanelHandle,HILPNL_CHART1_YRANGE_RING,ringindex,&dring_val);
						if(pExMem->testPara[chart1_ch].dAmp < dring_val)
						{
							SetCtrlIndex(HILPanelHandle,HILPNL_CHART1_YRANGE_RING,ringindex);
							SetCtrlIndex(HILPanelHandle,HILPNL_CHART2_YRANGE_RING,ringindex);
							SetAxisRange(HILPanelHandle,HILPNL_STRIPCHART_1,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dring_val,dring_val);
							SetAxisRange(HILPanelHandle,HILPNL_STRIPCHART_2,VAL_NO_CHANGE,0.0,1.0,VAL_MANUAL,-dring_val,dring_val);
							break;
						}
					}
					*/
				}
			}
			break;
	}
	return 0;
}


  //读下位机返回的数据
static int CVICALLBACK TCPDataCallback (unsigned handle, int xType, int errCode, void *cbData)
{
	int iCommand;
	int iDataLen;
	int iChannels;
	
	char msgBuf[512];
	char tempBuf1[32];
	char tempValueBuf[128];
	char *tempBuf;
	char path[MAX_PATHNAME_LEN];  
	char datestr[16];
	char datestr1[16];
	char timestr[16];
	int i,j;
	short shortBuf[2];
	int iDataNum;
	
	
	int DataSize; 
	int bytesToRead;
	int bytesRead;
	int messageLen;
	int dataLen;
	int iDataLoops; 
	
	double * readData = NULL;
	char * readBuff = NULL;
	double * chart1 = NULL;
	double * chart2 = NULL;
	double * chart3 = NULL;
	double * chart4 = NULL;
	
	
    switch (xType)
    {
        case TCP_DISCONNECT:
			//CmtGetLock (gLock);
            gTCPConnection = TCP_INVALID_CONNECTION;
			//CmtReleaseLock (gLock);
            break;
        case TCP_DATAREADY: // Client executable sent a command.	  //得到数据
			// ClientTCPRead does not guarantee that all data will be transferred in
			// a single call, but this code assumes that it does for simplicity.
			
			// ClientTCPRead does not guarantee that all data will be transferred in
			// a single call, but this code assumes that it does for simplicity.
			//InsertTextBoxLine(runningpanel, RUNPANEL_MSGSTRING,0,"收到命令反馈！");   
			bytesToRead = MESSAGEHEADSIZE;
			
			//printf("address1 = %x,address2=%x", &readCommandMessage, (char*)(&readCommandMessage) +1);
			
			while (bytesToRead > 0)		 //读帧头
			{
				bytesRead = ClientTCPRead (gTCPConnection, (char*)(&readMessageHead) + (MESSAGEHEADSIZE - bytesToRead), bytesToRead, 0);
				bytesToRead -= bytesRead;
			}
			
			iCommand = readMessageHead.iCommand;
			iDataLen = readMessageHead.iMessageLen;
			iDataLoops =  iDataLen /transdatasizeH; //每个数据包中包含的控制采样循环次数
		
			if(iCommand == 6)	   //如果是数据，就读出所有的数据
			{
				bytesToRead =  iDataLen*sizeof(double);  
				DataSize = bytesToRead;
				
				readData = (double *)malloc(iDataLen*sizeof(double));
				readBuff = (char *) malloc(iDataLen*sizeof(double));
				
				chart1 = (double *)malloc((sizeof(double)*iDataLoops) * 2);
				chart2 = (double *)malloc(sizeof(double)*iDataLoops);
				chart3 = (double *)malloc((sizeof(double)*iDataLoops) * 1);
				chart4 = (double *)malloc(sizeof(double)*iDataLoops);
				
				while (bytesToRead > 0)		 //读数据
				{
					bytesRead = ClientTCPRead (gTCPConnection, &readBuff[DataSize - bytesToRead], bytesToRead, 0);
					bytesToRead -= bytesRead;
				}
				
				memcpy(readData,readBuff, DataSize);
				
				if(recording == 1)
				{
					//memcpy((saveBuf + saveIndex ),readBuff,dataStructSize);
					memcpy((saveBuf + saveIndex),readData,DataSize); 
					saveIndex += iDataLoops;
				}
				
				for(i=0;i<iDataLoops;i++)
				{
					//下位机发上来的数每个周期是36个double型的数，分为4组，0-7个是指令，是角度指令还是力矩指令由测试项目决定
					//8-11侧向力矩采样，12-15个为轴向力采样，16-23个为控制输出，24-27是侧向位移采样，28-31是侧向角度采样，
					//32-35是侧向加速度采样，每种类型的数占8个，如果通道数不到8个，剩余的就留作备用
					
					chart1[2*i] = readData[transdatasizeH*i+0+chart3_ch-1];
					chart1[2*i+1] = readData[transdatasizeH*i+8+chart3_ch-1]; 
					
				//	chart2[i] = readData[transdatasizeH*i+0+chart1_ch-1] - readData[transdatasizeH*i+8+chart1_ch-1];
					
					chart3[i] = readData[transdatasizeH*i+28+chart1_ch-1];   //码盘角度采样
					
				//	chart4[i] = readData[transdatasizeH*i+16+chart1_ch-1];
					switch(chart2_content)
					{
						case POSITION_SAMPLE_INDEX:
								chart2[i] = readData[transdatasizeH*i+24+chart2_ch-1];
							break;
						case ACCELEROMETER_SAMPLE_INDEX:
								chart2[i] = readData[transdatasizeH*i+32+chart2_ch-1];  
							break;
						case TORQUE_SAMPLE_INDEX:
								chart2[i] = readData[transdatasizeH*i+8+chart2_ch-1];  
							break;
						case TORQUE_GIVEN_INDEX:
								chart2[i] = readData[transdatasizeH*i+0+chart2_ch-1];      
							break;
						case SERVO_OUT_INDEX:
								chart2[i] = readData[transdatasizeH*i+16+chart2_ch-1];      
							break;
						case ANGLE_SAMPLE_INDEX:
								//chart2[i] = chart1[2*i]- chart1[2*i+1];   ///////////////
								chart2[i] = readData[transdatasizeH*i+28+chart2_ch-1];
							break;
						default:
								chart2[i] = 0.0;
							break;
					}
				
					switch(chart4_content)
					{
						/*case ANGLE_SAMPLE_INDEX:
								chart4[i] = readData[transdatasizeH*i+8+chart4_ch-1];
							break;
						case ANGLE_GIVEN_INDEX:
								chart4[i] = readData[transdatasizeH*i+0+chart4_ch-1]; 
							break;*/
						case TORQUE_SAMPLE_INDEX:
								chart4[i] = readData[transdatasizeH*i+8+chart4_ch-1]; 
							break;
						case TORQUE_GIVEN_INDEX:
								chart4[i] = readData[transdatasizeH*i+0+chart4_ch-1];;
							break;
						case SERVO_OUT_INDEX:
								chart4[i] = readData[transdatasizeH*i+16+chart4_ch-1]; 
							break;
						case TORQUE_ERROR_INDEX:
								chart4[i] = chart1[2*i]- chart1[2*i+1];   ///////////////
							break;
						default:
								chart4[i] = 0.0;
							break;
					
				
					}
				
					
				}
				
				if(iDataLoops>0)
				{
					PlotStripChart(HILPanelHandle, HILPNL_STRIPCHART_1,chart3,1*iDataLoops,0,0,VAL_DOUBLE);
					PlotStripChart(HILPanelHandle, HILPNL_STRIPCHART_2,chart2,iDataLoops,0,0,VAL_DOUBLE);  
					PlotStripChart(HILPanelHandle, HILPNL_STRIPCHART_3,chart1,2*iDataLoops,0,0,VAL_DOUBLE);
					PlotStripChart(HILPanelHandle, HILPNL_STRIPCHART_4,chart4,iDataLoops,0,0,VAL_DOUBLE); 
				}
				
				if(readData != NULL)
				{
					SetCtrlVal(HILPanelHandle, HILPNL_NUMERIC, readData[8]);  
					SetCtrlVal(HILPanelHandle, HILPNL_NUMERIC_2, readData[9]);    
					SetCtrlVal(HILPanelHandle, HILPNL_NUMERIC_3, readData[10]);    
					SetCtrlVal(HILPanelHandle, HILPNL_NUMERIC_4, readData[11]);   
					SetCtrlVal(HILPanelHandle, HILPNL_NUMERIC_5, readData[12]);  
					SetCtrlVal(HILPanelHandle, HILPNL_NUMERIC_6, readData[13]);    
					SetCtrlVal(HILPanelHandle, HILPNL_NUMERIC_7, readData[14]);    
					SetCtrlVal(HILPanelHandle, HILPNL_NUMERIC_8, readData[15]); 
				}
				
			}
		
            break;
    }
	
	
	if(readData!= NULL) free(readData);
	if(readBuff!=NULL) free(readBuff);
	if(chart1 != NULL) free(chart1);
	if(chart2 != NULL) free(chart2);  
	if(chart3 != NULL) free(chart3); 
	if(chart4 != NULL) free(chart4);  
    
    return 0;
}

int CVICALLBACK NetWatchDog3 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	
	switch (event)
	{
		case EVENT_TIMER_TICK:
			
			//将指令信息发送给实时系统，消息头，包括指令以及后续具体指令包的长度
			sendMessageHead.iCommand = WATCHDOG;   
			
			sendMessageHead.iMessageLen = 0;		 //也没有实际的内容
		
			//ClientTCPWrite(gTCPConnection, &sendMessageHead, MESSAGEHEADSIZE, 1000);         //发送命令头
			
			udpsendmsg.command = udpmsg.command;//返回命令字
			
			source_p1= ((float)tag_reg[1]-5530)/  (27648.0-5530.0) *40;
			source_p2= ((float)tag_reg[2]-5530)/  (27648.0-5530.0) *40;
			
			udpsendmsg.PressureStateValue1 = (short)(source_p1*100);
			udpsendmsg.PressureStateValue2 = (short)(source_p2*100);
			
			sys_temp= ((float)tag_reg[0]-5530)/ (27648.0-5530.0) *200-50;	   //温度范围   
			
			udpsendmsg.TemperatureValue = (short)(sys_temp*100);
			
			udpsendmsg.SourceState = (short)tag_reg[5];
			
			
			if(gUDPChannel != NULL)
				UDPWrite(gUDPChannel, 20000, SEND_COMPUTER_ADDR, &udpsendmsg, 14);
			
			//调用油源的更新程序
			//HPStimer(0, 0, EVENT_TIMER_TICK, 0, 0, 0);
			
			break;
	}
	return 0;
}


int CVICALLBACK	UDPChatCallback (unsigned channel, int eventType, int errCode, void *callbackData)
{
	int				error = 0,
					size;
	unsigned char	*msg = NULL;
	float value;   
	
	if (eventType == UDP_DATAREADY)
	{
		IPAddr srcAddr;
		
		// Pass NULL as the input buffer to determine the size of the arrived data.
		size = UDPRead(channel, NULL, 0, UDP_DO_NOT_WAIT, NULL, NULL);
		
		msg = malloc(size);
		
		// Read the waiting message into the allocated buffer.
		size = UDPRead(channel, msg, size, UDP_DO_NOT_WAIT, NULL, (char*)srcAddr);
		
		sprintf(msgBuff,"size = %d",size);
		//MessagePopup("UDP", msgBuff);  
		
		
		// Process the message.
		if(size == 10)
		{
			memcpy(&udpmsg, msg, 10);
			if(udpmsg.command == 0x01)   //液压泵调压
			{
				value = udpmsg.commandvalue1 / 100.0 / 21.0  * (13824);
				send_reg[0]  = (unsigned short int)value;
				value = udpmsg.commandvalue2 / 100.0 / 21.0  * (13824);
				send_reg[1]  = (unsigned short int)value;
			}
			
			if(udpmsg.command == 0x02)	  //启停泵
			{
				if(udpmsg.commandvalue1 == 0 )
				{
					control_word = control_word & 0xF700;    
					control_word = control_word | 0x1000;
					//SetCtrlVal (HPSPANEL, HPSPANEL_MOTOR_RING, !state); 
				}
				else
				{
					control_word = control_word & 0xEF00;    
					control_word = control_word | 0x0800;
					//SetCtrlVal(HPSPANEL, HPSPANEL_MOTOR_RING, !state); 
			
				}
				
				if(udpmsg.commandvalue2 == 0)
				{
					control_word = control_word & 0xFD00;    
					control_word = control_word | 0x0400;
					//SetCtrlVal (HPSPANEL, HPSPANEL_MOTOR_RING_2, !state); 
				}
				else
				{
					control_word = control_word & 0xFB00;    
					control_word = control_word | 0x0200;
					//SetCtrlVal(HPSPANEL, HPSPANEL_MOTOR_RING_2, !state); 
				}
			
				send_reg[2] = control_word;
				
			}
			if(udpmsg.command == 0x03)		//加载启动
			{
				//将指令信息发送给实时系统，消息头，包括指令以及后续具体指令包的长度
			
				sendMessageHead.iCommand = START_PROCESS;
				sendMessageHead.iMessageLen = COMMANDSIZE;
				ClientTCPWrite(gTCPConnection, &sendMessageHead, MESSAGEHEADSIZE, 1000);         //发送命令头
			
				Sleep(10);
			
				Hctrlparadown();     
				InitHILTestPara();	   //初始化测试参数 
				ClientTCPWrite(gTCPConnection, pExMem, COMMANDSIZE, 5000);		    //发送命令  
				SetCtrlAttribute(HILPanelHandle,HILPNL_PB_START,ATTR_LABEL_TEXT,"已启");
				
			}
			
			if(udpmsg.command == 0x04)   //加载复位
			{
				//将指令信息发送给实时系统，消息头，包括指令以及后续具体指令包的长度
			
				 sendMessageHead.iCommand = RESET_PROCESS;
				 sendMessageHead.iMessageLen = COMMANDSIZE;
				 ClientTCPWrite(gTCPConnection, &sendMessageHead, MESSAGEHEADSIZE, 1000);         //发送命令头
				 Sleep(10);  
		 
				 Hctrlparadown();     
				 InitHILTestPara();
				 ClientTCPWrite(gTCPConnection, pExMem, COMMANDSIZE, 5000);		    //发送命令   
				 SetCtrlAttribute(HILPanelHandle,HILPNL_PB_START,ATTR_LABEL_TEXT,"启动");
				
			}
			
		}
	}
Error:	
	
	return 0;
}

 
int CVICALLBACK HILDownParaCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			sendMessageHead.iCommand = SETPARA_PROCESS; 
			sendMessageHead.iMessageLen = COMMANDSIZE;
			ClientTCPWrite(gTCPConnection, &sendMessageHead, MESSAGEHEADSIZE, 1000);         //发送命令头
			Hctrlparadown(); 
			ClientTCPWrite(gTCPConnection, pExMem, COMMANDSIZE, 5000);		    //发送命令和参数
			
			break;
	}
	return 0;
}

int CVICALLBACK HILOliDisplay (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			DisplayHPSPanel();   
			break;
	}
	return 0;
}
