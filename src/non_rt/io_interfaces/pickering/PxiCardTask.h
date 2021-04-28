#ifndef _PXICARDTASK_H_
#define _PXICARDTASK_H_

#include <stdio.h>

#include <Pilpxi.h>

class PxiCardTask
{
public:
  static DWORD mBit;
  static DWORD mBus;
  static DWORD mBuses[100];
  static DWORD mCardNum;
  static DWORD mData[100];
  static DWORD mDevices[100];
  static DWORD mDevice;
  static DWORD mNumInputSubunits;
  static DWORD mNumOfFreeCards;
  static DWORD mNumOutputSubunits;
  static DWORD mResistance;
  static DWORD mResistances[100];
  static DWORD mSubunit;
  static DWORD mTypeNum;
  static DWORD mRows;
  static DWORD mCols;

  static CHAR mCardId[100];

  static BOOL mState;
  static BOOL mOut;

public:
  PxiCardTask();
  void DisplayCardType(DWORD cardNum);
  void ListAllCards();
  void OpenCard(DWORD cardNum);
  void ViewAllSubunits(DWORD cardNum);
};

#endif // _PXICARDTASK_H_
