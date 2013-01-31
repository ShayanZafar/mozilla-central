/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WEBVTTLoadListener.h"
#include "TextTrack.h"
#include "TextTrackCue.h"
#include "TextTrackCueList.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS5(WebVTTLoadListener, nsIRequestObserver,
                   nsIStreamListener, nsIChannelEventSink,
                   nsIInterfaceRequestor, nsIObserver)

WebVTTLoadListener(HTMLTrackElement *aElement)
  : mElement(aElement),
    mLoadID(aElement->GetCurrentLoadID())
{
  NS_ABORT_IF_FALSE(mElement, "Must pass an element to the callback");

  // TODO: Should we use webvtt_status or just check for success == 1?
  if (!webvtt_create_parser(&parsedCue, &reportError, this, &mParser))
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

void 
WebVTTLoadListener::parsedCue(void *aUserData, webvtt_cue *aCue) 
{
  TextTrackCue domCue = cCuetoDomCue(*aCue);
  DocumentFragment documentFragment = cNodeListToDomFragment(aCue->head);

  mElement.Track.addCue(domCue);

  nsHTMLMediaElement* parent =
      static_cast<nsHTMLMediaElement*>(mElement->mMediaParent.get());

  nsIFrame* frame = mElement->mMediaParent->GetPrimaryFrame();
  if (frame && frame->GetType() == nsGkAtoms::HTMLVideoFrame) {
    nsIContent *overlay = static_cast<nsVideoFrame*>(frame)->GetCaptionOverlay();
    nsCOMPtr<nsIDOMHTMLElement> div = do_QueryInterface(overlay);

    // TODO: Not sure if this is correct yet
    div->SetInnerHTML(documentFragment);
  }
}

void 
WebVTTLoadListener::reportError(void *aUserData, uint32_t aLine, uint32_t aCol,
                                webvtt_error aError)
{
  // TODO: Handle error here
}

TextTrackCue 
WebVTTLoadListener::cCuetoDomCue(const webvtt_cue aCue)
{
  // TODO: Have to figure out what the constructor is here. aNodeInfo??
  TextTrackCue domCue(/* nsISupports *aGlobal here */);

  domeCue.Init(aCue.from, aCue.until, NS_ConvertUTF8toUTF16(aCue->id.d->text), /* ErrorResult &rv */);
  
  domCue.SetSnapToLines(aCue.snap_to_lines);
  domCue.SetSize(aCue.settings.size);
  domCue.SetPosition(aCue.settings.position);
  
  // TODO: Accept nsAString but webvtt_cue represents as an enum - have to figure this out.
  domCue.SetVertical();
  domCue.SetAlign();

  // Not specified in webvtt so we may not need this.
  domCue.SetPauseOnExit();

  return domCue;
}

DocumentFragment 
WebVTTLoadListener::cNodeListToDomFragment(const webvtt_node aNode)
{
  // TODO: Initialize document fragment here
  DocumentFragment documentFragment = nullptr;


  for (int i = 0; i < aNode.data->internal_data.length; i++)
  {
    HtmlElement htmlElement = cNodeToHtmlElement(aNode.data->internal_data.children[i]);
    documentFragment.appendChild(htmlElement);
  }

  return documentFragment;
}

HtmlElement
WebVTTLoadListener::cNodeToHtmlElement(const webvtt_node aNode)
{
  // TODO: Create an HtmlElement here. It seems like HtmlElement is just an interface
  //       and we are going to have to create one of the  concrete implementations like
  //       HtmlBodyElement or Font element depeneding on what type of node it is.
  //    
  //       Then we need to loop though all of node's children and recursively or iteratively 
  //       add it to the HtmlElement's children.
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

