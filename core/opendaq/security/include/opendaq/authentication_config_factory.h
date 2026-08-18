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
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/credential_payload_descriptor_ptr.h>
#include <opendaq/credential_request_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates an `AuthenticationConfig` describing the credential payload expected by an authentication method.
 * @param payloadId The id of the payload the authentication method requires.
 * @param payloadDescriptor The descriptor of the payload the authentication method uses.
 * @param config Additional configuration defined by the component type alongside the specific authentication method and payload.
 * In case of a null value, an empty configuration is used.
 */
inline AuthenticationConfigPtr AuthenticationConfig(const StringPtr& payloadId,
                                                    const CredentialPayloadDescriptorPtr& payloadDescriptor,
                                                    const PropertyObjectPtr& config = nullptr)
{
    AuthenticationConfigPtr obj(AuthenticationConfig_Create(payloadId, payloadDescriptor, config));
    return obj;
}

/*!
 * @brief Reconstructs an `AuthenticationConfig` from a previously formed, saved `CredentialRequest`. Used only
 * when reloading a saved device that had previously been added with authentication - not meant for regular
 * user code, which should use the `AuthenticationConfig` factory above instead.
 * @param credentialRequest The previously formed credential request to reconstruct the config from.
 */
inline AuthenticationConfigPtr AuthenticationConfigFromCredentialRequest(const CredentialRequestPtr& credentialRequest)
{
    AuthenticationConfigPtr obj(AuthenticationConfigFromCredentialRequest_Create(credentialRequest));
    return obj;
}

END_NAMESPACE_OPENDAQ
