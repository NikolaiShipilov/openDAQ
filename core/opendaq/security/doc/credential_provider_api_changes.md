# Credential Provider Framework — API Changes (Review Draft)

Checklist format for walkthrough review. Each item: ✅ approve / ✏️ edit / ❌ remove, or add new ones inline.

Every item is tagged **[NEW]** (a brand-new interface/enum/method/factory) or **[CHANGED]** (an existing method/factory whose signature changed). A member added to a pre-existing interface is still tagged **[NEW]** — the interface existed before, the member didn't.

---

## New Enum

### `CredentialPayloadFormat` — **[NEW]**
- [ ] `KeyValuePairs` — N string pairs (e.g. UserName / Password)
- [ ] `String` — one string (token, API key, PIN)
- [ ] `FilePath` — one string, path to a file containing the secret
- [ ] `BinaryBlob` — one raw byte buffer (pointer + size)

---

## New Interfaces

### `ICredentialPayloadDescriptor` — **[NEW]**
- [ ] `getFormat(CredentialPayloadFormat* format)`
  - `format` — out: the payload's format
- [ ] `getParameters(IPropertyObject** parameters)`
  - `parameters` — out: format-specific parameter object (e.g. `"Keys"` dict for `KeyValuePairs`, `"Hidden"` bool for `String`)
- [ ] `getDescription(IString** description)`
  - `description` — out: human-readable description of the payload
- [ ] Factory: `KeyValuePayloadDescriptor(IDict<IString, Bool>* keys, IString* description)`
  - `keys` — in: expected key names, each mapped to whether input should be masked
  - `description` — in: human-readable description
- [ ] Factory: `StringPayloadDescriptor(IString* description, Bool hidden = True)`
  - `description` — in: human-readable description
  - `hidden` — in, default `True`: whether input should be masked when entered
- [ ] Factory: `FilePathPayloadDescriptor(IString* description)`
  - `description` — in: human-readable description
- [ ] Factory: `BinaryBlobPayloadDescriptor(IString* description)`
  - `description` — in: human-readable description

### `IAuthenticationConfig` — **[NEW]**
- [ ] `getCredentialPayloadId(IString** payloadId)`
  - `payloadId` — out: id of the payload for the selected authentication method
- [ ] `getCredentialPayloadDescriptor(ICredentialPayloadDescriptor** payloadDescriptor)`
  - `payloadDescriptor` — out: descriptor of the payload the selected method uses
- [ ] `getConfig(IPropertyObject** config)`
  - `config` — out: additional method-specific settings, for this attempt only
- [ ] `getCredentialProviderId(IString** providerId)`
  - `providerId` — out: explicitly selected provider's id, or `nullptr` if auto-selection applies
- [ ] `getSuppliedSecret(IBaseObject** suppliedSecret)`
  - `suppliedSecret` — out: caller-supplied secret bypassing a provider, or `nullptr` if none was supplied
- [ ] `getStreamingAuthenticationConfigs(IDict<IString, IAuthenticationConfig>** configs)`
  - `configs` — out: nested authentication configs, keyed by streaming type id
- [ ] Factory: `AuthenticationConfig(IString* payloadId, ICredentialPayloadDescriptor* payloadDescriptor, IPropertyObject* config = nullptr)`
  - `payloadId` — in: id of the payload for the selected method
  - `payloadDescriptor` — in: descriptor of the payload the selected method uses
  - `config` — in, optional: additional method-specific settings
- [ ] Factory: `AuthenticationConfigFromCredentialRequest(ICredentialRequest* credentialRequest)` — hidden, load-path only
  - `credentialRequest` — in: previously saved request to reconstruct the config from
- [ ] Factory: `AuthenticationConfigBuilder()` — no parameters

### `IAuthenticationConfigBuilder` — **[NEW]**
- [ ] `build(IAuthenticationConfig** authenticationConfig)`
  - `authenticationConfig` — out: built config from the currently set values
- [ ] `setPayloadId(IString* payloadId)` / `getPayloadId(IString** payloadId)`
  - `payloadId` — in/out: id of the payload for the selected method
- [ ] `setPayloadDescriptor(ICredentialPayloadDescriptor* payloadDescriptor)` / `getPayloadDescriptor(ICredentialPayloadDescriptor** payloadDescriptor)`
  - `payloadDescriptor` — in/out: descriptor of the payload the selected method uses
- [ ] `setConfig(IPropertyObject* config)` / `getConfig(IPropertyObject** config)`
  - `config` — in/out: additional method-specific settings
- [ ] `setCredentialProviderId(IString* providerId)` / `getCredentialProviderId(IString** providerId)`
  - `providerId` — in/out: id of a specific provider to use, bypassing auto-selection
- [ ] `setSuppliedSecret(IBaseObject* suppliedSecret)` / `getSuppliedSecret(IBaseObject** suppliedSecret)`
  - `suppliedSecret` — in/out: secret supplied directly, in the format the payload descriptor expects
- [ ] `addStreamingAuthenticationConfig(IStreamingType* streamingType, IAuthenticationConfig* streamingAuthenticationConfig)`
  - `streamingType` — in: streaming type this nested config applies to (keyed internally by its id)
  - `streamingAuthenticationConfig` — in: the nested config itself
- [ ] `getStreamingAuthenticationConfigs(IDict<IString, IAuthenticationConfig>** configs)`
  - `configs` — out: nested configs accumulated so far, keyed by streaming type id
- [ ] Factory: `AuthenticationConfigBuilder()` — no parameters

### `IAuthenticationConfigPrivate` — **[NEW]**
- [ ] `getCredentialRequest(ICredentialRequest** credentialRequest)`
  - `credentialRequest` — out: the request this config was reconstructed from, or `nullptr` for a live (non-reload) attempt

### `ICredentialRequest` — **[NEW]**
- [ ] `getComponentType(IComponentType** componentType)`
  - `componentType` — out: type of component the request is for
- [ ] `getConnectionString(IString** connectionString)`
  - `connectionString` — out: connection string used for this attempt
- [ ] `getMetaData(IPropertyObject** metaData)`
  - `metaData` — out: additional info for the provider to present to the user
- [ ] `getManufacturer(IString** manufacturer)`
  - `manufacturer` — out: manufacturer of the device the request is for
- [ ] `getSerialNumber(IString** serialNumber)`
  - `serialNumber` — out: serial number of the device the request is for
- [ ] `getPayloadId(IString** payloadId)`
  - `payloadId` — out: id of the negotiated payload
- [ ] `getPayloadDescriptor(ICredentialPayloadDescriptor** payloadDescriptor)`
  - `payloadDescriptor` — out: descriptor of the payload the provider must supply
- [ ] Factory: `CredentialRequestFromBuilder(ICredentialRequestBuilder* builder)` — hidden
  - `builder` — in: builder to construct the request from

### `ICredentialRequestBuilder` — **[NEW]**
- [ ] `build(ICredentialRequest** credentialRequest)`
  - `credentialRequest` — out: built request from the currently set values
- [ ] `setComponentType(IComponentType* componentType)` / `getComponentType(IComponentType** componentType)`
  - `componentType` — in/out: type of component the request is being built for
- [ ] `setConnectionString(IString* connectionString)` / `getConnectionString(IString** connectionString)`
  - `connectionString` — in/out: connection string for this attempt
- [ ] `setManufacturer(IString* manufacturer)` / `getManufacturer(IString** manufacturer)`
  - `manufacturer` — in/out: device manufacturer
- [ ] `setSerialNumber(IString* serialNumber)` / `getSerialNumber(IString** serialNumber)`
  - `serialNumber` — in/out: device serial number
- [ ] `addMetaDataProperty(IProperty* property)`
  - `property` — in: one metadata property for the provider to present to the user
- [ ] `getMetaData(IPropertyObject** metaData)`
  - `metaData` — out: accumulated metadata property object
- [ ] `setPayloadId(IString* payloadId)` / `getPayloadId(IString** payloadId)`
  - `payloadId` — in/out: id of the negotiated payload
- [ ] `setPayloadDescriptor(ICredentialPayloadDescriptor* payloadDescriptor)` / `getPayloadDescriptor(ICredentialPayloadDescriptor** payloadDescriptor)`
  - `payloadDescriptor` — in/out: descriptor of the payload the provider must supply
- [ ] Factory: `CredentialRequestBuilder()` — no parameters

### `ICredentialPayload` — **[NEW]**
- [ ] `getSecrets(IBaseObject** secrets)`
  - `secrets` — out: the secret(s) carried by the payload; concrete type depends on payload format
- [ ] Factory: `KeyValueCredentialPayload(IFunction* getValuesCb)`
  - `getValuesCb` — in: callback returning the key/value secrets
- [ ] Factory: `StringCredentialPayload(IFunction* getSecretCb)` — also used for `FilePath`-format payloads
  - `getSecretCb` — in: callback returning the single secret string
- [ ] Factory: `BinaryBlobCredentialPayload(IFunction* getBlobCb)`
  - `getBlobCb` — in: callback returning the raw byte buffer

### `ICredentialProvider` — **[NEW]**
- [ ] `getName(IString** name)`
  - `name` — out: the provider's name
- [ ] `requestCredentials(ICredentialRequest* request, ICredentialPayload** payload)`
  - `request` — in: identifies what's being authenticated and what's needed
  - `payload` — out: the obtained credentials
- [ ] `cacheCredentials(ICredentialRequest* request, IBaseObject* secret)`
  - `request` — in: identifies the context the secret applies to
  - `secret` — in: secret already known in advance, to be cached as if obtained interactively
- [ ] `getSupportedPayloadFormats(IList<CredentialPayloadFormat>** formats)`
  - `formats` — out: payload formats this provider can supply
- [ ] Factory: `CmdLineCredentialProvider()` — no parameters
- [ ] Factory: `FileCredentialProvider()` — no parameters

---

## Extensions to Existing Interfaces

### `IComponentType`
- [ ] **[NEW]** `createDefaultAuthenticationConfig(IAuthenticationConfig** authenticationConfig)`
  - `authenticationConfig` — out: clone of the type's default authentication config
- [ ] **[NEW]** `getSupportedAuthenticationConfigs(IDict<IString, IAuthenticationConfig>** configs)`
  - `configs` — out: configs supported by this type, keyed by payload id
- [ ] **[NEW]** `isAuthenticationSupported(Bool* supported)`
  - `supported` — out: whether this type supports authentication at all

### `IComponentTypeBuilder`
- [ ] **[NEW]** `setDefaultAuthenticationConfigId(IString* id)`
  - `id` — in: id of the added config to mark as default
- [ ] **[NEW]** `getDefaultAuthenticationConfigId(IString** id)`
  - `id` — out: the default id set above, or `nullptr`
- [ ] **[NEW]** `addSupportedAuthenticationConfig(IString* id, ICredentialPayloadDescriptor* payloadDescriptor, IPropertyObject* config = nullptr)`
  - `id` — in: key this supported config is registered under
  - `payloadDescriptor` — in: descriptor of the payload this method uses
  - `config` — in, optional: additional method-specific settings
- [ ] **[NEW]** `getSupportedAuthenticationConfigs(IDict<IString, IAuthenticationConfig>** configs)`
  - `configs` — out: configs added so far, keyed by id

### `IDevice`
- [ ] **[NEW]** `addAuthenticatedDevice(IDevice** device, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr)`
  - `device` — out: created device
  - `connectionString` — in: device connection string
  - `config` — in, optional: add-component config
  - `authenticationConfig` — in, optional: authentication settings for this attempt
- [ ] **[CHANGED]** `addStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr)` — added trailing `authenticationConfig` param (default `nullptr`; existing calls unaffected)
  - `streaming` — out: created streaming component
  - `connectionString` — in: streaming connection string
  - `config` — in, optional: add-component config
  - `authenticationConfig` — in, optional, **new**: authentication settings for this attempt; `nullptr` = unauthenticated (prior behavior)

### `IModuleManagerUtils`
- [ ] **[NEW]** `createAuthenticatedDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config, IAuthenticationConfig* authenticationConfig)`
  - `device` — out: created device
  - `connectionString` — in: device connection string
  - `parent` — in: parent component in the tree
  - `config` — in: add-component config
  - `authenticationConfig` — in: authentication settings for this attempt
- [ ] **[CHANGED]** `createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr, IString* manufacturer = nullptr, IString* serialNumber = nullptr)` — added trailing `authenticationConfig`, `manufacturer`, `serialNumber` params (all defaulted `nullptr`; existing calls unaffected). Confirmed against source: an unauthenticated `createStreaming(streaming, connectionString, config)` already existed here.
  - `streaming` — out: created streaming component
  - `connectionString` — in: streaming connection string
  - `config` — in, optional: add-component config
  - `authenticationConfig` — in, optional: authentication settings for this attempt
  - `manufacturer` — in, optional: forwarded as given (no discovery resolution for streaming)
  - `serialNumber` — in, optional: forwarded as given

### `IModule`
- [ ] **[NEW]** `createAuthenticatedDevice(IDevice** device, IString* connectionString, IString* manufacturer, IString* serialNumber, IComponent* parent, IPropertyObject* config, IAuthenticationConfig* authenticationConfig)`
  - `device` — out: created device
  - `connectionString` — in: device connection string
  - `manufacturer` — in: resolved by the module manager before this call
  - `serialNumber` — in: resolved by the module manager before this call
  - `parent` — in: parent component in the tree
  - `config` — in: add-component config
  - `authenticationConfig` — in: authentication settings for this attempt
- [ ] **[CHANGED]** `createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr, IString* manufacturer = nullptr, IString* serialNumber = nullptr)` — added trailing `authenticationConfig`, `manufacturer`, `serialNumber` params (all defaulted `nullptr`, as is `config`; existing calls unaffected). Confirmed against source: an unauthenticated `createStreaming(streaming, connectionString, config)` already existed here.
  - `streaming` — out: created streaming component
  - `connectionString` — in: streaming connection string
  - `config` — in, optional: add-component config
  - `authenticationConfig` — in, optional: authentication settings for this attempt
  - `manufacturer` — in, optional: forwarded as given
  - `serialNumber` — in, optional: forwarded as given

### `IInstanceBuilder`
- [ ] **[NEW]** `getCredentialProviders(IDict<IString, ICredentialProvider>** providers)`
  - `providers` — out: registered providers, keyed by name
- [ ] **[NEW]** `addCredentialProvider(IString* providerName, ICredentialProvider* provider)`
  - `providerName` — in: unique key to register the provider under
  - `provider` — in: the provider instance

### `IContext`
- [ ] **[NEW]** `getCredentialProviders(IDict<IString, ICredentialProvider>** providers)`
  - `providers` — out: providers flowed through from the instance builder
- [ ] **[CHANGED]** `Context(SchedulerPtr scheduler, LoggerPtr logger, TypeManagerPtr typeManager, ModuleManagerPtr moduleManager, AuthenticationProviderPtr authenticationProvider = nullptr, DictPtr<IString, IBaseObject> options = Dict<IString, IBaseObject>(), DictPtr<IString, IDiscoveryServer> discoveryServers = Dict<IString, IDiscoveryServer>(), DictPtr<IString, ICredentialProvider> credentialProviders = Dict<IString, ICredentialProvider>())` — added trailing `credentialProviders` param (defaulted; existing calls unaffected)
  - `scheduler`, `logger`, `typeManager`, `moduleManager`, `authenticationProvider`, `options`, `discoveryServers` — unchanged, existing params
  - `credentialProviders` — in, optional, **new**: providers registered on the instance builder, made available to modules via the context

---

## Open items for discussion
