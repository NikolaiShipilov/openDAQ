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
#include <opendaq/credential_payload_descriptor_ptr.h>
#include <opendaq/credential_request_ptr.h>
#include <opendaq/component_type_ptr.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

namespace authentication
{
    /*
     * Verifies credentials for one of the three showcased auth methods (UserName/Password, PIN,
     * PrivateKeyFile). Shared by both the device (authenticating a connection to it) and the streaming
     * implementation (authenticating a streaming connection).
     */
    void Authenticate(const ContextPtr& ctx, const PropertyObjectPtr& credentials, const StringPtr& payloadId);

    /*
     * Builds a credential request for one of the three showcased auth methods. Shared by both the device
     * and the streaming implementation - `componentType` (the device type or the streaming type, as
     * returned by their respective `CreateType()`) is the one thing that has to come from the caller, so
     * the request's "Component type" always reflects which connection is actually being authenticated.
     */
    CredentialRequestPtr CreateCredentialRequest(const StringPtr& payloadId,
                                                 const StringPtr& connectionString,
                                                 const StringPtr& manufacturer,
                                                 const StringPtr& serialNumber,
                                                 const ComponentTypePtr& componentType);
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
