/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined(nsWebVTTDecoder_h_)
#define nsWebVTTDecoder_h_

#include "MediaDecoder.h"
#include "MediaDecoderStateMachine.h"

class nsWebVTTDecoder : public mozilla::MediaDecoder
{
public:
  virtual mozilla::MediaDecoder* Clone() { return new nsWebVTTDecoder(); }
  virtual mozilla::MediaDecoderStateMachine* CreateStateMachine();
};

#endif
