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
#include <credential_demo_module/common.h>
#include <opendaq/module_impl.h>

/*
 * Showcase module for authentication-method integration with the credential
 * framework (ICredentialProvider / ICredentialRequest / ICredentialPayload).
 * This first step only registers a single device with no authentication;
 * credential-provider wiring is added in a later step.
 */

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

class CredentialDemoModule final : public Module
{
public:
    explicit CredentialDemoModule(const ContextPtr& context);

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;

private:
    WeakRefPtr<IDevice> device;
    std::mutex sync;

    void clearRemovedDevice();
};

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
