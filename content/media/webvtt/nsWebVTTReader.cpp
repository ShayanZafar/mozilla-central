/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined(nsWebVTTReader_h_)
#define nsWebVTTReader_h_

#include "nsBuiltinDecoderReader.h"
#include "nsWebVTTReader.h"

#include "webvtt/webvtt.h"

nsWebVTTReader::nsWebVTTReader(nsBuildinDecoder* aDecoder)
  : nsBuiltinDecoderReader(aDecoder)
{
  MOZ_COUNT_CTOR(nsWebVTTReader);
}

nsWebVTTReader::~nsWebVTTReader()
{
  MOZ_COUNT_DTOR(nsWebMReader);
}

nsresult
nsWebVTTReader::ReadMetadata(mozilla::VideoInfo* aInfo,
                             mozilla::MetadataTags** aTags)
{
  // TODO:
  return NS_ERROR_NOT_IMPLEMENTED;
}

#endif /* nsWebVTTReader_h_ */
