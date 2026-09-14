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
#include <opendaq/credential_descriptor_ptr.h>
#include <opendaq/context_ptr.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Builds an `AuthenticationConfig` supporting every authentication method described in
 * `credentialDescriptors`. Each entry becomes one candidate value of the resulting config's
 * `"CredentialDescriptor"` Selection property , so a caller can later switch between methods
 * by changing that property's selection.
 * @param credentialDescriptors The supported credential descriptors, keyed by their own id.
 * @param defaultAuthenticationMethodId The id of the authentication method to select by default.
 * @param context The `Context` to live-filter `"CredentialProviderId"`'s candidates from (see
 * `IAuthenticationConfig`) - must be assigned. Throws otherwise.
 * @param typeId The id of the component type this config was built for, carried through serialization so a
 * reload can re-resolve everything fresh - required, no default. Pass `nullptr` explicitly for a config with
 * no type behind it; such a config can't meaningfully round-trip through save/reload.
 */
inline AuthenticationConfigPtr AuthenticationConfig(const DictPtr<IString, ICredentialDescriptor>& credentialDescriptors,
                                                     const StringPtr& defaultAuthenticationMethodId,
                                                     const ContextPtr& context,
                                                     const StringPtr& typeId)
{
    AuthenticationConfigPtr obj(AuthenticationConfig_Create(credentialDescriptors, defaultAuthenticationMethodId, context, typeId));
    return obj;
}

END_NAMESPACE_OPENDAQ
