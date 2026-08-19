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
#include <opendaq/credential_payload.h>
#include <coretypes/impl.h>
#include <coretypes/function_ptr.h>
#include <coretypes/dictobject.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/string_ptr.h>
#include <coretypes/binarydata_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief `ICredentialPayload` impl for all formats: the callback's result is cast to `SecretInterface`
 * (wrapped as `SecretPtr`, defaulting to its usual smart pointer) and returned directly by `getSecrets`.
 * Used for `KeyValuePairs` (`IDict` wrapped as `DictPtr<IString, IString>`), `String`/`FilePath`
 * (`IString`), and `BinaryBlob` (`IBinaryData`).
 */
template <typename SecretInterface, typename SecretPtr = typename InterfaceToSmartPtr<SecretInterface>::SmartPtr>
class CredentialPayloadImpl final : public ImplementationOf<ICredentialPayload>
{
public:
    explicit CredentialPayloadImpl(const FunctionPtr& getSecretCb);

    ErrCode INTERFACE_FUNC getSecrets(IBaseObject** secrets) override;

private:
    FunctionPtr getSecretCallback;
};

using KeyValueCredentialPayloadImpl = CredentialPayloadImpl<IDict, DictPtr<IString, IString>>;
using StringCredentialPayloadImpl = CredentialPayloadImpl<IString>;
using BinaryBlobCredentialPayloadImpl = CredentialPayloadImpl<IBinaryData>;

END_NAMESPACE_OPENDAQ
