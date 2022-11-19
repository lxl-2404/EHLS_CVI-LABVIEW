#pragma once

typedef struct
{
   	double AngleCom[4];
	double TorqueSample[4]; 
	double AngleGiven[4];
	double ServoOut[4];
	double synOut[4]; 
	double forward[4]; 
	double controlout[4];  
	double difffoeward[4];  
	double reserve[4]; 
}TESTDATA;

typedef struct
{
	int AngleSample[5];
	int AngleGiven[5];
	int TorqueSample[5];
	int TorqueGiven[5];
	int ServoOut[5];
}DataMessage;

typedef struct
{
   	int AngleSample[5];
	int AngleGiven[5];
	int TorqueSample[5];
	int TorqueGiven[5];
	int ServoOut[5];
}TESTDATANEW;


typedef struct
{
	//力指令五个
   double dTrace[5];
   //舵机角度指令
   double dTrace_A[5];
   
   double Pos[453031];
   double Tor[453031];
   
}TRACEDATA;

typedef struct
{
   	int iWave;
	double dAmp;
	double dFreq;
	double dRamp;
	int    iCycle;
	double distance;
}WAVESTRUCT;



//上位机往下位机发送数据的头
typedef struct
{
	int iCh;
	int iLen; 
}DataFrameHead;


 
