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
#include <opendaq/component_type_ptr.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Builds an `AuthenticationConfig` for `componentType`, out of its own
 * `IComponentType::getSupportedAuthenticationMethods()`.
 * @param componentType The device or streaming type to build the config for.
 * @param context The `Context` to live-filter `"CredentialProviderId"`'s candidates from (see
 * `IAuthenticationConfig`) - must be assigned. Throws otherwise.
 */
inline AuthenticationConfigPtr AuthenticationConfig(const ComponentTypePtr& componentType, const ContextPtr& context)
{
    AuthenticationConfigPtr obj(AuthenticationConfig_Create(componentType.getSupportedAuthenticationMethods(), context));
    return obj;
}

END_NAMESPACE_OPENDAQ
