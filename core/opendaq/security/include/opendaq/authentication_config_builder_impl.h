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

#include <coretypes/impl.h>
#include <opendaq/authentication_config_builder.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/credential_payload_descriptor_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AuthenticationConfigBuilderImpl : public ImplementationOf<IAuthenticationConfigBuilder>
{
public:
    explicit AuthenticationConfigBuilderImpl();

    ErrCode INTERFACE_FUNC build(IAuthenticationConfig** authenticationConfig) override;

    ErrCode INTERFACE_FUNC setPayloadDescriptor(ICredentialPayloadDescriptor* descriptor) override;
    ErrCode INTERFACE_FUNC getPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC setCredentialProviderId(IString* providerId) override;
    ErrCode INTERFACE_FUNC getCredentialProviderId(IString** providerId) override;
    ErrCode INTERFACE_FUNC setSuppliedSecret(IPropertyObject* suppliedSecret) override;
    ErrCode INTERFACE_FUNC getSuppliedSecret(IPropertyObject** suppliedSecret) override;

private:
    CredentialPayloadDescriptorPtr payloadDescriptor;
    StringPtr credentialProviderId;
    PropertyObjectPtr suppliedSecret;
};

END_NAMESPACE_OPENDAQ
