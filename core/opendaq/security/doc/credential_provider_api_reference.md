# Credential Provider Framework — API Reference & Authentication Flow

Credentials are modelled by their **payload shape** (`CredentialPayloadFormat`: `KeyValuePairs`, `String`, `FilePath`, or `BinaryBlob`) and a **payload descriptor** (`ICredentialPayloadDescriptor`) carrying format-specific parameters and a human description. Authentication method selection happens through an `IAuthenticationConfig` object, built per component type via `addSupportedAuthenticationConfig`, or assembled ad hoc via `IAuthenticationConfigBuilder`.

---

## 1. Core Interfaces

### `ICredentialPayloadDescriptor`

Describes the shape and presentation of the payload an authentication method expects.

| Member | Description |
|---|---|
| `getFormat(CredentialPayloadFormat*)` | The payload's format — `KeyValuePairs`, `String`, `FilePath`, or `BinaryBlob`. |
| `getParameters(IPropertyObject**)` | The format's standard parameter set — for `KeyValuePairs`, a `"Keys"` dict mapping each expected key to a hidden flag (e.g. `{"UserName": False, "Password": True}`); for `String`, a single `"Hidden"` bool. |
| `getDescription(IString**)` | Human-readable description of the payload, e.g. *"PIN-code"*, *"username and password"*, *"Raw bytes of the SSH private key"*. |

**Factories:** `KeyValuePayloadDescriptor(keys, description)`, `StringPayloadDescriptor(description, hidden)`, `FilePathPayloadDescriptor(description)`, `BinaryBlobPayloadDescriptor(description)`

```cpp
enum class CredentialPayloadFormat : EnumType
{
    KeyValuePairs,  // N string pairs — e.g. UserName / Password
    String,         // one string — token, API key, PIN
    FilePath,       // one string — path to a file containing the secret, e.g. a private key
    BinaryBlob      // one raw byte buffer — pointer + size
};
```

---

### `IAuthenticationConfig`

Carries the authentication settings for a single connection attempt. Lives alongside the base add-component config, never serialized as part of it.

| Member | Description |
|---|---|
| `getCredentialPayloadId(IString**)` | The id of the payload associated with the selected authentication method. |
| `getCredentialPayloadDescriptor(ICredentialPayloadDescriptor**)` | The descriptor of the payload the selected method uses. |
| `getConfig(IPropertyObject**)` | Additional configuration specific to the selected method — settings that may travel with the credential request to the provider (e.g. hide input as typed). Supplied for this connection attempt only; never saved. |
| `getCredentialProviderId(IString**)` | The id of a specifically selected credential provider, or `nullptr` (the default) if none was chosen — in which case the module auto-selects a registered provider supporting the payload descriptor's format. |
| `getSuppliedSecret(IBaseObject**)` | A secret supplied directly by the caller, to be used instead of a provider obtaining it — or `nullptr` (the default), leaving the module to obtain it from a provider as usual. See [§5](#5-credential-provider-selection--supplied-secrets). |
| `getStreamingAuthenticationConfigs(IDict**)` | Authentication configs nested under this one, keyed by streaming type id — each an ordinary `IAuthenticationConfig` in its own right. See [§4](#4-streaming-authentication). |

**Factories:**
- `AuthenticationConfig(payloadId, payloadDescriptor, config = nullptr)` — normal construction path for a live connection attempt, with none of the builder-only settings below (provider id, supplied secret, nested streaming configs) set.
- `AuthenticationConfigFromCredentialRequest(credentialRequest)` — reconstructs a config from a previously saved `CredentialRequest`; used only when reloading a device that was previously added with authentication. Hidden from other language bindings; not for regular user code.
- `AuthenticationConfigBuilder()` — see `IAuthenticationConfigBuilder` below; the only way to set a provider id, a supplied secret, or nested streaming configs.

---

### `IAuthenticationConfigBuilder`

Builds `IAuthenticationConfig` objects, exposing every setting the plain factory doesn't.

| Member | Description |
|---|---|
| `build(IAuthenticationConfig**)` | Builds and returns an `AuthenticationConfig` from the currently configured values. |
| `setPayloadId` / `getPayloadId` | The id of the payload associated with the selected authentication method. |
| `setPayloadDescriptor` / `getPayloadDescriptor` | The descriptor of the payload the selected method uses. |
| `setConfig` / `getConfig` | Additional configuration specific to the selected method. |
| `setCredentialProviderId` / `getCredentialProviderId` | Selects a specific registered provider by id, bypassing format-based auto-selection. `nullptr` (the default) leaves auto-selection in place. |
| `setSuppliedSecret` / `getSuppliedSecret` | Supplies the secret directly, in the format described by `setPayloadDescriptor`. `nullptr` (the default) leaves the module to obtain it from a provider. |
| `addStreamingAuthenticationConfig(IStreamingType*, IAuthenticationConfig*)` | Adds (or replaces) an authentication config nested under the given streaming type — keyed internally by the type's own id (`IComponentType::getId`), so only a real, registered streaming type can ever be used as the key, not an arbitrary string. |
| `getStreamingAuthenticationConfigs(IDict**)` | The nested configs accumulated so far, keyed by streaming type id. |

A single built config can therefore carry, at once: the settings for the connection it's directly used for, plus one nested config per streaming type for anything else that's part of the same overall connection attempt (see [§4](#4-streaming-authentication)).

**Factory:** `AuthenticationConfigBuilder()` — starts with no values set.

---

### `IAuthenticationConfigPrivate`

| Member | Description |
|---|---|
| `getCredentialRequest(ICredentialRequest**)` | The previously formed credential request this config was reconstructed from, or `nullptr` for a config built for a live attempt. When assigned, a module reuses this request as-is via `ICredentialProvider::requestCredentials` instead of forming a new one — it already carries the resolved, non-secret shape of the original request. |

---

### `ICredentialRequest`

Carries the non-secret details of a credential request, handed to `ICredentialProvider::requestCredentials`/`cacheCredentials`. Built via `ICredentialRequestBuilder`, or reconstructed on load. Never carries actual secrets.

| Member | Description |
|---|---|
| `getComponentType(IComponentType**)` | The type of component the request is for. |
| `getConnectionString(IString**)` | The connection string used for this connection attempt. |
| `getMetaData(IPropertyObject**)` | Additional metadata for the provider to present to the user (e.g. device type name/id/description). |
| `getManufacturer(IString**)` | The manufacturer of the device the request is for. |
| `getSerialNumber(IString**)` | The serial number of the device the request is for. |
| `getPayloadId(IString**)` | The id of the negotiated payload (from `IAuthenticationConfig`) — serialized on save, replayed on load. |
| `getPayloadDescriptor(ICredentialPayloadDescriptor**)` | The descriptor of the payload the provider must provide — serialized on save, or re-attached from the device type on load. |

**Factory:** `CredentialRequestFromBuilder(builder)` — hidden factory, built from a `ICredentialRequestBuilder`.

---

### `ICredentialRequestBuilder`

Builds `ICredentialRequest` objects.

| Member | Description |
|---|---|
| `build(ICredentialRequest**)` | Builds and returns a `CredentialRequest` from the currently configured values. |
| `setComponentType` / `getComponentType` | The component type the request is being built for. |
| `setConnectionString` / `getConnectionString` | The connection string for this attempt. |
| `setManufacturer` / `getManufacturer` | The device manufacturer. |
| `setSerialNumber` / `getSerialNumber` | The device serial number. |
| `addMetaDataProperty(IProperty*)` | Adds a metadata property, for the provider to present to the user. |
| `getMetaData(IPropertyObject**)` | The accumulated metadata property object. |
| `setPayloadId` / `getPayloadId` | The id of the negotiated payload. |
| `setPayloadDescriptor` / `getPayloadDescriptor` | The descriptor of the payload the provider must supply. |

**Factory:** `CredentialRequestBuilder()`

---

### `ICredentialPayload`

Container providing access to the secrets obtained from a provider.

| Member | Description |
|---|---|
| `getSecrets(IBaseObject**)` | The secret(s) carried by the payload. Concrete type depends on the payload format: `IString` for `String`- or `FilePath`-format, `IDict<IString, IString>` for `KeyValuePairs`, `IBinaryData` for `BinaryBlob` (raw bytes/size via `getAddress`/`getSize`). Callers are expected to know the format (from the `IAuthenticationConfig`/`ICredentialPayloadDescriptor` used) and cast accordingly. |

**Factories:**
- `KeyValueCredentialPayload(getValuesCb)` — `KeyValuePairs`-format payload; secrets returned as `IDict<IString, IString>`, keyed the same as the descriptor's `"Keys"` parameter.
- `StringCredentialPayload(getSecretCb)` — `String`-format payload; single secret returned directly as `IString`. Also used for `FilePath`-format payloads, which likewise resolve to a single `IString`.
- `BinaryBlobCredentialPayload(getBlobCb)` — `BinaryBlob`-format payload; single secret returned as `IBinaryData`.

**Implementation note:** `KeyValueCredentialPayloadImpl`, `StringCredentialPayloadImpl`, and `BinaryBlobCredentialPayloadImpl` are all type aliases of one templated `CredentialPayloadImpl<SecretInterface>`.

---

### `ICredentialProvider`

Supplies the secrets requested via an `ICredentialRequest` — by prompting the user, reading a file, or fetching from a secret store.

| Member | Description |
|---|---|
| `getName(IString**)` | The provider's name. |
| `requestCredentials(ICredentialRequest*, ICredentialPayload**)` | Requests credentials for the given request, in the format described by its payload descriptor — obtaining them interactively (prompting, reading a file, etc.) unless a cached value from an earlier `cacheCredentials`/`requestCredentials` call for the same context already covers it. |
| `cacheCredentials(ICredentialRequest*, IBaseObject* secret)` | Accepts a secret already known in advance (see `IAuthenticationConfig::getSuppliedSecret`) so an implementation that would otherwise cache a value obtained interactively caches this one the same way — a later `requestCredentials` call for the same context then reuses it instead of prompting. Produces no payload itself; the caller already has the secret and wraps it directly. Implementations for which caching doesn't apply (or doesn't apply to the request's format) may treat this as a no-op. |
| `getSupportedPayloadFormats(IList**)` | The list of `CredentialPayloadFormat` values this provider can supply — used for format-matching against a device type's supported formats. |

**Factories:**
- `CmdLineCredentialProvider()` — prompts the user for secrets via the command line. Caches `FilePath`-format secrets in-memory for its own lifetime (i.e. for the active session), keyed by `(manufacturer, serialNumber)` — a second interactive request for the same device and format reuses the path already entered (or supplied via `cacheCredentials`) instead of prompting again. `String`/`KeyValuePairs` secrets are never cached.
- `FileCredentialProvider()` — dedicated to file-backed secrets. Prompts for the file's path via the command line, the same way `CmdLineCredentialProvider` does. For a `FilePath`-format request it hands back the path itself; for a `BinaryBlob`-format request it reads the file and hands back its raw bytes instead, so the caller never has to touch the file itself. Supports both `FilePath` and `BinaryBlob` in `getSupportedPayloadFormats`. Retries the path prompt up to 3 times if the given path isn't accessible, then fails authentication. Never caches anything — `cacheCredentials` is a no-op.

---

## 2. Extensions to Existing Interfaces

### `IComponentType`

| New member | Description |
|---|---|
| `createDefaultAuthenticationConfig(IAuthenticationConfig**)` | Clones and returns the default authentication config; a new object on each call, same as `createDefaultConfig`. Returns `OPENDAQ_ERR_NOT_SUPPORTED` if the type doesn't support authentication (no default config set on its builder). |
| `getSupportedAuthenticationConfigs(IDict**)` | The authentication configs supported by this type, keyed by payload id. |
| `isAuthenticationSupported(Bool*)` | `True` if at least one config was added and a matching default id was set — in which case `createDefaultAuthenticationConfig` is guaranteed to succeed. |

### `IComponentTypeBuilder`

| New member | Description |
|---|---|
| `setDefaultAuthenticationConfigId(IString*)` | Sets which added config (by id) is the default. Left unset ⇒ the built type doesn't support authentication. |
| `getDefaultAuthenticationConfigId(IString**)` | Gets the id set above, or `nullptr`. |
| `addSupportedAuthenticationConfig(IString* id, ICredentialPayloadDescriptor*, IPropertyObject* config = nullptr)` | Adds a supported payload; builds and stores a full `AuthenticationConfig` immediately, keyed by `id`. |
| `getSupportedAuthenticationConfigs(IDict**)` | The configs built so far, keyed by payload id. |

**Validation on build:** if configs were added but no default id was set (or vice versa), or the default id doesn't match any added config, `build()` fails with `OPENDAQ_ERR_INVALIDPARAMETER`.

### `IDevice`

| New member | Description |
|---|---|
| `addAuthenticatedDevice(IDevice**, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr)` | Connects to a device using the given authentication configuration. |
| `addStreaming(IStreaming**, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr)` | Attaches a streaming connection, optionally authenticated — a `nullptr` config (the default) uses the plain, unauthenticated path. See [§4](#4-streaming-authentication). |

### `IModuleManagerUtils`

| New member | Description |
|---|---|
| `createAuthenticatedDevice(IDevice**, IString* connectionString, IComponent* parent, IPropertyObject* config, IAuthenticationConfig* authenticationConfig)` | Iterates loaded modules, creating a device with the first one accepting the connection string and supporting authentication. Manufacturer/serial number are resolved from discovery info only for smart (`daq://`) connection strings; otherwise left unset. |
| `createStreaming(IStreaming**, IString* connectionString, IPropertyObject* config = nullptr, IAuthenticationConfig* authenticationConfig = nullptr, IString* manufacturer = nullptr, IString* serialNumber = nullptr)` | Iterates loaded modules, creating a streaming connection with the first one accepting the connection string. Unlike `createAuthenticatedDevice`, there is no smart-string/discovery resolution here — streaming connection strings are always concrete, protocol-specific ones — so `manufacturer`/`serialNumber` are simply forwarded as given. |

### `IModule`

| New member | Description |
|---|---|
| `createAuthenticatedDevice(IDevice**, IString* connectionString, IString* manufacturer, IString* serialNumber, IComponent* parent, IPropertyObject* config, IAuthenticationConfig* authenticationConfig)` | Module-level counterpart — receives manufacturer/serial resolved by the module manager, in addition to the authentication config. |
| `createStreaming(IStreaming**, IString* connectionString, IPropertyObject* config, IAuthenticationConfig* authenticationConfig, IString* manufacturer, IString* serialNumber)` | Module-level counterpart of `IModuleManagerUtils::createStreaming`. |

### `IInstanceBuilder`

| New member | Description |
|---|---|
| `getCredentialProviders(IDict**)` | The registered providers, keyed by name. |
| `addCredentialProvider(IString* providerName, ICredentialProvider*)` | Registers a provider under a unique name. |

### `IContext`

Extended with an additional `credentialProviders` parameter (`DictPtr<IString, ICredentialProvider>`) on the `Context` factory, and `getCredentialProviders(IDict**)` to retrieve them — providers registered on the instance builder flow through to the context, from which modules resolve them at authentication time.

---

## 3. Two Ways to Add a Device

The device can be added either **without authentication** (the plain, anonymous path) or **with authentication**. Both paths exist side by side — a device type that supports authentication doesn't lose its plain connection option, and the two are chosen simply by which method is called.

### The plain path (no authentication)

```cpp
auto device = instance.addDevice("daq://openDAQ_1234");
```

At the application level, this is a single call — no credential provider needs to be registered, and no authentication config is involved at all.

At the module level, this resolves to `Module::createDevice` (unchanged from before the credential framework existed). The device is constructed with `authenticated = false`, and the module's own secret-verification step is skipped entirely — no payload, no provider lookup, no challenge or comparison of any kind.

### The authenticated path

```cpp
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, authenticationConfig);
```

The same connection string is used, but an `IAuthenticationConfig` is supplied, and everything described in the rest of this document — payload negotiation, provider lookup, credential retrieval, verification — is triggered as a result.

A device type only exposes this path if it supports authentication at all (`IComponentType::isAuthenticationSupported`); calling `addAuthenticatedDevice` against a type that doesn't returns `OPENDAQ_ERR_NOT_SUPPORTED`.

---

## 4. Streaming Authentication

Attaching a streaming connection is authenticated **independently** of however the device itself got connected — a device authenticated via PIN can have a streaming source attached to it that goes through its own, entirely separate credential request. `IDevice::addStreaming`'s last parameter is the authentication config, exactly mirroring `addAuthenticatedDevice`: a `nullptr` config uses the plain, unauthenticated path; a supplied one triggers the same provider-lookup/credential-request machinery described for devices.

```cpp
// Plain, unauthenticated streaming attach:
device.addStreaming("daq.credential_demo_streaming://credential_demo_device");

// Authenticated streaming attach, with its own independent authentication config:
auto streamingAuthConfig = streamingType.getSupportedAuthenticationConfigs().get("PrivateKeyBlob");
device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, streamingAuthConfig);
```

### Nested per-type configs (device + streaming together)

A single authentication config can carry settings for more than one connection at once: the device's own, plus one nested per streaming type, via `IAuthenticationConfigBuilder::addStreamingAuthenticationConfig`. This lets one object formed for connecting to a device also carry the settings needed to authenticate a streaming source attached to that device — potentially via a different method than the device itself.

```cpp
auto streamingType = instance.getModuleManager().asPtr<IModuleManagerUtils>().getAvailableStreamingTypes().get("CredentialDemoStreaming");
auto streamingAuthConfig = streamingType.getSupportedAuthenticationConfigs().get("PrivateKeyBlob");
auto deviceDefaultAuthConfig = deviceType.createDefaultAuthenticationConfig();

auto deviceAuthConfig = AuthenticationConfigBuilder()
                             .setPayloadId(deviceDefaultAuthConfig.getCredentialPayloadId())
                             .setPayloadDescriptor(deviceDefaultAuthConfig.getCredentialPayloadDescriptor())
                             .setConfig(deviceDefaultAuthConfig.getConfig())
                             .addStreamingAuthenticationConfig(streamingType, streamingAuthConfig)
                             .build();

auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, deviceAuthConfig);

// Pulling the nested config back out of the very same object used to authenticate the device above:
auto nestedConfig = deviceAuthConfig.getStreamingAuthenticationConfigs().get(streamingType.getId());
device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, nestedConfig);
```

The nested config is keyed internally by the streaming type's own id — passing the type itself into `addStreamingAuthenticationConfig` (rather than a bare id string) means only a real, registered streaming type can ever be used as the key. Once pulled back out, it's an ordinary `AuthenticationConfig` in its own right, usable anywhere a standalone one would be.

### Auto-attach only for explicitly authorized streaming types

`StreamingSourceManager` (the implementation behind the `PrioritizedStreamingProtocols`/`AutomaticallyConnectStreaming` add-device config) auto-attaches streaming sources for a device once it's added. If the device itself was authenticated (`addAuthenticatedDevice`), the manager consults the device's own nested streaming configs (`getStreamingAuthenticationConfigs`, resolved via `MirroredDeviceBase::setAuthenticationConfig`) for each candidate streaming capability:

- A capability whose protocol id has **no matching nested entry** is never auto-attached — only explicitly authorized streaming types are, so an authenticated device doesn't silently pick up unauthenticated (or differently-authenticated) streaming connections on its own.
- A capability whose protocol id **does** have a matching entry is auto-attached using that nested config as its authentication config.

If the device itself was **not** authenticated (plain `addDevice`), auto-attach behaves exactly as it did before nested configs existed — no gating, no authentication, for every discovered capability.

![Streaming authentication — manual attach (three variants), and auto-attach gated by nested configs](credential_flow_diagram_streaming.png)
*Diagram 2 — manual `addStreaming` (unauthenticated, independently authenticated, or via a config nested inside the device's own), and `StreamingSourceManager`'s auto-attach gating. Both authenticated paths hand off to the same credential resolution shown in Diagram 1 ([§6](#6-authentication-flow)).*

---

## 5. Credential Provider Selection & Supplied Secrets

Two independent, combinable settings on `IAuthenticationConfig`/`IAuthenticationConfigBuilder` let a caller take over parts of the credential-provider machinery that would otherwise happen automatically.

### Explicit provider selection

By default, the module auto-selects the first registered provider whose `getSupportedPayloadFormats()` includes the required format. `setCredentialProviderId` overrides this — the module looks the given id up directly among the registered providers instead:

```cpp
auto config = AuthenticationConfigBuilder()
                   .setPayloadId(privateKeyFileConfig.getCredentialPayloadId())
                   .setPayloadDescriptor(privateKeyFileConfig.getCredentialPayloadDescriptor())
                   .setConfig(privateKeyFileConfig.getConfig())
                   .setCredentialProviderId(cmdLineCredentialProvider.getName())
                   .build();
```

This only makes an observable difference for a format more than one registered provider supports — e.g. `FilePath`, supported by both `FileCredentialProvider` and `CmdLineCredentialProvider`. An id naming no registered provider, or one that doesn't support the required format, fails authentication immediately with a message identifying the problem — it never silently falls back to auto-selection.

### Supplying the secret directly

`setSuppliedSecret` hands the module a secret it already has — in the format described by the config's payload descriptor (see `ICredentialPayload::getSecrets` for the expected concrete type per format) — instead of having a provider obtain one interactively:

```cpp
auto config = AuthenticationConfigBuilder()
                   .setPayloadId(userNamePasswordConfig.getCredentialPayloadId())
                   .setPayloadDescriptor(userNamePasswordConfig.getCredentialPayloadDescriptor())
                   .setConfig(userNamePasswordConfig.getConfig())
                   .setSuppliedSecret(Dict<IString, IString>({{"UserName", "user"}, {"Password", "pass"}}))
                   .build();
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, config);
```

- **No provider id set:** no provider is looked up at all — the module wraps the supplied secret into the credential payload itself and authenticates with it directly, with no prompt of any kind.
- **A provider id is also set:** the module still wraps the secret itself for this connection, but first hands both the secret and the credential request to that specific provider via `ICredentialProvider::cacheCredentials`, so the provider can remember it the same way it would one obtained interactively.

The second combination is what makes it possible for a *later*, ordinary `requestCredentials` call — e.g. authenticating a streaming connection attached to the same device — to be served from the provider's cache instead of prompting, provided the provider actually caches for that format (see `CmdLineCredentialProvider`'s `FilePath` caching) and the two requests share the same `(manufacturer, serialNumber)`:

```cpp
auto deviceConfig = AuthenticationConfigBuilder()
                         .setPayloadId(devicePrivateKeyFileConfig.getCredentialPayloadId())
                         .setPayloadDescriptor(devicePrivateKeyFileConfig.getCredentialPayloadDescriptor())
                         .setConfig(devicePrivateKeyFileConfig.getConfig())
                         .setCredentialProviderId(cmdLineCredentialProvider.getName())
                         .setSuppliedSecret(String("/path/to/private_key.pem"))
                         .build();
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, deviceConfig);

// Same device, same FilePath-format method, same provider - no path prompt this time; the provider
// serves it from the cache `cacheCredentials` populated above.
auto streamingConfig = AuthenticationConfigBuilder()
                           .setPayloadId(streamingPrivateKeyFileConfig.getCredentialPayloadId())
                           .setPayloadDescriptor(streamingPrivateKeyFileConfig.getCredentialPayloadDescriptor())
                           .setConfig(streamingPrivateKeyFileConfig.getConfig())
                           .setCredentialProviderId(cmdLineCredentialProvider.getName())
                           .build();
device.addStreaming("daq.credential_demo_streaming://credential_demo_device", nullptr, streamingConfig);
```

For the credential-demo module specifically, the device's own manufacturer/serial number are what `MirroredDeviceBase::onAddStreaming` resolves (from `IDeviceInfo`) and forwards into the streaming connection attempt, so a manually-attached streaming connection for the same device naturally lands in the same cache bucket.

---

## 6. Authentication Flow

Both ways of adding a device converge on the same entry point at the application level — a connection string and a config object — and diverge based on whether an `IAuthenticationConfig` is supplied.

Without one, the call resolves straight down to the module's plain device-construction path: the module manager locates the appropriate device type from the connection string, and the module builds the device with no further involvement from the credential framework at all.

With one, the module manager first confirms the target device type actually supports authentication, then hands off to the module together with the authentication config. From there, the module works out which payload it needs, resolves the credentials for it — a provider (auto-selected or explicitly chosen by id) obtaining them interactively, or a directly-supplied secret used instead (see [§5](#5-credential-provider-selection--supplied-secrets)) — and obtains a credential request, either reusing one carried over from a previous save (on reload) or building a fresh one. The module then verifies the obtained credentials and constructs the device. Whether the request was fresh or reused, the resulting credential request is stored on the device afterward, so a future reload can repeat this same process rather than needing the original secrets to be saved anywhere.

![Adding a device — with vs without authentication, and credential resolution (API-level flow)](credential_flow_diagram_device.png)
*Diagram 1 — the plain vs. authenticated add-device paths, and the credential-resolution branching (provider auto-selection vs. explicit id, interactive `requestCredentials` vs. a directly-supplied secret). The same resolution applies verbatim to authenticating a streaming connection (Diagram 2, [§4](#4-streaming-authentication)) — only the surrounding API call differs. The pink steps (provider selection policy, secret-wrapping) are how one module (the credential-demo prototype) chose to implement this — not something the core interfaces prescribe; see the note at the top of [§8](#8-module-level-implementation).*

---

## 7. Application-Level Usage

### Registering credential providers

```cpp
auto credentialProvider = CmdLineCredentialProvider();

auto instanceBuilder = InstanceBuilder();
instanceBuilder.addCredentialProvider(credentialProvider.getName(), credentialProvider);
auto instance = instanceBuilder.build();
```

Provider registration happens once, at instance-build time, and is **never serialized** — a reloaded instance must register its own providers independently, since provider setup is platform-/host-specific. Multiple providers can be registered; when more than one supports the same payload format, the first one registered is enumerated first by the module during auto-selection — unless a specific provider id is set on the authentication config, which bypasses that enumeration entirely (see [§5](#5-credential-provider-selection--supplied-secrets)).

### Selecting an authentication method

A device type may support several authentication methods at once. The application either accepts the type's default, or explicitly picks a different supported one:

```cpp
auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

// Use the type's default authentication method:
auto config = deviceType.createDefaultAuthenticationConfig();
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, config);

// Or explicitly select a specific supported method instead (e.g. "Pin"):
auto pinConfig = deviceType.getSupportedAuthenticationConfigs().get("Pin");
auto device2 = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, pinConfig);
```

### Example: private-key challenge authentication

```cpp
auto fileCredentialProvider = FileCredentialProvider();
auto cmdLineCredentialProvider = CmdLineCredentialProvider();

auto instanceBuilder = InstanceBuilder();
// Registered first, so it — not CmdLineCredentialProvider — is picked for FilePath/BinaryBlob requests.
instanceBuilder.addCredentialProvider(fileCredentialProvider.getName(), fileCredentialProvider);
instanceBuilder.addCredentialProvider(cmdLineCredentialProvider.getName(), cmdLineCredentialProvider);
auto instance = instanceBuilder.build();

auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

// FilePath variant — module reads and parses the PEM file itself.
auto privateKeyFileConfig = deviceType.getSupportedAuthenticationConfigs().get("PrivateKeyFile");
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, privateKeyFileConfig);

// BinaryBlob variant — provider reads the file, module only ever sees raw key bytes.
auto privateKeyBlobConfig = deviceType.getSupportedAuthenticationConfigs().get("PrivateKeyBlob");
auto device2 = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, privateKeyBlobConfig);
```

See [§4](#4-streaming-authentication) for authenticating a streaming connection (independently, or nested inside the device's own config) and [§5](#5-credential-provider-selection--supplied-secrets) for selecting a specific provider or supplying a secret directly.

### Save and reload

Saving an instance persists the connected device's `CredentialRequest` (connection info, payload id and descriptor, metadata) as part of the tree — but never the `AuthenticationConfig`, the actual secrets, a selected provider id, or a supplied secret. Reloading that saved configuration into a new instance re-authenticates the device from scratch: the new instance must have its own compatible credential provider registered, or the reload fails.

---

## 8. Module-Level Implementation

This section describes what a module does internally when `Module::createAuthenticatedDevice`/`createStreaming` is called — none of this is visible to, or called directly by, application code.

> **Note:** the steps below describe how the credential-demo prototype implements this — provider lookup (currently: first registered provider whose supported formats match, or a direct lookup by id when one was explicitly selected) and retry/fallback behavior on failure may differ in a production implementation, since none of it is prescribed by the core interfaces themselves.

1. **Resolve the payload to provide.** The module reads the payload id and descriptor off the supplied `IAuthenticationConfig` (`getCredentialPayloadId`, `getCredentialPayloadDescriptor`).

2. **Obtain or reuse the credential request.** If the `IAuthenticationConfig` was reconstructed from a saved device (i.e. this is a reload, not a fresh connection), `IAuthenticationConfigPrivate::getCredentialRequest()` returns the original request as-is, and the module reuses it unchanged. Otherwise, the module builds a new `ICredentialRequest` via `ICredentialRequestBuilder`, populating connection string, manufacturer/serial number (if resolved), and metadata for the provider to present to the user.

3. **Resolve the credentials.** The module reads `getCredentialProviderId()` and `getSuppliedSecret()` off the authentication config and branches:
   - **A secret is supplied:** no provider obtains anything — the module wraps the secret into a credential payload directly, matching the shape the payload's format expects. If a provider id was *also* set, that specific provider is first looked up and handed the secret via `provider.cacheCredentials(request, secret)`, so it can remember it the same way it would one obtained interactively — but the payload used for *this* connection is always the one the module just wrapped, regardless of whether caching succeeds.
   - **No secret supplied:** the module finds a matching provider — the one named by `getCredentialProviderId()` if set (failing immediately, with a message identifying the problem, if that id names no registered provider or one that doesn't support the required format), otherwise the first registered provider whose `getSupportedPayloadFormats()` includes the required format. Either way, `provider.requestCredentials(request)` is then called, obtaining an `ICredentialPayload` — interactively, or served from that provider's own cache if a matching entry exists (e.g. from an earlier `cacheCredentials` call, or an earlier interactive request for the same context).

4. **Construct the device (or streaming) and authenticate.** The component is constructed and its authentication step runs — extracting the secrets (`getSecrets()`) and verifying them against whatever the specific method requires (a fixed value, a signed challenge, etc.). A mismatch throws `AuthenticationFailedException`, and construction fails.

5. **Persist the credential request for later reload (devices only).** On success, the module stores the credential request on the newly created device (`IComponentPrivate::setCredentialRequest`) — this is what step 2 reads back on a future reload, without ever needing to persist the secrets, provider id, or supplied secret themselves.

![Application / Module / Credential Provider — sequence view of the same steps](credential_flow_diagram_sequence.png)
*Diagram 3 — the same steps as above, as a sequence diagram across the three parties involved: the Application first discovers which authentication methods a type supports (`getSupportedAuthenticationConfigs`) before building its `authConfig`; the Module then either resolves credentials through a Credential Provider (querying `getSupportedPayloadFormats()` to find or validate one, then `requestCredentials`/`cacheCredentials`) or wraps a supplied secret itself — matching the branching in Diagram 1. As there, the pink notes are prototype-specific policy, not core-mandated behavior.*
