#ifndef CM_KunI
#define CM_KunI 1
/* ------------------------------------------------ *
 * This is new version of Controller Module         *
 * File Name   : CM.H                               *
 * Author      : Kun Yi Chen                        *
 * Version     : 1.00                               *
 * Create Date : 2002/ 4/ 15                        *
 * Modification:                                    *
 * Developer Platform : Keil C v6.2x & Intel 8031   *
 * ------------------------------------------------ */

/* include all header file                          */
#include <REG51.H>
#include <ABSACC.H>
/* Peripheral Address                               */

/* ------------------------------------------------ *
 * External RAM Address                             *
 * 0xF000 ~ 0xFFFF                                  *
 * use 62128 or 62256 or 61128 or 61256             *
 * have 16K or 32 Byte                              *
 * ------------------------------------------------ */

/* System constance                                 */
#define  XTAL                             (11059200)
#define  TIMER0H                  ((65535-9207)/256)
#define  TIMER0L                  ((65535-9207)%256)

/* Programable Interface Device 8255                */
#define  PID_BASE        0x2000
#define  PID_A           (XBYTE[PID_BASE])
#define  PID_B           (XBYTE[PID_BASE+1])
#define  PID_C           (XBYTE[PID_BASE+2])
#define  PID_CTRL        (XBYTE[PID_BASE+3])

/* ------------------------------------------------ *
 * Pin assign                                       *
 * DIP_SW   use Port1                               *
 * MUX_SEL  use P1.4                                *
 * UART_DIR use P3.4                                *
 * WatchDOg use P3.5                                *
 *                                                  *
 * ------------------------------------------------ */
#define  DIP_SW  P1
sbit MUXSEL = P1^4;
sbit UART_DIR = P3^4;
sbit WATCH_DOG = P3^5;

/* PID 8255 CTL(controll) register attrib           */
#define  PID_AIN        (0x90)    /* 10010000B bit4 */
#define  PID_AOUT       (0x80)    /* 10000000B      */
#define  PID_BIN        (0x82)    /* 10000010B bit1 */
#define  PID_BOUT       (0X80)    /* 10000000B      */
#define  PID_CHIN       (0X88)    /* 10001000B bit3 */
#define  PID_CHOUT      (0X80)    /* 10000000B      */
#define  PID_CLIN       (0x81)    /* 10000001B bit0 */
#define  PID_CLOUT      (0x80)    /* 10000000B      */
/* Directional mode                                 */
#define  DIRWITHIN         1      /* Dir with in    */
#define  DIRWITHOUT        0      /* Dir with out   */

/* Serial communication packet                      */
/* ------------------------------------------------ *
 * Packet format defined                            *
 * <SOH> : Start Of Header, it's always 0x01        *
 * <NUM> : CM Number                                *
 * <CMD> : Command                                  *
 * <VAL> : Command parameter                        *
 * <OPT> : <CMD> Optional parameter                 *
 * <CHK> : CheckSum, from <NUM> field XOR each all  *
 *         date field                               *
 * ------------------------------------------------ */
#define     SOH            (0x01)
#define     MASTERID       (0x00)

/* defined element                                  */
/* ------------------------------------------------ *
 * Command define                                   *
 * character 'D': direction setting of PID Port     *
 * character 'R': read port value of PID            *
 * character 'W': write port value of PID           *
 * character 'P': generation a pulse                *
 * character 'T': pulse width parameter             *
 * character 'S': send a 9HZ, CM self produce,      *
 * ------------------------------------------------ */

 /* <CMD> defined                                   */
#define CMD_DIRECTION        ('D')
#define CMD_READ             ('R')
#define CMD_WRITE            ('W')
#define CMD_PULSE            ('P')
#define CMD_TERM             ('T')
#define CMD_AIOWATCHDOG      ('S')

/* <VAL> defined                                    */
#define PORT_A               ('A')
#define PORT_B               ('B')
#define PORT_CH              ('H')
#define PORT_CL              ('L')
#define DIRECTION_STATE      ('d')
#define CLOSEAIO             ('N')

/* <OPT> defined                                    */
#define SET_INPUT            ('I')
#define SET_OUTPUT           ('O')


/* defined customer data type                       */
typedef enum { WAIT_SOH,     /* wait SOH      */
               RCV_ID,       /* get module ID */
	       RCV_CMD,      /* get command   */
               RCV_VAL,      /* get value     */
	       RCV_OPT,      /* get optional  */
	       RCV_CHK       /* get checksum  */
} RcvPacketState;

typedef enum { FALSE = 0, TRUE = 1 } Boolean;
typedef enum { RX_DIR = 0, TX_DIR  = 1 } UartDir;
typedef enum { ID_SEL = 0, DIR_SEL = 1} MuxSel;
typedef unsigned char   Byte;
typedef Byte *   PByte;
typedef unsigned short  Word;
typedef Word *   PWord;


typedef struct {  Byte QueH;
                  Byte QueT;
		  Byte QueLen;
} QueCtrl;

/* functions declarative                            */
void Initialization(void);
void GeneraltionPulse(void);
Byte GetParameterLen(Byte cmd);
void DeCommand(void);
void ReplyCMD(Byte typed);
void DispatchCMD(void);
void SettingPID(void);

void GetID(void);
void GetDirSW(void);

/* interrupt vector function                        */
void UART_INT(void);
void TIMER0_INT(void);

/* data struct operation macro define               */
#define MaxQueLen  (128)

#define CreateQue(x)        Byte xdata x[MaxQueLen];                          \
                            QueCtrl data x##C;                                \
			    Boolean _PutTo##x(Byte Val);                      \
			    Boolean _GetFrom##x(PByte Val);

#define ImplementQue(x)     Boolean _PutTo##x(Byte Val)                       \
                            {                                                 \
			     ES = FALSE;                                      \
                             if (IsFullOfQue(x))                              \
			       { ES = TRUE; return FALSE; }                   \
			     x##C.QueLen++;                                   \
                             x[x##C.QueT++] = Val;                            \
			                                                      \
                             if(x##C.QueT >= MaxQueLen) x##C.QueT = 0;        \
			     ES = TRUE;                                       \
                             return TRUE;                                     \
                            }                                                 \
                                                                              \
                            Boolean _GetFrom##x(PByte Val)                    \
                            {                                                 \
			     ES = FALSE;                                      \
			     if (IsEmptyOfQue(x))                             \
			       { ES = TRUE; return FALSE; }                   \
			     *Val = x[x##C.QueH++];                           \
			     x##C.QueLen--;                                   \
			     if(x##C.QueH >= MaxQueLen) x##C.QueH = 0;        \
			     ES = TRUE;                                       \
			     return TRUE;                                     \
			    }

#define InitialQue(x)   x##C.QueH = x##C.QueT = x##C.QueLen = 0
#define IsEmptyOfQue(x) (x##C.QueLen == 0) ? TRUE:FALSE
#define IsFullOfQue(x)  (x##C.QueLen >= MaxQueLen) ? TRUE:FALSE
#define GetQueLen(x)    x##C.QueLen
#define PutToQue(x, value) _PutTo##x((value))
#define GetFromQue(x, value) _GetFrom##x(value)

#define CONFIRMOK        (0)
#define MAINCMDERR       (CONFIRMOK+1)
#define DIRSUBCMDERR     (CONFIRMOK+2)
#define RDDIRERR         (CONFIRMOK+3)
#define RDSUBCMDERR      (CONFIRMOK+4)
#define WRDIRERR         (CONFIRMOK+5)
#define WRSUBCMDERR      (CONFIRMOK+6)
#define FAILPULSECMD     (CONFIRMOK+7)
#define PULSEDIRERR      (CONFIRMOK+8)
#define PULSESUBCMDERR   (CONFIRMOK+9)
#define RETURNVALUE      (CONFIRMOK+10)

#endif
