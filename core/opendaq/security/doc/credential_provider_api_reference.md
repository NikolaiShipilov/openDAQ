# Credential Provider Framework — API Reference & Authentication Flow

Credentials are modelled by their **payload shape** (`CredentialPayloadFormat`: `KeyValuePairs`, `String`, `FilePath`, or `BinaryBlob`) and a **payload descriptor** (`ICredentialPayloadDescriptor`) carrying format-specific parameters and a human description. Authentication method selection happens through an `IAuthenticationConfig` object, built per component type via `addSupportedAuthenticationConfig`.

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

`FilePath` and `BinaryBlob` formats cover two different ways of handing over a file-backed secret (e.g. a private key) — as a path the module reads itself, or as raw bytes the provider has already read on the module's behalf. Neither format has descriptor parameters (no hidden-input flag applies, unlike `String`/`KeyValuePairs`).

---

### `IAuthenticationConfig`

Carries the authentication settings for a single connection attempt. Lives alongside the base add-component config, never serialized as part of it.

| Member | Description |
|---|---|
| `getCredentialPayloadId(IString**)` | The id of the payload associated with the selected authentication method. |
| `getCredentialPayloadDescriptor(ICredentialPayloadDescriptor**)` | The descriptor of the payload the selected method uses. |
| `getConfig(IPropertyObject**)` | Additional configuration specific to the selected method — settings that may travel with the credential request to the provider (e.g. hide input as typed) and, in some cases, directly-supplied credentials (e.g. a certificate file path). Supplied for this connection attempt only; never saved. |

**Factories:**
- `AuthenticationConfig(payloadId, payloadDescriptor, config = nullptr)` — normal construction path for a live connection attempt.
- `AuthenticationConfigFromCredentialRequest(credentialRequest)` — reconstructs a config from a previously saved `CredentialRequest`; used only when reloading a device that was previously added with authentication. Hidden from other language bindings; not for regular user code.

---

### `IAuthenticationConfigPrivate`

| Member | Description |
|---|---|
| `getCredentialRequest(ICredentialRequest**)` | The previously formed credential request this config was reconstructed from, or `nullptr` for a config built for a live attempt. When assigned, a module reuses this request as-is via `ICredentialProvider::requestCredentials` instead of forming a new one — it already carries the resolved, non-secret shape of the original request. |

---

### `ICredentialRequest`

Carries the non-secret details of a credential request, handed to `ICredentialProvider::requestCredentials`. Built via `ICredentialRequestBuilder`, or reconstructed on load. Never carries actual secrets.

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
| `requestCredentials(ICredentialRequest*, ICredentialPayload**)` | Requests credentials for the given request, in the format described by its payload descriptor. |
| `getSupportedPayloadFormats(IList**)` | The list of `CredentialPayloadFormat` values this provider can supply — used for format-matching against a device type's supported formats. |

**Factories:**
- `CmdLineCredentialProvider()` — prompts the user for secrets via the command line.
- `FileCredentialProvider()` — dedicated to file-backed secrets. Prompts for the file's path via the command line, the same way `CmdLineCredentialProvider` does. For a `FilePath`-format request it hands back the path itself; for a `BinaryBlob`-format request it reads the file and hands back its raw bytes instead, so the caller never has to touch the file itself. Supports both `FilePath` and `BinaryBlob` in `getSupportedPayloadFormats`. Retries the path prompt up to 3 times if the given path isn't accessible, then fails authentication.

---

## 1a. Private-Key Challenge Authentication

Demonstrated in the credential demo module, using two new authentication configs — `PrivateKeyFile` (`FilePath`-format payload) and `PrivateKeyBlob` (`BinaryBlob`-format payload) — both authenticating against the same challenge mechanism, differing only in whether the module or the provider reads the key file.

**The challenge itself** (`VerifyPrivateKeyChallenge`, using OpenSSL's EVP API):
1. A random 32-byte challenge is generated (`RAND_bytes`).
2. The challenge is signed using the private key supplied via the credential payload (`EVP_DigestSign`, SHA-256).
3. The signature is verified against a public key loaded from a path configured as a module option (`PublicKeyPath`) (`EVP_DigestVerify`).
4. Authentication succeeds only if the signature verifies — proving whoever supplied the private key genuinely holds the key matching the module's known public key, without the module ever needing to store or compare the private key itself.

**`PrivateKeyFile` path:** the credential provider (e.g. `FileCredentialProvider`) hands back only the *path* to the key file (`IString`); the module reads and parses the PEM file itself (`ReadPemKeyFile`).

**`PrivateKeyBlob` path:** the credential provider reads the file itself and hands back the raw key bytes directly (`IBinaryData`); the module parses the key from memory (`ReadPemPrivateKeyFromMemory`) and never sees the file or its path at all.

```
Module                          Credential Provider (FileCredentialProvider)
  │── requestCredentials(request, FilePath descriptor) ──▶│
  │                                                         │── prompts user for path
  │◀── StringCredentialPayload(path) ─────────────────────│
  │                                                         │
  │── reads & parses PEM file itself ──                      │
  │── signs/verifies challenge against configured           │
  │   public key ──                                            │

  ── OR, for the BinaryBlob variant ──

  │── requestCredentials(request, BinaryBlob descriptor) ─▶│
  │                                                         │── prompts user for path
  │                                                         │── reads file itself
  │◀── BinaryBlobCredentialPayload(raw bytes) ─────────────│
  │                                                         │
  │── parses key from memory, no file access ──               │
  │── signs/verifies challenge against configured           │
  │   public key ──                                            │
```

**Provider selection in practice:** the example application registers `FileCredentialProvider` *before* `CmdLineCredentialProvider`, since `FindMatchingCredentialProvider` (in the demo module) picks the first registered provider whose supported formats include the requested one — confirming that, as implemented, provider selection is governed purely by registration order and format support, with no default-provider-binding mechanism actually present in the code.

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

### `IModuleManagerUtils`

| New member | Description |
|---|---|
| `createAuthenticatedDevice(IDevice**, IString* connectionString, IComponent* parent, IPropertyObject* config, IAuthenticationConfig* authenticationConfig)` | Iterates loaded modules, creating a device with the first one accepting the connection string and supporting authentication. Manufacturer/serial number are resolved from discovery info only for smart (`daq://`) connection strings; otherwise left unset. |

### `IModule`

| New member | Description |
|---|---|
| `createAuthenticatedDevice(IDevice**, IString* connectionString, IString* manufacturer, IString* serialNumber, IComponent* parent, IPropertyObject* config, IAuthenticationConfig* authenticationConfig)` | Module-level counterpart — receives manufacturer/serial resolved by the module manager, in addition to the authentication config. |

### `IInstanceBuilder`

| New member | Description |
|---|---|
| `getCredentialProviders(IDict**)` | The registered providers, keyed by name. |
| `addCredentialProvider(IString* providerName, ICredentialProvider*)` | Registers a provider under a unique name. |

### `IContext`

Extended with an additional `credentialProviders` parameter (`DictPtr<IString, ICredentialProvider>`) on the `Context` factory, and `getCredentialProviders(IDict**)` to retrieve them — providers registered on the instance builder flow through to the context, from which modules resolve them at authentication time.

---

## 3. The Authentication Flow, End to End

The flow below reflects the actual wiring across `ModuleManagerImpl`, `GenericDevice`, and the credential demo module.

### Adding a component with authentication

```
Application                    Instance/ModuleManager              Module (e.g. CredentialDemoModule)
    │                                    │                                    │
    │── instance.addAuthenticatedDevice( ─────────────────────────────────────▶
    │     connectionString,               │                                    │
    │     config,                          │                                    │
    │     authenticationConfig) ──────────▶│                                    │
    │                                    │                                    │
    │                                    │── resolve smart connection string    │
    │                                    │   (if daq://) → manufacturer/serial  │
    │                                    │                                    │
    │                                    │── find device type from connection   │
    │                                    │   string; check isAuthenticationSupported
    │                                    │   (fail early with OPENDAQ_ERR_NOT_SUPPORTED
    │                                    │    if the matching type doesn't support it)
    │                                    │                                    │
    │                                    │── module.createAuthenticatedDevice( ──▶
    │                                    │     connectionString, manufacturer,   │
    │                                    │     serialNumber, parent, config,     │
    │                                    │     authenticationConfig)             │
    │                                    │                                    │
    │                                    │                                    │── get payloadId + payloadDescriptor
    │                                    │                                    │   from authenticationConfig
    │                                    │                                    │
    │                                    │                                    │── find a registered credential
    │                                    │                                    │   provider whose supported payload
    │                                    │                                    │   formats include this descriptor's
    │                                    │                                    │   format (fail if none found)
    │                                    │                                    │
    │                                    │                                    │── check IAuthenticationConfigPrivate
    │                                    │                                    │   for an existing CredentialRequest
    │                                    │                                    │   (present only on reload) — reuse
    │                                    │                                    │   it as-is if present, otherwise
    │                                    │                                    │   build a NEW one via
    │                                    │                                    │   CredentialRequestBuilder, dispatching
    │                                    │                                    │   on payloadId (format alone is
    │                                    │                                    │   ambiguous across multiple
    │                                    │                                    │   payload kinds)
    │                                    │                                    │
    │                                    │                                    │── provider.requestCredentials(
    │                                    │                                    │     request) → CredentialPayload
    │                                    │                                    │
    │                                    │                                    │── construct device, calling
    │                                    │                                    │   authenticate(payload, payloadId)
    │                                    │                                    │   — verifies secrets match
    │                                    │                                    │     expected value; throws
    │                                    │                                    │     AuthenticationFailedException
    │                                    │                                    │     on mismatch
    │                                    │                                    │
    │                                    │                                    │── componentPrivate.setCredentialRequest(
    │                                    │                                    │     request) — persisted alongside
    │                                    │                                    │     the device for later reload
    │                                    │◀── device ─────────────────────────│
    │◀── device ─────────────────────────│                                    │
```

### Save / reload

```
Save:
  device tree serialized, including the device's stored CredentialRequest
  (payload id, descriptor, connection info, metadata) — NEVER the
  AuthenticationConfig or the actual secrets

Reload (new Instance, own registered credential providers):
  updateDevice() reads the "CredentialRequest" key from the serialized tree
        │
        ▼
  AuthenticationConfigFromCredentialRequest(credentialRequest)
        │
        ▼
  onAddAuthenticatedDevice(connectionString, config, reconstructedConfig)
        │
        ▼
  (same module flow as above) — but since IAuthenticationConfigPrivate::
  getCredentialRequest() now returns the reconstructed request, the module
  reuses it as-is rather than building a new one, and calls
  provider.requestCredentials(request) again — re-authenticating with
  FRESH credentials from whatever provider is registered on the new instance
```

**Key property:** a device added with authentication is never silently reconnected on reload without re-authenticating — the reloaded instance must have a compatible credential provider registered, or reload fails.

### Default / supported authentication configs, at the component-type level

```cpp
// Building a component type with two supported authentication methods:
DeviceTypeBuilder()
    .setId("CredentialDemoDevice")
    .addSupportedAuthenticationConfig(
        "UserNamePassword",
        KeyValuePayloadDescriptor({"UserName": false, "Password": true}, "Username and password"),
        userNamePasswordConfig)
    .addSupportedAuthenticationConfig(
        "Pin",
        StringPayloadDescriptor("PIN code", /*hidden*/true),
        pinConfig)
    .setDefaultAuthenticationConfigId("UserNamePassword")
    .build();
```

```cpp
// Application-side usage:
auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

// Default config (UserNamePassword, per setDefaultAuthenticationConfigId):
auto config = deviceType.createDefaultAuthenticationConfig();
auto device = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, config);

// Explicitly selecting the non-default (PIN) config instead:
auto pinConfig = deviceType.getSupportedAuthenticationConfigs().get("Pin");
auto device2 = instance.addAuthenticatedDevice("daq://openDAQ_1234", nullptr, pinConfig);
```

### Private-key challenge authentication

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

The module's public key is supplied via a module option (`PublicKeyPath`), set through the instance's JSON config provider:

```json
{
  "Modules": {
    "CredentialDemoModule": {
      "Manufacturer": "openDAQ",
      "SerialNumber": "1234",
      "PublicKeyPath": "/path/to/keys/public_key.pem"
    }
  }
}
```

### Registering credential providers on an instance

```cpp
auto credentialProvider = CmdLineCredentialProvider();

auto instanceBuilder = InstanceBuilder();
instanceBuilder.addCredentialProvider(credentialProvider.getName(), credentialProvider);
auto instance = instanceBuilder.build();
```

Provider registration happens at instance-build time and is **never serialized** — a reloaded instance must register its own providers independently, since provider setup is platform-/host-specific.

