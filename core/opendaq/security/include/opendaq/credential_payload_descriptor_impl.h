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
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief `IStruct` impl for the parameter set nested inside a `CredentialPayloadDescriptorImpl`.
 */
class CredentialPayloadDescriptorParametersImpl final : public GenericStructImpl<IStruct>
{
public:
    CredentialPayloadDescriptorParametersImpl(const StructTypePtr& structType, const DictPtr<IString, IBaseObject>& fields);
};

/*!
 * @brief `ICredentialPayloadDescriptor` impl for all four payload formats.
 */
class CredentialPayloadDescriptorImpl final : public GenericStructImpl<ICredentialPayloadDescriptor, IStruct>
{
public:
    // `payloadClassName`, if given and registered with `typeManager`, is the `IPropertyObjectClass`
    // `createDefaultPayload()` builds the returned payload from. `None` has none - there is no payload to build.

    // KeyValuePairs
    CredentialPayloadDescriptorImpl(const StringPtr& id,
                                    const DictPtr<IString, IBoolean>& keys,
                                    const StringPtr& description,
                                    const TypeManagerPtr& typeManager,
                                    const StringPtr& payloadClassName);
    // String
    CredentialPayloadDescriptorImpl(const StringPtr& id,
                                    const StringPtr& description,
                                    Bool hidden,
                                    const TypeManagerPtr& typeManager,
                                    const StringPtr& payloadClassName);
    // FilePath
    CredentialPayloadDescriptorImpl(const StringPtr& id,
                                    const StringPtr& description,
                                    const TypeManagerPtr& typeManager,
                                    const StringPtr& payloadClassName);
    // None
    CredentialPayloadDescriptorImpl(const StringPtr& id, const StringPtr& description, const TypeManagerPtr& typeManager);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) override;
    ErrCode INTERFACE_FUNC getParameters(IStruct** parameters) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createDefaultPayload(IPropertyObject** payload) override;

private:
    // Shared implementation constructor the four format-specific constructors above delegate to.
    CredentialPayloadDescriptorImpl(CredentialPayloadFormat format,
                                    const StructTypePtr& structType,
                                    const DictPtr<IString, IBaseObject>& fields,
                                    const TypeManagerPtr& typeManager,
                                    const StringPtr& payloadClassName);

    static DictPtr<IString, IBaseObject> BuildFields(const StringPtr& id,
                                                     const DictPtr<IString, IBoolean>& keys,
                                                     const StringPtr& description,
                                                     const StructTypePtr& parametersType);
    static DictPtr<IString, IBaseObject> BuildFields(const StringPtr& id, const StringPtr& description, Bool hidden, const StructTypePtr& parametersType);
    // For a format with no format-specific parameters - "Id"/"Description" only, no "Parameters" field.
    static DictPtr<IString, IBaseObject> BuildFields(const StringPtr& id, const StringPtr& description);

    CredentialPayloadFormat format;
    TypeManagerPtr typeManager;
    StringPtr payloadClassName;
};

using KeyValuePayloadDescriptorImpl = CredentialPayloadDescriptorImpl;
using StringPayloadDescriptorImpl = CredentialPayloadDescriptorImpl;
using FilePathPayloadDescriptorImpl = CredentialPayloadDescriptorImpl;
using NonePayloadDescriptorImpl = CredentialPayloadDescriptorImpl;

END_NAMESPACE_OPENDAQ
