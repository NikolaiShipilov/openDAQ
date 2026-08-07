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
#include <coretypes/serializable.h>
#include <opendaq/credential_request.h>
#include <opendaq/credential_request_builder.h>
#include <opendaq/component_type_ptr.h>
#include <opendaq/credential_payload_descriptor_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class CredentialRequestImpl : public ImplementationOf<ICredentialRequest, ISerializable>
{
public:
    explicit CredentialRequestImpl(ICredentialRequestBuilder* credentialRequestBuilder);

    ErrCode INTERFACE_FUNC getComponentType(IComponentType** componentType) override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC getMetaData(IPropertyObject** metaData) override;
    ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) override;
    ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) override;
    ErrCode INTERFACE_FUNC getPayloadId(IString** payloadId) override;
    ErrCode INTERFACE_FUNC getPayloadDescriptor(ICredentialPayloadDescriptor** descriptor) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    explicit CredentialRequestImpl(const DictPtr<IString, IBaseObject>& packedBuilder);
    static DictPtr<IString, IBaseObject> PackBuilder(ICredentialRequestBuilder* dimensionBuilder);

    StringPtr connectionString;
    ComponentTypePtr componentType;
    PropertyObjectPtr metaData;
    StringPtr manufacturer;
    StringPtr serialNumber;
    StringPtr payloadId;
    CredentialPayloadDescriptorPtr payloadDescriptor;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(CredentialRequestImpl)

END_NAMESPACE_OPENDAQ
