/*
 * This class is resposible for communicating between the DOM and the WebVTT Parser 
 */
#ifndef mozilla_dom_WebVTTLoadListener_h
#define mozilla_dom_WebVTTLoadListener_h

#include "webvtt/Include/parser.h"
#include "HTMLTrackElement.h"


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
		
		nsresult WebVTTLoadListener_init();
		nsresult WebVTTLoadListener_destroy();

		NS_IMETHODIMP
		Observe(nsISupports* aSubject,
							  const char *aTopic,
							  const PRUnichar* aData);
		NS_IMETHODIMP
		OnStartRequest(nsIRequest* aRequest,nsISupports* aContext);
		NS_IMETHODIMP
		OnStopRequest(nsIRequest* aRequest,nsISupports* aContext,
													  nsresult aStatus);
		NS_IMETHODIMP OnDataAvailable(nsIRequest* aRequest,
									  nsISupports *aContext,
									  nsIInputStream* aStream,
									  uint64_t aOffset,
									  uint32_t aCount);
		NS_IMETHODIMP
		AsyncOnChannelRedirect(nsIChannel* aOldChannel,
							   nsIChannel* aNewChannel,
							   uint32_t aFlags,
							   nsIAsyncVerifyRedirectCallback* cb);
		NS_IMETHODIMP
		GetInterface(const nsIID &aIID,void **aResult);
	private:
		/* State */
		nsRefPtr<WebVTTLoadListener> mElement;
		nsCOMPtr<nsIStreamListener> mNextListener;
		uint32_t mLoadID;
		webvtt_parser_t mParser;
	};
}}
#endif