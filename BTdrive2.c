/*
формат компоновки отправляемого пакета.
Первые два байта являются шапкой для NXT. 
До исполняемой программы доходят только последующие байты начиная с 3его.

char * message; 
message[0]=32; // Системный байт для NXT.
message[1]=0;	 // Системный байт для NXT.
message[2]=left_wheel_speed; // Скорость подаваемая на левое колесо (порт А) '-100'..'0'..'100'
message[3]=right_wheel_speed; // Скорость подаваемая на правое колесо (порт С) '-100'..'0'..'100'
message[4]=kick; // '1' - удар пиналкой, '-1' - инвертированный удар пиналкой, '0' - ничего.
message[5]=sound_signal; // '1' - издать звук, '0' - ничего.
message[6]=sensor_port; // '1'..'4' - порт сенсора, с которого необходимо собрать информацию, '0' - ничего.

Каждый приходящий пакет разбирается отдельно. Каждые полученные данные отображаются на мониторе.
При инициализации удара или звука запускаются соответствующая функция, игнорирующая новые инициализации до окончания своей работы.
При инициализации сенсора с соответствующего порта берётся замер, который незамедлительно отсылается обратно по форме re_message[0]=port, re_message[1:2]=data.
*/
#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
  
#define DEVICE_NAME "MYNXT" /* Bluetooth device name */
#define PASS_KEY    "LEJ0S-OSEK" /* Bluetooth pass key */ //"1234" 

// OSEK declarations 
#define MA NXT_PORT_A
#define MB NXT_PORT_B
#define MC NXT_PORT_C
const U8 NXT_PORT[5]={0,NXT_PORT_S1,NXT_PORT_S2,NXT_PORT_S3,NXT_PORT_S4};
DeclareCounter(SysTimerCnt);
DeclareTask(ReadBtPacket);
DeclareTask(StatusMonitor);
DeclareTask(Kicker);

//------------system_func---------------------
// Initialization

// Termination 
void ecrobot_device_terminate()
{
	ecrobot_set_motor_speed(MA, 0);
	ecrobot_set_motor_speed(MB, 0);
	ecrobot_set_motor_speed(MC, 0);
    ecrobot_term_bt_connection();
	ecrobot_sound_tone(100, 800, 30);
	systick_wait_ms(500);
}
 
// Increment SysTimerCnt 
void user_1ms_isr_type2(void)
{ 
	(void)SignalCounter(SysTimerCnt);
}

//-----------user_func-----------------------
#define BYTE_MA 0
#define BYTE_MC 1
#define BYTE_BKick 2
#define BYTE_BBeep 3
#define BYTE_BSens 4
S8 BKick; // "1" - kick; "-1" - reverse kick, "0" -nothing
S8 BBeep; // "1" - beep, "0" - nothing
S8 BSens; // "1".."4" - the port number, "0" - nothing
U8 status[32]="+000+000K0B0S0R0"; //Status Monitor ="+000+000K0B0S0";
S16 rssi; //signal power
S8 speedMA=0;
S8 speedMC=0;
U32 recvtime=0;
S8 ININOW=1;
//---INI---
void ecrobot_device_initialize()
{
	if (ININOW==1) 
	{
		ININOW=0;
		ecrobot_sound_tone(800, 500, 30);
	}
	ecrobot_init_bt_slave(PASS_KEY);    
	if (systick_get_ms()-recvtime>5000)
	{
		recvtime=systick_get_ms();
		ecrobot_sound_tone(200, 100, 30);
	} 
}

S8 sign (S8 number)
{
   return( number>=0 ? 1:-1);	
}
U8 abs (S8 number)
{
	return( number>=0 ? number:-number);	
}
void save()
{
	status[0]=(speedMA>=0 ? '+':'-');
	status[1]=(abs(speedMA)/100)+'0';
	status[2]=(abs(speedMA)/10)%10+'0';
	status[3]=(abs(speedMA)%10)+'0';
	status[4]=(speedMC>=0 ? '+':'-');
	status[5]=(abs(speedMC)/100)+'0';
	status[6]=(abs(speedMC)/10)%10+'0';
	status[7]=(abs(speedMC))%10+'0';
	status[8]='K';
	status[9]='0'+BKick;
	status[10]='B';
	status[11]='0'+BBeep;
	status[12]='S';
	status[13]='0'+BSens;
	
	//	rssi = 0;//ecrobot_get_bt_signal_strength();
			/*if (rssi==0)
	{
		//BBeep=2;
	}
	if (rssi<0)
	{
		BBeep=3;
	}	*/
		status[14]='P';
		switch (ecrobot_get_bt_status())
		{
		case BT_NO_INIT:
			status[15]='1';break;
		case BT_INITIALIZED:
			status[15]='2';break;
		case BT_CONNECTED:
			status[15]='3';break;
		case BT_STREAM:
			status[15]='4';break;
		}
		status[16]=0;

		//status[14]='P';	
		//if (rssi<10)
		//	status[15]=rssi+48;
		//else 
		//status[15]='+';
		//status[16]=0;
}
TASK(Beeper)
{
	if (BBeep==1)
	{
		ecrobot_sound_tone(300, 180, 20);
//		systick_wait_ms(1000);
		BBeep=0;
	}
	if (BBeep==2)
	{
		ecrobot_sound_tone(100, 10, 5);
		systick_wait_ms(100);
		ecrobot_sound_tone(100, 10, 5);
//		systick_wait_ms(1000);
		BBeep=0;
	}
	if (BBeep==3)
	{
		ecrobot_sound_tone(800, 100, 30);
		systick_wait_ms(200);
		ecrobot_sound_tone(600, 100, 30);
		systick_wait_ms(200);
		ecrobot_sound_tone(400, 100, 30);
		systick_wait_ms(200);
		BBeep=0;
	}
	if (BBeep==4)
	{
		ecrobot_sound_tone(800, 50, 30);
		systick_wait_ms(200);
		ecrobot_sound_tone(800, 50, 30);
		BBeep=0;
	}


	TerminateTask();
}
TASK(Kicker)
{
	if (BKick==1)
	{
		ecrobot_set_motor_speed(MB,-100);
		systick_wait_ms(150);
		ecrobot_set_motor_speed(MB,50);
		systick_wait_ms(350);		
		ecrobot_set_motor_speed(MB,0);	
		BKick=0;
	}
	if (BKick==-1)
	{
		ecrobot_set_motor_speed(MB,100);
		systick_wait_ms(150);
		ecrobot_set_motor_speed(MB,-50);
		systick_wait_ms(350);		
		ecrobot_set_motor_speed(MB,0);
		BKick=0;
	}
	TerminateTask();
}

TASK(ReadBtPacket)
{
	//	ecrobot_set_bt_device_name(DEVICE_NAME);
	static S8 bt_receive_buf[34]; 
	static U16 SensRe;	
	static U8 bt_feedback_buf[34];
	U32 re;
	//ecrobot_term_bt_connection();	
	
	if (ecrobot_get_bt_status()!=BT_STREAM) 
	{
		BBeep=2;
		//systick_wait_ms(500);
		ecrobot_init_bt_slave("LEJOS-OSEK");   
	}
	else
	{
		re=ecrobot_read_bt_packet((U8*)bt_receive_buf, 32);		
		//re=ecrobot_read_bt((U8*)bt_receive_buf,0,34);	
		if (re>0)
		{			
			recvtime=systick_get_ms();
			speedMA=bt_receive_buf[BYTE_MA];
			speedMC=bt_receive_buf[BYTE_MC];
			if (abs(speedMA)<=100)
				ecrobot_set_motor_speed(MA,speedMA);
			else 
				ecrobot_set_motor_speed(MA,100*sign(speedMA));			
			if (abs(speedMC)<=100)
				ecrobot_set_motor_speed(MC,speedMC);
			else
				ecrobot_set_motor_speed(MC,100*sign(speedMC));			

			if ((re!=34) &&(re!=32) )
			{
				BBeep=4;
				speedMC=re;
			}
			
			if ((bt_receive_buf[BYTE_BKick]==1) && (BKick==0)) BKick=1;
			if ((bt_receive_buf[BYTE_BKick]==-1) && (BKick==0)) BKick=-1;
			if (bt_receive_buf[BYTE_BBeep]==1) BBeep=1;
			if (bt_receive_buf[BYTE_BSens]>=1 && bt_receive_buf[BYTE_BSens]<=4) BSens=bt_receive_buf[BYTE_BSens];			
			if (BSens)
			{
				SensRe=ecrobot_get_gyro_sensor(NXT_PORT[BSens]);
				bt_feedback_buf[0]=BSens;
				bt_feedback_buf[1]=((U8*)(&SensRe))[0];
				bt_feedback_buf[2]=((U8*)(&SensRe))[1];
				bt_feedback_buf[3]=0;
				ecrobot_send_bt_packet(bt_feedback_buf,32);
				BSens=0;
			}
		}
		else
			if (systick_get_ms()-recvtime>10000)
			{
				rssi=ecrobot_get_bt_signal_strength();
				if (rssi<=0)
				{						
					ecrobot_set_motor_speed(MA, 0);
					ecrobot_set_motor_speed(MB, 0);
					ecrobot_set_motor_speed(MC, 0);
					ecrobot_term_bt_connection();	
					BBeep=3;
				}
				else
				{
					recvtime=systick_get_ms();
					BBeep=2;
				}
			}
	}
	TerminateTask();
}


//U16 SensData=ecrobot_get_sound_sensor(U8 port_id);
//U8  SensData=ecrobot_get_touch_sensor(U8 port_id);
//U16 SensData=ecrobot_get_light_sensor(U8 port_id);
TASK(StatusMonitor)          //display motor_sensors, bluetooth_cond, ...
{	
	save();
	ecrobot_status_monitor(status);
	display_goto_xy(9,1);
	display_unsigned((systick_get_ms()-recvtime)%10000,5);
	display_goto_xy(9,2);
	display_int(rssi%1000,5);
	display_update();
	TerminateTask();
}
