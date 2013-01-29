/*
 * This class is resposible for communicating between the DOM and the WebVTT Parser 
 */
#ifndef mozilla_dom_WebVTTLoadListener_h
#define mozilla_dom_WebVTTLoadListener_h

#include "webvtt/parser.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"

namespace mozilla{
namespace dom{
		
class WebVTTLoadListener MOZ_FINAL: public nsIStreamListener,
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
	private:
		/* State */
		nsRefPtr<WebVTTLoadListener> mElement;
		nsCOMPtr<nsIStreamListener> mNextListener;
		uint32_t mLoadID;
		webvtt_parser_t mParser;
	};

NS_IMETHODIMP
WebVTTLoadListener::Observe(nsISupports* aSubject,
                      const char *aTopic,
                      const PRUnichar* aData);
NS_IMETHODIMP
WebVTTLoadListener::OnStartRequest(nsIRequest* aRequest,nsISupports* aContext);

NS_IMETHODIMP
WebVTTLoadListener::OnStopRequest(nsIRequest* aRequest,
                                              nsISupports* aContext,
                                              nsresult aStatus);
NS_IMETHODIMP
WebVTTLoadListener::OnDataAvailable(nsIRequest* aRequest,
							  nsISupports *aContext,
							  nsIInputStream* aStream,
							  uint64_t aOffset,
							  uint32_t aCount);
NS_IMETHODIMP
WebVTTLoadListener::AsyncOnChannelRedirect(
                        nsIChannel* aOldChannel,
                        nsIChannel* aNewChannel,
                        uint32_t aFlags,
                        nsIAsyncVerifyRedirectCallback* cb);
NS_IMETHODIMP
WebVTTLoadListener::GetInterface(const nsIID &aIID,void **aResult);

}}
#endif