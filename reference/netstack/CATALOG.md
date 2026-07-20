# ClayEngine Network-Stack Reconciliation Catalog

**Purpose:** Unify a decade of scattered ClayEngine iterations (5 archived repos + the live
`VoxelmancyProject` repo) down to a canonical keeper for each network-stack artifact, so that
everything else can be confidently flushed from Azure DevOps.

**Method:** Signal-grep of the whole archive for Winsock/IOCP/NBIO/JSON-config symbols, followed
by four parallel deep-read passes (modern OSS solutions; JAMSystem POCs; Voxelmancy/GlitterGreed
middle era; Foundation + orphan triage). Findings cross-verified by actual syscalls, not folder
names. All paths relative to `~/Repos/clay-archive/` unless noted `[live]` = `~/Repos/VoxelmancyProject/`.

> **Authoritative intent notes (from the author, not inferred):**
> - **Go-forward = `ClayEngineOSS-GitHub`** on GitHub. Curated reference; relevant pieces were moved
>   here. Does **not** include the IP-controlled VoxelFarm code (referenced only).
> - **Flicker** = the custom Rust voxel-cluster project that **completely replaces VoxelFarm**. The
>   client now lives in Flicker. ⇒ All VoxelFarm code is superseded → DROP.
> - **IOCP servers are the hardest boundary** in the app: they run the actual game simulation +
>   procedural processing, Windows-only to leverage IOCP. Preserve the IOCP design comprehensively.
> - **Dynamo** = the old-school DLL for running the server as a **Windows service** (pre-Docker).
>   Superseded by **headless console apps in containers** (still Windows for IOCP). Implementation is
>   an empty skeleton → drop; the *concept* is retired, not the code.
> - **GlitterGreed** = standalone game, original EMPS IP, **design credit: Yentlyn**. Abandoned but
>   reusable; migrate to the web projects and rebuild. Game logic ≠ network — separate track.

---

## 1. Repository inventory

| Repo / tree | Origin | Age signal | Role | Verdict |
|---|---|---|---|---|
| `ClayEngineOSS-GitHub` | GitHub `elide-us` | last commit **2026-02-15**, 75 commits | **Go-forward reference** | **KEEP (home)** |
| `ClayEngineOSS-elideus` | Azure `elideus` | last commit 2024-07-25, 32 commits | Older mirror; **network layer removed** | FLUSH (subset of GitHub) |
| `VoxelmancyProject` `[live]` | Azure (working) | checked-out Jul 2026 | Live rebuild; simpler `Services`, **no IOCP** | KEEP (active) — see §4.4 |
| `Historical-elideus/…` | Azure bulk backup | one "Initial" commit 2024-06-11 | 9 legacy trees, ~20 yrs | Mostly FLUSH after extract |
| `ClayEngineProject-empsystems` | Azure (zip) | — | `ClayEngineProject.zip` (24.8 MB) | FLUSH — dup of Historical twin |
| `Foundation-empsystems` | Azure (zip) | — | `Foundation.zip` (309 KB) | FLUSH — no network keepers |

> **Chronology caveat:** git history is worthless for dating the Historical trees (single bulk
> import). Use embedded copyright years: VoxelFarm SDK 2016/2018 → GlitterGreed (VF 2018/2019) →
> Voxelmancy "© 2022 Epoch Meridian" → ClayEngineOSS (2024) → live repo (2026).

---

## 2. Lineage of the network stack (oldest → newest)

1. **C#/XNA "ClayEngine"** (`carryingthewhale-ArchiveProjects-XNA/PlanetaryEmpire`) — a gameplay/graphics
   lib that *predates the C++ engine* by years. No network. Tombstone.
2. **VoxelFarm SDK** (`carryingthewhale-…/VoxelFarmIOLibrary`, © 2016) — third-party IOCP networking
   lib the author **studied IOCP from**. Vendored. Drop (replaced by Flicker).
3. **JAMSystem POCs** (`empsystems-ClayEngineProject/JAMSystem`) — clean but **textbook-derived**
   (Jones & Ohlund `iocpserver`/`iocpclient`; Glazer/Madhav socket wrapper). Reference only.
4. **GlitterGreed `clay_engine`** (`empsystems-GlitterGreedProject/ClayEngine/clay_engine`) — the most
   sophisticated **original hand-written** IOCP scaffold, but skeleton (~90% of the worker's OPCODE
   handlers commented out).
5. **VoxelmancyProject stable → latest** (© 2022) — PascalCase System framework, `type_index`
   `Services`, JSON-config `CoreSystem`, hand-written NBIO `NetworkSystem`, plus imported MS IOCP
   samples (`OverlappedServer`/`OverlappedClient`). Direct predecessor of ClayEngineOSS.
6. **ClayEngineOSS-elideus** (2024-07) — network layer stripped out. Dead end.
7. **ClayEngineOSS-GitHub** (2024-09 → 2026-02) — modern culmination: `AsyncNetworkSystem` (IOCP),
   `NetworkSystem` (NBIO, reference), affinity thread-per-context `Services` + JSON `startup[]`.
8. **VoxelmancyProject `[live]`** (2026) — active rebuild; rolled back to a *simpler* flat `Services`
   and JSON `appmode`, **IOCP dropped**. Newest by date but a narrower foundation.

---

## 3. The four artifacts — canonical keepers

### 3.1 IOCP server  — the hard boundary (game sim + procedural processing)
No single copy is both original *and* complete. Preserve a **composite reference**:

| Copy | Path | Original? | State |
|---|---|---|---|
| **Modern engine-integrated** ⭐ | `ClayEngineOSS-GitHub/ClayEngineLibrary/AsyncNetworkSystem.cpp` (653) `.h` (495) | **Yes** (author) | Working IOCP **accept** (AcceptEx, completion loop, 4-thread pool, socket tuning). **Data pump WSARecv/WSASend is an explicit placeholder** (`:212-219`); Bulk/Chat data modules empty. |
| **Complete data-pump pattern** ⭐ | `Historical-elideus/empsystems-ClayEngineProject/JAMSystem/JAMServer-Overlapped/JAMServer-Overlapped.cpp` (928) + `ClayEngine/Utility.h` | No (Jones/Ohlund) | Complete self-contained TCP+UDP IOCP **echo** server, heavily commented. The finished data-path reference `AsyncNetworkSystem` lacks. |
| Original scaffold | `…/empsystems-GlitterGreedProject/ClayEngine/clay_engine/network_server.h` (308) + `network_common.h` (96) | **Yes** (author) | Original design (OPCODE enum, `CompletionPort` RAII, `hw_concurrency*2` workers). Skeleton, ~90% commented. |
| Production study copy | `…/VoxelFarmIOLibrary/Inc/ServerChannel.cpp` | No (Voxel Farm Inc.) | The *most complete* IOCP server in the archive — but vendored. **Drop** (study reference only). |
| MS sample | `…/VoxelmancyProject-latest/ClayEngine/OverlappedServer/OverlappedServer.cpp` (1360) | No (MS SDK) | Mid-refactor, won't compile as-is. Skip. |

**Keep:** `AsyncNetworkSystem.*` (already in the go-forward) **+** `JAMServer-Overlapped.cpp`+`Utility.h`
as the documented "finished data-pump" reference. Optionally keep GlitterGreed `clay_engine` as the
original-design snapshot.

### 3.2 NBIO (non-blocking) client
| Copy | Path | State |
|---|---|---|
| **Canonical** ⭐ | `ClayEngineOSS-GitHub/ClayEngineLibrary/NetworkSystem.cpp` (418) `.h` (235), © 2024-09 | Hand-written. Non-blocking connect + threaded non-blocking accept via `ioctlsocket(FIONBIO)` + `WSAEWOULDBLOCK` polling. **Data path stubbed** (`Run()` is a `std::cin` dummy; `// Compress()->Encrypt()->Send()`). Present in the go-forward but **not compiled** (superseded by IOCP). |
| Twin | `…/VoxelmancyProject-{latest,stable}/ClayEngine/ClayEngineLibrary/NetworkSystem.cpp`, © 2022 | Predecessor; near-identical logic. |
| Frozen | `ClayEngineOSS-*/ClayEngineLibrary/Source/NetworkSystem.cpp` (2024-06-30) | Older twin, uncompiled, in both OSS repos. |

**Keep:** the go-forward top-level `NetworkSystem.*`. **It IS present in `ClayEngineOSS-GitHub`** (this
resolves the "the NBIO client may not be there" worry — it's there, just not wired into the build).

### 3.3 Blocking-TCP predecessor  — model correction
The thing labeled "blocking predecessor" (`CoreSystem` + `Source/NetworkSystem`) is **already NBIO**
(FIONBIO), *not* truly blocking — there is no blocking `recv`/`send` loop in it. The only genuinely
**blocking** TCP code is the JAMSystem **Chat** pair:

| Copy | Path | State |
|---|---|---|
| **Reference** ⭐ | `…/JAMSystem/JAMClient-Chat/JAMClient-Chat.cpp` (154) | Minimal blocking connect → send fixed buffer → recv echo. Complete, clearly tested. The true "hello-world" blocking client. |
| Server half | `…/JAMSystem/JAMServer-Chat/JAMServer-Chat.cpp` (`NetworkBase` path) | Blocking, but the accept is a stub (`WSASocketAccept` commented). |

**Keep:** `JAMClient-Chat.cpp` as the minimal blocking-TCP reference. Note in the catalog that the
"CoreSystem predecessor" was mislabeled — it's NBIO.

### 3.4 Threading / JSON-config component POC
Two live designs — reconcile deliberately:

| Design | Where | Shape |
|---|---|---|
| **Advanced** ⭐ | `ClayEngineOSS-GitHub`: `ClayEngine.cpp` (209) + `ClayEngineContext.cpp/.h` + `Services.h` (253) + `Storage.h` | JSON `startup[]` of `{type,class,address,port}` → **thread-per-context**; `Services` = **thread-affinity** locator `map<thread::id, map<type_index,void*>>`; wired to `AsyncNetworkSystem` per-thread. This is the "complex threading POC." |
| Predecessor | `…/VoxelmancyProject-{latest,stable}`: `Services.h` + `Storage.h` + `CoreSystem.h` | `type_index` `Services` + JSON `appmode[]` (`debug`/`client`/`guiserver`/`conserver`). |
| Earliest draft | `…/GlitterGreedProject/ClayEngine/ClayEngineLibrary/Services.h` | `string→uint64` handle map, no JSON. |
| **Live (simplified)** | `VoxelmancyProject` `[live]`: `ClayEngineLibrary/Services.h` (flat `type_index`), `ClayEngineServer/clayengine*.json` (`appmode:["conserver"]`) | The current rebuild uses the *flat* locator, not affinity. **Decision needed:** re-adopt the affinity/thread-per-context model from OSS, or keep the simpler one. |

---

## 4. Originality tiers (the real keep/drop axis)

- **KEEP — original author IP:** `AsyncNetworkSystem` (IOCP), `NetworkSystem` (NBIO), GlitterGreed
  `clay_engine` IOCP scaffold, the `Services`/`Storage`/JSON threading framework (all generations),
  `CoreSystem` state machines, **`GlitterGreed.h`** (game logic — Yentlyn).
- **KEEP AS REFERENCE ONLY — textbook/MSDN-derived (low IP value):** JAMSystem `JAMServer-Overlapped`,
  `JAMClient-Chat`, `JAMClient-Sockets/-Overlapped`; `OverlappedServer`/`OverlappedClient` (MS samples).
- **DROP — third-party vendored:** all `VoxelFarmIOLibrary` (Voxel Farm Inc., replaced by Flicker),
  `AzureUtility` (Microsoft azure-c-shared-utility), `DirectXTK`.

---

## 5. Duplication & divergence map

- `ClayEngineProject.zip` **≈** `Historical/empsystems-ClayEngineProject` (SOURCE byte-identical;
  JAMSystem identical modulo CRLF). → one is redundant.
- `Foundation.zip` **==** `_extracted/Foundation`; **≠** `Historical/…-Foundation/Foundation`
  (different snapshot — but neither holds network keepers).
- `VoxelmancyProject-latest` **≈** `-stable` (network files byte-identical; `latest` = WIP churn with
  console/window orchestration commented out; `stable` = last clean checkpoint). `latest` alone carries
  the `Overlapped*` MS samples.
- `ClayEngineOSS-elideus` **⊂** `ClayEngineOSS-GitHub` (elideus network files identical subset; GitHub
  adds the whole IOCP generation). elideus's only unique asset = an empty `ClayEngineHeadless` shell.
- `VoxelFarmIOLibrary` — 4 copies (© 2016 personal + three © 2018 embedded).
- `Source/` tier in both OSS repos — frozen 2024-06-30, compiled by nothing.

---

## 6. Tombstone / flush list  (execute in Azure DevOps — after extraction in §7)

| Confidence | Target | Reason |
|---|---|---|
| High | `ClayEngineOSS-elideus` | Strict subset of GitHub; network removed |
| High | `Foundation-empsystems` + `Foundation` (Historical) | Dynamo/PrototypeDR empty stubs; rest = DX11/text-adventure; AzureUtility vendored |
| High | `carryingthewhale-ArchiveProjects-XNA` | C#/XNA eBook sample games; no network |
| High | `rdigames-WebProjects-Angular` (`rdigames-azure`) | Unrelated web frontend |
| High | `empsystems-GlitterGreedProject/Modeling` | UML diagrams only |
| High | all `VoxelFarmIOLibrary` copies | Vendored; replaced by Flicker |
| High | `ClayEngineProject.zip` container | Dup of Historical twin |
| Med | `JAMSystem` (rest of it) | After extracting `JAMServer-Overlapped`+`Utility.h`+`JAMClient-Chat` |
| Med | `VoxelmancyProject-{latest,stable}` | After confirming nothing unique beyond OSS + `GlitterGreed.h` |
| Med | `GlitterGreedProject` (rest) | After extracting `GlitterGreed.h` + `clay_engine` scaffold |

---

## 7. Recommended consolidation

**Home = `ClayEngineOSS-GitHub`** (already the go-forward). It already holds the canonical IOCP server,
NBIO client, and advanced threading/JSON framework. Graft in only what's missing as *documented
reference*, then flush the rest:

1. `reference/netstack/JAMServer-Overlapped/` ← `JAMServer-Overlapped.cpp` + `Utility.h`
   (the finished IOCP data-pump pattern `AsyncNetworkSystem` still needs).
2. `reference/netstack/blocking/` ← `JAMClient-Chat.cpp` (minimal blocking-TCP reference).
3. `reference/netstack/original-scaffold/` ← GlitterGreed `clay_engine/` (author's original IOCP design).
4. Each with a short `README` noting provenance + originality tier + "why kept."
5. Copy this catalog in as `reference/netstack/CATALOG.md`.

**Separate track — game IP:** `GlitterGreed.h` → the web-projects rebuild. Credit **Yentlyn**.

**Then flush** everything in §6 from Azure DevOps (author action — no automated access).

---

## 8. Open decisions

1. **Home confirmation:** consolidate into `ClayEngineOSS-GitHub` as above, or a fresh `clay-netstack` repo?
2. **Threading model:** re-adopt the OSS affinity/thread-per-context `Services`, or keep the live repo's
   simpler flat locator?
3. **Staging:** extract the §7 keepers into a clean local staging folder now (so nothing is lost before
   the flush)?
