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
#include <opendaq/authentication_method.h>
#include <coretypes/impl.h>
#include <coretypes/dict_ptr.h>
#include <coretypes/boolean_factory.h>
#include <coretypes/struct_impl.h>
#include <coretypes/serializable.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief `IStruct` impl for the parameter set nested inside a `AuthenticationMethodImpl` - built from a
 * `StructTypePtr` that is never registered with any `ITypeManager` (construction doesn't require one; only
 * generic Struct deserialization-by-name would, which `AuthenticationMethodImpl` never delegates to for this
 * value - see its own `serialize`/`Deserialize`).
 */
class AuthenticationMethodParametersImpl final : public GenericStructImpl<IStruct>
{
public:
    AuthenticationMethodParametersImpl(const StructTypePtr& structType, const DictPtr<IString, IBaseObject>& fields);
};

/*!
 * @brief `IAuthenticationMethod` impl for all four formats. A plain object, not a Struct - no `ITypeManager`
 * involved anywhere in its construction, (de)serialization, or `createEmptyCredential()`.
 */
class AuthenticationMethodImpl final : public ImplementationOf<IAuthenticationMethod, ISerializable>
{
public:
    // KeyValuePairs - raw pointers (not StringPtr/DictPtr) since this and the FilePath constructor below
    // would otherwise collide in arity and risk ObjectPtr's generic converting-constructor ambiguity when
    // called from the factory macro's raw-pointer `new Impl(params...)`.
    AuthenticationMethodImpl(IString* id, IDict* keys, IString* description);
    // String
    AuthenticationMethodImpl(IString* id, IString* description, Bool hidden, IString* valuePropertyName);
    // FilePath
    AuthenticationMethodImpl(IString* id, IString* description, IString* valuePropertyName);
    // None
    AuthenticationMethodImpl(IString* id, IString* description);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getFormat(CredentialFormat* format) override;
    ErrCode INTERFACE_FUNC getParameters(IStruct** parameters) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createEmptyCredential(IPropertyObject** credential) override;

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    AuthenticationMethodImpl(CredentialFormat format, const StringPtr& id, const StringPtr& description, const StructPtr& parameters, const StringPtr& valuePropertyName);

    static StructPtr BuildKeyValueParameters(const DictPtr<IString, IBoolean>& keys);
    static StructPtr BuildStringParameters(Bool hidden);

    CredentialFormat format;
    StringPtr id;
    StringPtr description;
    StructPtr parameters;
    // The single property name `createEmptyCredential()` builds for `String`/`FilePath` - unused otherwise.
    StringPtr valuePropertyName;
};

using KeyValueAuthenticationMethodImpl = AuthenticationMethodImpl;
using StringAuthenticationMethodImpl = AuthenticationMethodImpl;
using FilePathAuthenticationMethodImpl = AuthenticationMethodImpl;
using NoneAuthenticationMethodImpl = AuthenticationMethodImpl;

END_NAMESPACE_OPENDAQ
