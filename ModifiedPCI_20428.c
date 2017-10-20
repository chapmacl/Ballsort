/************************************************************************/
/* PCI-20428W-1 Funktionen fuer E/A					*/
/*                                                                      */
/*                                                                      */
/*                                                                      */
/************************************************************************/

#include <stdio.h>


#define IO_BASIS	0x320		/* Grundeinstellung PCI20428	*/
#define IO_CARD_ID	0x30		/* ID der PCI20428		*/

#define True		1
#define False		0

#define AE_FAKTOR       0.0048818125

#define TEST
#define TEST_X



int     ERR_428      = True;
char    A_WERT_428   = 0;



/****************************************************************/
/****************************************************************/
/* Initialisierung der IO-Karte PCI20428			*/
/****************************************************************/

int init_428 (void)
{
   int	id;

   id = sysInByte (IO_BASIS);

   if (id != IO_CARD_ID) 
      {
       ERR_428 = True;
      }
   else
      {
       sysOutByte (IO_BASIS, 0x00);
       sysOutByte (IO_BASIS+1, 0x00);
       sysOutByte (IO_BASIS+8, 0x00);
       ERR_428 = False;
      }
   return (ERR_428);
}



/****************************************************************/
/* Digitalausgabe ueber IO-Karte PCI20428			*/
/****************************************************************/

void dgau_428 (char wert)
{
   if (! ERR_428)
   {
    sysOutByte (IO_BASIS+2, wert);
    A_WERT_428 = wert;
   }
}


/****************************************************************/
/* Digitaleingabe ueber IO-Karte PCI20428			*/
/****************************************************************/

char dgei_428 (void)
{
   char dgei = 0;

   if (! ERR_428)
   {
    dgei = sysInByte (IO_BASIS+2);
   }
   return (dgei);
}


/****************************************************************/
/* Analogeingabe ueber IO-Karte PCI20428			*/
/****************************************************************/

void anau_428 (char chn, float wert)
{
   char h_byte, l_byte;
   int  anau_wert;

   if (! ERR_428)
   {
    if ((chn == 0) || (chn == 1))
    {
     anau_wert = (int) ((wert + 10.0) / AE_FAKTOR);
     l_byte    = (char) (anau_wert & 0x00ff);
     h_byte    = (char) ((anau_wert >> 8) & 0x000f);
     
     sysOutByte (IO_BASIS+13 + 2*chn, h_byte);
     sysOutByte (IO_BASIS+12 + 2*chn, l_byte);
     sysOutByte (IO_BASIS+11, 0x00);
    }
   }
}


/****************************************************************/
/* Analogausgabe ueber IO-Karte PCI20428			*/
/****************************************************************/

float anei_428 (char chn)
{
   float anei  = 0;
   char  verst = 0;
   char  h_byte, l_byte;
   int	 aewert;
   int   aewert1, aewert2;

   if (! ERR_428)
   {
    if ((chn >= 0) && (chn <= 15))
    {
     sysOutByte (IO_BASIS+9, (((verst & 0x03) << 4) + chn)); 
/*   taskDelay (1);  */
     sysOutByte (IO_BASIS+10, 0);
/*   taskDelay (1);  */
     do
     {  
/*    taskDelay (1); */
     }  
     while ((sysInByte (IO_BASIS+1) & 0x01) != 0x01);   
     h_byte = sysInByte (IO_BASIS+11) & 0x0f;
     l_byte = sysInByte (IO_BASIS+10);
     aewert = (((int) (h_byte) << 8) & 0x0f00) + 
               ((int) (l_byte) & 0x00ff);
     anei   = (float) (aewert) * AE_FAKTOR - 10.0;
    }
   }
   return (anei);
}



/****************************************************************/
/* Ausgabe eines Bitmusters auf BS				*/
/****************************************************************/

void  bit_ (char laenge, char wert)
{
  int i, j;

  for (i = laenge-1; i >= 0; i--)
  {
   j = (wert >> i) & 1;
   printf ("%i", j);
   if ((i == 4) || (i == 8) || (i == 12))
     printf (" ");
  }
}



/****************************************************************/
/* Funktion SetBit                               		*/
/****************************************************************/

int SetBit_428 (char bit, char wert)
{
   int err_SetBit;

/*   semTake (mySemId_SetBit, WAIT_FOREVER);  */
   switch (wert)
   {
   case 0 :  A_WERT_428 = A_WERT_428 & ((1 << bit) ^ 0xff);
             err_SetBit = 0;
             break;
   case 1 :  A_WERT_428 = A_WERT_428 | (1 << bit);
             err_SetBit = 0;
             break;
   default :
             err_SetBit = 1;
   }
   dgau_428 (A_WERT_428);

/*   semGive (mySemId_SetBit);  */
   return (err_SetBit);
}




/****************************************************************/
/* Ende der E/A-Funktionen fuer PCI-20428W-1                    */
/****************************************************************/


