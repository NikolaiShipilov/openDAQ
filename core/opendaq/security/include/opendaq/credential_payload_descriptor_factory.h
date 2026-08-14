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
#include <opendaq/credential_payload_descriptor_ptr.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates a `CredentialPayloadDescriptor` describing a `KeyValuePairs`-format payload.
 * @param keys The expected key names (e.g. ["UserName", "Password"]).
 * @param description A human-readable description of the payload, for the user.
 */
inline CredentialPayloadDescriptorPtr KeyValuePayloadDescriptor(const ListPtr<IString>& keys, const StringPtr& description)
{
    CredentialPayloadDescriptorPtr obj(KeyValuePayloadDescriptor_Create(keys, description));
    return obj;
}

/*!
 * @brief Creates a `CredentialPayloadDescriptor` describing a `String`-format payload - a single secret,
 * e.g. a PIN, token, or API key.
 * @param description A human-readable description of the payload, for the user.
 */
inline CredentialPayloadDescriptorPtr StringPayloadDescriptor(const StringPtr& description)
{
    CredentialPayloadDescriptorPtr obj(StringPayloadDescriptor_Create(description));
    return obj;
}

END_NAMESPACE_OPENDAQ
