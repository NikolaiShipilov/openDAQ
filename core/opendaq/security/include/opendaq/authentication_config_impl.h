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
#include <opendaq/authentication_config.h>
#include <coretypes/impl.h>
#include <opendaq/credential_payload_descriptor_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AuthenticationConfigImpl : public ImplementationOf<IAuthenticationConfig>
{
public:
    AuthenticationConfigImpl(const StringPtr& payloadId, const CredentialPayloadDescriptorPtr& payloadDescriptor, const PropertyObjectPtr& config = nullptr);

    ErrCode INTERFACE_FUNC getCredentialPayloadId(IString** payloadId) override;
    ErrCode INTERFACE_FUNC getCredentialPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getConfig(IPropertyObject** config) override;

private:
    StringPtr payloadId;
    CredentialPayloadDescriptorPtr payloadDescriptor;
    PropertyObjectPtr config;
};

END_NAMESPACE_OPENDAQ
