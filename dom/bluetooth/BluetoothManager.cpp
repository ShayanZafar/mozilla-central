/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/basictypes.h"
#include "BluetoothManager.h"
#include "BluetoothCommon.h"
#include "BluetoothAdapter.h"
#include "BluetoothService.h"
#include "BluetoothTypes.h"
#include "BluetoothReplyRunnable.h"

#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"
#include "nsDOMEvent.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMDOMRequest.h"
#include "nsIJSContextStack.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "mozilla/LazyIdleThread.h"
#include "mozilla/Services.h"
#include "mozilla/Util.h"
#include "nsIDOMDOMRequest.h"

using namespace mozilla;

USING_BLUETOOTH_NAMESPACE

DOMCI_DATA(BluetoothManager, BluetoothManager)

NS_IMPL_CYCLE_COLLECTION_CLASS(BluetoothManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BluetoothManager,
                                                  nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BluetoothManager,
                                                nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothManager)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBluetoothManager)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BluetoothManager)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothManager, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothManager, nsDOMEventTargetHelper)

class GetAdapterTask : public BluetoothReplyRunnable
{
public:
  GetAdapterTask(BluetoothManager* aManager,
                 nsIDOMDOMRequest* aReq) :
    BluetoothReplyRunnable(aReq),
    mManagerPtr(aManager)
  {
  }

  bool
  ParseSuccessfulReply(jsval* aValue)
  {
    nsCOMPtr<nsIDOMBluetoothAdapter> adapter;
    *aValue = JSVAL_VOID;

    const InfallibleTArray<BluetoothNamedValue>& v =
      mReply->get_BluetoothReplySuccess().value().get_ArrayOfBluetoothNamedValue();
    adapter = BluetoothAdapter::Create(mManagerPtr->GetOwner(), v);

    nsresult rv;
    nsIScriptContext* sc = mManagerPtr->GetContextForEventHandlers(&rv);
    if (!sc) {
      NS_WARNING("Cannot create script context!");
      SetError(NS_LITERAL_STRING("BluetoothScriptContextError"));
      return false;
    }

    rv = nsContentUtils::WrapNative(sc->GetNativeContext(),
                                    sc->GetNativeGlobal(),
                                    adapter,
                                    aValue);
    bool result = NS_SUCCEEDED(rv);
    if (!result) {
      NS_WARNING("Cannot create native object!");
      SetError(NS_LITERAL_STRING("BluetoothNativeObjectError"));
    }

    return result;
  }

  void
  ReleaseMembers()
  {
    BluetoothReplyRunnable::ReleaseMembers();
    mManagerPtr = nullptr;
  }
  
private:
  nsRefPtr<BluetoothManager> mManagerPtr;
};

class ToggleBtResultTask : public nsRunnable
{
public:
  ToggleBtResultTask(BluetoothManager* aManager, bool aEnabled)
    : mManagerPtr(aManager),
      mEnabled(aEnabled)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    mManagerPtr->SetEnabledInternal(mEnabled);
    mManagerPtr->FireEnabledDisabledEvent();

    // mManagerPtr must be null before returning to prevent the background
    // thread from racing to release it during the destruction of this runnable.
    mManagerPtr = nullptr;

    return NS_OK;
  }

private:
  nsRefPtr<BluetoothManager> mManagerPtr;
  bool mEnabled;
};

nsresult
BluetoothManager::FireEnabledDisabledEvent()
{
  nsString eventName;

  if (mEnabled) {
    eventName.AssignLiteral("enabled");
  } else {
    eventName.AssignLiteral("disabled");
  }

  nsRefPtr<nsDOMEvent> event = new nsDOMEvent(nullptr, nullptr);
  nsresult rv = event->InitEvent(eventName, false, false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = event->SetTrusted(true);
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy;
  rv = DispatchEvent(event, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

BluetoothManager::BluetoothManager(nsPIDOMWindow *aWindow) :
  BluetoothPropertyContainer(BluetoothObjectType::TYPE_MANAGER),
  mEnabled(false)
{
  BindToOwner(aWindow);
  mPath.AssignLiteral("/");

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->AddObserver(this, "mozsettings-changed", false);
  }
}

BluetoothManager::~BluetoothManager()
{
  BluetoothService* bs = BluetoothService::Get();
  // We can be null on shutdown, where this might happen
  if (bs) {
    if (NS_FAILED(bs->UnregisterBluetoothSignalHandler(mPath, this))) {
      NS_WARNING("Failed to unregister object with observer!");
    }
  }

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->RemoveObserver(this, "mozsettings-changed");
  }
}

nsresult
BluetoothManager::HandleMozsettingChanged(const PRUnichar* aData)
{
  // The string that we're interested in will be a JSON string that looks like:
  //  {"key":"bluetooth.enabled","value":true}
  nsresult rv;

  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  if (NS_FAILED(rv)) {
    return NS_ERROR_UNEXPECTED;
  }

  JSContext *cx = sc->GetNativeContext();
  if (!cx) {
    return NS_OK;
  }

  // In the following [if] blocks, NS_OK will be returned even if JS_* functions
  // return false. That's because this function gets called whenever mozSettings
  // changes, so that we'll receive signals we're not interested in and it would
  // be one of the reasons for making JS_* functions return false.
  nsDependentString dataStr(aData);
  JS::Value val;
  if (!JS_ParseJSON(cx, dataStr.get(), dataStr.Length(), &val)) {
    return NS_OK;
  }

  if (!val.isObject()) {
    return NS_OK;
  }

  JSObject &obj(val.toObject());
  JS::Value key;
  if (!JS_GetProperty(cx, &obj, "key", &key)) {
    return NS_OK;
  }

  if (!key.isString()) {
    return NS_OK;
  }

  JSBool match;
  if (!JS_StringEqualsAscii(cx, key.toString(), "bluetooth.enabled", &match)) {
    return NS_OK;
  }

  if (!match) {
    return NS_OK;
  }

  JS::Value value;
  if (!JS_GetProperty(cx, &obj, "value", &value)) {
    return NS_OK;
  }

  if (!value.isBoolean()) {
    return NS_OK;
  }

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return NS_ERROR_FAILURE;
  }

  bool enabled = value.toBoolean();
  bool isEnabled = (bs->IsEnabledInternal() > 0);
  if (!isEnabled && enabled) {
    if (NS_FAILED(bs->RegisterBluetoothSignalHandler(NS_LITERAL_STRING("/"), this))) {
      NS_ERROR("Failed to register object with observer!");
      return NS_ERROR_FAILURE;
    }
  } else if (isEnabled && !enabled){
    if (NS_FAILED(bs->UnregisterBluetoothSignalHandler(NS_LITERAL_STRING("/"), this))) {
      NS_WARNING("Failed to unregister object with observer!");
    }
  } else {
    return NS_OK;
  }

  nsCOMPtr<nsIRunnable> resultTask = new ToggleBtResultTask(this, enabled);

  if (enabled) {
    if (NS_FAILED(bs->Start(resultTask))) {
      return NS_ERROR_FAILURE;
    }
  } else {
    if (NS_FAILED(bs->Stop(resultTask))) {
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
BluetoothManager::Observe(nsISupports* aSubject,
                          const char* aTopic,
                          const PRUnichar* aData)
{
  nsresult rv = NS_OK;

  if (!strcmp("mozsettings-changed", aTopic)) {
    rv = HandleMozsettingChanged(aData);
  }

  return rv;
}

void
BluetoothManager::SetPropertyByValue(const BluetoothNamedValue& aValue)
{
#ifdef DEBUG
    const nsString& name = aValue.name();
    nsCString warningMsg;
    warningMsg.AssignLiteral("Not handling manager property: ");
    warningMsg.Append(NS_ConvertUTF16toUTF8(name));
    NS_WARNING(warningMsg.get());
#endif
}

NS_IMETHODIMP
BluetoothManager::GetEnabled(bool* aEnabled)
{
  *aEnabled = mEnabled;
  return NS_OK;
}

NS_IMETHODIMP
BluetoothManager::GetDefaultAdapter(nsIDOMDOMRequest** aAdapter)
{
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return NS_ERROR_FAILURE;
  }
  
  nsCOMPtr<nsIDOMRequestService> rs = do_GetService("@mozilla.org/dom/dom-request-service;1");

  if (!rs) {
    NS_WARNING("No DOMRequest Service!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMDOMRequest> request;
  nsresult rv = rs->CreateRequest(GetOwner(), getter_AddRefs(request));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create DOMRequest!");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<BluetoothReplyRunnable> results = new GetAdapterTask(this, request);

  if (NS_FAILED(bs->GetDefaultAdapterPathInternal(results))) {
    return NS_ERROR_FAILURE;
  }
  request.forget(aAdapter);
  return NS_OK;
}

// static
already_AddRefed<BluetoothManager>
BluetoothManager::Create(nsPIDOMWindow* aWindow) {
  nsRefPtr<BluetoothManager> manager = new BluetoothManager(aWindow);
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return nullptr;
  }

  bool isEnabled = (bs->IsEnabledInternal() > 0);
  if (isEnabled) {
    if (NS_FAILED(bs->RegisterBluetoothSignalHandler(NS_LITERAL_STRING("/"), manager))) {
      NS_ERROR("Failed to register object with observer!");
      return nullptr;
    }
  }
  
  return manager.forget();
}

nsresult
NS_NewBluetoothManager(nsPIDOMWindow* aWindow,
                       nsIDOMBluetoothManager** aBluetoothManager)
{
  NS_ASSERTION(aWindow, "Null pointer!");

  nsPIDOMWindow* innerWindow = aWindow->IsInnerWindow() ?
    aWindow :
    aWindow->GetCurrentInnerWindow();

  // Need the document for security check.
  nsCOMPtr<nsIDocument> document = innerWindow->GetExtantDoc();
  NS_ENSURE_TRUE(document, NS_NOINTERFACE);

  nsCOMPtr<nsIPrincipal> principal = document->NodePrincipal();
  NS_ENSURE_TRUE(principal, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, NS_ERROR_UNEXPECTED);

  uint32_t permission;
  nsresult rv =
    permMgr->TestPermissionFromPrincipal(principal, "mozBluetooth", &permission);
  NS_ENSURE_SUCCESS(rv, rv);

  if (permission != nsIPermissionManager::ALLOW_ACTION) {
    *aBluetoothManager = nullptr;
    return NS_OK;
  }

  nsRefPtr<BluetoothManager> bluetoothManager = BluetoothManager::Create(aWindow);
  if (!bluetoothManager) {
    NS_ERROR("Cannot create bluetooth manager!");
    return NS_ERROR_FAILURE;
  }

  bluetoothManager.forget(aBluetoothManager);
  return NS_OK;
}

void
BluetoothManager::Notify(const BluetoothSignal& aData)
{
  if (aData.name().EqualsLiteral("AdapterAdded")) {
    nsRefPtr<nsDOMEvent> event = new nsDOMEvent(nullptr, nullptr);
    nsresult rv = event->InitEvent(NS_LITERAL_STRING("adapteradded"), false, false);

    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to init the adapteradded event!!!");
      return;
    }

    event->SetTrusted(true);
    bool dummy;
    DispatchEvent(event, &dummy);
  } else {
#ifdef DEBUG
    nsCString warningMsg;
    warningMsg.AssignLiteral("Not handling manager signal: ");
    warningMsg.Append(NS_ConvertUTF16toUTF8(aData.name()));
    NS_WARNING(warningMsg.get());
#endif
  }
}

NS_IMPL_EVENT_HANDLER(BluetoothManager, enabled)
NS_IMPL_EVENT_HANDLER(BluetoothManager, disabled)
NS_IMPL_EVENT_HANDLER(BluetoothManager, adapteradded)
