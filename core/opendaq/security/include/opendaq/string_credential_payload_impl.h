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

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Generic single-secret `ICredentialPayload`, used by `String`-format authentication methods
 * (e.g. a PIN code, a token, an API key). The secret returned by the callback is returned directly
 * by `getSecrets`, as an `IString`.
 */
class StringCredentialPayloadImpl final : public ImplementationOf<ICredentialPayload>
{
public:
    explicit StringCredentialPayloadImpl(const FunctionPtr& getSecretCb);

    ErrCode INTERFACE_FUNC getSecrets(IBaseObject** secrets) override;

private:
    FunctionPtr getSecretCallback;
};

END_NAMESPACE_OPENDAQ
