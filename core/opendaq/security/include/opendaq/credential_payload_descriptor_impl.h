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
#include <coretypes/struct_impl.h>
#include <coretypes/serializable.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief `IStruct` impl for the parameter set nested inside a `CredentialPayloadDescriptorImpl`,
 * parameterized by `Format`. Which constructor is used - and so which fields the built Struct has -
 * depends on the format: `KeyValuePairs` takes a `"Keys"` dict field, `String` takes a `"Hidden"` bool
 * field, `FilePath` has no fields at all. Only one constructor is ever exercised per `Format` alias
 * below; the others are simply unused for that alias.
 */
template <CredentialPayloadFormat Format>
class CredentialPayloadParametersImpl final : public GenericStructImpl<IStruct>
{
public:
    // KeyValuePairs
    explicit CredentialPayloadParametersImpl(const DictPtr<IString, IBoolean>& keys);
    // String
    explicit CredentialPayloadParametersImpl(Bool hidden);
    // FilePath
    CredentialPayloadParametersImpl();

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

using KeyValuePayloadParametersImpl = CredentialPayloadParametersImpl<CredentialPayloadFormat::KeyValuePairs>;
using StringPayloadParametersImpl = CredentialPayloadParametersImpl<CredentialPayloadFormat::String>;
using FilePathPayloadParametersImpl = CredentialPayloadParametersImpl<CredentialPayloadFormat::FilePath>;

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(KeyValuePayloadParametersImpl)
OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StringPayloadParametersImpl)
OPENDAQ_REGISTER_DESERIALIZE_FACTORY(FilePathPayloadParametersImpl)

/*!
 * @brief `ICredentialPayloadDescriptor` impl for all formats, parameterized by `Format`. Which
 * constructor is used - and so which `Parameters` Struct gets built - depends on the format:
 * `KeyValuePairs` takes a `"Keys"` dict, `String` takes a `"Hidden"` bool, `FilePath` takes neither. Only
 * one constructor is ever exercised per `Format` alias below; the others are simply unused for that alias.
 */
template <CredentialPayloadFormat Format>
class CredentialPayloadDescriptorImpl final : public GenericStructImpl<ICredentialPayloadDescriptor, IStruct>
{
public:
    // KeyValuePairs
    CredentialPayloadDescriptorImpl(const DictPtr<IString, IBoolean>& keys, const StringPtr& description);
    // String
    CredentialPayloadDescriptorImpl(const StringPtr& description, Bool hidden);
    // FilePath
    explicit CredentialPayloadDescriptorImpl(const StringPtr& description);

    ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) override;
    ErrCode INTERFACE_FUNC getParameters(IStruct** parameters) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    static DictPtr<IString, IBaseObject> BuildFields(const DictPtr<IString, IBoolean>& keys, const StringPtr& description);
    static DictPtr<IString, IBaseObject> BuildFields(const StringPtr& description, Bool hidden);
    static DictPtr<IString, IBaseObject> BuildFields(const StringPtr& description);
};

using KeyValuePayloadDescriptorImpl = CredentialPayloadDescriptorImpl<CredentialPayloadFormat::KeyValuePairs>;
using StringPayloadDescriptorImpl = CredentialPayloadDescriptorImpl<CredentialPayloadFormat::String>;
using FilePathPayloadDescriptorImpl = CredentialPayloadDescriptorImpl<CredentialPayloadFormat::FilePath>;

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(KeyValuePayloadDescriptorImpl)
OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StringPayloadDescriptorImpl)
OPENDAQ_REGISTER_DESERIALIZE_FACTORY(FilePathPayloadDescriptorImpl)

END_NAMESPACE_OPENDAQ
