#ifndef ALIITSMODULEDASSD_H
#define ALIITSMODULEDASSD_H


/* Copyright(c) 2007-2009, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */
/*                                                                        */
/* $Id$ */
///////////////////////////////////////////////////////////////////////////////
///
/// This class provides storage container ITS SSD module calibration data
/// used by DA. 
/// Date: 18/07/2008
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TArrayF.h"
#include "TArrayS.h"
#include "AliITSChannelDaSSD.h"

class AliITSNoiseSSD;
class AliITSPedestalSSD;
class AliITSBadChannelsSSD;

class AliITSModuleDaSSD : public TObject {
  public :
    AliITSModuleDaSSD();
    AliITSModuleDaSSD(const Int_t numberofstrips);
    AliITSModuleDaSSD(const Int_t numberofstrips, const Long_t eventsnumber);
    AliITSModuleDaSSD(const UChar_t ddlID, const UChar_t ad, const UChar_t adc, const UShort_t moduleID);
    AliITSModuleDaSSD(const AliITSModuleDaSSD& module);
    AliITSModuleDaSSD& operator = (const AliITSModuleDaSSD& module);
    virtual ~AliITSModuleDaSSD();
    
    UChar_t      GetDdlId()    const { return fDdlId; }
    UChar_t      GetAD()       const { return fAd; }
    UChar_t      GetADC()      const { return fAdc; }
    Short_t      GetModuleId() const { return fModuleId; }
    Int_t        GetModuleRorcEquipId()   const { return fEquipId; }
    Int_t        GetModuleRorcEquipType() const { return fEquipType; }
    Int_t        GetNumberOfStrips() const { return fNumberOfStrips; }
    Long_t       GetEventsNumber()   const { return fEventsNumber; }
    Float_t*     GetCM(const Int_t chipn)   const { return chipn < fNumberOfChips ? fCm[chipn].GetArray() : NULL; }
    Float_t      GetCM(const Int_t chipn, const Long_t evn)   const;
    TArrayF*     GetCM() const { return fCm; }
    Short_t*     GetCMFerom(const Int_t chipn)   const { return (fCmFerom && (chipn < fgkChipsPerModule)) ? fCmFerom[chipn].GetArray() : NULL; }
    Short_t      GetCMFerom(const Int_t chipn, const Long_t evn)   const;
    TArrayS*     GetCMFerom() const { return fCmFerom; }
    Int_t        GetNumberOfChips() const  { return fNumberOfChips; }
    AliITSChannelDaSSD*  GetStrip(const Int_t stripnumber)  const 
                                { return (fStrips) ? fStrips[stripnumber] : NULL; }
    Bool_t  SetEventsNumber(const Long_t eventsnumber);
    Bool_t  SetNumberOfStrips(const Int_t numberofstrips);
    Bool_t  SetNumberOfChips(const Int_t nchips);
    Bool_t  SetModuleIdData (const UChar_t ddlID, const UChar_t ad, const UChar_t adc, const Short_t moduleID);
    void    SetModuleFEEId (const UChar_t ddlID, const UChar_t ad, const UChar_t adc);
    void    SetModuleRorcId (const Int_t equipid, const Int_t equiptype);
    void    SetModuleId (const Short_t moduleID) { fModuleId = moduleID; }
    void    SetStrip(AliITSChannelDaSSD* strip, const Int_t strID) { if ((fStrips) && (strID <= fNumberOfStrips)) fStrips[strID] = strip; }
    void    SetCM (Float_t* cm, const Int_t chipn)  { if (chipn < fNumberOfChips) fCm[chipn].Set(fCm[chipn].GetSize(), cm); }
    Bool_t  SetCM (const Float_t cm, const Int_t chipn, const Int_t evn);
    void    DeleteCM () {if (fCm) { delete [] fCm; fNumberOfChips = 0; fCm = NULL; } }
    void    DeleteSignal() {if (fStrips) for (Int_t i = 0; i < fNumberOfStrips; i++) 
                                            if (fStrips[i]) fStrips[i]->DeleteSignal(); fEventsNumber = 0; }
	Bool_t  AllocateCMFeromArray(void);
    void    SetCMFerom (Short_t* cm, const Int_t chipn);
    Bool_t  SetCMFerom (const Short_t cm, const Int_t chipn, const Int_t evn);
    Bool_t  SetCMFeromEventsNumber(const Long_t eventsnumber); 
    void    DeleteCMFerom () {if (fCmFerom) { delete [] fCmFerom; fCmFerom = NULL; } }

    static Int_t GetStripsPerModuleConst() { return  fgkStripsPerModule;  }
    static Int_t GetPNStripsPerModule()    { return  fgkPNStripsPerModule;}
    static Int_t GetStripsPerChip()        { return  fgkStripsPerChip;    }
    static Int_t GetChipsPerModuleConst()  { return  fgkChipsPerModule;   }

  protected :
    static const Int_t   fgkStripsPerModule;    // Number of strips per SSD module
    static const Int_t   fgkPNStripsPerModule;  // Number of N/P strips per SSD module
    static const Int_t   fgkStripsPerChip;      // Number of strips per chip HAL25
    static const UChar_t fgkMaxAdNumber;        // MAx SSD FEROM AD number
    static const UChar_t fgkMaxAdcNumber;       // MAx SSD FEROM ADC number
    static const Int_t   fgkChipsPerModule;     // Number of HAL25 chips per SSD module

    Int_t          fEquipId;        // required to access to rorc
    Int_t          fEquipType;      // fEquipType, required to access to rorc
    UChar_t        fDdlId;          // index of DDL, ITS SSD: 33-48
    UChar_t        fAd;             // index of AD module     0-9
    UChar_t        fAdc;            // index of ADC module    0-5, 8-13
    Short_t        fModuleId;       // Module number          500-2197
    
    Int_t                 fNumberOfStrips;     // Number of AliITSChannelDaSSD* allocated
    AliITSChannelDaSSD  **fStrips;             //[fNumberOfStrips]  Array of *AliITSChannelDaSSD
    
    Int_t                 fNumberOfChips;      // Number of TArrayF objects allocated for CM   
    TArrayF              *fCm;                 //[fNumberOfChips]    CM
    TArrayS              *fCmFerom;            //                       CM calculated in FEROM

    Long_t            fEventsNumber;           // number of events for fsignal memory allocation

  private:
    Bool_t ForbiddenAdcNumber (const UChar_t adcn) const { return ((adcn == 6) || (adcn == 7)); }
 
    ClassDef(AliITSModuleDaSSD, 6) 
 
};

#endif

