/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

//_________________________________________________________________________
// This is a TTask that constructs SDigits out of Hits
// A Summable Digits is the sum of all hits in a pad
// 
//
//-- Author: F. Pierella
//////////////////////////////////////////////////////////////////////////////


#include "TTask.h"
#include "TTree.h"
#include "TSystem.h"
#include "TFile.h"

#include "AliTOFHitMap.h"
#include "AliTOFSDigit.h"
#include "AliTOFConstants.h"
#include "AliTOFhit.h"
#include "AliTOF.h"
#include "AliTOFv1.h"
#include "AliTOFv2.h"
#include "AliTOFv3.h"
#include "AliTOFv4.h"
#include "AliTOFSDigitizer.h"
#include "AliRun.h"
#include "AliDetector.h"
#include "AliMC.h"

#include "TFile.h"
#include "TTask.h"
#include "TTree.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TFolder.h"
#include <TF1.h>
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>

ClassImp(AliTOFSDigitizer)

//____________________________________________________________________________ 
  AliTOFSDigitizer::AliTOFSDigitizer():TTask("AliTOFSDigitizer","") 
{
  // ctor
  fNevents = 0 ;     
//  fSDigits = 0 ;
  fHits = 0 ;
  ftail    = 0;
}
           
//____________________________________________________________________________ 
  AliTOFSDigitizer::AliTOFSDigitizer(char* HeaderFile,char *SdigitsFile ):TTask("AliTOFSDigitizer","") 
{
  fNevents = 0 ;     // Number of events to digitize, 0 means all evens in current file
  ftail    = 0;

  // init parameters for sdigitization
  InitParameters();

  // add Task to //root/Tasks folder
  TTask * roottasks = (TTask*)gROOT->GetRootFolder()->FindObject("Tasks") ; 
  roottasks->Add(this) ; 
}

//____________________________________________________________________________ 
  AliTOFSDigitizer::~AliTOFSDigitizer()
{
  // dtor
  if (ftail)
    {
      delete ftail;
      ftail = 0;
    }
}

//____________________________________________________________________________ 
void AliTOFSDigitizer::InitParameters()
{
  // set parameters for detector simulation

  fTimeResolution =0.120;
  fpadefficiency  =0.99 ;
  fEdgeEffect     = 2   ;
  fEdgeTails      = 0   ;
  fHparameter     = 0.4 ;
  fH2parameter    = 0.15;
  fKparameter     = 0.5 ;
  fK2parameter    = 0.35;
  fEffCenter      = fpadefficiency;
  fEffBoundary    = 0.65;
  fEff2Boundary   = 0.90;
  fEff3Boundary   = 0.08;
  fResCenter      = 50. ;
  fResBoundary    = 70. ;
  fResSlope       = 40. ;
  fTimeWalkCenter = 0.  ;
  fTimeWalkBoundary=0.  ;
  fTimeWalkSlope  = 0.  ;
  fTimeDelayFlag  = 1   ;
  fPulseHeightSlope=2.0 ;
  fTimeDelaySlope =0.060;
  // was fMinimumCharge = TMath::Exp(fPulseHeightSlope*fKparameter/2.);
  fMinimumCharge = TMath::Exp(-fPulseHeightSlope*fHparameter);
  fChargeSmearing=0.0   ;
  fLogChargeSmearing=0.13;
  fTimeSmearing   =0.022;
  fAverageTimeFlag=0    ;

}

//__________________________________________________________________
Double_t TimeWithTail(Double_t* x, Double_t* par)
{
  // sigma - par[0], alpha - par[1], part - par[2]
  //  at x<part*sigma - gauss
  //  at x>part*sigma - TMath::Exp(-x/alpha)
  Float_t xx =x[0];
  Double_t f;
  if(xx<par[0]*par[2]) {
    f = TMath::Exp(-xx*xx/(2*par[0]*par[0]));
  } else {
    f = TMath::Exp(-(xx-par[0]*par[2])/par[1]-0.5*par[2]*par[2]);
  }
  return f;
}


//____________________________________________________________________________
void AliTOFSDigitizer::Exec(Option_t *option) { 


  AliTOF *TOF = (AliTOF *) gAlice->GetDetector ("TOF");

  if (!TOF) {
    Error("AliTOFSDigitizer","TOF not found");
    return;
  }

  if (fEdgeTails) ftail = new TF1("tail",TimeWithTail,-2,2,3);

  if (fNevents == 0)
    fNevents = (Int_t) gAlice->TreeE()->GetEntries();

  for (Int_t ievent = 0; ievent < fNevents; ievent++) {
    gAlice->GetEvent(ievent);
    TTree *TH = gAlice->TreeH ();
    if (!TH)
      return;
    if (gAlice->TreeS () == 0)
      gAlice->MakeTree ("S");

      
    //Make branches
    char branchname[20];
    sprintf (branchname, "%s", TOF->GetName ());
    //Make branch for digits
    TOF->MakeBranch ("S");
    
    //Now made SDigits from hits

    Int_t    vol[5];       // location for a digit
    Float_t  digit[2];     // TOF digit variables
    TParticle *particle;
    AliTOFhit *tofHit;
    TClonesArray *TOFhits = TOF->Hits();

    // create hit map
    AliTOFHitMap *hitMap = new AliTOFHitMap(TOF->SDigits());

    Int_t ntracks = static_cast<Int_t>(TH->GetEntries());
    for (Int_t track = 0; track < ntracks; track++)
    {
      gAlice->ResetHits();
      TH->GetEvent(track);
      particle = gAlice->Particle(track);
      Int_t nhits = TOFhits->GetEntriesFast();
      // cleaning all hits of the same track in the same pad volume
      // it is a rare event, however it happens

      Int_t previousTrack =0;
      Int_t previousSector=0;
      Int_t previousPlate =0;
      Int_t previousStrip =0;
      Int_t previousPadX  =0;
      Int_t previousPadZ  =0;

      for (Int_t hit = 0; hit < nhits; hit++)
      {
	tofHit = (AliTOFhit *) TOFhits->UncheckedAt(hit);
	Int_t tracknum = tofHit->GetTrack();
	vol[0] = tofHit->GetSector();
	vol[1] = tofHit->GetPlate();
	vol[2] = tofHit->GetStrip();
	vol[3] = tofHit->GetPadx();
	vol[4] = tofHit->GetPadz();

	Bool_t isCloneOfThePrevious=((tracknum==previousTrack) && (vol[0]==previousSector) && (vol[1]==previousPlate) && (vol[2]==previousStrip) && (vol[3]==previousPadX) && (vol[4]==previousPadZ));
	
	if(!isCloneOfThePrevious){
	  // update "previous" values
	  // in fact, we are yet in the future, so the present is past
	  previousTrack=tracknum;
	  previousSector=vol[0];
	  previousPlate=vol[1];
	  previousStrip=vol[2];
	  previousPadX=vol[3];
	  previousPadZ=vol[4];

	  // 95% of efficiency to be inserted here
	  // edge effect to be inserted here
	  // cross talk  to be inserted here
	  
	  Float_t idealtime = tofHit->GetTof(); // unit s
	  idealtime *= 1.E+12;  // conversion from s to ps
	  // fTimeRes is given usually in ps
	  Float_t tdctime   = gRandom->Gaus(idealtime, TOF->GetTimeRes());
	  digit[0] = tdctime;
	  
	  // typical Landau Distribution to be inserted here
	  // instead of Gaussian Distribution
	  Float_t idealcharge = tofHit->GetEdep();
	  Float_t adccharge = gRandom->Gaus(idealcharge, TOF->GetChrgRes());
	  digit[1] = adccharge;
	  
	  // check if two digit are on the same pad; in that case we sum
	  // the two or more digits
	  if (hitMap->TestHit(vol) != kEmpty) {
	    AliTOFSDigit *sdig = static_cast<AliTOFSDigit*>(hitMap->GetHit(vol));
	    sdig->Update(tdctime,adccharge,tracknum);
	  } else {
	    TOF->AddSDigit(tracknum, vol, digit);
	    hitMap->SetHit(vol);
	  }
	} // close if(!isCloneOfThePrevious)
      } // end loop on hits for the current track
    } // end loop on ntracks

    delete hitMap;
      
    gAlice->TreeS()->Reset();
    gAlice->TreeS()->Fill();
    gAlice->TreeS()->Write(0,TObject::kOverwrite) ;
  }				//event loop


}
 
//__________________________________________________________________
void AliTOFSDigitizer::SetSDigitsFile(char * file ){
  if(!fSDigitsFile.IsNull())
    cout << "Changing SDigits file from " <<(char *)fSDigitsFile.Data() << " to " << file << endl ;
  fSDigitsFile=file ;
}
//__________________________________________________________________
void AliTOFSDigitizer::Print(Option_t* option)const
{
  cout << "------------------- "<< GetName() << " -------------" << endl ;
  if(fSDigitsFile.IsNull())
    cout << " Writing SDigitis to file galice.root "<< endl ;
  else
    cout << "    Writing SDigitis to file  " << (char*) fSDigitsFile.Data() << endl ;

}

//__________________________________________________________________
void AliTOFSDigitizer::SimulateDetectorResponse(Float_t z0, Float_t x0, Float_t geantTime, Int_t& nActivatedPads, Int_t& nFiredPads, Bool_t* isFired, Int_t* nPlace, Float_t* qInduced, Float_t* tofTime, Float_t& averageTime)
{
  // Description:
  // Input:  z0, x0 - hit position in the strip system (0,0 - center of the strip), cm
  //         geantTime - time generated by Geant, ns
  // Output: nActivatedPads - the number of pads activated by the hit (1 || 2 || 4)
  //         nFiredPads - the number of pads fired (really activated) by the hit (nFiredPads <= nActivatedPads)
  //         qInduced[iPad]- charge induced on pad, arb. units
  //                         this array is initialized at zero by the caller
  //         tofAfterSimul[iPad] - time calculated with edge effect algorithm, ns
  //                                   this array is initialized at zero by the caller
  //         averageTime - time given by pad hited by the Geant track taking into account the times (weighted) given by the pads fired for edge effect also.
  //                       The weight is given by the qInduced[iPad]/qCenterPad
  //                                   this variable is initialized at zero by the caller
  //         nPlace[iPad] - the number of the pad place, iPad = 0, 1, 2, 3
  //                                   this variable is initialized at zero by the caller
  //
  // Description of used variables:
  //         eff[iPad] - efficiency of the pad
  //         res[iPad] - resolution of the pad, ns
  //         timeWalk[iPad] - time walk of the pad, ns
  //         timeDelay[iPad] - time delay for neighbouring pad to hited pad, ns
  //         PadId[iPad] - Pad Identifier
  //                    E | F    -->   PadId[iPad] = 5 | 6
  //                    A | B    -->   PadId[iPad] = 1 | 2
  //                    C | D    -->   PadId[iPad] = 3 | 4
  //         nTail[iPad] - the tail number, = 1 for tailA, = 2 for tailB
  //         qCenterPad - charge extimated for each pad, arb. units
  //         weightsSum - sum of weights extimated for each pad fired, arb. units
  
  const Float_t kSigmaForTail[2] = {AliTOFConstants::fgkSigmaForTail1,AliTOFConstants::fgkSigmaForTail2}; //for tail                                                   
  Int_t iz = 0, ix = 0;
  Float_t dX = 0., dZ = 0., x = 0., z = 0.;
  Float_t h = fHparameter, h2 = fH2parameter, k = fKparameter, k2 = fK2parameter;
  Float_t effX = 0., effZ = 0., resX = 0., resZ = 0., timeWalkX = 0., timeWalkZ = 0.;
  Float_t logOfqInd = 0.;
  Float_t weightsSum = 0.;
  Int_t nTail[4]  = {0,0,0,0};
  Int_t padId[4]  = {0,0,0,0};
  Float_t eff[4]  = {0.,0.,0.,0.};
  Float_t res[4]  = {0.,0.,0.,0.};
  //  Float_t qCenterPad = fMinimumCharge * fMinimumCharge;
  Float_t qCenterPad = 1.;
  Float_t timeWalk[4]  = {0.,0.,0.,0.};
  Float_t timeDelay[4] = {0.,0.,0.,0.};
  
  nActivatedPads = 0;
  nFiredPads = 0;
  
  (z0 <= 0) ? iz = 0 : iz = 1;
  dZ = z0 + (0.5 * AliTOFConstants::fgkNpadZ - iz - 0.5) * AliTOFConstants::fgkZPad; // hit position in the pad frame, (0,0) - center of the pad
  z = 0.5 * AliTOFConstants::fgkZPad - TMath::Abs(dZ);                               // variable for eff., res. and timeWalk. functions
  iz++;                                                                              // z row: 1, ..., AliTOFConstants::fgkNpadZ = 2
  ix = (Int_t)((x0 + 0.5 * AliTOFConstants::fgkNpadX * AliTOFConstants::fgkXPad) / AliTOFConstants::fgkXPad);
  dX = x0 + (0.5 * AliTOFConstants::fgkNpadX - ix - 0.5) * AliTOFConstants::fgkXPad; // hit position in the pad frame, (0,0) - center of the pad
  x = 0.5 * AliTOFConstants::fgkXPad - TMath::Abs(dX);                               // variable for eff., res. and timeWalk. functions;
  ix++;                                                                              // x row: 1, ..., AliTOFConstants::fgkNpadX = 48
  
  ////// Pad A:
  nActivatedPads++;
  nPlace[nActivatedPads-1] = (iz - 1) * AliTOFConstants::fgkNpadX + ix;
  qInduced[nActivatedPads-1] = qCenterPad;
  padId[nActivatedPads-1] = 1;
  
  if (fEdgeEffect == 0) {
    eff[nActivatedPads-1] = fEffCenter;
    if (gRandom->Rndm() < eff[nActivatedPads-1]) {
      nFiredPads = 1;
      res[nActivatedPads-1] = 0.001 * TMath::Sqrt(10400 + fResCenter * fResCenter); // 10400=30^2+20^2+40^2+50^2+50^2+50^2  ns;
      isFired[nActivatedPads-1] = kTRUE;
      tofTime[nActivatedPads-1] = gRandom->Gaus(geantTime + fTimeWalkCenter, res[0]);
      averageTime = tofTime[nActivatedPads-1];
    }
  } else {
     
    if(z < h) {
      if(z < h2) {
	effZ = fEffBoundary + (fEff2Boundary - fEffBoundary) * z / h2;
      } else {
	effZ = fEff2Boundary + (fEffCenter - fEff2Boundary) * (z - h2) / (h - h2);
      }
      resZ = fResBoundary + (fResCenter - fResBoundary) * z / h;
      timeWalkZ = fTimeWalkBoundary + (fTimeWalkCenter - fTimeWalkBoundary) * z / h;
      nTail[nActivatedPads-1] = 1;
    } else {
      effZ = fEffCenter;
      resZ = fResCenter;
      timeWalkZ = fTimeWalkCenter;
    }
    
    if(x < h) {
      if(x < h2) {
	effX = fEffBoundary + (fEff2Boundary - fEffBoundary) * x / h2;
      } else {
	effX = fEff2Boundary + (fEffCenter - fEff2Boundary) * (x - h2) / (h - h2);
      }
      resX = fResBoundary + (fResCenter - fResBoundary) * x / h;
      timeWalkX = fTimeWalkBoundary + (fTimeWalkCenter - fTimeWalkBoundary) * x / h;
      nTail[nActivatedPads-1] = 1;
    } else {
      effX = fEffCenter;
      resX = fResCenter;
      timeWalkX = fTimeWalkCenter;
    }
    
    (effZ<effX) ? eff[nActivatedPads-1] = effZ : eff[nActivatedPads-1] = effX;
    (resZ<resX) ? res[nActivatedPads-1] = 0.001 * TMath::Sqrt(10400 + resX * resX) : res[nActivatedPads-1] = 0.001 * TMath::Sqrt(10400 + resZ * resZ); // 10400=30^2+20^2+40^2+50^2+50^2+50^2  ns
    (timeWalkZ<timeWalkX) ? timeWalk[nActivatedPads-1] = 0.001 *  timeWalkZ : timeWalk[nActivatedPads-1] = 0.001 * timeWalkX; // ns


    ////// Pad B:
    if(z < k2) {
      effZ = fEffBoundary - (fEffBoundary - fEff3Boundary) * (z / k2);
    } else {
      effZ = fEff3Boundary * (k - z) / (k - k2);
    }
    resZ = fResBoundary + fResSlope * z / k;
    timeWalkZ = fTimeWalkBoundary + fTimeWalkSlope * z / k;
    
    if(z < k && z > 0) {
      if( (iz == 1 && dZ > 0) || (iz == 2 && dZ < 0) ) {
	nActivatedPads++;
	nPlace[nActivatedPads-1] = nPlace[0] + (3 - 2 * iz) * AliTOFConstants::fgkNpadX;
	eff[nActivatedPads-1] = effZ;
	res[nActivatedPads-1] = 0.001 * TMath::Sqrt(10400 + resZ * resZ); // 10400=30^2+20^2+40^2+50^2+50^2+50^2 ns 
	timeWalk[nActivatedPads-1] = 0.001 * timeWalkZ; // ns
	nTail[nActivatedPads-1] = 2;
	if (fTimeDelayFlag) {
	  //	  qInduced[0] = fMinimumCharge * TMath::Exp(fPulseHeightSlope * z / 2.);
	  //	  qInduced[nActivatedPads-1] = fMinimumCharge * TMath::Exp(-fPulseHeightSlope * z / 2.);
	  qInduced[nActivatedPads-1] = TMath::Exp(-fPulseHeightSlope * z);
	  logOfqInd = gRandom->Gaus(-fPulseHeightSlope * z, fLogChargeSmearing);
	  timeDelay[nActivatedPads-1] = gRandom->Gaus(-fTimeDelaySlope * logOfqInd, fTimeSmearing);
	} else {
	  timeDelay[nActivatedPads-1] = 0.;
	}
	padId[nActivatedPads-1] = 2;
      }
    }

    
    ////// Pad C, D, E, F:
    if(x < k2) {
      effX = fEffBoundary - (fEffBoundary - fEff3Boundary) * (x / k2);
    } else {
      effX = fEff3Boundary * (k - x) / (k - k2);
    }
    resX = fResBoundary + fResSlope*x/k;
    timeWalkX = fTimeWalkBoundary + fTimeWalkSlope*x/k;
    
    if(x < k && x > 0) {
      //   C:
      if(ix > 1 && dX < 0) {
	nActivatedPads++;
	nPlace[nActivatedPads-1] = nPlace[0] - 1;
	eff[nActivatedPads-1] = effX;
	res[nActivatedPads-1] = 0.001 * TMath::Sqrt(10400 + resX * resX); // 10400=30^2+20^2+40^2+50^2+50^2+50^2 ns 
	timeWalk[nActivatedPads-1] = 0.001 * timeWalkX; // ns
	nTail[nActivatedPads-1] = 2;
	if (fTimeDelayFlag) {
	  //	  qInduced[0] = fMinimumCharge * TMath::Exp(fPulseHeightSlope * x / 2.);
	  //	  qInduced[nActivatedPads-1] = fMinimumCharge * TMath::Exp(-fPulseHeightSlope * x / 2.);
	  qInduced[nActivatedPads-1] = TMath::Exp(-fPulseHeightSlope * x);
	  logOfqInd = gRandom->Gaus(-fPulseHeightSlope * x, fLogChargeSmearing);
	  timeDelay[nActivatedPads-1] = gRandom->Gaus(-fTimeDelaySlope * logOfqInd, fTimeSmearing);
	} else {
	  timeDelay[nActivatedPads-1] = 0.;
	}
	padId[nActivatedPads-1] = 3;

	//     D:
	if(z < k && z > 0) {
	  if( (iz == 1 && dZ > 0) || (iz == 2 && dZ < 0) ) {
	    nActivatedPads++;
	    nPlace[nActivatedPads-1] = nPlace[0] + (3 - 2 * iz) * AliTOFConstants::fgkNpadX - 1;
	    eff[nActivatedPads-1] = effX * effZ;
	    (resZ<resX) ? res[nActivatedPads-1] = 0.001 * TMath::Sqrt(10400 + resX * resX) : res[nActivatedPads-1] = 0.001 * TMath::Sqrt(10400 + resZ * resZ); // 10400=30^2+20^2+40^2+50^2+50^2+50^2 ns
	    (timeWalkZ<timeWalkX) ? timeWalk[nActivatedPads-1] = 0.001 * timeWalkZ : timeWalk[nActivatedPads-1] = 0.001 * timeWalkX; // ns
	    
	    nTail[nActivatedPads-1] = 2;
	    if (fTimeDelayFlag) {
	      if (TMath::Abs(x) < TMath::Abs(z)) {
		//		qInduced[0] = fMinimumCharge * TMath::Exp(fPulseHeightSlope * z / 2.);
		//		qInduced[nActivatedPads-1] = fMinimumCharge * TMath::Exp(-fPulseHeightSlope * z / 2.);
		qInduced[nActivatedPads-1] = TMath::Exp(-fPulseHeightSlope * z);
		logOfqInd = gRandom->Gaus(-fPulseHeightSlope * z, fLogChargeSmearing);
	      } else {
		//		qInduced[0] = fMinimumCharge * TMath::Exp(fPulseHeightSlope * x / 2.);
		//		qInduced[nActivatedPads-1] = fMinimumCharge * TMath::Exp(-fPulseHeightSlope * x / 2.);
		qInduced[nActivatedPads-1] = TMath::Exp(-fPulseHeightSlope * x);
		logOfqInd = gRandom->Gaus(-fPulseHeightSlope * x, fLogChargeSmearing);
	      }
	      timeDelay[nActivatedPads-1] = gRandom->Gaus(-fTimeDelaySlope * logOfqInd, fTimeSmearing);
	    } else {
	      timeDelay[nActivatedPads-1] = 0.;
	    }
	    padId[nActivatedPads-1] = 4;
	  }
	}  // end D
      }  // end C
      
      //   E:
      if(ix < AliTOFConstants::fgkNpadX && dX > 0) {
	nActivatedPads++;
	nPlace[nActivatedPads-1] = nPlace[0] + 1;
	eff[nActivatedPads-1] = effX;
	res[nActivatedPads-1] = 0.001 * (TMath::Sqrt(10400 + resX * resX)); // ns
	timeWalk[nActivatedPads-1] = 0.001 * timeWalkX; // ns
	nTail[nActivatedPads-1] = 2;
	if (fTimeDelayFlag) {
	  //	  qInduced[0] = fMinimumCharge * TMath::Exp(fPulseHeightSlope * x / 2.);
	  //	  qInduced[nActivatedPads-1] = fMinimumCharge * TMath::Exp(-fPulseHeightSlope * x / 2.);
	  qInduced[nActivatedPads-1] = TMath::Exp(-fPulseHeightSlope * x);
	  logOfqInd = gRandom->Gaus(-fPulseHeightSlope * x, fLogChargeSmearing);
	  timeDelay[nActivatedPads-1] = gRandom->Gaus(-fTimeDelaySlope * logOfqInd, fTimeSmearing);
	} else {
	  timeDelay[nActivatedPads-1] = 0.;
	}
	padId[nActivatedPads-1] = 5;


	//     F:
	if(z < k && z > 0) {
	  if( (iz == 1 && dZ > 0) || (iz == 2 && dZ < 0) ) {
	    nActivatedPads++;
	    nPlace[nActivatedPads - 1] = nPlace[0] + (3 - 2 * iz) * AliTOFConstants::fgkNpadX + 1;
	    eff[nActivatedPads - 1] = effX * effZ;
	    (resZ<resX) ? res[nActivatedPads-1] = 0.001 * TMath::Sqrt(10400 + resX * resX) : res[nActivatedPads-1] = 0.001 * TMath::Sqrt(10400 + resZ * resZ); // 10400=30^2+20^2+40^2+50^2+50^2+50^2 ns
	    (timeWalkZ<timeWalkX) ? timeWalk[nActivatedPads-1] = 0.001 * timeWalkZ : timeWalk[nActivatedPads-1] = 0.001*timeWalkX; // ns
	    nTail[nActivatedPads-1] = 2;
	    if (fTimeDelayFlag) {
	      if (TMath::Abs(x) < TMath::Abs(z)) {
		//		qInduced[0] = fMinimumCharge * TMath::Exp(fPulseHeightSlope * z / 2.);
		//		qInduced[nActivatedPads-1] = fMinimumCharge * TMath::Exp(-fPulseHeightSlope * z / 2.);
		qInduced[nActivatedPads-1] = TMath::Exp(-fPulseHeightSlope * z);
		logOfqInd = gRandom->Gaus(-fPulseHeightSlope * z, fLogChargeSmearing);
	      } else {
		//		qInduced[0] = fMinimumCharge * TMath::Exp(fPulseHeightSlope * x / 2.);
		//		qInduced[nActivatedPads-1] = fMinimumCharge * TMath::Exp(-fPulseHeightSlope * x / 2.);
		qInduced[nActivatedPads-1] = TMath::Exp(-fPulseHeightSlope * x);
		logOfqInd = gRandom->Gaus(-fPulseHeightSlope * x, fLogChargeSmearing);
	      }
	      timeDelay[nActivatedPads-1] = gRandom->Gaus(-fTimeDelaySlope * logOfqInd, fTimeSmearing);
	    } else {
	      timeDelay[nActivatedPads-1] = 0.;
	    }
	    padId[nActivatedPads-1] = 6;
	  }
	}  // end F
      }  // end E
    } // end if(x < k)


    for (Int_t iPad = 0; iPad < nActivatedPads; iPad++) {
      if (res[iPad] < fTimeResolution) res[iPad] = fTimeResolution;
      if(gRandom->Rndm() < eff[iPad]) {
	isFired[iPad] = kTRUE;
	nFiredPads++;
	if(fEdgeTails) {
	  if(nTail[iPad] == 0) {
	    tofTime[iPad] = gRandom->Gaus(geantTime + timeWalk[iPad] + timeDelay[iPad], res[iPad]);
	  } else {
	    ftail->SetParameters(res[iPad], 2. * res[iPad], kSigmaForTail[nTail[iPad]-1]);
	    Double_t timeAB = ftail->GetRandom();
	    tofTime[iPad] = geantTime + timeWalk[iPad] + timeDelay[iPad] + timeAB;
	  }
	} else {
	  tofTime[iPad] = gRandom->Gaus(geantTime + timeWalk[iPad] + timeDelay[iPad], res[iPad]);
	}
	if (fAverageTimeFlag) {
	  averageTime += tofTime[iPad] * qInduced[iPad];
	  weightsSum += qInduced[iPad];
	} else {
	  averageTime += tofTime[iPad];
	  weightsSum += 1.;
	}
      }
    }
    if (weightsSum!=0) averageTime /= weightsSum;
  } // end else (fEdgeEffect != 0)
}

//__________________________________________________________________
void AliTOFSDigitizer::PrintParameters()const
{
  //
  // Print parameters used for sdigitization
  //
  cout << " ------------------- "<< GetName() << " -------------" << endl ;
  cout << " Parameters used for TOF SDigitization " << endl ;
  //  Printing the parameters
  
  cout << " Number of events:                        " << fNevents << endl; 

  cout << " Time Resolution (ns) "<< fTimeResolution <<" Pad Efficiency: "<< fpadefficiency << endl;
  cout << " Edge Effect option:  "<<  fEdgeEffect<< endl;

  cout << " Boundary Effect Simulation Parameters " << endl;
  cout << " Hparameter: "<< fHparameter<<"  H2parameter:"<< fH2parameter <<"  Kparameter:"<< fKparameter<<"  K2parameter: "<< fK2parameter << endl;
  cout << " Efficiency in the central region of the pad: "<< fEffCenter << endl;
  cout << " Efficiency at the boundary region of the pad: "<< fEffBoundary << endl;
  cout << " Efficiency value at H2parameter "<< fEff2Boundary << endl;
  cout << " Efficiency value at K2parameter "<< fEff3Boundary << endl;
  cout << " Resolution (ps) in the central region of the pad: "<< fResCenter << endl;
  cout << " Resolution (ps) at the boundary of the pad      : "<< fResBoundary << endl;
  cout << " Slope (ps/K) for neighbouring pad               : "<< fResSlope <<endl;
  cout << " Time walk (ps) in the central region of the pad : "<< fTimeWalkCenter << endl;
  cout << " Time walk (ps) at the boundary of the pad       : "<< fTimeWalkBoundary<< endl;
  cout << " Slope (ps/K) for neighbouring pad               : "<< fTimeWalkSlope<<endl;
  cout << " Pulse Heigth Simulation Parameters " << endl;
  cout << " Flag for delay due to the PulseHeightEffect: "<< fTimeDelayFlag <<endl;
  cout << " Pulse Height Slope                           : "<< fPulseHeightSlope<<endl;
  cout << " Time Delay Slope                             : "<< fTimeDelaySlope<<endl;
  cout << " Minimum charge amount which could be induced : "<< fMinimumCharge<<endl;
  cout << " Smearing in charge in (q1/q2) vs x plot      : "<< fChargeSmearing<<endl;
  cout << " Smearing in log of charge ratio              : "<< fLogChargeSmearing<<endl;
  cout << " Smearing in time in time vs log(q1/q2) plot  : "<< fTimeSmearing<<endl;
  cout << " Flag for average time                        : "<< fAverageTimeFlag<<endl;
  cout << " Edge tails option                            : "<< fEdgeTails << endl;
  
}
