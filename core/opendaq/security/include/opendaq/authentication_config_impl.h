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
#include <coreobjects/property_object_impl.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/credential_payload_descriptor_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AuthenticationConfigImpl : public GenericPropertyObjectImpl<IAuthenticationConfig>
{
public:
    using Super = GenericPropertyObjectImpl<IAuthenticationConfig>;

    // `payloadDescriptor`/`payloadDescriptors` are raw interface pointers, not smart pointers, so that
    // these overloads stay unambiguous - each raw pointer type is an exact match for its own overload and
    // not implicitly inter-convertible with the other, whereas sibling smart-pointer parameter types would
    // be (`ObjectPtr`'s generic converting constructor accepts any interface pointer via a runtime
    // `queryInterface`).
    explicit AuthenticationConfigImpl(ICredentialPayloadDescriptor* payloadDescriptor,
                                      const StringPtr& credentialProviderId = nullptr,
                                      const PropertyObjectPtr& suppliedSecret = nullptr);

    AuthenticationConfigImpl(IList* payloadDescriptors, IString* defaultPayloadId);

    // Bare - adds no properties of its own. Used only as the empty instance that deserialization then fills
    // in from the serialized property values (mirroring `AddressInfoImpl`'s pattern) - regular user code
    // should always go through one of the two constructors above instead, both of which require an actual
    // payload descriptor.
    explicit AuthenticationConfigImpl();

    ErrCode INTERFACE_FUNC getCredentialPayloadId(IString** payloadId) override;
    ErrCode INTERFACE_FUNC getCredentialPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getCredentialProviderId(IString** providerId) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    static constexpr const char* PayloadDescriptorPropertyName = "PayloadDescriptor";
    static constexpr const char* CredentialProviderIdPropertyName = "CredentialProviderId";
    static constexpr const char* SuppliedSecretPropertyName = "SuppliedSecret";

    void initProperties(const ListPtr<ICredentialPayloadDescriptor>& payloadDescriptors,
                        const StringPtr& defaultPayloadId,
                        const StringPtr& credentialProviderId,
                        const PropertyObjectPtr& suppliedSecret);
};

END_NAMESPACE_OPENDAQ
