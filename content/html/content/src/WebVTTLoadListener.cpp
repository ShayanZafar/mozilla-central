/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WEBVTTLoadListener.h"
#include "mozilla/dom/TextTrack.h"
#include "mozilla/dom/TextTrackCue.h"
#include "mozilla/dom/TextTrackCueList.h"

#ifdef PR_LOGGING
#warning enabling nspr logging
static PRLogModuleInfo* gTrackElementLog;
#define LOG(type, msg) PR_LOG(gTrackElementLog, type, msg)
#else
#define LOG(type, msg)
#endif

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS5(WEBVTTLoadListener, nsIRequestObserver,
                   nsIStreamListener, nsIChannelEventSink,
                   nsIInterfaceRequestor, nsIObserver)

WebVTTLoadListener(HTMLTrackElement *aElement)
  : mElement(aElement),
    mLoadID(aElement->GetCurrentLoadID())
{
  NS_ABORT_IF_FALSE(mElement, "Must pass an element to the callback");

  // TODO: Should we use webvtt_status or just check for success == 1?
  if (!webvtt_create_parser(&parsedCue, &reportError, this, &mParser))_
  {
    // TODO: Error here?
  }
}
nsresult
WebVTTLoadListener::WebVTTLoadListener_init(){
	mParser = nullptr;
	 // TODO: Should we use webvtt_status or just check for success == 1?
  if (!webvtt_create_parser(&parsedCue, &reportError, this, &mParser))_
  {

      return NS_OK;
  }else{
	  // TODO: throw some sort of error and fail safely.
	  return NS_OK;
  }
}

WebVTTLoadListener::~WebVTTLoadListener()
{
  webvtt_delete_parser(mParser);
}

void WebVTTLoadListener::parsedCue(void *userData, webvtt_cue *cue) 
{
  // TODO: Add to text track list here
  // TODO: Possibly create separate functions to create the DOM elements
  //       then this function passes those off to the HTMLTrackElement.
}

void WebVTTLoadListener::reportError(void *userData, uint32_t line, uint32_t col,
                                     webvtt_error error)
{
  // TODO: Handle error here
}

NS_IMETHODIMP
WebVTTLoadListener::Observe(nsISupports* aSubject,
                            const char *aTopic,
                            const PRUnichar* aData)
{
  nsContentUtils::UnregisterShutdownObserver(this);

  // Clear mElement to break cycle so we don't leak on shutdown
  mElement = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::OnStartRequest(nsIRequest* aRequest,
                                   nsISupports* aContext)
{
  printf("track got start request\n");
  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::OnStopRequest(nsIRequest* aRequest,
                                  nsISupports* aContext,
                                  nsresult aStatus)
{
  printf("track got stop request\n");
  nsContentUtils::UnregisterShutdownObserver(this);
  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::OnDataAvailable(nsIRequest* aRequest,
                                    nsISupports *aContext,
                                    nsIInputStream* aStream,
                                    uint64_t aOffset,
                                    uint32_t aCount)
{
  printf("Track got data! %u bytes at offset %llu\n", aCount, aOffset);

  nsresult rv;
  uint64_t available;
  bool blocking;

  rv = aStream->IsNonBlocking(&blocking);
  NS_ENSURE_SUCCESS(rv,rv);

  if (blocking)
    printf("Track data stream is non blocking\n");
  else
    printf("Track data stream is BLOCKING!\n");

  rv = aStream->Available(&available);
  NS_ENSURE_SUCCESS(rv, rv);
  printf("Track has %llu bytes available\n", available);

  char *buffer = (char *)malloc(aCount);
  if (buffer) {
    uint32_t length;

    rv = aStream->Read(buffer, aCount, &length);
    NS_ENSURE_SUCCESS(rv, rv);

    if (length >= aCount)
      length = aCount - 1;
    buffer[length] = '\0';

    printf("Track data:\n%s\n", buffer);

    // TODO: Use webvtt status or just check for true here?
    // TODO: How to determine if it is the final chunk which is the last
    //       argument to the function here.
    if (!webvtt_parse_chunk(parser, buffer, length, 0))
    {
      // TODO: Handle error
    }

    free(buffer);
  }

  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::AsyncOnChannelRedirect(nsIChannel* aOldChannel,
                                           nsIChannel* aNewChannel,
                                           uint32_t aFlags,
                                           nsIAsyncVerifyRedirectCallback* cb)
{
  return NS_OK;
}

NS_IMETHODIMP
WebVTTLoadListener::GetInterface(const nsIID &aIID,
                                 void **aResult)
{
  return QueryInterface(aIID, aResult);
}

} // namespace dom
} // namespace mozilla

