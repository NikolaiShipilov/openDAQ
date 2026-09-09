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
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief `IStruct` impl for the parameter set nested inside a `CredentialPayloadDescriptorImpl`.
 */
class CredentialPayloadDescriptorParametersImpl final : public GenericStructImpl<IStruct>
{
public:
    CredentialPayloadDescriptorParametersImpl(const StructTypePtr& structType, const DictPtr<IString, IBaseObject>& fields);

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    // One per registered dispatch key (see the .cpp).
    static ErrCode DeserializeKeyValuePairs(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
    static ErrCode DeserializeString(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
    static ErrCode DeserializeFilePath(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

/*!
 * @brief `ICredentialPayloadDescriptor` impl for all three payload formats.
 */
class CredentialPayloadDescriptorImpl final : public GenericStructImpl<ICredentialPayloadDescriptor, IStruct>
{
public:
    // `payloadClassName`, if given and registered with `typeManager`, is the `IPropertyObjectClass`
    // `createDefaultPayload()` builds the returned payload from.

    // KeyValuePairs
    CredentialPayloadDescriptorImpl(IString* id, IDict* keys, IString* description, ITypeManager* typeManager, IString* payloadClassName);
    // String
    CredentialPayloadDescriptorImpl(IString* id, IString* description, Bool hidden, ITypeManager* typeManager, IString* payloadClassName);
    // FilePath
    CredentialPayloadDescriptorImpl(IString* id, IString* description, ITypeManager* typeManager, IString* payloadClassName);

    // Builds directly from an already-resolved `structType`, bypassing the registered-type requirement above - used by `Deserialize`.
    CredentialPayloadDescriptorImpl(CredentialPayloadFormat format,
                                    const StructTypePtr& structType,
                                    const DictPtr<IString, IBaseObject>& fields,
                                    const TypeManagerPtr& typeManager,
                                    const StringPtr& payloadClassName);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) override;
    ErrCode INTERFACE_FUNC getParameters(IStruct** parameters) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createDefaultPayload(IPropertyObject** payload) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    // One per registered dispatch key
    static ErrCode DeserializeKeyValuePairs(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
    static ErrCode DeserializeString(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
    static ErrCode DeserializeFilePath(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    static DictPtr<IString, IBaseObject> BuildFields(IString* id, IDict* keys, IString* description, const StructTypePtr& parametersType);
    static DictPtr<IString, IBaseObject> BuildFields(IString* id, IString* description, Bool hidden, const StructTypePtr& parametersType);
    static DictPtr<IString, IBaseObject> BuildFields(IString* id, IString* description, const StructTypePtr& parametersType);

    CredentialPayloadFormat format;
    TypeManagerPtr typeManager;
    StringPtr payloadClassName;
};

using KeyValuePayloadDescriptorImpl = CredentialPayloadDescriptorImpl;
using StringPayloadDescriptorImpl = CredentialPayloadDescriptorImpl;
using FilePathPayloadDescriptorImpl = CredentialPayloadDescriptorImpl;

END_NAMESPACE_OPENDAQ
