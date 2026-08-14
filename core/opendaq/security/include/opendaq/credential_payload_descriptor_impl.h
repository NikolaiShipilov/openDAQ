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
#include <coretypes/list_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/serializable.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Common base for `ICredentialPayloadDescriptor` implementations. Stores and returns the
 * `parameters`/`description` given at construction; `getFormat` and `ISerializable` implementation are left to the
 * format-specific subclass, which is also responsible for shaping the `parameters` property object it
 * passes in.
 */
class CredentialPayloadDescriptorBaseImpl : public ImplementationOf<ICredentialPayloadDescriptor, ISerializable>
{
public:
    CredentialPayloadDescriptorBaseImpl(const PropertyObjectPtr& parameters, const StringPtr& description);

    ErrCode INTERFACE_FUNC getParameters(IPropertyObject** parameters) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;

protected:
    virtual void serializeCustomValues(ISerializer* serializer);

    PropertyObjectPtr parameters;
    StringPtr description;
};

class KeyValuePayloadDescriptorImpl final : public CredentialPayloadDescriptorBaseImpl
{
public:
    KeyValuePayloadDescriptorImpl(const ListPtr<IString>& keys, const StringPtr& description);

    ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void serializeCustomValues(ISerializer* serializer) override;

private:
    static PropertyObjectPtr BuildParameters(const ListPtr<IString>& keys);

    ListPtr<IString> keys;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(KeyValuePayloadDescriptorImpl)

class StringPayloadDescriptorImpl final : public CredentialPayloadDescriptorBaseImpl
{
public:
    explicit StringPayloadDescriptorImpl(const StringPtr& description);

    ErrCode INTERFACE_FUNC getFormat(CredentialPayloadFormat* format) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StringPayloadDescriptorImpl)

END_NAMESPACE_OPENDAQ
