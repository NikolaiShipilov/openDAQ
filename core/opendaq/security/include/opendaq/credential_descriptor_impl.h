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
#include <opendaq/credential_descriptor.h>
#include <coretypes/impl.h>
#include <coretypes/dict_ptr.h>
#include <coretypes/boolean_factory.h>
#include <coretypes/struct_impl.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief `IStruct` impl for the parameter set nested inside a `CredentialDescriptorImpl`.
 */
class CredentialDescriptorParametersImpl final : public GenericStructImpl<IStruct>
{
public:
    CredentialDescriptorParametersImpl(const StructTypePtr& structType, const DictPtr<IString, IBaseObject>& fields);
};

/*!
 * @brief `ICredentialDescriptor` impl for all four formats.
 */
class CredentialDescriptorImpl final : public GenericStructImpl<ICredentialDescriptor, IStruct>
{
public:
    // `secretClassName`, if given and registered with `typeManager`, is the `IPropertyObjectClass`
    // `createEmptySecret()` builds the returned secret from. `None` has none - there is no secret to build -
    // and, unlike the other three formats, its Struct type is never registered with a type manager either.

    // KeyValuePairs
    CredentialDescriptorImpl(const StringPtr& id,
                             const DictPtr<IString, IBoolean>& keys,
                             const StringPtr& description,
                             const TypeManagerPtr& typeManager,
                             const StringPtr& secretClassName);
    // String
    CredentialDescriptorImpl(const StringPtr& id,
                             const StringPtr& description,
                             Bool hidden,
                             const TypeManagerPtr& typeManager,
                             const StringPtr& secretClassName);
    // FilePath
    CredentialDescriptorImpl(const StringPtr& id,
                             const StringPtr& description,
                             const TypeManagerPtr& typeManager,
                             const StringPtr& secretClassName);
    // None
    CredentialDescriptorImpl(const StringPtr& id, const StringPtr& description);

    ErrCode INTERFACE_FUNC getAuthenticationMethodId(IString** authenticationMethodId) override;
    ErrCode INTERFACE_FUNC getFormat(CredentialFormat* format) override;
    ErrCode INTERFACE_FUNC getParameters(IStruct** parameters) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createEmptySecret(IPropertyObject** secret) override;

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    static constexpr const char* SecretClassNameSerializedKey = "SecretClassName";

    // Shared implementation constructor the four format-specific constructors above delegate to.
    CredentialDescriptorImpl(CredentialFormat format,
                             const StructTypePtr& structType,
                             const DictPtr<IString, IBaseObject>& fields,
                             const TypeManagerPtr& typeManager,
                             const StringPtr& secretClassName);

    static DictPtr<IString, IBaseObject> BuildFields(const StringPtr& id,
                                                     const DictPtr<IString, IBoolean>& keys,
                                                     const StringPtr& description,
                                                     const StructTypePtr& parametersType);
    static DictPtr<IString, IBaseObject> BuildFields(const StringPtr& id, const StringPtr& description, Bool hidden, const StructTypePtr& parametersType);
    // For a format with no format-specific parameters - "AuthenticationMethodId"/"Description" only, no "Parameters" field.
    static DictPtr<IString, IBaseObject> BuildFields(const StringPtr& id, const StringPtr& description);

    CredentialFormat format;
    TypeManagerPtr typeManager;
    StringPtr secretClassName;
};

using KeyValueDescriptorImpl = CredentialDescriptorImpl;
using StringDescriptorImpl = CredentialDescriptorImpl;
using FilePathDescriptorImpl = CredentialDescriptorImpl;
using NoneDescriptorImpl = CredentialDescriptorImpl;

END_NAMESPACE_OPENDAQ
