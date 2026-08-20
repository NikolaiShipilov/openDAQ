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
#include <opendaq/context_ptr.h>
#include <opendaq/credential_payload_ptr.h>
#include <opendaq/credential_payload_descriptor_ptr.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

inline const std::string UserNamePasswordPayloadId = "UserNamePassword";
inline const std::string PinPayloadId = "Pin";
inline const std::string PrivateKeyFilePayloadId = "PrivateKeyFile";
inline const std::string PrivateKeyBlobPayloadId = "PrivateKeyBlob";

namespace authentication
{
    /*
     * Descriptors for the four showcased auth methods - shared between the device's and the streaming
     * type's supported authentication configs, so the descriptor shape is defined in exactly one place.
     */
    CredentialPayloadDescriptorPtr BuildUserNamePasswordDescriptor(bool hidePassword);
    CredentialPayloadDescriptorPtr BuildPinDescriptor(bool hidePin);
    CredentialPayloadDescriptorPtr BuildPrivateKeyFileDescriptor();
    CredentialPayloadDescriptorPtr BuildPrivateKeyBlobDescriptor();

    /*
     * Builds the "additional config" property object for one of the four payload ids - a
     * "VerboseCredentialRequest" bool every method gets, plus a "HidePasswordInput"/"HidePinInput" bool
     * for the two methods that have something to hide on input. Shared between the device's and the
     * streaming type's supported authentication configs (`addSupportedAuthenticationConfig`).
     */
    PropertyObjectPtr BuildAdditionalConfig(const StringPtr& payloadId);

    /*
     * Verifies credentials for one of the four showcased auth methods (UserName/Password, PIN,
     * PrivateKeyFile, PrivateKeyBlob). Shared by both the device (authenticating a connection to it) and
     * the streaming implementation (authenticating a streaming attachment), so the verification logic -
     * including the OpenSSL-based private-key challenge - lives in exactly one place.
     */
    void Authenticate(const ContextPtr& ctx, const CredentialPayloadPtr& credentials, const StringPtr& payloadId);
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
