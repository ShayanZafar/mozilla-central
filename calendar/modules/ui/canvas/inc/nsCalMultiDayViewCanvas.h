/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsCalMultiDayViewCanvas_h___
#define nsCalMultiDayViewCanvas_h___

#include "nsCalMultiViewCanvas.h"
#include "nsCalTimebarComponentCanvas.h"
#include "nsCalTimebarCanvas.h"
#include "nsDateTime.h"
#include "nsBoxLayout.h"

class nsCalMultiDayViewCanvas : public nsCalMultiViewCanvas
{

public:
  nsCalMultiDayViewCanvas(nsISupports* outer);

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init();
  NS_IMETHOD_(nsEventStatus) PaintBackground(nsIRenderingContext& aRenderingContext,
                                             const nsRect& aDirtyRect);

  NS_IMETHOD_(nsIXPFCCanvas*) AddDayViewCanvas();
  NS_IMETHOD_(PRUint32) GetNumberViewableDays();
  NS_IMETHOD SetNumberViewableDays(PRUint32 aNumberViewableDays);

  NS_IMETHOD SetParameter(nsString& aKey, nsString& aValue) ;
  NS_IMETHOD SetTimeContext(nsICalTimeContext * aContext);

  // nsIXPFCCommandReceiver methods
  NS_IMETHOD_(nsEventStatus) Action(nsIXPFCCommand * aCommand);

private:
  NS_IMETHOD SetMultiDayLayout(nsLayoutAlignment aLayoutAlignment);

protected:
  ~nsCalMultiDayViewCanvas();

private:
  PRUint32 mNumberViewableDays;
  PRUint32 mMaxRepeat;
  PRUint32 mMinRepeat;

};

#endif /* nsCalMultiDayViewCanvas_h___ */
