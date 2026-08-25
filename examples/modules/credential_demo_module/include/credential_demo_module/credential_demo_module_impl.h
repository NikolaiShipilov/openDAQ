/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <credential_demo_module/common.h>
#include <opendaq/module_impl.h>
#include <opendaq/credential_payload_descriptor_ptr.h>
#include <opendaq/credential_payload_ptr.h>
#include <opendaq/credential_request_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <opendaq/streaming_ptr.h>

/*
 * Showcase module for authentication-method integration with the credential
 * framework.
 */

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

class CredentialDemoModule final : public Module
{
public:
    explicit CredentialDemoModule(const ContextPtr& context);

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;
    DevicePtr onCreateAuthenticatedDevice(const StringPtr& connectionString,
                                          const StringPtr& manufacturer,
                                          const StringPtr& serialNumber,
                                          const ComponentPtr& parent,
                                          const PropertyObjectPtr& config,
                                          const AuthenticationConfigPtr& authenticationConfig) override;

    DictPtr<IString, IStreamingType> onGetAvailableStreamingTypes() override;
    StreamingPtr onCreateStreaming(const StringPtr& connectionString,
                                   const PropertyObjectPtr& config,
                                   const AuthenticationConfigPtr& authenticationConfig,
                                   const StringPtr& manufacturer,
                                   const StringPtr& serialNumber) override;

private:
    static DictPtr<IString, IBaseObject> populateDefaultModuleOptions(const DictPtr<IString, IBaseObject>& inputOptions);
    static CredentialProviderPtr FindMatchingCredentialProvider(const DictPtr<IString, ICredentialProvider>& providers,
                                                                 const CredentialPayloadDescriptorPtr& payloadDescriptor,
                                                                 const StringPtr& providerId = nullptr);

    // Obtains the credentials for `credentialRequest`, honoring the way an authentication config can supply
    // the secret directly rather than have a provider obtain it interactively:
    // - A secret is supplied: no provider is asked to obtain anything - the secret is wrapped into the
    //   credential payload directly. If a provider id was also supplied, that specific provider is still
    //   handed the secret via `ICredentialProvider::cacheCredentials`, so it can cache it the same way it
    //   would one obtained interactively (see `CmdLineCredentialProvider`'s `FilePath` caching) - but the
    //   payload used for this connection is wrapped here regardless of whether that succeeds.
    // - No secret supplied: behaves exactly as before - the provider (auto-selected or explicitly chosen by
    //   id) is asked to obtain the secret itself via `requestCredentials`.
    static CredentialPayloadPtr ObtainCredentials(const AuthenticationConfigPtr& authenticationConfig,
                                                   const CredentialRequestPtr& credentialRequest,
                                                   const DictPtr<IString, ICredentialProvider>& providers,
                                                   const CredentialPayloadDescriptorPtr& payloadDescriptor);
};

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
