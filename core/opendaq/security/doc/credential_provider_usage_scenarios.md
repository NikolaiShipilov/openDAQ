# Credential Provider Framework — Usage Scenarios

A backlog of user stories and use-case scenarios for exercising `IDevice::createDefaultAuthenticationConfig`, `addAuthenticatedDevice`/`addStreaming`, and the credential provider framework end to end - meant to drive example/test implementations that verify the API is actually usable, not just that individual methods return the right type. Each scenario names the concrete API calls and expected outcome (return value, exception, or observable side effect) so it can be implemented directly, ideally against `credential_demo_module` the way `credential_providers.cpp` already does.

Persona used throughout: **an application developer** integrating openDAQ's credential framework into a client app or tool - deciding whether/how to offer authenticated connections to their users.

---

## 1. Discovery & feasibility

Before ever prompting a user or attempting a connection, an application needs to know what's even possible.

### 1.1 — Does this type support authentication at all?

As an application developer, I want to check whether a given device/streaming type supports authentication, so I can decide whether to offer an "authenticated connect" option in my UI at all.

- **Given** a type id that has supported authentication descriptors and a default id set on its builder (e.g. `"CredentialDemoDevice"`)
  **When** I call `instance.createDefaultAuthenticationConfig(typeId)`
  **Then** it succeeds and returns a config whose `"PayloadDescriptor"` selection has at least one candidate.
- **Given** a type id that never had `addSupportedAuthenticationDescriptor`/`setDefaultAuthenticationConfigId` called on its builder
  **When** I call `instance.createDefaultAuthenticationConfig(typeId)`
  **Then** it fails with `OPENDAQ_ERR_NOT_SUPPORTED` - not a crash, not an empty-but-successful config.
- **Given** a type id that names neither an available device type nor an available streaming type
  **When** I call `instance.createDefaultAuthenticationConfig(typeId)`
  **Then** it fails with `OPENDAQ_ERR_NOTFOUND`, distinguishable from the "not supported" case above (a developer needs to tell "unknown type" from "known type, no auth" apart - e.g. to detect a typo in `typeId` vs. correctly reporting "no auth available" to a user).
- **Given** the plain, anonymous path is what's wanted instead
  **When** I call `instance.addDevice(connectionString)` on a type that doesn't support authentication (or one that does, but I chose not to authenticate)
  **Then** it succeeds regardless - the two paths are fully independent (see the API reference, §3).

### 1.2 — Is authentication actually *usable* right now, given my registered providers?

As an application developer, I want to know not just whether a type *supports* authentication, but whether I can actually *complete* it with the providers currently registered on my instance - these are different questions, and the API doesn't collapse them into one boolean.

- **Given** a type supporting only a `String`-format method (e.g. `"Pin"`), and only a `FileCredentialProvider` (FilePath-only) registered
  **When** I inspect `context.getCredentialProviders()` and cross-reference each provider's `getSupportedPayloadFormats()` against the type's `getSupportedAuthenticationDescriptors()` formats
  **Then** I can determine, entirely client-side and without attempting a connection, that authentication would fail before ever calling `addAuthenticatedDevice` - `createDefaultAuthenticationConfig` itself doesn't tell me this; it happily builds a config naming a provider that can't actually serve the request. This is worth writing as its own small helper in the example (`FindCompatibleProviders(instance, typeId)` or similar), since the core API has no single call for it.
- **Given** zero credential providers registered on the instance at all
  **When** I build the default config and call `addAuthenticatedDevice`
  **Then** it fails with `AuthenticationFailedException` ("no credential provider supporting a compatible payload format is registered") - a good negative-path example distinct from 1.1's "type doesn't support auth" case: here the *type* supports it, the *instance* just can't currently serve it.

### 1.3 — Feasibility for a device deeper in the tree, not just the root

As an application developer working with nested topologies, I want `createDefaultAuthenticationConfig` to work the same way when called on a sub-device, not only on the root instance.

- **Given** a device tree where an intermediate `IDevice` (not `Instance` itself) exposes `typeId` via its own `getAvailableDeviceTypes()`/`getAvailableStreamingTypes()`
  **When** I call `subDevice.createDefaultAuthenticationConfig(typeId)` directly on that sub-device rather than on `instance`
  **Then** it resolves and builds a config identically to calling it on the root - since `Instance` is a pure forwarder to `rootDevice`, and any `IDevice` implements this the same way via `GenericDevice`. Worth one explicit example beyond the always-root-instance calls in `credential_providers.cpp` today, to confirm the "parent device" framing (not "root device") actually holds.

---

## 2. Manual authenticated device — default config, unmodified

The user's original bullets, filled in.

### 2.1 — Default config + correct credentials, compatible provider registered

As an application developer, I want the simplest possible path to work: get the default config, hand it straight to `addAuthenticatedDevice` with no changes, and have it succeed when the user enters correct credentials.

- **Given** a `CmdLineCredentialProvider` (or equivalent, supporting the default method's format) registered on the instance
  **And** `authConfig = instance.createDefaultAuthenticationConfig(typeId)` used as-is (default `"PayloadDescriptor"` selection, default `"CredentialProviderId"` selection - see the gotcha in §4.1 below)
  **When** I call `instance.addAuthenticatedDevice(connectionString, nullptr, authConfig)` and supply correct credentials at the prompt
  **Then** the device is added, `device.getInfo()` is queryable, and `device.getAvailableDeviceTypes()`/other calls on it work normally.

### 2.2 — Default config + incorrect credentials

- **Given** the same setup as 2.1
  **When** incorrect credentials are supplied at the prompt
  **Then** `addAuthenticatedDevice` throws `AuthenticationFailedException`, the device is **not** added (verify `instance.getDevices()` doesn't contain it), and no partially-constructed device is left behind.

### 2.3 — Default config, no provider registered at all

- **Given** an instance built with no `addCredentialProvider` calls
  **When** I call `addAuthenticatedDevice` with a default config
  **Then** it throws `AuthenticationFailedException` before ever attempting to prompt for anything (no hang waiting on stdin) - the "no compatible provider" message, not a null-pointer/crash.

---

## 3. Manual authenticated device — supplied credential

### 3.1 — Correct supplied secret, provider registered and explicitly named

- **Given** `authConfig` built via `AuthenticationConfigBuilder().setPayloadDescriptor(...).setCredentialProviderId(providerId).setSuppliedSecret(payload).build()`, with `payload` correctly shaped and correct
  **When** I call `addAuthenticatedDevice`
  **Then** authentication succeeds with **no interactive prompt at all** (verify via a non-interactive/scripted run - if the harness would hang on stdin, that's a bug or a wrong assumption about this path), and the named provider's `cacheCredentials(request, secret)` is invoked - confirmed indirectly by then making a *second*, separate connection attempt (same manufacturer/serial or canonical connection string) using only `requestCredentials` (no supplied secret) and observing it also completes without a prompt.

### 3.2 — Correct supplied secret, no provider named

- **Given** the same as 3.1 but `setCredentialProviderId` never called (or an `AuthenticationConfig(...)`-built config with no provider selection touched)
  **When** I call `addAuthenticatedDevice`
  **Then** authentication succeeds with no prompt (the supplied secret is used directly, no provider ever consulted per §8 step 3), **and** nothing is cached anywhere - confirmed by then authenticating a second, otherwise-identical connection *without* a supplied secret and observing that it **does** prompt (proving nothing leaked into a cache with no provider to own it).

### 3.3 — Incorrect supplied secret

- **Given** a supplied secret shaped correctly but with a wrong value (e.g. wrong password, wrong PIN)
  **When** I call `addAuthenticatedDevice`
  **Then** it throws `AuthenticationFailedException` - verifying that a caller can't bypass the module's own verification step just by constructing the payload shape correctly; supplying a secret skips the *provider*, not the *authenticator*.

### 3.4 — Malformed supplied secret (robustness)

As an application developer, I want a clear failure, not a confusing one, if I get the supplied-secret shape wrong.

- **Given** a supplied secret property object missing an expected property (e.g. a `KeyValuePairs` payload missing `"Password"`), or with an extra/misnamed one
  **When** I call `addAuthenticatedDevice`
  **Then** document (via the example) exactly what happens today - does it throw a clear, actionable error, or fail cryptically deep inside verification? This is worth an explicit example even if the answer turns out to be "not very friendly today," since it tells us whether `createDefaultPayload()` really needs to be used as the template (as documented) rather than treated as optional guidance.

---

## 4. Tuning the default config before use

Between "use the unmodified default" (§2) and "build one entirely by hand via the Builder" (§3), there's the actual expected common path: get the default, change one or two things, use it.

### 4.1 — The default-provider/format mismatch trap

As an application developer, I want to understand - and the example to make obvious - that the unmodified default config's `"CredentialProviderId"` is *not* guaranteed to be format-compatible with the *default* `"PayloadDescriptor"`.

- **Given** two providers registered, the first (`FileCredentialProvider`, FilePath-only) registered before a second (`CmdLineCredentialProvider`, all formats)
  **And** the type's default payload method is `KeyValuePairs` or `String` (not `FilePath`)
  **When** I use `instance.createDefaultAuthenticationConfig(typeId)` completely unmodified and call `addAuthenticatedDevice`
  **Then** it fails with `AuthenticationFailedException` ("the explicitly selected credential provider ... does not support the required payload format") - **not** because nothing is registered, but because the default `"CredentialProviderId"` selection (first-registered provider, no format filtering - see the API reference, §2/§8) happened to pick an incompatible one. This is a real trap discovered while building the example app this session (`demoUserNamePasswordAuthentication` broke exactly this way) and deserves its own scenario precisely because it's non-obvious: "I used the default and it failed" needs a story explaining *why*, and the fix (explicitly select a compatible provider).

### 4.2 — Selecting a non-default payload method

- **Given** the default config lists more than one `"PayloadDescriptor"` candidate
  **When** I switch the selection to a non-default one (matching by candidate `Id`, the pattern already in `credential_providers.cpp`'s `SelectAuthenticationMethod`) before calling `addAuthenticatedDevice`
  **Then** the *other* method's credentials are requested/verified, not the default's - confirming the selection genuinely drives which descriptor (and thus which prompt/verification) is used, not just cosmetic state.

### 4.3 — Selecting a non-default credential provider explicitly

- **Given** two format-compatible providers registered
  **When** I set `"CredentialProviderId"` (via `setPropertySelectionValue`, since it's a Selection when built through `createDefaultAuthenticationConfig`) to the *second* one before calling `addAuthenticatedDevice`
  **Then** the second provider - not the auto/default-selected first one - is the one whose `requestCredentials`/`cacheCredentials` gets called; verify by checking which provider's cache holds the entry afterward (§3.1's technique), or by giving each provider observably different prompts/behavior.

### 4.4 — Explicit provider id that's registered but format-incompatible

- **Given** a provider id that *is* registered but doesn't support the selected payload's format
  **When** I select it explicitly and call `addAuthenticatedDevice`
  **Then** it fails immediately with a message naming the problem (not a silent fallback to auto-selection) - confirming `FindMatchingCredentialProvider`'s documented behavior that an explicit selection is never treated as "a hint," it's binding.

### 4.5 — Explicit provider id that names no registered provider

- **Given** a provider id string that doesn't match any `context.getCredentialProviders()` key (e.g. a typo, or a provider registered on a *different* instance)
  **When** I select it and call `addAuthenticatedDevice`
  **Then** it fails immediately with a message identifying the missing provider id.

---

## 5. Credential provider caching

### 5.1 — Same context, same provider, same format → cache hit

- **Given** a device authenticated via a caching-capable provider and format (e.g. `CmdLineCredentialProvider` + `FilePath`)
  **When** a *second* connection sharing the same manufacturer/serial (or, absent those, the same canonical connection string - e.g. a streaming attach to the same device) is authenticated via the same provider and format
  **Then** no prompt occurs the second time - this is the scenario `demoCachedFilePathCredentialAcrossDeviceAndStreaming` already demonstrates; worth keeping as a named, minimal example independent of the bigger demo.

### 5.2 — Different context → cache miss (isolation)

The inverse of 5.1, and just as important to verify explicitly.

- **Given** the same setup as 5.1
  **When** a connection with a *different* manufacturer/serial pair (or a different canonical connection string) is authenticated via the same provider and format
  **Then** it prompts independently - confirming the cache key genuinely discriminates by connection identity and doesn't leak credentials across unrelated devices.

### 5.3 — Non-caching provider or format

- **Given** `FileCredentialProvider` (documented as never caching) or a `KeyValuePairs`/`String` request against `CmdLineCredentialProvider` (only `FilePath` is cached)
  **When** the same context is authenticated twice
  **Then** it prompts both times - confirming caching is opt-in per provider/format, not a blanket assumption an application can rely on for every combination.

---

## 6. Streaming authentication

### 6.1 — Streaming authenticated independently of its device

- **Given** a device connected via one method (or not authenticated at all)
  **When** a streaming connection to it is added via `device.addStreaming(connectionString, nullptr, streamingAuthConfig)` using a *different* method/provider
  **Then** both succeed independently - the streaming leg's authentication outcome (success or failure) is unaffected by how the device itself was connected, and vice versa.

### 6.2 — Auto-attached streaming is always unauthenticated

As an application developer relying on `PrioritizedStreamingProtocols`/`AutomaticallyConnectStreaming`, I want to know this path never authenticates on my behalf, so I don't assume protection I don't have.

- **Given** a device added with auto-attach streaming config enabled, to a streaming type that *does* support authentication
  **When** the device connects
  **Then** the auto-attached streaming source is present and working, but was never asked for credentials of any kind - verified by registering *no* compatible provider at all and confirming auto-attach still succeeds (where a manual `addStreaming` with an auth config would instead fail per §2.3).

---

## 7. Save & load

The user's last bullet, filled in with the corrected persistence model (see the API reference, §7/§8 - credentials are never part of what's saved).

### 7.1 — Reload with the same, compatible provider re-registered

- **Given** an authenticated device saved via `instance.saveConfiguration()`
  **When** a new instance registers a provider with the **same id and format support** as the original, then calls `loadConfiguration(savedConfiguration)`
  **Then** the device reconnects, re-authenticating from scratch through the normal `requestCredentials` path - prompting again unless that provider happens to have something cached for this context (§5.1), never because anything was persisted in the saved config itself.

### 7.2 — Reload with no compatible provider registered

- **Given** the same saved configuration
  **When** the new instance registers no provider at all, or one that doesn't support the persisted method's format
  **Then** `loadConfiguration` fails to reconnect the device (confirm via `reloadedInstance.getDevices()` being empty, matching `demoPinAuthenticationAndReload`'s own check) - not a silent, disconnected-but-present device.

### 7.3 — Reload with a *different-id* but format-compatible provider (the persisted-id gotcha)

As an application developer, I want to understand that the persisted `"CredentialProviderId"` is matched **by id**, not by capability - a real gotcha worth its own scenario.

- **Given** the original session authenticated using a provider explicitly selected by id (e.g. `"CmdLineCredentialProvider"`)
  **When** the new instance registers a *different*, format-compatible provider under a *different* id (e.g. a custom one, or the same class re-registered under a new id) but not one matching the persisted id exactly
  **Then** confirm what actually happens - does reload fail outright (since `FindMatchingCredentialProvider` treats an explicit id as binding, per §4.4), even though a perfectly capable provider *is* registered? If so, this is worth documenting as a real constraint: **the new instance must re-register a provider under the exact same id**, not merely "a compatible one."

### 7.4 — A supplied-secret device does *not* skip the prompt on reload

Directly verifies the corrected persistence model from this session.

- **Given** a device originally authenticated via `setSuppliedSecret` (§3.1/3.2), so no provider's cache was ever touched by the original supplied value
  **When** it's saved and reloaded into a new instance with a fresh, non-caching-relevant provider registered
  **Then** the reload **prompts** (or fails, if no provider is registered) exactly as a from-scratch connection would - confirming the supplied secret itself never survives into the saved configuration, only the payload descriptor and provider id do.

---

## 8. Boundary & negative checks worth having somewhere, even briefly

- Calling `createDefaultAuthenticationConfig` with a `typeId` that's valid but for a component sort with no notion of authentication at all (e.g. a function block type, if one is ever passed) - confirm it's treated the same as "unsupported" (`OPENDAQ_ERR_NOT_SUPPORTED`) rather than something type-confused.
- Calling `addAuthenticatedDevice` with `authenticationConfig = nullptr` against a type that supports authentication - confirm this is rejected clearly (`AuthenticationFailedException`, per `onCreateAuthenticatedDevice`'s existing check), distinct from silently falling back to the anonymous path.
- Two authentication attempts to the *same* connection string in quick succession without an intervening `removeDevice` (e.g. accidental double-click in a UI) - confirm the second attempt's failure/success mode is well-defined rather than racing the first.
