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
#include <opendaq/context_ptr.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Builds an `AuthenticationConfig` listing every one of `payloadDescriptors` (keyed by each
 * descriptor's own `ICredentialPayloadDescriptor::getId()`) as a candidate of its `"PayloadDescriptor"`
 * selection property. A single-method config is simply the one-entry case of this.
 * @param payloadDescriptors The supported payload descriptors, keyed by their own id.
 * @param defaultPayloadId The payload id of the descriptor to select by default.
 * @param context The `Context` to live-filter `"CredentialProviderId"`'s candidates from (see
 * `IAuthenticationConfig`) - required, no default. Pass `nullptr` explicitly for a config with no
 * `"CredentialProviderId"` selection at all.
 * @param typeId The id of the component type this config was built for, carried through serialization so a
 * reload can re-resolve everything fresh - required, no default. Pass `nullptr` explicitly for a config with
 * no type behind it; such a config can't meaningfully round-trip through save/reload.
 */
inline AuthenticationConfigPtr AuthenticationConfig(const DictPtr<IString, ICredentialPayloadDescriptor>& payloadDescriptors,
                                                     const StringPtr& defaultPayloadId,
                                                     const ContextPtr& context,
                                                     const StringPtr& typeId)
{
    AuthenticationConfigPtr obj(AuthenticationConfig_Create(payloadDescriptors, defaultPayloadId, context, typeId));
    return obj;
}

END_NAMESPACE_OPENDAQ
