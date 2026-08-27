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
#include <opendaq/authentication_config_builder_ptr.h>
#include <opendaq/credential_payload_descriptor_ptr.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates an `AuthenticationConfig` describing the credential payload expected by an authentication method.
 * @param payloadDescriptor The descriptor of the payload the authentication method uses - its own
 * `ICredentialPayloadDescriptor::getId()` becomes the config's payload id.
 */
inline AuthenticationConfigPtr AuthenticationConfig(const CredentialPayloadDescriptorPtr& payloadDescriptor)
{
    AuthenticationConfigPtr obj(AuthenticationConfig_Create(payloadDescriptor));
    return obj;
}

/*!
 * @brief Builds a self-contained `AuthenticationConfig` listing every one of `payloadDescriptors` as a
 * candidate of its `"PayloadDescriptor"` selection property. Used by
 * `IComponentType::createDefaultAuthenticationConfig` - not meant for regular user code.
 * @param payloadDescriptors The full set of supported payload descriptors.
 * @param defaultPayloadId The payload id of the descriptor to select by default.
 */
inline AuthenticationConfigPtr AuthenticationConfigFromSupportedMethods(const ListPtr<ICredentialPayloadDescriptor>& payloadDescriptors,
                                                                        const StringPtr& defaultPayloadId)
{
    AuthenticationConfigPtr obj(AuthenticationConfigFromSupportedMethods_Create(payloadDescriptors, defaultPayloadId));
    return obj;
}

/*!
 * @brief Creates an `AuthenticationConfigBuilder` with no values set.
 */
inline AuthenticationConfigBuilderPtr AuthenticationConfigBuilder()
{
    AuthenticationConfigBuilderPtr obj(AuthenticationConfigBuilder_Create());
    return obj;
}

END_NAMESPACE_OPENDAQ
