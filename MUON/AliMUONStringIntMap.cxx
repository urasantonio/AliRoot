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

// $Id$
//
// Class AliMUONStringIntMap
// ------------------------------------ 
// Helper class that substitutes map <string, int> 
// which ALICE does not allow to use 
//
// Author: Ivana Hrivnacova, IPN Orsay

#include <Riostream.h>
#include <TObjString.h>

#include "AliMUONStringIntMap.h"
#include "AliLog.h"

ClassImp(AliMUONStringIntMap)

//______________________________________________________________________________
AliMUONStringIntMap::AliMUONStringIntMap()
 : TObject(),
   fNofItems(0),
   fFirstArray(100),
   fSecondArray(100)
{
/// Standard constructor

  fFirstArray.SetOwner(true);
}

//______________________________________________________________________________
AliMUONStringIntMap::AliMUONStringIntMap(const AliMUONStringIntMap& rhs)
  : TObject(rhs)
{
/// Protected copy constructor

  AliFatal("Copy constructor is not implemented.");
}

//______________________________________________________________________________
AliMUONStringIntMap::~AliMUONStringIntMap()
{
/// Destructor

  fFirstArray.Delete();
}

//______________________________________________________________________________
AliMUONStringIntMap& 
AliMUONStringIntMap::operator = (const AliMUONStringIntMap& rhs) 
{
/// Protected assignement operator

  // check assignement to self
  if (this == &rhs) return *this;

  AliFatal("Assignment operator is not implemented.");
    
  return *this;  
}


//
// public methods
//

//______________________________________________________________________________
Bool_t  AliMUONStringIntMap::Add(const TString& first, Int_t second)
{
/// Add map element if first not yet present

  Int_t second2 = Get(first);
  if ( second2 > 0 ) {
    AliError(Form("%s is already present in the map", first.Data()));
    return false;
  }
  
  // Resize TArrayI if needed
  if (fSecondArray.GetSize() == fNofItems) fSecondArray.Set(2*fNofItems);
  
  fFirstArray.Add(new TObjString(first)); 
  fSecondArray.AddAt(second, fNofItems);
  fNofItems++;
   
  return true;
}  

//______________________________________________________________________________
Int_t  AliMUONStringIntMap::Get(const TString& first) const
{
/// Find the element with specified key (first)
  
  for (Int_t i=0; i<fNofItems; i++) {
    if ( ((TObjString*)fFirstArray.At(i))->GetString() == first )
      return fSecondArray.At(i);
  }
  
  return 0;
}      

//______________________________________________________________________________
Int_t  AliMUONStringIntMap::GetNofItems() const
{
/// Return the number of elements

  return fNofItems;
}  

//______________________________________________________________________________
void  AliMUONStringIntMap::Clear(Option_t* /*option*/)
{
/// Delete the elements

  fNofItems = 0;
  fFirstArray.Delete();
  fSecondArray.Reset();
}  
    
//______________________________________________________________________________
void AliMUONStringIntMap::Print(const char* /*option*/) const
{
/// Print the map elements

  for (Int_t i=0; i<fNofItems; i++) {
    cout << setw(4)
         << i << "  "
         << ((TObjString*)fFirstArray.At(i))->GetString()
	 << "  "
	 << setw(5)
	 << fSecondArray.At(i)
	 << endl;
  }
}  	 

//______________________________________________________________________________
void AliMUONStringIntMap::Print(const TString& key, ofstream& out) const
{
// Prints the map elements

  for (Int_t i=0; i<fNofItems; i++) {
    out  << key << "  "
         << ((TObjString*)fFirstArray.At(i))->GetString()
	 << "  "
	 << setw(5)
	 << fSecondArray.At(i)
	 << endl;
  }
}  	 
