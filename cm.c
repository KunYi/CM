#include "cm.h"

/* ------------------------------------------------ *
 * File Name   : CM.C                               *
 * Author      : Kun Yi Chen                        *
 * Version     : 1.00                               *
 * Create Date : 2002/ 4/ 15                        *
 * Modification:                                    *
 * Developer Platform : Keil C v6.2x                *
 * ------------------------------------------------ */
 
Byte bdata   _BitsArray;
Byte bdata   _PulsePortMask;
/* Direction of PID port                    */ 
sbit PORTA_DIR      = _BitsArray^3;
sbit PORTB_DIR      = _BitsArray^2;
sbit PORTCH_DIR     = _BitsArray^1;
sbit PORTCL_DIR     = _BitsArray^0;
/* Recvice packet OK                        */
sbit _bRcvOK        = _BitsArray^4;
sbit _bNowPulse     = _BitsArray^5;
sbit _bAIO_WatchDog = _BitsArray^6;
/* Pulse of PID port                        */
sbit _bPulseOfPA    = _PulsePortMask ^ 3;  
sbit _bPulseOfPB    = _PulsePortMask ^ 2;
sbit _bPulseOfPCH   = _PulsePortMask ^ 1;
sbit _bPulseOfPCL   = _PulsePortMask ^ 0;
/* receive command buffer                   */
Byte data  _RcvID, _RcvCMD, _RcvVAL, _RcvOPT;
/* Phase packet state                       */
Byte data  _RcvState;
/* Transmit command buffer                  */
Byte data  _TxVAL;
/* Control module ID                        */
Byte data  _SelfID;
/* AIO Reset count                          */
Byte data  _TimerCount;
Word data  _10SCount;
Byte data  _AIOCount;
/* 9Hz = 11.1ms                             */
#define AIOOneCycle      11 
#define AIOWDLength    1000
/* PID output bufffer                       */
Byte data  _PortABuff, _PortBBuff, _PortCBuff;
/* Generaltion pulse mask                   */
Byte data  _PortAMask, _PortBMask, _PortCMask;
/* Current pulse lenght count               */
Byte data  _PulseCount;
/* Pulse width                              */
Byte data  _PulseWidth;

/* declear network fifo(circle quene)       */         
CreateQue(_RxQue)
CreateQue(_TxQue)
/* Implement opration function              */
ImplementQue(_RxQue)
ImplementQue(_TxQue)

void main(void)
{
  Initialization();

    
  while(1)
  {
   /* Reset watchdog timer  */
   WATCH_DOG = ~WATCH_DOG;
   /* get controller module ID */
   GetID();
   
   /* recvice packet        */
   if (GetQueLen(_RxQue) > 0)
     DeCommand();

   /* Process host command  */
   if (_bRcvOK) 
   { 
      _bRcvOK = FALSE;
	  DispatchCMD();
   }
   
   /* Process pulse        */
   if (_bNowPulse)
   {
    if ((Byte)(_PulseWidth + _PulseCount) == _TimerCount)
    {
       GeneraltionPulse();
	   _bNowPulse = FALSE;
	}
   }
   
   /* Process AIO pulse       */
   if (_bAIO_WatchDog)
   {
     if ((Byte)(AIOOneCycle + _AIOCount) == _TimerCount)
	 {
       /* should check PID_C directional        
	      but old CM no check            */
	   _AIOCount = _TimerCount;

       // NOT ( bit 7 of _PortCBuff)
       if ((_PortCBuff & 0x80) == 0x80)
          _PortCBuff &= 0x7F;
       else
          _PortCBuff |= 0x80;
     }

     if (_10SCount >= AIOWDLength)
	 { 
	   _bAIO_WatchDog = FALSE;
       /* disable AIO watch dog then
	      must be clear PID_C bit 7 */
	   _PortCBuff &= 0x7F;
	 }
	  
     PID_C = _PortCBuff;
	 
   } /* end of if(_bAIO_WatchDog) */
  
  } /* main loop */
}

/* --------------------------------------------------- *
 * Function : Initialization                                  *
 * Parameter: void                                     *
 * result   : void                                     *
 * descript : initial system                           *
 * --------------------------------------------------- */ 
void Initialization(void)
{
  WATCH_DOG = ~WATCH_DOG;

  _bRcvOK = _bNowPulse = FALSE;
  _RcvCMD = _RcvVAL = _RcvOPT = 0;
  _10SCount = _AIOCount = _TimerCount = 0;
  _PortABuff = _PortBBuff = _PortCBuff = 0;
  // default Port out;
  P1 = 0x0F;
  P3 = 0xCF;
  _bAIO_WatchDog = TRUE;
  _RcvState = WAIT_SOH;

  WATCH_DOG = ~WATCH_DOG;
  
  /* getting control module ID and PID port directional */
  GetDirSW();
  SettingPID();
  UART_DIR = RX_DIR;

  WATCH_DOG = ~WATCH_DOG;

  /* Initial circuit         */
  InitialQue(_RxQue);
  InitialQue(_TxQue);

  WATCH_DOG = ~WATCH_DOG;

  /* setting timer 0 with mode 1
             timer 1 with mode 2 */
  TMOD = 0x21;

  TH0 = TIMER0H;
  TL0 = TIMER0L;
  TH1 = 253;   /* 9600 of baudrate   */
  SCON = 0x50; /* 8bits of char      */

  TCON = 0x50; /* TR0, TR1 enable    */
  IE = 0x92;   /* EA, ES, ET0 enable */
  IP = 0x10;   /* PS enable          */

  TR1 = 1;
  TR0 = 1;
  ES = 1;
  ET0 = 1;
  EA = 1;

  WATCH_DOG = ~WATCH_DOG;

}

/* --------------------------------------------------- *
 * Function : GetID                                    *
 * Parameter: void                                     *
 * result   : void                                     *
 * descript : get DIP switch setting for CM ID         *
 * --------------------------------------------------- */ 
void GetID(void)
{
  MUXSEL = ID_SEL;
  _SelfID = (DIP_SW & 0x0f);
}

/* --------------------------------------------------- *
 * Function : GetDirSw                                 *
 * Parameter: void                                     *
 * result   : void                                     *
 * descript : get DIP switch setting for PID port      *
 *            directional                              *
 * --------------------------------------------------- */ 
void GetDirSW(void)
{
  MUXSEL = DIR_SEL;
  _BitsArray &= 0xF0;
  _BitsArray |= (DIP_SW & 0x0F);
}

/* --------------------------------------------------- *
 * Function : SettingPID                               *
 * Parameter: void                                     *
 * result   : void                                     *
 * descript : Setting PID port directional             *
 * --------------------------------------------------- */ 
void SettingPID(void)
{
  Byte Ctrl = 0x80;
 
  if (PORTA_DIR == DIRWITHIN) Ctrl |= 0x10;
  if (PORTB_DIR == DIRWITHIN) Ctrl |= 0x02;
  if (PORTCH_DIR == DIRWITHIN) Ctrl |= 0x08;
  if (PORTCL_DIR == DIRWITHIN) Ctrl |= 0x01;
  
  PID_CTRL = Ctrl;
}

/* --------------------------------------------------- *
 * Function : DispatchCMD                              *
 * Parameter: cmd, val, opt                            *
 * result   : void                                     *
 * descript : process cmd and dispatch                 *
 * --------------------------------------------------- */ 
void DispatchCMD(void)
{
  Byte ReCode = CONFIRMOK;

  switch (_RcvCMD)
  {
   case CMD_DIRECTION: 
   /* Setting PID port direction */
        switch (_RcvVAL)
        {
         case PORT_A:
              switch (_RcvOPT)
              {
              	case SET_INPUT:
				    PORTA_DIR = DIRWITHIN;
              	    break;
              	case SET_OUTPUT:
				    PORTA_DIR = DIRWITHOUT;
              	    break;
              	default:
                    goto DIRSUBERR;
					break;
              }
              break;
         case PORT_B:
              switch (_RcvOPT)
              {
              	case SET_INPUT:
				    PORTB_DIR = DIRWITHIN;
              	    break;
              	case SET_OUTPUT:
				    PORTB_DIR = DIRWITHOUT;
              	    break;
              	default:
                    goto DIRSUBERR;
              	    break;
              }
              break;
         case PORT_CH:
              switch (_RcvOPT)
              {
              	case SET_INPUT:
				    PORTCH_DIR = DIRWITHIN;
              	    break;
              	case SET_OUTPUT:
				    PORTCH_DIR = DIRWITHOUT; 
              	    break;
              	default:
                    goto DIRSUBERR;
              	    break;
              }
              break;
         case PORT_CL:
              switch (_RcvOPT)
              {
              	case SET_INPUT:
				    PORTCL_DIR = DIRWITHIN;
              	    break;
              	case SET_OUTPUT:
				    PORTCL_DIR = DIRWITHOUT;
              	    break;
              	default:
                    goto DIRSUBERR;
					break;
              }
              break;
         default:
DIRSUBERR:
		      ReCode = DIRSUBCMDERR;
              break;
        }

        if (ReCode == CONFIRMOK)
          SettingPID();
        break;
   /* End CMD_DIRECTION case     */
   
   case CMD_READ:
   /* Reading PID port value     */
        switch (_RcvVAL)
        {
         case PORT_A:
		      if (PORTA_DIR == DIRWITHIN)
			  {
                 _TxVAL = PID_A;
	             ReCode = RETURNVALUE;
			  }
			  else
			     ReCode = RDDIRERR;
              break;
         case PORT_B:
		      if (PORTB_DIR == DIRWITHIN)
			  {
			     _TxVAL = PID_B;
	             ReCode = RETURNVALUE;
			  }
              else			    
			     ReCode = RDDIRERR;
              break;
         case PORT_CH:
		      if (PORTCH_DIR == DIRWITHIN)
			  {
			     _TxVAL = (PID_C >> 4);
	             ReCode = RETURNVALUE;
			  }
              else			    
			     ReCode = RDDIRERR;
              break;
         case PORT_CL:
		      if (PORTCL_DIR == DIRWITHIN)
			  {
			     _TxVAL = PID_C & 0x0F;
	             ReCode = RETURNVALUE;
			  }
              else			    
			     ReCode = RDDIRERR;
              break;
         case DIRECTION_STATE:
		      /* Read directional with all port */
			  _TxVAL = _BitsArray & 0x0F;
			  ReCode = RETURNVALUE;
              break;
         default:
 		      ReCode = RDSUBCMDERR;
		      break;
        }
        
        break;
   /* End CMD_READ case          */
   
   case CMD_WRITE:
   /* Reading PID port value     */
        switch (_RcvVAL)
        {
         case PORT_A:
              if (PORTA_DIR == DIRWITHOUT)
			  {
			     _PortABuff = _RcvOPT;
			     PID_A = _RcvOPT;
			  }
		      else
                ReCode = WRDIRERR;			  
              break;
         case PORT_B:
              if (PORTB_DIR == DIRWITHOUT)
			  {
			     _PortBBuff = _RcvOPT;
			     PID_B = _RcvOPT;
			  }
		      else
                ReCode = WRDIRERR;			  
              break;
         case PORT_CH:
              if (PORTCH_DIR == DIRWITHOUT)
			  {
			     _PortCBuff &= 0x0F;
			     _PortCBuff = (_RcvOPT << 4);
			     PID_C = _PortCBuff;
			  }
		      else
                ReCode = WRDIRERR;			  
              break;
         case PORT_CL:
              if (PORTCL_DIR == DIRWITHOUT)
			  {
			    _PortCBuff &= 0xF0;
				_PortCBuff |= (0x0F & _RcvOPT);
			    PID_C = _PortCBuff;
			  }
		      else
                ReCode = WRDIRERR;			  
              break;
         case DIRECTION_STATE:
		      /* Setting directional of all port */
			  _BitsArray &= 0xF0;
			  _BitsArray |= (0x0F & _RcvOPT );
			  SettingPID();
              break;
         default:
		      ReCode = WRSUBCMDERR;
		      break;
        }
        
        break;
   /* End CMD_WRITE case         */
   
   case CMD_PULSE:
   /* Port 'val' generation pulse 
           with (opt*10ms) width */
        if (!_bNowPulse)
        {
          switch(_RcvVAL)
          {
		    Byte temp;

            case PORT_A:
		         if (PORTA_DIR == DIRWITHOUT)
                 {
			       _bNowPulse   = TRUE;
		           _bPulseOfPA  = TRUE;
				   _PortAMask   = _RcvOPT;
				   _PortABuff  ^= _PortAMask;
				   PID_A        = _PortABuff;
			     }
			     else
			       ReCode = PULSEDIRERR;
		         break;
            case PORT_B:
		         if (PORTB_DIR == DIRWITHOUT)
			     {
			       _bNowPulse   = TRUE;
			       _bPulseOfPB  = TRUE;
				   _PortBMask   = _RcvOPT;
				   _PortBBuff  ^= _PortBMask;
				   PID_B        = _PortBBuff;
			     }
			     else
			       ReCode = PULSEDIRERR;
		         break;
            case PORT_CH:
		         if (PORTCH_DIR == DIRWITHOUT)
			     {
			       _bNowPulse   = TRUE;
			       _bPulseOfPCH = TRUE;
				   _PortCMask  &= 0x0F;
				   _PortCMask  |= (_RcvOPT << 4);
				   temp  = _PortCBuff ^ _PortCMask;
				   temp &= 0xF0;
                   _PortCBuff |= temp;
				   PID_C = _PortCBuff;
			     }
			     else
			       ReCode = PULSEDIRERR;
		         break;
            case PORT_CL:
		         if (PORTCL_DIR == DIRWITHOUT)
			     {
			       _bNowPulse    = TRUE;
			       _bPulseOfPCL  = TRUE;
				   _PortCMask   &= 0xF0;
				   _PortCMask   |= (_RcvOPT & 0x0F);
				   temp  = _PortCBuff ^ _PortCMask;
				   temp &= 0x0F;
				   _PortCBuff |= temp;
				   PID_C = _PortCBuff;
			     }
			     else
			       ReCode = PULSEDIRERR;
		         break;
            default:
               ReCode = PULSESUBCMDERR;
          } /* end switch(_RcvVAL) */
		}
        else { ReCode = FAILPULSECMD; }

        if (ReCode == CONFIRMOK)
		  _PulseCount = _TimerCount;
        break;
   /* End CMD_PULSE case         */ 
   
   case CMD_TERM:
   /* setting a pulse width      */

   /* Is generaltion pulse       */
      if (!_bNowPulse)
        _PulseWidth = _RcvVAL;
	  else
	    ReCode = FAILPULSECMD ;
      break;
        
   /* End CMD_TEST case          */ 

   case CMD_AIOWATCHDOG:
   /* CM use bit 7 of PID port C 
      generation 10Hz signal and 
	  length 10sec               */
	  if (_RcvVAL == 'N') _AIOCount = _TimerCount;
      _bAIO_WatchDog = TRUE;
	  _10SCount = 0;
        break;
   /* End CMD_SEND case          */

   default:
        ReCode = MAINCMDERR;
        break;
  }
  ReplyCMD(ReCode);
}

void GeneraltionPulse(void)
{
    Byte temp;

    if (_bPulseOfPA)
	{
	  _bPulseOfPA = FALSE;
	  if (DIRWITHOUT == PORTA_DIR)
	  {
	    _PortABuff ^= _PortAMask;
	    PID_A = _PortABuff;
	  }
	}

	if (_bPulseOfPB)
	{
	  _bPulseOfPB = FALSE;
	  if (DIRWITHOUT == PORTB_DIR)
	  {
	    _PortBBuff ^= _PortBMask;
	    PID_B = _PortBBuff;
	  }
	}

	if (_bPulseOfPCH)
	{
	  _bPulseOfPCH = FALSE;
	  if (DIRWITHOUT == PORTCH_DIR)
	  {
	    temp  = _PortCMask ^ _PortCBuff;
	    temp &= 0xF0;
	    _PortCBuff |= temp;
	    PID_C = _PortCBuff;
	  }
	}

	if (_bPulseOfPCL)
	{
	  _bPulseOfPCL = FALSE;
	  if (DIRWITHOUT == PORTCL_DIR)
	  {
        temp  = _PortCMask ^ _PortCBuff;
        temp &= 0x0F;
	    _PortCBuff |= temp;
	    PID_C = _PortCBuff;
	  }
	}
}

/* --------------------------------------------------- *
 * Function : GetParameterLen                          *
 * Parameter: cmd                                      *
 * result   : parameter number                         *
 * descript : get CMD parameter number                 *
 * --------------------------------------------------- */ 
Byte GetParameterLen(Byte cmd)
{
  register Byte result = 0;
 
  switch (cmd) {
  case CMD_DIRECTION:
  case CMD_WRITE:
  case CMD_PULSE:
       result = 2;
	   break;
  case CMD_READ:
  case CMD_TERM:
  case CMD_AIOWATCHDOG:
       result = 1;
	   break;
  }
  return result;
/*
  Byte code CMD_TBL[] = 
         { CMD_DIRECTION, CMD_READ, CMD_WRITE, 
	       CMD_PULSE, CMD_TERM, CMD_AIOWATCHDOG };
  Byte code CMD_LEN[(sizeof(CMD_TBL)/sizeof(Byte))] = 
	     { 2, 1, 2, 2, 1, 1 };
  Byte i;
 
  for( i = 0; i < (sizeof(CMD_TBL)/sizeof(Byte)); i++)
     if (CMD_TBL[i] == cmd) return CMD_LEN[i];
  return 0;
*/
}

/* --------------------------------------------------- *
 * Function : ReplyCMD                                 *
 * Parameter: typed                                    *
 * result   : null                                     *
 * descript : reply master command                     *
 * --------------------------------------------------- */ 
void ReplyCMD(Byte typed)
{
  Byte Cmd, Val;
  Byte Chk;

  switch(typed)
  {
   case CONFIRMOK:
        Cmd = 'O';
        Val = 'K';
        break;
   case RETURNVALUE:
        Cmd = 'V';
		Val = _TxVAL;
		break;
   case MAINCMDERR:
   case DIRSUBCMDERR:
   case RDDIRERR:
   case RDSUBCMDERR:
   case WRDIRERR:
   case WRSUBCMDERR:
   case FAILPULSECMD:
   case PULSEDIRERR:
   case PULSESUBCMDERR:
        Cmd = 'X';
	    Val = '1' + (typed - 1);
	    break;
   default:
        /* CM error */
        Cmd = 'E';
		Val = 'R';
		break;
  }
  
  Chk = MASTERID;
  Chk ^= Val;
  Chk ^= Cmd;

  PutToQue(_TxQue, SOH);
  PutToQue(_TxQue, MASTERID);
  PutToQue(_TxQue, Cmd);
  PutToQue(_TxQue, Val);
  PutToQue(_TxQue, Chk);
  UART_DIR = TX_DIR;
  TI = 1;
}

/* --------------------------------------------------- *
 * Function : UART_INT                                 *
 * Parameter: null                                     *
 * result   : null                                     *
 * descript : UART event interrupter, it using 2 of    *
 *            Register Bank                            *
 * --------------------------------------------------- */ 
void UART_INT(void) interrupt 4 using 2
{
  Byte tmp;

  if (RI)
  {
    RI = 0;
    _bAIO_WatchDog = TRUE;
	ET0 = 0;
    _10SCount = 0;
	ET0 = 1;
	PutToQue(_RxQue,SBUF);
  }

  if (TI)
  {
    TI = 0;
    if (GetFromQue(_TxQue,&tmp) == TRUE)
	{
	  if (IsEmptyOfQue(_TxQue))
	  {
	    // using RS485 network interface
  	    // then the last of transmit stream
	    // switch to 9bits format with UART registers
		// corrected problem with cutoff with stop bit
		TB8 = 1;     /* dummy stop bit       */
		SCON = 0xF0; /* switch to 9bits mode */
	  }
    SBUF = tmp;
	} 
	else 
	{
	   // Tx queue is empty
	   // re-switch 8bits format and chage to receiver
	   SCON = 0x50;
	   UART_DIR = RX_DIR;
	}
  }
}

/* --------------------------------------------------- *
 * Function : TIMER0_INT                               *
 * Parameter: null                                     *
 * result   : null                                     *
 * descript : timer0 event interrupter, it using 2 of  *
 *            Register Bank,Timer 0 is Mode 1, adjust  *
 *            10ms length                              *
 * --------------------------------------------------- */ 
void TIMER0_INT(void) interrupt 1 using 2
{
  TH0 = TIMER0H;
  TL0 = TIMER0L;
  _10SCount++;
  _TimerCount++;
}


/* --------------------------------------------------- *
 * Function : DeCommand                                *
 * Parameter: null                                     *
 * result   : null                                     *
 * descript : check correct packet of recvice stream   *
 *            and decode it to command type            *
 * --------------------------------------------------- */ 
void DeCommand(void)
{
 static Byte RcvCHK;
 Byte tmp;

 while (!IsEmptyOfQue(_RxQue))
 {
    if (GetFromQue(_RxQue,&tmp)== FALSE)
	  return;

	switch (_RcvState)
	{
	 case WAIT_SOH:
	      if (tmp == SOH) 
		  {  
		    _RcvState = RCV_ID;
			_RcvCMD = 0;
		  }
		  break;

     case RCV_ID:
		  RcvCHK = tmp;
	      _RcvID = tmp;
		  _RcvState = RCV_CMD;
	      break;

	 case RCV_CMD:
		  RcvCHK ^= tmp;
	      _RcvCMD = tmp;
		  _RcvState = RCV_VAL;
	      break;

     case RCV_VAL:
		  RcvCHK ^= tmp;
	      _RcvVAL = tmp;
		  switch (GetParameterLen(_RcvCMD))
		  {
		    case 1:
			    _RcvState = RCV_CHK;
				break;
			case 2:
			    _RcvState = RCV_OPT;
				break;
			default:
			/* otherwise length not defined   */
			    _RcvState = WAIT_SOH;
				break;
		  }
	      break;

	 case RCV_OPT:
		  RcvCHK ^= tmp;
	      _RcvOPT = tmp;
		  _RcvState = RCV_CHK;
	      break;

	 case RCV_CHK:
	      /* _RcvID equal zero nothing */
		  /* because it be MasterID    */ 
		  if ((tmp == RcvCHK) && (_SelfID == _RcvID) && (_RcvID != 0)) 
		     _bRcvOK = TRUE;

 	 default:
	      _RcvState = WAIT_SOH;
		  break;
	}
  } 
}

