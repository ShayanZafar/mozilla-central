/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_WEBVTTLoadListner_h
#define mozilla_dom_WEBVTTLoadListner_h

#include "HTMLTrackElement.h"
// Update this when build system is integrated
#include "webvtt/include/parser.h"

namespace mozilla {
namespace dom {

/** Channel Listener helper class */
class WEBVTTLoadListener MOZ_FINAL : public nsIStreamListener,
                                     public nsIChannelEventSink,
                                     public nsIInterfaceRequestor,
                                     public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR

public:
  WebVTTLoadListener(HTMLTrackElement *aElement);
  ~WebVTTLoadListener();
  NS_IMETHODIMP Observe(nsISupports* aSubject, const char *aTopic,
                        const PRUnichar* aData);
  NS_IMETHODIMP OnStartRequest(nsIRequest* aRequest, nsISupports* aContext);
  NS_IMETHODIMP OnStopRequest(nsIRequest* aRequest, nsISupports* aContext,
                              nsresult aStatus);
  NS_IMETHODIMP OnDataAvailable(nsIRequest* aRequest, nsISupports *aContext,
                                nsIInputStream* aStream, uint64_t aOffset,
                                uint32_t aCount);
  NS_IMETHODIMP AsyncOnChannelRedirect(nsIChannel* aOldChannel, 
                                       nsIChannel* aNewChannel,
                                       uint32_t aFlags,
                                       nsIAsyncVerifyRedirectCallback* cb);
  NS_IMETHODIMP GetInterface(const nsIID &aIID, void **aResult);

protected:
  void parsedCue(void *userData, webvtt_cue *cue);
  void reportError(void *userData, uint32_t line, 
                   uint32_t col, webvtt_error error);

private:
  nsRefPtr<HTMLTrackElement> mElement;
  nsCOMPtr<nsIStreamListener> mNextListener;
  uint32_t mLoadID;
  webvtt_parser mParser;
};

} // namespace dom
} // namespace mozilla
