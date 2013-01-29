/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HTMLTrackElement.h"
#include "mozilla/dom/HTMLTrackElementBinding.h"

#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"
#include "nsContentUtils.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsILoadGroup.h"
#include "nsIDocument.h"
#include "nsICachingChannel.h"
#include "nsHTMLMediaElement.h"
#include "nsIHttpChannel.h"
#include "nsNetUtil.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIChannelPolicy.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsIFrame.h"
#include "nsVideoFrame.h"
#include "webvtt.h"

#ifdef PR_LOGGING
#warning enabling nspr logging
static PRLogModuleInfo* gTrackElementLog;
#define LOG(type, msg) PR_LOG(gTrackElementLog, type, msg)
#else
#define LOG(type, msg)
#endif

// XXXhumph: doing this manually, since
// NS_IMPL_NS_NEW_HTML_ELEMENT(Track) assumes names with nsHTML* vs. HTML*
nsGenericHTMLElement*
NS_NewHTMLTrackElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                       mozilla::dom::FromParser aFromParser)
{
  return new mozilla::dom::HTMLTrackElement(aNodeInfo);
}

namespace mozilla {
namespace dom {

/** HTMLTrackElement */
HTMLTrackElement::HTMLTrackElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
{
#ifdef PR_LOGGING
  if (!gTrackElementLog) {
    gTrackElementLog = PR_NewLogModule("nsTrackElement");
  }
#endif

  SetIsDOMBinding();
}

HTMLTrackElement::~HTMLTrackElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLTrackElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLTrackElement, Element)

NS_INTERFACE_TABLE_HEAD(HTMLTrackElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLTrackElement,
                                   nsIDOMHTMLElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLTrackElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE(HTMLTrackElement)

JSObject*
HTMLTrackElement::WrapNode(JSContext* cx, JSObject* scope, bool* triedToWrap)
{
  return HTMLTrackElementBinding::Wrap(cx, scope, this, triedToWrap);
}

nsresult
HTMLTrackElement::SetAcceptHeader(nsIHttpChannel *aChannel)
{
#ifdef MOZ_WEBVTT
  nsCAutoString value(
      "text/webvtt"
      );

  return aChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
                                    value,
                                    false);
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

/** copied from nsHTMLMediaElement::NewURIFromString */
nsresult
HTMLTrackElement::NewURIFromString(const nsAutoString& aURISpec,
                                   nsIURI** aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  *aURI = nullptr;

  nsCOMPtr<nsIDocument> doc = OwnerDoc();

  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsresult rv = nsContentUtils::NewURIWithDocumentCharset(aURI, aURISpec,
                                                          doc, baseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool equal;
  if (aURISpec.IsEmpty() &&
      doc->GetDocumentURI() &&
      NS_SUCCEEDED(doc->GetDocumentURI()->Equals(*aURI, &equal)) &&
      equal) {
    // give up
    NS_RELEASE(*aURI);
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  return NS_OK;
}

nsresult
HTMLTrackElement::LoadResource(nsIURI* aURI)
{
  nsresult rv;

  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nullptr;
  }

  // Not sure if we need to do this.
  if (mWebVTTLoadListener) {
    mWebVTTLoadListener = nullptr;
  }

  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_MEDIA,
                                 aURI,
                                 NodePrincipal(),
                                 static_cast<nsGenericHTMLElement*>(this),
                                 EmptyCString(), // mime type
                                 nullptr, // extra
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  NS_ENSURE_SUCCESS(rv,rv);
  if (NS_CP_REJECTED(shouldLoad))
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILoadGroup> loadGroup = OwnerDoc()->GetDocumentLoadGroup();

  // check for a Content Security Policy to pass down to the channel
  // created to load the media content
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv,rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_MEDIA);
  }
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     aURI,
                     nullptr,
                     loadGroup,
                     nullptr,
                     nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY,
                     channelPolicy);
  NS_ENSURE_SUCCESS(rv,rv);

  mWebVTTLoadListener = new WebVTTLoadListener(this);
  channel->SetNotificationCallbacks(mWebVTTLoadListener);

  printf("opening webvtt channel\n");
  rv = channel->AsyncOpen(mWebVTTLoadListener, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  mChannel = channel;

  nsContentUtils::RegisterShutdownObserver(mWebVTTLoadListener);

  return NS_OK;
}

nsresult
HTMLTrackElement::BindToTree(nsIDocument *aDocument,
                             nsIContent *aParent,
                             nsIContent *aBindingParent,
                             bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument,
                                                 aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  if(NS_FAILED(rv))
    return rv;

  LOG(PR_LOG_DEBUG, ("Track Element bound to tree."));
  fprintf(stderr, "Track element bound to tree.\n");
  if (!aParent || !aParent->IsNodeOfType(nsINode::eMEDIA))
    return NS_OK;

  // Store our parent so we can look up its frame for display
  mMediaParent = getter_AddRefs(aParent);

  nsHTMLMediaElement* media = static_cast<nsHTMLMediaElement*>(aParent);
  // TODO: separate notification for 'alternate' tracks?
  media->NotifyAddedSource();
  LOG(PR_LOG_DEBUG, ("Track element sent notification to parent."));

  // Find our 'src' url
  nsAutoString src;
  nsCOMPtr<nsIURI> uri;

  if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
    nsresult rv = NewURIFromString(src, getter_AddRefs(uri));
    if (NS_SUCCEEDED(rv)) {
      LOG(PR_LOG_ALWAYS, ("%p Trying to load from src=%s", this,
             NS_ConvertUTF16toUTF8(src).get()));
      printf("%p Trying to load from src=%s\n", this,
             NS_ConvertUTF16toUTF8(src).get());
      LoadResource(uri);
    }
  }

  return NS_OK;
}

} // namespace dom
} // namespace mozilla

