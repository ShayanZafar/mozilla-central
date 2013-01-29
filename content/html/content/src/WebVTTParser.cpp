#ifdef PR_LOGGING
#warning enabling nspr logging
static PRLogModuleInfo* gTrackElementLog;
#define LOG(type, msg) PR_LOG(gTrackElementLog, type, msg)
#else
#define LOG(type, msg)
#endif

#include "WebVTTLoadListener.h"
#include "webvtt.h"

namespace mozilla{
namespace dom{

nsresult WebVTTLoadListener_init(){
	mParser = nullptr;
	mParser = webvtt_create_parser();

	if (mParser)
		return NS_OK;
	else{
		NS_ERROR("Unable to Initialize WebVTT Parser Object");
		return NS_OK;// fail peacefully
	}
}

nsresult WebVTTLoadListener_destroy(){
	webvtt_parser_destroy(aParser);

	if (mParser){
		NS_ERROR("Unable to Initialize WebVTT Parser Object");
		return NS_OK;
	}else
		return NS_OK;// fail peacefully
	
}


NS_IMETHODIMP
Observe(nsISupports* aSubject, const char *aTopic,const PRUnichar* aData){
  
  nsContentUtils::UnregisterShutdownObserver(this);

  // Clear mElement to break cycle so we don't leak on shutdown
  mElement = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
OnStartRequest(nsIRequest* aRequest,
                                               nsISupports* aContext)
{
  printf("track got start request\n");
  return NS_OK;
}

NS_IMETHODIMP
OnStopRequest(nsIRequest* aRequest,
			  nsISupports* aContext,
              nsresult aStatus)
{
  printf("track got stop request\n");
  nsContentUtils::UnregisterShutdownObserver(this);
  return NS_OK;
}

NS_IMETHODIMP
OnDataAvailable(nsIRequest* aRequest,nsISupports *aContext,
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

  char *buf = (char *)malloc(aCount);
  if (buf) {
    uint32_t read;
    rv = aStream->Read(buf, aCount, &read);
    NS_ENSURE_SUCCESS(rv, rv);
    if (read >= aCount)
      read = aCount - 1;
    buf[read] = '\0';
    printf("Track data:\n%s\n", buf);

    webvtt_parser *webvtt = webvtt_parse_new();
    NS_ENSURE_TRUE(webvtt, NS_ERROR_FAILURE);
    webvtt_cue *cue = webvtt_parse_buffer(webvtt, buf, read);

    webvtt_parse_free(webvtt);

    // poke the cues into the parent object
    nsHTMLMediaElement* parent =
      static_cast<nsHTMLMediaElement*>(mElement->mMediaParent.get());
    parent->mCues = cue;

    // Get the parent media element's frame
    nsIFrame* frame = mElement->mMediaParent->GetPrimaryFrame();
    if (frame && frame->GetType() == nsGkAtoms::HTMLVideoFrame) {
      nsIContent *overlay = static_cast<nsVideoFrame*>(frame)->GetCaptionOverlay();
      nsCOMPtr<nsIDOMHTMLElement> div = do_QueryInterface(overlay);
      div->SetInnerHTML(NS_ConvertUTF8toUTF16(cue->text));
    }

    free(buf);
  }

  return NS_OK;
}

NS_IMETHODIMP
AsyncOnChannelRedirect( nsIChannel* aOldChannel,
                        nsIChannel* aNewChannel,
                        uint32_t aFlags,
                        nsIAsyncVerifyRedirectCallback* cb)
{
  return NS_OK;
}

NS_IMETHODIMP
GetInterface(const nsIID &aIID,void **aResult)
{
  return QueryInterface(aIID, aResult);
}
}}