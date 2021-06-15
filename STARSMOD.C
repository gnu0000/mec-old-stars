/*
 *
 * starsmod.c
 * Sunday, 4/27/1997.
 * Craig Fitzgerald
 *
 */

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GnuType.h>
#include <GnuMisc.h>
#include <GnuVar.h>
#include <GnuStr.h>
#include <GnuMem.h>
#include <GnuArg.h>

#define OBJ_SHIP       1
#define OBJ_BEAM       2
#define OBJ_TORP       3
#define OBJ_BOMB       4
#define OBJ_TERRAFORM  5
#define OBJ_PLANETARY  6
#define OBJ_MINER      7
#define OBJ_MINE       8
#define OBJ_MECH       9
#define OBJ_ELECT      10
#define OBJ_SHIELD     11
#define OBJ_SCANNER    12
#define OBJ_ARMOR      13
#define OBJ_ENGINE     14
#define OBJ_STARGATE   15
#define OBJ_MASSDRIVER 16
#define OBJ_STARBASE   17

typedef struct
   {
   USHORT   uType;
   ULONG  ulOffset;
   USHORT uSize;
   USHORT uCount;
   PSZ    pszName;
   } MET;
typedef MET *PMET;

MET ObjData [] =
   {
   {OBJ_SHIP      , 0x30CF, 0x8F,  0x20,  "SHIP"       },
   {OBJ_BEAM      , 0x42AF, 0x3C,  0x18,  "BEAM"       },
   {OBJ_TORP      , 0x484F, 0x3C,  0x0C,  "TORP"       },
   {OBJ_BOMB      , 0x4B1F, 0x3A,  0x0F,  "BOMB"       },
   {OBJ_TERRAFORM , 0x4E85, 0x36,  0x14,  "TERRAFORM"  },
   {OBJ_PLANETARY , 0x52BD, 0x36,  0x0F,  "PLANETARY"  },
   {OBJ_MINER     , 0x55E7, 0x36,  0x08,  "MINER"      },
   {OBJ_MINE      , 0x5797, 0x36,  0x0A,  "MINE"       },
   {OBJ_MECH      , 0x59B3, 0x36,  0x0B,  "MECH"       },
   {OBJ_ELECT     , 0x5C05, 0x36,  0x11,  "ELECT"      },
   {OBJ_SHIELD    , 0x5F9B, 0x36,  0x0A,  "SHIELD"     },
   {OBJ_SCANNER   , 0x61B7, 0x38,  0x10,  "SCANNER"    },
   {OBJ_ARMOR     , 0x6537, 0x36,  0x0C,  "ARMOR"      },
   {OBJ_ENGINE    , 0x67BF, 0x4E,  0x10,  "ENGINE"     },
   {OBJ_STARGATE  , 0x6C9F, 0x38,  0x07,  "STARGATE"   },
   {OBJ_MASSDRIVER, 0x6E27, 0x38,  0x08,  "MASSDRIVER" },
   {OBJ_STARBASE  , 0x7253, 0x8F,  0x05,  "STARBASE"   },
   {0             , 0     , 0   ,  0   ,  ""           }
   };

typedef struct
   {
   USHORT uType;
   UCHAR  cUnknown1;
   UCHAR  cCount;
   } SLOT;
typedef SLOT *PSLOT;


typedef struct
   {
   USHORT uCargo;
   USHORT uFuel;
   USHORT uArmor;
   SLOT   slot[16];
   UCHAR  cSlotsUsed;
   UCHAR  cInitiative;
   UCHAR  cUnknown1;
   UCHAR  cBRCargoLoc;
   UCHAR  cTLCargoLoc;
   UCHAR  cSlotLoc[16];
   } HULL;
typedef HULL *PHULL;

typedef struct
   {
   USHORT uItem;
   UCHAR  cTech[6];      // tech requirements
   CHAR   szName[32];
   USHORT uMass;
   USHORT uCost;
   USHORT uIro;
   USHORT uBor;
   USHORT uGer;
   USHORT uPic;
   } HEAD;
typedef HEAD *PHEAD;


void DumpHead (FILE *fp, PHEAD phead, PSZ pszName, USHORT uIndex);
void DumpExtents (FILE *fp, PVOID pData, USHORT uType);
void DumpFileHeader (FILE *fp);


BOOL bDEBUG;
BOOL bOVER;
PSZ  pszSECT;

PSZ GetAtt (USHORT uAtt)
   {
   switch (uAtt)
      {
      case 0 : return "En ";
      case 1 : return "Sc ";
      case 2 : return "Sh ";
      case 3 : return "Ar ";
      case 4 : return "Be ";
      case 5 : return "To ";
      case 6 : return "Bo ";
      case 7 : return "Ro ";
      case 8 : return "Mi ";
      case 9 : return "Or ";
      case 10: return "Un ";
      case 11: return "El ";
      case 12: return "Me ";
      case 13: return "13 ";
      case 14: return "14 ";
      case 15: return "15 ";
      }
   }

void PrintSlotAtts (FILE *fp, USHORT uVal)
   {
   USHORT i;

   for (i=0; i<16; i++)
      if (uVal & (1 << i))
         fprintf (fp, "%s", GetAtt (i));
   }


USHORT AttribVal (PSZ pszAtts)
   {
   USHORT i, uVal;

   uVal = 0;
   while (*pszAtts)
      {
      for (; *pszAtts == ' ' || *pszAtts == '\t'; pszAtts++)
         ;
      for (i=0; i<16; i++)
         if (!strnicmp (pszAtts, GetAtt (i), 2))
            break;
      if (i == 16)
         Error ("Bad attribute: %s", pszAtts);
      uVal |= 1 << i;
      pszAtts += 2;
      }
   return uVal;
   }


void IntVar (PVAR pvList, PSZ pszName, PUSHORT puVal)
   {
   char szVal [256];
   PSZ  pszJunk;

   if (VarGet (pvList, pszName, szVal))
      *puVal = (USHORT) strtoul (szVal, &pszJunk, 0);
   }

void SIntVar (PVAR pvList, PSZ pszName, PSHORT pVal)
   {
   char szVal [256];
   PSZ  pszJunk;

   if (VarGet (pvList, pszName, szVal))
      *pVal = (SHORT) strtol (szVal, &pszJunk, 0);
   }



void CharVar (PVAR pvList, PSZ pszName, PUCHAR pcVal)
   {
   char szVal [256];
   PSZ  pszJunk;

   if (VarGet (pvList, pszName, szVal))
      *pcVal = (UCHAR) strtoul (szVal, &pszJunk, 0);
   }

/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/


void LoadHead (PVAR pvList, PHEAD phead)
   {
   CHAR szVal [256];
   PPSZ ppsz;
   USHORT i, uCols;

   if (VarGet (pvList, "Name" , szVal)) strcpy (phead->szName, szVal);
   IntVar (pvList, "Index"    , &phead->uItem);
   IntVar (pvList, "Mass"     , &phead->uMass);
   IntVar (pvList, "Resources", &phead->uCost);
   IntVar (pvList, "Iron"     , &phead->uIro );
   IntVar (pvList, "Boranium" , &phead->uBor );
   IntVar (pvList, "Germanium", &phead->uGer );
   IntVar (pvList, "PictIdx"  , &phead->uPic );

   if (VarGet (pvList, "Tech" , szVal))
      {
      ppsz = StrMakePPSZ (szVal, ",", FALSE, TRUE, &uCols);
      for (i=0; i<6; i++)
         phead->cTech[i] = atoi (ppsz[i]);         
      MemFreePPSZ (ppsz, 0);
      }
   }


void LoadHull (PVAR pvList, PHULL phull)
   {
   CHAR szVal [256], szName [256];
   PPSZ ppsz;
   INT  i, uCols;

   IntVar  (pvList, "Cargo"     , &phull->uCargo     );
   IntVar  (pvList, "Fuel"      , &phull->uFuel      );
   IntVar  (pvList, "Armor"     , &phull->uArmor     );
   CharVar (pvList, "Initiative", &phull->cInitiative);
   CharVar (pvList, "unknown"   , &phull->cUnknown1  );
   CharVar (pvList, "SlotsUsed" , &phull->cSlotsUsed );

   if (VarGet (pvList, "CargoPos" , szVal))
      {
      ppsz = StrMakePPSZ (szVal, ",", FALSE, TRUE, &uCols);
      phull->cTLCargoLoc  = atoi (ppsz[0]) << 4;
      phull->cTLCargoLoc += atoi (ppsz[1]) % 16;
      phull->cBRCargoLoc  = atoi (ppsz[2]) << 4;
      phull->cBRCargoLoc += atoi (ppsz[3]) % 16;
      MemFreePPSZ (ppsz, 0);
      }

   for (i=0; i<16; i++)
      {
      sprintf (szName, "SLOT_%2.2d", i);
      if (VarGet (pvList, szName, szVal))
         {
         ppsz = StrMakePPSZ (szVal, ",", FALSE, TRUE, &uCols);
         phull->cSlotLoc[i]    = atoi (ppsz[0]) << 4;
         phull->cSlotLoc[i]   += atoi (ppsz[1]) % 16;
         phull->slot[i].cCount = atoi (ppsz[2]);
         phull->slot[i].uType  = AttribVal (ppsz[3]);
         MemFreePPSZ (ppsz, 0);
         }
      }
   }


void LoadExtents (PVAR pvList, PUSHORT puData, USHORT uType)
   {
   PPSZ ppsz;
   UINT i, uCols;
   CHAR szVal [256];

   switch (uType)
      {
      case OBJ_SHIP         :
      case OBJ_STARBASE     :
         LoadHull (pvList, (PHULL)puData);
         break;

      case OBJ_BEAM       :
         IntVar (pvList, "Range",      puData+0);
         IntVar (pvList, "Power",      puData+1);
         IntVar (pvList, "Initiative", puData+2);
         IntVar (pvList, "Flags",      puData+3);
         break;

      case OBJ_TORP       :
         IntVar (pvList, "Range",      puData+0);
         IntVar (pvList, "Power",      puData+1);
         IntVar (pvList, "Initiative", puData+2);
         IntVar (pvList, "Accuracy",   puData+3);
         break;

      case OBJ_BOMB         :
         IntVar (pvList, "Unknown",  puData+0);
         IntVar (pvList, "PopKill",  puData+1);
         IntVar (pvList, "FactKill", puData+2);
         break;
         
      case OBJ_TERRAFORM    :
         IntVar (pvList, "Terraform", puData);
         break;
         
      case OBJ_PLANETARY:
         SIntVar (pvList, "Range", puData);
         break;
         
      case OBJ_MINER        :
         IntVar (pvList, "MineRate", puData);
         break;
         
      case OBJ_MINE         :
         IntVar (pvList, "Mines", puData);
         break;
         
      case OBJ_MECH     :
      case OBJ_ELECT    :
         IntVar (pvList, "Attribute", puData);
         break;
         
      case OBJ_SHIELD   :
         IntVar (pvList, "Strength", puData);
         break;
         
      case OBJ_SCANNER  :
         IntVar (pvList, "Attribute1", puData+0);
         IntVar (pvList, "Attribute2", puData+1);
         break;
         
      case OBJ_ARMOR    :
         IntVar (pvList, "Strength", puData);
         break;

      case OBJ_ENGINE  :
         if (VarGet (pvList, "FuelUse" , szVal))
            {
            ppsz = StrMakePPSZ (szVal, ",", FALSE, TRUE, &uCols);
            for (i=0; i<uCols; i++)
               puData[i+1] = atoi (ppsz[i]);
            MemFreePPSZ (ppsz, 0);
            }
         IntVar (pvList, "Unknown1", puData+0);
         IntVar (pvList, "Unknown2", puData+12);
         break;

      case OBJ_STARGATE     :
         IntVar (pvList, "SafeMass", puData+0);
         IntVar (pvList, "SafeDist", puData+1);
         break;
         
      case OBJ_MASSDRIVER   :
         IntVar (pvList, "Speed",   puData+0);
         IntVar (pvList, "Unknown", puData+1);
         break;
         
      }
   }


void InsertObjects (FILE *fpExe, PSZ pszDatFile)
   {
   PMET  pMet;
   HEAD  head;
   PVAR  pvList = NULL;
   FILE  *fpDebug;
   UCHAR szBuff [256], szSection[256];
   USHORT  i;

   if (bDEBUG)
      {
      fpDebug = fopen ("DEBUG.DAT", "wt");
      DumpFileHeader (fpDebug);
      }
   for (pMet=ObjData; pMet->uType; pMet++)
      {
      if (pszSECT && stricmp (pszSECT, pMet->pszName))
         continue;

      printf ("\nPutting %s data...\n", pMet->pszName);
      for (i=0; i< pMet->uCount; i++)
         {
         if (fseek (fpExe, pMet->ulOffset + i * pMet->uSize, SEEK_SET))
            Error ("Unable to seek");
         if (!fread (&head, sizeof (HEAD), 1, fpExe))
            Error ("could not read object: %s [%d]", pMet->pszName, i);
         if (!fread (szBuff, pMet->uSize - sizeof (HEAD), 1, fpExe))
            Error ("could not read object extents: %s [%d]", pMet->pszName, i);

         /*--- update data from dat file ---*/
         sprintf (szSection, "%s_%2.2d", pMet->pszName, i);
         if (VarReadCfg (&pvList, pszDatFile, szSection))
            {
            printf ("Reading Section: %s\n", szSection);

            LoadHead    (pvList, &head);
            LoadExtents (pvList, (PUSHORT)szBuff, pMet->uType);
            pvList = FreeVar (pvList);
            }
         if (bDEBUG)
            {
            DumpHead (fpDebug, &head, pMet->pszName, i);
            DumpExtents (fpDebug, szBuff, pMet->uType);
            continue;
            }

         /*--- write data to exe ---*/
         fseek (fpExe, pMet->ulOffset + i * pMet->uSize, SEEK_SET);
         if (!fwrite (&head, sizeof (HEAD), 1, fpExe))
            Error ("could not write object: %s [%d]", pMet->pszName, i);
         if (!fwrite (szBuff, pMet->uSize - sizeof (HEAD), 1, fpExe))
            Error ("could not write object extents: %s [%d]", pMet->pszName, i);
         }
      }
   if (bDEBUG)
      fclose (fpDebug);
   }



void InsertData (PSZ pszExe, PSZ pszDat)
   {
   FILE *fpExe = NULL;

   if (!(fpExe = fopen (pszExe, "rb+")))
      Error ("Cannot open :%s", pszExe);

   if (access (pszDat, 0))
      Error ("Cannot find dat file: %s", pszDat);

   InsertObjects (fpExe, pszDat);
   fclose (fpExe);
   }


/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/


void DumpFileHeader (FILE *fp)
   {
   fprintf (fp, ";\n");
   fprintf (fp, "; STARS! data file  Generated by STARSMOD v1.0\n");
   fprintf (fp, "; works only with STARS! version 2.6d\n");
   fprintf (fp, ";\n");
   fprintf (fp, "; Modify this file with a text editor and then\n");
   fprintf (fp, "; apply the changes by typing STARSMOD /Put\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";-----------------------------------------------\n");
   fprintf (fp, ";Slot entries for ship and starbase hulls:\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";format:  SLOT_00 = y-pos, x-pos, count, attributes\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";attributes are:\n");
   fprintf (fp, ";   En = Engine     Ro = Mining Robot\n");
   fprintf (fp, ";   Sc = Scanner    Mi = Mine        \n");
   fprintf (fp, ";   Sh = Shield     Or = Orbital     \n");
   fprintf (fp, ";   Ar = Armor      Un = Unknown     \n");
   fprintf (fp, ";   Be = Beam       El = Electrical  \n");
   fprintf (fp, ";   To = Torp       Me = Mech        \n");
   fprintf (fp, ";   Bo = Bomb    \n");
   fprintf (fp, ";\n");
   fprintf (fp, ";\n");
   }


void DumpHead (FILE *fp, PHEAD phead, PSZ pszName, USHORT uIndex)
   {
   fprintf (fp, "[%s_%2.2d]\n", pszName, uIndex);

   fprintf (fp, "Name        = %s\n", phead->szName  );
   fprintf (fp, "Index       = %u\n", phead->uItem   );
   fprintf (fp, "Mass        = %u\n", phead->uMass   );
   fprintf (fp, "Resources   = %u\n", phead->uCost   );
   fprintf (fp, "Iron        = %u\n", phead->uIro    );
   fprintf (fp, "Boranium    = %u\n", phead->uBor    );
   fprintf (fp, "Germanium   = %u\n", phead->uGer    );
   fprintf (fp, "PictIdx     = %u\n", phead->uPic    );
   fprintf (fp, "Tech        = %u, %u, %u, %u, %u, %u // ener,weap,propul,constr,elect,bio\n",
               phead->cTech[0],   phead->cTech[1],   phead->cTech[2],
               phead->cTech[3],   phead->cTech[4],   phead->cTech[5]);
   }


void DumpHull (FILE *fp, PHULL phull)
   {
   USHORT i;

   fprintf (fp, "Cargo       = %u\n", phull->uCargo  );
   fprintf (fp, "Fuel        = %u\n", phull->uFuel   );
   fprintf (fp, "Armor       = %u\n", phull->uArmor  );
   fprintf (fp, "Initiative  = %u\n", phull->cInitiative);
   fprintf (fp, "unknown     = %u\n", phull->cUnknown1);
   fprintf (fp, "CargoPos    = %u, %u, %u, %u // top, left, bottom, right\n", 
                        phull->cTLCargoLoc >> 4,
                        phull->cTLCargoLoc % 16,
                        phull->cBRCargoLoc >> 4,
                        phull->cBRCargoLoc % 16);

   fprintf (fp, "SlotsUsed   = %u\n", phull->cSlotsUsed);
   for (i=0; i<16; i++)
      {
      fprintf (fp, "SLOT_%2.2d     = %d, %d, %2d, ",
               i,
               phull->cSlotLoc[i] >> 4, 
               phull->cSlotLoc[i] % 16, 
               phull->slot[i].cCount);
      PrintSlotAtts (fp, phull->slot[i].uType);
      fprintf (fp, "\n");
      }
   }


void DumpExtents (FILE *fp, PUSHORT puData, USHORT uType)
   {
   UINT i;

   switch (uType)
      {
      case OBJ_SHIP         :
      case OBJ_STARBASE     :
         DumpHull (fp, (PHULL)puData);
         break;

      case OBJ_BEAM       :
         fprintf (fp, "Range       = %u\n", puData[0]);
         fprintf (fp, "Power       = %u\n", puData[1]);
         fprintf (fp, "Initiative  = %u\n", puData[2]);
         fprintf (fp, "Flags       = %u\n", puData[3]);
         break;

      case OBJ_TORP       :
         fprintf (fp, "Range       = %u\n", puData[0]);
         fprintf (fp, "Power       = %u\n", puData[1]);
         fprintf (fp, "Initiative  = %u\n", puData[2]);
         fprintf (fp, "Accuracy    = %u\n", puData[3]);
         break;

      case OBJ_BOMB       :
         fprintf (fp, "Unknown     = %u\n", puData[0]);
         fprintf (fp, "PopKill     = %u\n", puData[1]);
         fprintf (fp, "FactKill    = %u\n", puData[2]);
         break;

      case OBJ_TERRAFORM  :
         fprintf (fp, "Terraform   = %u\n", *puData);
         break;

      case OBJ_PLANETARY:
         fprintf (fp, "Range       = %d\n", *puData);
         break;

      case OBJ_MINER      :
         fprintf (fp, "MineRate    = %u\n", *puData);
         break;

      case OBJ_MINE       :
         fprintf (fp, "Mines       = %u\n", *puData);
         break;

      case OBJ_MECH       :
      case OBJ_ELECT      :
         fprintf (fp, "Attribute   = %u\n", *puData);
         break;

      case OBJ_SHIELD     :
         fprintf (fp, "Strength    = %u\n", *puData);
         break;

      case OBJ_SCANNER    :
         fprintf (fp, "Attribute1  = %u\n", puData[0]);
         fprintf (fp, "Attribute2  = %u\n", puData[1]);
         break;

      case OBJ_ARMOR      :
         fprintf (fp, "Strength    = %u\n", *puData);
         break;

      case OBJ_ENGINE    :
         fprintf (fp, "Unknown1    = %u\n", puData[0]);
         fprintf (fp, "FuelUse     = ");
         for (i=1; i<12; i++)
            fprintf (fp, "%d%s", puData[i], (i<11 ? ", " : " // warp 0 thru 10\n"));
         fprintf (fp, "Unknown2    = %u\n", puData[12]);
         break;

      case OBJ_STARGATE   :
         fprintf (fp, "SafeMass    = %u\n", puData[0]);
         fprintf (fp, "SafeDist    = %u\n", puData[1]);
         break;

      case OBJ_MASSDRIVER :
         fprintf (fp, "Speed       = %u\n", puData[0]);
         fprintf (fp, "Unknown     = %u\n", puData[1]);
         break;
      }
   fprintf (fp, "\n");
   }


void ExtractObjects (FILE *fpExe, FILE *fpDat)
   {
   PMET  pMet;
   HEAD  head;
   UCHAR szBuff [256];
   USHORT  i;

   for (pMet=ObjData; pMet->uType; pMet++)
      {
      if (pszSECT && stricmp (pszSECT, pMet->pszName))
         continue;

      printf ("Getting %s data...\n", pMet->pszName);

      if (fseek (fpExe, pMet->ulOffset, SEEK_SET))
         Error ("Unable to seek");

      for (i=0; i< pMet->uCount; i++)
         {
         if (!fread (&head, sizeof (HEAD), 1, fpExe))
            Error ("could not read object: %s [%d]", pMet->pszName, i);
         if (!fread (szBuff, pMet->uSize - sizeof (HEAD), 1, fpExe))
            Error ("could not read object extents: %s [%d]", pMet->pszName, i);

         DumpHead (fpDat, &head, pMet->pszName, i);
         DumpExtents (fpDat, (PUSHORT)szBuff, pMet->uType);
         }
      }
   }

void ExtractData (PSZ pszExe, PSZ pszDat)
   {
   FILE *fpExe, *fpDat;

   if (!access (pszDat, 0) && !bOVER)
      Error ("File %s exists", pszDat);
   if (!(fpExe = fopen (pszExe, "rb")))
      Error ("Cannot open :%s", pszExe);
   if (!(fpDat = fopen (pszDat, "wt")))
      Error ("Cannot open : %s", pszDat);

   printf ("Extracting Data from %s to file: %s\n", pszExe, pszDat);
   DumpFileHeader (fpDat);
   ExtractObjects (fpExe, fpDat);
   fclose (fpExe);
   fclose (fpDat);
   }


/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

void Usage (void)
   {
   printf ("STARSMOD   Attribute editor for Stars 2.6d           %s  C. Fitzgerald\n\n", __DATE__);

   printf ("USAGE:  STARSMOD [options]\n\n");
   printf ("WHERE:  [options] are 0 or more of:\n");
   printf ("           /? ........... This help.\n");
   printf ("           /Get ......... Extract data from EXE to DAT file.\n");
   printf ("           /Put ......... Insert  data from DAT to EXE file.\n");
   printf ("           /Exe=file .... specify STARS! exe file.\n");
   printf ("           /Dat=file .... specify data file.\n");
   printf ("           /Overwrite ... Overwrite existing dat file.\n");
   printf ("           /Section=str . Only Get/Put this section (ex: /Sect=SHIP).\n");
   printf ("           /Debug ....... Don't actually put, produce compare file.\n");
   printf ("\n");
   printf (" To use:\n");
   printf (" > Backup your STARS!.EXE file\n");
   printf (" > type STARSMOD.EXE /get\n");
   printf (" > edit the STARSMOD.DAT file\n");
   printf (" > type STARSMOD.EXE /put\n");
   printf ("\n");
   exit (0);
   }


int _cdecl main (int argc, char *argv[])
   {
   PSZ pszExe, pszDat;

   if (ArgBuildBlk ("*^Get *^Put *^Exe% *^Dat% *^Debug "
                    "*^Overwrite *^Section% ?"))

      Error ("%s", ArgGetErr ());
   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());
   if (ArgIs ("?") || ArgIs ("Get") == ArgIs ("Put"))
      Usage ();

   pszExe  = (ArgIs ("Exe")     ? ArgGet ("Exe", 0) : "STARS!.EXE");
   pszDat  = (ArgIs ("Dat")     ? ArgGet ("Dat", 0) : "STARSMOD.DAT");
   pszSECT = (ArgIs ("Section") ? ArgGet ("Section", 0) : NULL);
   bDEBUG  = ArgIs ("Debug");
   bOVER   = ArgIs ("Overwrite");

   if (ArgIs ("Get"))
      ExtractData (pszExe, pszDat);

   if (ArgIs ("Put"))
      InsertData (pszExe, pszDat);

   printf ("Done.");
   exit (0);
   return 0;
   }

