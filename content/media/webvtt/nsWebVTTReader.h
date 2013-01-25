/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined(nsWebVTTReader_h_)
#define nsWebVTTReader_h_

#include "MediaDecoderReader.h"
#include "MediaDecoder.h"

class nsWebVTTReader : public mozilla::MediaDecoderReader
{
public:
  nsWebVTTReader(mozilla::MediaDecoder* aDecoder);
  ~nsWebVTTReader();

  virtual nsresult Init(mozilla::MediaDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();
  virtual bool DecodeAudioData();
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip, PRInt64 aTimeThreshold);

  virtual bool HasAudio()
  {
    NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
    return false;
  }

  virtual bool HasVideo()
  {
    NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
    return false;
  }

  virtual nsresult ReadMetadata(mozilla::VideoInfo* aInfo, mozilla::MetadataTags** aTags);
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);
};

#endif /* nsWebVTTReader_h_ */
