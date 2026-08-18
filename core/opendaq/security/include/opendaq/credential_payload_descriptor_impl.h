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
#include <opendaq/credential_payload_descriptor.h>
#include <coretypes/impl.h>
#include <coretypes/dict_ptr.h>
#include <coretypes/boolean_factory.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/serializable.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief `ICredentialPayloadDescriptor` impl for all formats, parameterized by `Format`. Which
 * constructor is used - and so which `parameters` get built - depends on the format: `KeyValuePairs`
 * takes a `"Keys"` dict, `String` takes a `"Hidden"` bool, `FilePath`/`BinaryBlob` take neither. Only one
 * constructor is ever exercised per `Format` alias below; the others are simply unused for that alias.
 */
template <CredentialPayloadFormat Format>
class CredentialPayloadDescriptorImpl final : public ImplementationOf<ICredentialPayloadDescriptor, ISerializable>
{
public:
    // KeyValuePairs
    CredentialPayloadDescriptorImpl(const DictPtr<IString, IBoolean>& keys, const StringPtr& description);
    // String
    CredentialPayloadDescriptorImpl(const StringPtr& description, Bool hidden);
    // FilePath, BinaryBlob
    explicit CredentialPayloadDescriptorImpl(const StringPtr& description);

    ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) override;
    ErrCode INTERFACE_FUNC getParameters(IPropertyObject** parameters) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    PropertyObjectPtr parameters;
    StringPtr description;
};

using KeyValuePayloadDescriptorImpl = CredentialPayloadDescriptorImpl<CredentialPayloadFormat::KeyValuePairs>;
using StringPayloadDescriptorImpl = CredentialPayloadDescriptorImpl<CredentialPayloadFormat::String>;
using FilePathPayloadDescriptorImpl = CredentialPayloadDescriptorImpl<CredentialPayloadFormat::FilePath>;
using BinaryBlobPayloadDescriptorImpl = CredentialPayloadDescriptorImpl<CredentialPayloadFormat::BinaryBlob>;

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(KeyValuePayloadDescriptorImpl)
OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StringPayloadDescriptorImpl)
OPENDAQ_REGISTER_DESERIALIZE_FACTORY(FilePathPayloadDescriptorImpl)
OPENDAQ_REGISTER_DESERIALIZE_FACTORY(BinaryBlobPayloadDescriptorImpl)

END_NAMESPACE_OPENDAQ
