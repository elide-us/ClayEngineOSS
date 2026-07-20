// fbximport - Part 1 of the FBX -> animation pipeline (bootstrap crutch).
//
// Reads Autodesk FBX (via FBX SDK 2018.1) and emits the "flicker.rig" JSON
// Format Contract: skeleton (flat bone array), skinned mesh, and baked
// animation clips. Deliberately small and legible - NOT a general pipeline.
//
// Key decisions (see design notes / task brief):
//  * NO axis/unit ConvertScene. We record the source FbxAxisSystem/FbxSystemUnit
//    in metadata but emit ALL data in source space unmodified. The downstream
//    consumer applies a single world matrix. Internal consistency > target axis.
//  * Transforms come from EvaluateLocalTransform(t) so the SDK folds in
//    pre/post-rotation and pivots. We never compose node transforms by hand.
//  * Animation is BAKED BY EVALUATION at a fixed tick rate; no raw curve reads,
//    no keyframe reduction.
//  * The geometric transform (Get{Geometric}Translation/Rotation/Scaling) is
//    applied to vertices+normals at import, never pushed into the bone hierarchy.
//
// CLI:
//   fbximport <input.fbx> <output.json> [--tick-rate 60] [--up Y] [--unit m] [--max-weights 4]
//   fbximport --batch <input_dir> <output_dir>

// Keep Windows headers from defining min/max macros (they clash with std::min).
#ifndef NOMINMAX
#  define NOMINMAX
#endif

#include <fbxsdk.h>

// The FBX SDK (fbxarch.h) does `#define snprintf _snprintf` for old-MSVC
// compatibility. On modern MSVC that both breaks std::snprintf and drags in the
// deprecated _snprintf. Undo it so we use the standard, bounded snprintf.
#ifdef snprintf
#  undef snprintf
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#  include <windows.h>
#endif

// stb single-header image I/O (declarations only; the implementations are compiled
// once in stb_impl.cpp). Used by the `--textures` TGA->PNG mode.
#include "stb_image.h"
#include "stb_image_write.h"

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

namespace {

struct Options {
    double tickRate = 60.0;
    std::string up = "Y";      // recorded only; we emit source space regardless
    std::string unit = "m";    // recorded only
    int maxWeights = 4;
};

// Path helpers ---------------------------------------------------------------

std::string fileStem(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t dot = base.find_last_of('.');
    if (dot == std::string::npos) return base;
    return base.substr(0, dot);
}

std::string fileName(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

bool endsWithFbxCaseInsensitive(const std::string& name) {
    if (name.size() < 4) return false;
    std::string tail = name.substr(name.size() - 4);
    for (char& c : tail) c = (char)tolower((unsigned char)c);
    return tail == ".fbx";
}

// JSON writing ---------------------------------------------------------------
// Hand-rolled, no external dependency. We stream directly to an ostream and
// escape strings; floats use %.9g for ~9 significant digits.

std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

void writeFloat(std::ostream& os, double v) {
    // Normalize -0 to 0 for cleanliness.
    if (v == 0.0) v = 0.0;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.9g", v);
    os << buf;
}

void writeMat16(std::ostream& os, const double m[16]) {
    os << '[';
    for (int i = 0; i < 16; ++i) {
        if (i) os << ',';
        writeFloat(os, m[i]);
    }
    os << ']';
}

// Convert an FbxAMatrix into a row-major 16-float array.
// FbxAMatrix::Get(row, col) is row-major access already (row = the y arg).
void fbxMatToRowMajor(const FbxAMatrix& src, double out[16]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[r * 4 + c] = src.Get(r, c);
}

// ---------------------------------------------------------------------------
// Intermediate data model
// ---------------------------------------------------------------------------

struct Bone {
    std::string name;
    int parent = -1;
    double local[16];         // rest LOCAL transform, row-major
    double inverseBind[16];   // inverse-bind, row-major (identity if no cluster)
    FbxNode* node = nullptr;  // source node, for animation baking
};

struct Vertex {
    double p[3] = {0, 0, 0};
    double n[3] = {0, 0, 0};
    double uv[2] = {0, 0};
    int joints[4] = {0, 0, 0, 0};
    double weights[4] = {0, 0, 0, 0};
};

struct InfluenceAcc {
    std::vector<std::pair<int, double>> items; // (boneIndex, weight)
};

struct Key {
    long long t = 0;
    double T[3];
    double R[4]; // x y z w
    double S[3];
};

struct Track {
    std::string bone;
    std::vector<Key> keys;
};

struct Clip {
    std::string name;
    double tickRate = 60.0;
    long long durationTicks = 0;
    std::vector<Track> tracks;
};

// A material slot on the mesh, mapped to its semantic Unreal slot name and its
// PBR texture set (PNG basenames; empty => that map is absent).
struct Material {
    std::string name;      // FBX material name (e.g. "Mat_Body_Color_1")
    std::string slot;      // semantic slot (Unreal slot name, e.g. "Cloth")
    std::string baseColor; // albedo PNG basename, or "" if unknown/untextured
    std::string normal;    // normal-map PNG basename, or "" (linear data)
    std::string roughness; // roughness PNG basename, or "" (linear data)
    std::string metalness; // metalness PNG basename, or "" (linear data)
    std::string ao;        // ambient-occlusion PNG basename, or "" (linear data)
    std::vector<double> color; // flat RGB (0..1), used when baseColor is empty; else empty
};

// Derive the PBR map basenames from an albedo basename by suffix substitution:
// `<stem>_BaseColor.png` -> `<stem>_{Normal,Roughness,Metalness,AO}.png`. If the
// albedo doesn't follow the `_BaseColor.png` convention (e.g. `Eyes_Albedo.png`),
// no PBR maps are derived (all left empty). Applied to both the character material
// mapping and the static-prop diffuse-ref path so props derive maps too.
void derivePbrMaps(Material& mat) {
    mat.normal.clear();
    mat.roughness.clear();
    mat.metalness.clear();
    mat.ao.clear();
    const std::string suffix = "_BaseColor.png";
    const std::string& bc = mat.baseColor;
    if (bc.size() <= suffix.size()) return;
    if (bc.compare(bc.size() - suffix.size(), suffix.size(), suffix) != 0) return;
    std::string stem = bc.substr(0, bc.size() - suffix.size()); // drops "_BaseColor.png"
    mat.normal    = stem + "_Normal.png";
    mat.roughness = stem + "_Roughness.png";
    mat.metalness = stem + "_Metalness.png";
    mat.ao        = stem + "_AO.png";
}

// A contiguous run of `indices` sharing one material (one draw per submesh).
struct Submesh {
    int material = 0; // index into Scene::materials
    int start = 0;    // first index in Scene::indices
    int count = 0;    // number of indices
};

struct Scene {
    std::string sourceFile;
    std::string fbxVersion;
    std::string sourceAxis = "Z_up";
    std::string sourceUnit = "cm";
    std::string appliedTransform = "none";
    std::vector<std::string> textures;

    std::vector<Bone> bones;
    std::vector<Vertex> vertices;
    std::vector<int> indices;
    std::vector<Material> materials;
    std::vector<Submesh> submeshes;
    std::vector<Clip> clips;

    bool hasMesh = false;
};

// FBX material-name -> (semantic slot, albedo PNG). The FBX material names AND its
// embedded texture references are unreliable for this asset (two model versions are
// blended into the source content folder), so the mapping is driven by the Unreal
// material-slot names the user read off the skeletal mesh: slot order 0-5 =
// Cloth, Body, Wine, Hair, Head, Eyes. PROVISIONAL — the Body/Head/Wine texture
// choices are best-guesses; empty base_color renders that submesh flat/gray so a
// wrong or missing mapping is visible in the viewer. Edit this table + re-run to
// correct; the three referenced PNGs are produced by the `--textures` mode.
void mapMaterial(const std::string& fbxName, std::string& slot, std::string& baseColor,
                 std::vector<double>& color) {
    // `flat` + r/g/b give a solid colour for untextured slots (baseColor empty).
    struct Entry { const char* name; const char* slot; const char* tex; bool flat; double r, g, b; };
    static const Entry kMap[] = {
        {"Mat_Body_Color_1",       "Cloth", "Katanami_Body_BaseColor.png", false, 0, 0, 0},
        {"Fbx Default Material 1", "Body",  "Katanami_Body_BaseColor.png", false, 0, 0, 0},
        {"Fbx Default Material 2", "Wine",  "",                            true, 0.45, 0.08, 0.14}, // wine red, flat
        {"Mat_Hair_Color_2",       "Hair",  "Katanami_Hair_BaseColor.png", false, 0, 0, 0},
        {"Fbx Default Material 4", "Head",  "Katanami_Body_BaseColor.png", false, 0, 0, 0},
        {"Mat_Eyes_Inst",          "Eyes",  "Eyes_Albedo.png",             false, 0, 0, 0},
    };
    color.clear();
    for (const Entry& e : kMap) {
        if (fbxName == e.name) {
            slot = e.slot;
            baseColor = e.tex;
            if (e.flat) color = {e.r, e.g, e.b};
            return;
        }
    }
    slot = fbxName; // unknown material -> flat/gray (no texture, no colour)
    baseColor.clear();
}

const std::vector<std::string> kColor1Textures = {
    "Katanami_Body_BaseColor.TGA", "Katanami_Body_Normal.TGA",
    "Katanami_Body_Roughness.TGA", "Katanami_Body_Metalness.TGA",
    "Katanami_Body_AO.TGA",        "Katanami_Hair_BaseColor.TGA",
    "Katanami_Hair_Normal.TGA",    "Katanami_Hair_Roughness.TGA",
    "Katanami_Hair_Metalness.TGA", "Katanami_Hair_AO.TGA"};

// ---------------------------------------------------------------------------
// Skeleton extraction
// ---------------------------------------------------------------------------

// Depth-first walk collecting FbxSkeleton nodes into a flat array so parents
// precede children. boneIndexByNode maps FbxNode* -> flat index.
void collectBones(FbxNode* node, int parentBoneIndex, Scene& scene,
                  std::unordered_map<FbxNode*, int>& boneIndexByNode) {
    int myIndex = parentBoneIndex;
    FbxNodeAttribute* attr = node->GetNodeAttribute();
    if (attr && attr->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
        Bone b;
        b.name = node->GetName();
        b.parent = parentBoneIndex;
        b.node = node;
        FbxAMatrix local = node->EvaluateLocalTransform(FBXSDK_TIME_INFINITE);
        fbxMatToRowMajor(local, b.local);
        FbxAMatrix ident; ident.SetIdentity();
        fbxMatToRowMajor(ident, b.inverseBind); // default; overwritten by skin
        myIndex = (int)scene.bones.size();
        scene.bones.push_back(b);
        boneIndexByNode[node] = myIndex;
    }
    for (int i = 0; i < node->GetChildCount(); ++i)
        collectBones(node->GetChild(i), myIndex, scene, boneIndexByNode);
}

// ---------------------------------------------------------------------------
// Mesh + skin extraction
// ---------------------------------------------------------------------------

FbxNode* findFirstMeshNode(FbxNode* node) {
    FbxNodeAttribute* attr = node->GetNodeAttribute();
    if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh)
        return node;
    for (int i = 0; i < node->GetChildCount(); ++i) {
        if (FbxNode* found = findFirstMeshNode(node->GetChild(i)))
            return found;
    }
    return nullptr;
}

// Returns true if the scene contains any FbxSkin deformer.
bool sceneHasSkin(FbxNode* node) {
    FbxNodeAttribute* attr = node->GetNodeAttribute();
    if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh) {
        FbxMesh* mesh = (FbxMesh*)attr;
        if (mesh->GetDeformerCount(FbxDeformer::eSkin) > 0) return true;
    }
    for (int i = 0; i < node->GetChildCount(); ++i)
        if (sceneHasSkin(node->GetChild(i))) return true;
    return false;
}

// The node's geometric transform (pivot-independent geometry offset). Applied
// to vertices/normals at import. Rotation/scale go to normals; full 4x4 to pos.
FbxAMatrix geometricTransform(FbxNode* node) {
    FbxVector4 t = node->GetGeometricTranslation(FbxNode::eSourcePivot);
    FbxVector4 r = node->GetGeometricRotation(FbxNode::eSourcePivot);
    FbxVector4 s = node->GetGeometricScaling(FbxNode::eSourcePivot);
    FbxAMatrix m;
    m.SetTRS(t, r, s);
    return m;
}

// Resolve a per-control-point normal (handles the common mapping/reference
// combinations). polyVertexIndex is the running per-polygon-vertex counter.
void resolveNormal(FbxMesh* mesh, int controlPoint, int polyVertexIndex,
                   double out[3]) {
    out[0] = out[1] = out[2] = 0.0;
    FbxGeometryElementNormal* elem = mesh->GetElementNormal(0);
    if (!elem) return;
    int idx = -1;
    switch (elem->GetMappingMode()) {
        case FbxGeometryElement::eByControlPoint: idx = controlPoint; break;
        case FbxGeometryElement::eByPolygonVertex: idx = polyVertexIndex; break;
        default: return; // unsupported mapping; leave zero
    }
    int di = idx;
    if (elem->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
        di = elem->GetIndexArray().GetAt(idx);
    FbxVector4 v = elem->GetDirectArray().GetAt(di);
    out[0] = v[0]; out[1] = v[1]; out[2] = v[2];
}

// Resolve a per-control-point UV.
void resolveUV(FbxMesh* mesh, int controlPoint, int polyVertexIndex,
               double out[2]) {
    out[0] = out[1] = 0.0;
    FbxGeometryElementUV* elem = mesh->GetElementUV(0);
    if (!elem) return;
    int idx = -1;
    switch (elem->GetMappingMode()) {
        case FbxGeometryElement::eByControlPoint: idx = controlPoint; break;
        case FbxGeometryElement::eByPolygonVertex: idx = polyVertexIndex; break;
        default: return;
    }
    int di = idx;
    if (elem->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
        di = elem->GetIndexArray().GetAt(idx);
    FbxVector2 v = elem->GetDirectArray().GetAt(di);
    out[0] = v[0]; out[1] = v[1];
}

// The basename of a material's diffuse/base-colour texture (FBX file reference), or
// empty. FBX refs are unreliable for this asset but a useful diagnostic.
std::string materialDiffuseTexture(FbxSurfaceMaterial* mat) {
    if (!mat) return "";
    FbxProperty prop = mat->FindProperty(FbxSurfaceMaterial::sDiffuse);
    if (!prop.IsValid()) return "";
    int n = prop.GetSrcObjectCount<FbxFileTexture>();
    if (n <= 0) return "";
    FbxFileTexture* tex = prop.GetSrcObject<FbxFileTexture>(0);
    if (!tex) return "";
    return fileName(tex->GetFileName());
}

// Keep the N largest influences, renormalize to sum 1.0, zero-pad to 4.
void finalizeInfluences(const InfluenceAcc& acc, int maxWeights, Vertex& v) {
    std::vector<std::pair<int, double>> items = acc.items;
    std::sort(items.begin(), items.end(),
              [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                  return a.second > b.second;
              });
    int keep = std::min<int>((int)items.size(), std::min(maxWeights, 4));
    double sum = 0.0;
    for (int i = 0; i < keep; ++i) sum += items[i].second;

    for (int i = 0; i < 4; ++i) { v.joints[i] = 0; v.weights[i] = 0.0; }

    if (keep == 0 || sum <= 0.0) {
        // Fully unweighted vertex: bind to bone 0 with weight 1 so downstream
        // skinning still produces a defined position.
        v.joints[0] = 0;
        v.weights[0] = 1.0;
        return;
    }
    for (int i = 0; i < keep; ++i) {
        v.joints[i] = items[i].first;
        v.weights[i] = items[i].second / sum;
    }
}

// Build vertices + triangle indices + skin influences. Returns false if no
// usable mesh. Assumes the scene has already been triangulated.
bool extractMesh(FbxNode* meshNode, Scene& scene,
                 const std::unordered_map<FbxNode*, int>& boneIndexByNode,
                 int maxWeights) {
    FbxMesh* mesh = meshNode->GetMesh();
    if (!mesh) return false;

    const int cpCount = mesh->GetControlPointsCount();
    const FbxVector4* controlPoints = mesh->GetControlPoints();
    if (cpCount <= 0 || !controlPoints) return false;

    // Geometric transform applied to positions (full matrix) and normals
    // (rotation/scale part only, i.e. inverse-transpose is overkill here; the
    // geometric transform is typically rigid, so the linear part suffices).
    FbxAMatrix geo = geometricTransform(meshNode);
    FbxAMatrix geoLinear = geo; // for normals: zero translation
    geoLinear.SetT(FbxVector4(0, 0, 0, 0));

    // --- Skin influences accumulated per control point ---------------------
    std::vector<InfluenceAcc> influences(cpCount);
    int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
    for (int sd = 0; sd < skinCount; ++sd) {
        FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(sd, FbxDeformer::eSkin);
        int clusterCount = skin->GetClusterCount();
        for (int c = 0; c < clusterCount; ++c) {
            FbxCluster* cluster = skin->GetCluster(c);
            FbxNode* link = cluster->GetLink();
            if (!link) continue;
            auto it = boneIndexByNode.find(link);
            if (it == boneIndexByNode.end()) continue; // link isn't a known bone
            int boneIndex = it->second;

            // inverse-bind = inverse(L) * M
            //   M = mesh-at-bind  (GetTransformMatrix)
            //   L = bone-at-bind  (GetTransformLinkMatrix)
            FbxAMatrix M, L;
            cluster->GetTransformMatrix(M);
            cluster->GetTransformLinkMatrix(L);
            FbxAMatrix invBind = L.Inverse() * M;
            fbxMatToRowMajor(invBind, scene.bones[boneIndex].inverseBind);

            int n = cluster->GetControlPointIndicesCount();
            const int* cpIdx = cluster->GetControlPointIndices();
            const double* cpW = cluster->GetControlPointWeights();
            for (int k = 0; k < n; ++k) {
                int cp = cpIdx[k];
                double w = cpW[k];
                if (cp < 0 || cp >= cpCount) continue;
                if (w == 0.0) continue;
                influences[cp].items.emplace_back(boneIndex, w);
            }
        }
    }

    // --- Materials -----------------------------------------------------------
    // The name-keyed slot/texture/colour table (`mapMaterial`) is CHARACTER-specific;
    // apply it only to skinned meshes. Static props keep their raw material name (FBX
    // default names like "Mat_Body_Color_1" collide across assets) and stay
    // untextured — the consumer assigns a flat colour/texture per prop.
    bool isSkinned = skinCount > 0;
    int matCount = meshNode->GetMaterialCount();
    for (int m = 0; m < matCount; ++m) {
        FbxSurfaceMaterial* fm = meshNode->GetMaterial(m);
        Material mat;
        mat.name = fm ? fm->GetName() : ("material_" + std::to_string(m));
        if (isSkinned) {
            // Character: name-keyed slot/texture/colour table (FBX refs unreliable here).
            mapMaterial(mat.name, mat.slot, mat.baseColor, mat.color);
        } else {
            // Static prop: use the FBX diffuse texture reference (as .png) — e.g. the
            // katana packs into Katanami_Body_BaseColor. Empty ref -> flat (consumer's
            // choice of colour).
            mat.slot = mat.name;
            std::string diff = materialDiffuseTexture(fm);
            if (!diff.empty()) {
                size_t dot = diff.find_last_of('.');
                mat.baseColor = (dot == std::string::npos ? diff : diff.substr(0, dot)) + ".png";
            }
        }
        // Derive the PBR map set from the albedo basename (both paths). Albedos that
        // don't follow the `_BaseColor.png` convention (Eyes_Albedo) get no PBR maps.
        derivePbrMaps(mat);
        scene.materials.push_back(std::move(mat));
    }
    if (scene.materials.empty()) {
        Material mat; mat.name = "default"; mat.slot = "default";
        scene.materials.push_back(std::move(mat));
        matCount = 1;
    }

    // Per-polygon material index. This asset is eByPolygon/eIndexToDirect; guard
    // eAllSame and missing elements (single material 0).
    FbxGeometryElementMaterial* matElem = mesh->GetElementMaterial(0);
    auto polyMaterial = [&](int p) -> int {
        if (!matElem) return 0;
        FbxGeometryElement::EMappingMode mm = matElem->GetMappingMode();
        const FbxLayerElementArrayTemplate<int>& idx = matElem->GetIndexArray();
        if (mm == FbxGeometryElement::eAllSame)
            return idx.GetCount() > 0 ? idx.GetAt(0) : 0;
        if (mm == FbxGeometryElement::eByPolygon && p < idx.GetCount())
            return idx.GetAt(p);
        return 0;
    };

    // --- Walk polygons: emit one Vertex per polygon-vertex, GROUPED BY MATERIAL
    // Non-deduplicated vertex list (each triangle corner becomes a vertex; keeps
    // normals/UVs per-vertex correct for eByPolygonVertex data). Triangles are
    // emitted material-by-material so each material occupies a contiguous submesh
    // range in `indices` (= a range in `vertices`, since indices are sequential).
    int polyCount = mesh->GetPolygonCount();

    // Each polygon's starting per-polygon-vertex index — needed for eByPolygonVertex
    // normal/UV lookups now that we emit polygons out of original order.
    std::vector<int> polyStart(polyCount, 0);
    for (int p = 0, acc = 0; p < polyCount; ++p) {
        polyStart[p] = acc;
        acc += mesh->GetPolygonSize(p);
    }

    // Bucket polygons by material index, preserving original order within a bucket.
    std::vector<std::vector<int>> polysByMat(std::max(matCount, 1));
    for (int p = 0; p < polyCount; ++p) {
        int mi = polyMaterial(p);
        if (mi < 0 || mi >= (int)polysByMat.size()) mi = 0;
        polysByMat[mi].push_back(p);
    }

    for (int mi = 0; mi < (int)polysByMat.size(); ++mi) {
        int subStart = (int)scene.indices.size();
        for (int p : polysByMat[mi]) {
            int size = mesh->GetPolygonSize(p);
            for (int t = 0; t < size; ++t) {
                int cp = mesh->GetPolygonVertex(p, t);
                int localPV = polyStart[p] + t;

                Vertex v;
                FbxVector4 pos = controlPoints[cp];
                pos = geo.MultT(pos);
                v.p[0] = pos[0]; v.p[1] = pos[1]; v.p[2] = pos[2];

                double nrm[3];
                resolveNormal(mesh, cp, localPV, nrm);
                FbxVector4 nv(nrm[0], nrm[1], nrm[2], 0.0);
                nv = geoLinear.MultT(nv);
                v.n[0] = nv[0]; v.n[1] = nv[1]; v.n[2] = nv[2];

                resolveUV(mesh, cp, localPV, v.uv);
                // FBX UVs use a bottom-left origin (V increases upward, Maya/OpenGL);
                // wgpu samples textures top-left (V increases downward). Flip V so the
                // emitted UVs are engine-ready (top-origin) and no consumer re-flips.
                v.uv[1] = 1.0 - v.uv[1];
                finalizeInfluences(influences[cp], maxWeights, v);

                int newIndex = (int)scene.vertices.size();
                scene.vertices.push_back(v);

                // Fan the polygon (already triangles normally).
                if (t >= 2) {
                    int a = newIndex - t;   // first corner of this polygon
                    int b = newIndex - 1;   // previous corner
                    scene.indices.push_back(a);
                    scene.indices.push_back(b);
                    scene.indices.push_back(newIndex);
                }
            }
        }
        int subCount = (int)scene.indices.size() - subStart;
        if (subCount > 0) {
            Submesh sm; sm.material = mi; sm.start = subStart; sm.count = subCount;
            scene.submeshes.push_back(sm);
        }
    }

    scene.hasMesh = true;
    return true;
}

// ---------------------------------------------------------------------------
// Animation baking
// ---------------------------------------------------------------------------

bool isGenericClipName(const std::string& n) {
    if (n.empty()) return true;
    std::string lower = n;
    for (char& c : lower) c = (char)tolower((unsigned char)c);
    // UE exports every take as "Unreal Take"; treat it (case-insensitive) as
    // generic alongside the other non-distinctive names.
    return lower == "take 001" || lower == "take001" ||
           lower == "default take" || lower == "base" || lower == "unnamed" ||
           lower == "unreal take";
}

void bakeClips(FbxScene* fbxScene, Scene& scene, const std::string& inputStem,
               double tickRate) {
    int stackCount = fbxScene->GetSrcObjectCount<FbxAnimStack>();
    for (int s = 0; s < stackCount; ++s) {
        FbxAnimStack* stack = fbxScene->GetSrcObject<FbxAnimStack>(s);
        if (!stack) continue;
        fbxScene->SetCurrentAnimationStack(stack);

        FbxTimeSpan span = stack->GetLocalTimeSpan();
        FbxTime start = span.GetStart();
        FbxTime stop = span.GetStop();
        double startSec = start.GetSecondDouble();
        double stopSec = stop.GetSecondDouble();
        if (stopSec < startSec) std::swap(startSec, stopSec);

        double durationSec = stopSec - startSec;
        long long tickCount = (long long)llround(durationSec * tickRate) + 1; // inclusive
        if (tickCount < 1) tickCount = 1;

        Clip clip;
        // For this POC there is one animation per file, and the input file stem
        // IS the meaningful clip identity (it's what the viewer's clip picker
        // shows). UE exports every take as "Unreal Take", so the AnimStack name
        // is never distinctive here. Name the clip after the input file stem
        // unconditionally (Attack_1.FBX -> "Attack_1"). We still consult the
        // AnimStack name only to keep the intent explicit / future-proof: if a
        // future file ever carried a genuinely distinctive stack name we could
        // prefer it, but that is deliberately NOT done for this bootstrap.
        std::string stackName = stack->GetName();
        (void)isGenericClipName(stackName);
        clip.name = inputStem;
        clip.tickRate = tickRate;
        clip.durationTicks = tickCount;

        // One track per bone (keyed by NAME - index resolution is downstream).
        clip.tracks.reserve(scene.bones.size());
        for (Bone& bone : scene.bones) {
            Track track;
            track.bone = bone.name;
            track.keys.reserve(tickCount);
            for (long long tk = 0; tk < tickCount; ++tk) {
                double sec = startSec + (double)tk / tickRate;
                if (sec > stopSec) sec = stopSec;
                FbxTime t;
                t.SetSecondDouble(sec);

                FbxAMatrix local = bone.node->EvaluateLocalTransform(t);
                FbxVector4 T = local.GetT();
                FbxQuaternion Q = local.GetQ();
                FbxVector4 S = local.GetS();

                Key key;
                key.t = tk;
                key.T[0] = T[0]; key.T[1] = T[1]; key.T[2] = T[2];
                key.R[0] = Q[0]; key.R[1] = Q[1]; key.R[2] = Q[2]; key.R[3] = Q[3];
                key.S[0] = S[0]; key.S[1] = S[1]; key.S[2] = S[2];
                track.keys.push_back(key);
            }
            clip.tracks.push_back(std::move(track));
        }
        scene.clips.push_back(std::move(clip));
    }
}

// ---------------------------------------------------------------------------
// Axis / unit metadata (recorded, NOT applied)
// ---------------------------------------------------------------------------

void recordAxisUnit(FbxScene* fbxScene, Scene& scene) {
    FbxAxisSystem axis = fbxScene->GetGlobalSettings().GetAxisSystem();
    int sign = 0;
    FbxAxisSystem::EUpVector up = axis.GetUpVector(sign);
    switch (up) {
        case FbxAxisSystem::eXAxis: scene.sourceAxis = "X_up"; break;
        case FbxAxisSystem::eYAxis: scene.sourceAxis = "Y_up"; break;
        case FbxAxisSystem::eZAxis: scene.sourceAxis = "Z_up"; break;
        default: scene.sourceAxis = "Z_up"; break;
    }

    FbxSystemUnit u = fbxScene->GetGlobalSettings().GetSystemUnit();
    double sf = u.GetScaleFactor(); // 1.0 == cm
    if (sf == 0.1) scene.sourceUnit = "mm";
    else if (sf == 1.0) scene.sourceUnit = "cm";
    else if (sf == 10.0) scene.sourceUnit = "dm";
    else if (sf == 100.0) scene.sourceUnit = "m";
    else if (sf == 100000.0) scene.sourceUnit = "km";
    else {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "scale_%.6g", sf);
        scene.sourceUnit = buf;
    }
    scene.appliedTransform = "none";
}

// ---------------------------------------------------------------------------
// JSON emission
// ---------------------------------------------------------------------------

void writeJson(std::ostream& os, const Scene& scene) {
    os << "{\n";
    os << "  \"format\": \"flicker.rig\",\n";
    os << "  \"version\": 1,\n";

    // source
    os << "  \"source\": {\n";
    os << "    \"file\": \"" << jsonEscape(scene.sourceFile) << "\",\n";
    os << "    \"fbx_version\": \"" << jsonEscape(scene.fbxVersion) << "\",\n";
    os << "    \"source_axis\": \"" << jsonEscape(scene.sourceAxis) << "\", ";
    os << "\"source_unit\": \"" << jsonEscape(scene.sourceUnit) << "\",\n";
    os << "    \"applied_transform\": \"" << jsonEscape(scene.appliedTransform) << "\",\n";
    os << "    \"textures\": [";
    for (size_t i = 0; i < scene.textures.size(); ++i) {
        if (i) os << ", ";
        os << '"' << jsonEscape(scene.textures[i]) << '"';
    }
    os << "]\n";
    os << "  },\n";

    // skeleton
    os << "  \"skeleton\": {\n";
    os << "    \"bones\": [\n";
    for (size_t i = 0; i < scene.bones.size(); ++i) {
        const Bone& b = scene.bones[i];
        os << "      { \"name\": \"" << jsonEscape(b.name) << "\", \"parent\": "
           << b.parent << ",\n";
        os << "        \"local\": ";
        writeMat16(os, b.local);
        os << ",\n";
        os << "        \"inverse_bind\": ";
        writeMat16(os, b.inverseBind);
        os << " }";
        if (i + 1 < scene.bones.size()) os << ',';
        os << '\n';
    }
    os << "    ]\n";
    os << "  },\n";

    // mesh
    os << "  \"mesh\": {\n";
    os << "    \"vertices\": [";
    for (size_t i = 0; i < scene.vertices.size(); ++i) {
        const Vertex& v = scene.vertices[i];
        if (i) os << ',';
        os << "\n      { \"p\":[";
        writeFloat(os, v.p[0]); os << ','; writeFloat(os, v.p[1]); os << ','; writeFloat(os, v.p[2]);
        os << "], \"n\":[";
        writeFloat(os, v.n[0]); os << ','; writeFloat(os, v.n[1]); os << ','; writeFloat(os, v.n[2]);
        os << "], \"uv\":[";
        writeFloat(os, v.uv[0]); os << ','; writeFloat(os, v.uv[1]);
        os << "], \"joints\":[" << v.joints[0] << ',' << v.joints[1] << ','
           << v.joints[2] << ',' << v.joints[3] << "], \"weights\":[";
        writeFloat(os, v.weights[0]); os << ','; writeFloat(os, v.weights[1]); os << ',';
        writeFloat(os, v.weights[2]); os << ','; writeFloat(os, v.weights[3]);
        os << "] }";
    }
    if (!scene.vertices.empty()) os << "\n    ";
    os << "],\n";
    os << "    \"indices\": [";
    for (size_t i = 0; i < scene.indices.size(); ++i) {
        if (i) os << ',';
        os << scene.indices[i];
    }
    os << "],\n";
    // submeshes (index ranges grouped by material)
    os << "    \"submeshes\": [";
    for (size_t i = 0; i < scene.submeshes.size(); ++i) {
        const Submesh& sm = scene.submeshes[i];
        if (i) os << ',';
        os << " { \"material\": " << sm.material << ", \"start\": " << sm.start
           << ", \"count\": " << sm.count << " }";
    }
    os << " ],\n";
    // materials (indexed by submesh.material)
    os << "    \"materials\": [";
    for (size_t i = 0; i < scene.materials.size(); ++i) {
        const Material& m = scene.materials[i];
        if (i) os << ',';
        os << " { \"name\": \"" << jsonEscape(m.name) << "\", \"slot\": \""
           << jsonEscape(m.slot) << "\", \"base_color\": \""
           << jsonEscape(m.baseColor) << "\", \"normal\": \""
           << jsonEscape(m.normal) << "\", \"roughness\": \""
           << jsonEscape(m.roughness) << "\", \"metalness\": \""
           << jsonEscape(m.metalness) << "\", \"ao\": \""
           << jsonEscape(m.ao) << "\", \"color\": [";
        for (size_t k = 0; k < m.color.size(); ++k) {
            if (k) os << ',';
            writeFloat(os, m.color[k]);
        }
        os << "] }";
    }
    os << " ]\n";
    os << "  },\n";

    // morphs (always empty for this POC)
    os << "  \"morphs\": [],\n";

    // clips
    os << "  \"clips\": [";
    for (size_t ci = 0; ci < scene.clips.size(); ++ci) {
        const Clip& clip = scene.clips[ci];
        if (ci) os << ',';
        os << "\n    { \"name\": \"" << jsonEscape(clip.name) << "\",\n";
        os << "      \"tick_rate_hz\": ";
        writeFloat(os, clip.tickRate);
        os << ",\n";
        os << "      \"duration_ticks\": " << clip.durationTicks << ",\n";
        os << "      \"tracks\": [";
        for (size_t ti = 0; ti < clip.tracks.size(); ++ti) {
            const Track& track = clip.tracks[ti];
            if (ti) os << ',';
            os << "\n        { \"bone\": \"" << jsonEscape(track.bone) << "\",\n";
            os << "          \"keys\": [";
            for (size_t ki = 0; ki < track.keys.size(); ++ki) {
                const Key& k = track.keys[ki];
                if (ki) os << ',';
                os << " { \"t\": " << k.t << ", \"T\":[";
                writeFloat(os, k.T[0]); os << ','; writeFloat(os, k.T[1]); os << ','; writeFloat(os, k.T[2]);
                os << "], \"R\":[";
                writeFloat(os, k.R[0]); os << ','; writeFloat(os, k.R[1]); os << ',';
                writeFloat(os, k.R[2]); os << ','; writeFloat(os, k.R[3]);
                os << "], \"S\":[";
                writeFloat(os, k.S[0]); os << ','; writeFloat(os, k.S[1]); os << ','; writeFloat(os, k.S[2]);
                os << "] }";
            }
            os << " ]\n        }";
        }
        os << "\n      ]\n    }";
    }
    if (!scene.clips.empty()) os << "\n  ";
    os << "]\n";   // clips is the last member: no trailing comma

    os << "}\n";
}

// ---------------------------------------------------------------------------
// Top-level per-file conversion
// ---------------------------------------------------------------------------

struct ConvertResult {
    bool ok = false;
    int boneCount = 0;
    int vertexCount = 0;
    int triangleCount = 0;
    int clipCount = 0;
    std::string firstClipName;
    long long firstClipDuration = 0;
    int firstClipTrackCount = 0;
    std::string error;
};

ConvertResult convertFile(FbxManager* manager, const std::string& inputPath,
                          const std::string& outputPath, const Options& opts) {
    ConvertResult res;

    FbxImporter* importer = FbxImporter::Create(manager, "");
    if (!importer->Initialize(inputPath.c_str(), -1, manager->GetIOSettings())) {
        FbxString err = importer->GetStatus().GetErrorString();
        res.error = std::string("Initialize failed: ") + err.Buffer();
        importer->Destroy();
        return res;
    }

    int vMajor = 0, vMinor = 0, vRevision = 0;
    importer->GetFileVersion(vMajor, vMinor, vRevision);

    FbxScene* fbxScene = FbxScene::Create(manager, "scene");
    if (!importer->Import(fbxScene)) {
        FbxString err = importer->GetStatus().GetErrorString();
        res.error = std::string("Import failed (file version ") +
                    std::to_string(vMajor) + "." + std::to_string(vMinor) + "." +
                    std::to_string(vRevision) + "): " + err.Buffer();
        importer->Destroy();
        fbxScene->Destroy();
        return res;
    }
    importer->Destroy();

    Scene scene;
    scene.sourceFile = fileName(inputPath);
    // fbx_version as a 4-digit code (7500 for FBX 7.5). vMajor/vMinor are like
    // 7 / 5, giving 7500; fall back to concatenation if unusual.
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d%d00", vMajor, vMinor);
        scene.fbxVersion = buf;
    }

    recordAxisUnit(fbxScene, scene);

    FbxNode* root = fbxScene->GetRootNode();
    if (!root) {
        res.error = "Scene has no root node";
        fbxScene->Destroy();
        return res;
    }

    // Skeleton
    std::unordered_map<FbxNode*, int> boneIndexByNode;
    for (int i = 0; i < root->GetChildCount(); ++i)
        collectBones(root->GetChild(i), -1, scene, boneIndexByNode);

    std::string stem = fileStem(inputPath);

    // Mesh (skinned OR static): if the scene has a mesh with geometry, triangulate and
    // extract it. A skinned mesh (has FbxSkin) gets per-vertex influences + inverse-bind
    // and a bone array; a static prop (no skin, no skeleton) gets the geometry only
    // (empty influences, no bones) — the consumer renders it rigid at an attach transform.
    FbxNode* meshProbe = findFirstMeshNode(root);
    bool hasMeshGeom = meshProbe && meshProbe->GetMesh() &&
                       meshProbe->GetMesh()->GetControlPointsCount() > 0;
    if (hasMeshGeom) {
        FbxGeometryConverter conv(manager);
        conv.Triangulate(fbxScene, /*replace=*/true);
        FbxNode* meshNode = findFirstMeshNode(fbxScene->GetRootNode());
        if (meshNode) {
            extractMesh(meshNode, scene, boneIndexByNode, opts.maxWeights);
        }
    }

    // Clips: bake animation stacks, but only when there's a skeleton to animate. A
    // static prop can carry a default (empty) anim stack — skip it so we don't emit a
    // 0-track clip.
    if (!scene.bones.empty() && fbxScene->GetSrcObjectCount<FbxAnimStack>() > 0) {
        bakeClips(fbxScene, scene, stem, opts.tickRate);
    }

    // Write JSON
    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        res.error = "Could not open output file: " + outputPath;
        fbxScene->Destroy();
        return res;
    }
    writeJson(out, scene);
    out.close();

    res.ok = true;
    res.boneCount = (int)scene.bones.size();
    res.vertexCount = (int)scene.vertices.size();
    res.triangleCount = (int)scene.indices.size() / 3;
    res.clipCount = (int)scene.clips.size();
    if (!scene.clips.empty()) {
        res.firstClipName = scene.clips[0].name;
        res.firstClipDuration = scene.clips[0].durationTicks;
        res.firstClipTrackCount = (int)scene.clips[0].tracks.size();
    }

    fbxScene->Destroy();
    return res;
}

// ---------------------------------------------------------------------------
// Directory listing (Win32)
// ---------------------------------------------------------------------------

std::vector<std::string> listFbxFiles(const std::string& dir) {
    std::vector<std::string> out;
#ifdef _WIN32
    std::string pattern = dir;
    if (!pattern.empty() && pattern.back() != '/' && pattern.back() != '\\')
        pattern += '\\';
    std::string search = pattern + "*";
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::string name = fd.cFileName;
        if (endsWithFbxCaseInsensitive(name))
            out.push_back(pattern + name);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#endif
    std::sort(out.begin(), out.end());
    return out;
}

std::string joinPath(const std::string& dir, const std::string& file) {
    std::string d = dir;
    if (!d.empty() && d.back() != '/' && d.back() != '\\') d += '\\';
    return d + file;
}

// ---------------------------------------------------------------------------
// Texture conversion (TGA -> PNG via stb)
// ---------------------------------------------------------------------------

bool convertTgaToPng(const std::string& inPath, const std::string& outPath) {
    int w = 0, h = 0, n = 0;
    unsigned char* pixels = stbi_load(inPath.c_str(), &w, &h, &n, 4); // force RGBA
    if (!pixels) {
        std::fprintf(stderr, "  [FAIL] load %s: %s\n", inPath.c_str(),
                     stbi_failure_reason() ? stbi_failure_reason() : "unknown");
        return false;
    }
    int ok = stbi_write_png(outPath.c_str(), w, h, 4, pixels, w * 4);
    stbi_image_free(pixels);
    if (!ok) {
        std::fprintf(stderr, "  [FAIL] write %s\n", outPath.c_str());
        return false;
    }
    std::printf("  [ok] %s -> %s (%dx%d)\n", fileName(inPath).c_str(),
                fileName(outPath).c_str(), w, h);
    return true;
}

// Convert the character's texture TGAs to PNG into outDir. The source set is
// hardcoded (bootstrap crutch): the albedos the material mapping references PLUS the
// PBR map set (normal/roughness/metalness/AO) for the Body and Hair atlases. Eyes has
// albedo only (no PBR maps). Albedo is sRGB colour; the PBR maps are LINEAR data — the
// TGA->PNG conversion is a straight byte copy either way, and the sRGB-vs-linear
// distinction is applied consumer-side at GPU upload.
int runTextures(const std::string& outDir) {
    struct Tex { const char* src; const char* out; };
    static const Tex kTex[] = {
        // Albedos (sRGB colour).
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Body_BaseColor.TGA",
         "Katanami_Body_BaseColor.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Hair_BaseColor.TGA",
         "Katanami_Hair_BaseColor.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Eyes\\Eyes_Albedo.TGA",
         "Eyes_Albedo.png"},
        // Skin variants (1/2/3 keys) — albedo only; Color_2 has no hair variant, so it
        // falls back to the Color_1 hair atlas at render time.
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_2\\Katanami2_Body_BaseColor.TGA",
         "Katanami2_Body_BaseColor.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_3\\Katanami3_Body_BaseColor.TGA",
         "Katanami3_Body_BaseColor.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_3\\Katanami3_Hair_BaseColor.TGA",
         "Katanami3_Hair_BaseColor.png"},
        // Body PBR maps (linear).
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Body_Normal.TGA",
         "Katanami_Body_Normal.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Body_Roughness.TGA",
         "Katanami_Body_Roughness.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Body_Metalness.TGA",
         "Katanami_Body_Metalness.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Body_AO.TGA",
         "Katanami_Body_AO.png"},
        // Hair PBR maps (linear) — also used by the katana prop (Hair atlas).
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Hair_Normal.TGA",
         "Katanami_Hair_Normal.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Hair_Roughness.TGA",
         "Katanami_Hair_Roughness.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Hair_Metalness.TGA",
         "Katanami_Hair_Metalness.png"},
        {"C:\\Users\\aaron\\Repos\\KatanamiExtraction\\Textures\\Color_1\\Katanami_Hair_AO.TGA",
         "Katanami_Hair_AO.png"},
    };
    int ok = 0, fail = 0;
    for (const Tex& t : kTex) {
        if (convertTgaToPng(t.src, joinPath(outDir, t.out))) ++ok; else ++fail;
    }
    std::printf("Textures: %d ok, %d failed.\n", ok, fail);
    return fail == 0 ? 0 : 1;
}

// ---------------------------------------------------------------------------
// CLI
// ---------------------------------------------------------------------------

void printUsage() {
    std::fprintf(stderr,
        "Usage:\n"
        "  fbximport <input.fbx> <output.json> [--tick-rate 60] [--up Y] [--unit m] [--max-weights 4]\n"
        "  fbximport --batch <input_dir> <output_dir>\n"
        "  fbximport --textures <output_dir>   (convert the character albedo TGAs to PNG)\n");
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) { printUsage(); return 2; }

    Options opts;

    // --textures mode: convert the character's albedo TGAs to PNG (no FBX needed).
    if (std::strcmp(argv[1], "--textures") == 0) {
        if (argc < 3) { printUsage(); return 2; }
        return runTextures(argv[2]);
    }

    // --batch mode
    if (std::strcmp(argv[1], "--batch") == 0) {
        if (argc < 4) { printUsage(); return 2; }
        std::string inDir = argv[2];
        std::string outDir = argv[3];

        FbxManager* manager = FbxManager::Create();
        FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
        manager->SetIOSettings(ios);

        std::vector<std::string> files = listFbxFiles(inDir);
        if (files.empty()) {
            std::fprintf(stderr, "No .fbx files found in: %s\n", inDir.c_str());
            manager->Destroy();
            return 1;
        }

        int okCount = 0, failCount = 0;
        for (const std::string& in : files) {
            std::string out = joinPath(outDir, fileStem(in) + ".json");
            ConvertResult r = convertFile(manager, in, out, opts);
            if (r.ok) {
                ++okCount;
                std::printf("[ok]   %s -> %s | bones=%d verts=%d tris=%d clips=%d",
                            fileName(in).c_str(), fileName(out).c_str(),
                            r.boneCount, r.vertexCount, r.triangleCount, r.clipCount);
                if (r.clipCount > 0)
                    std::printf(" | clip0='%s' dur=%lld tracks=%d",
                                r.firstClipName.c_str(), r.firstClipDuration,
                                r.firstClipTrackCount);
                std::printf("\n");
            } else {
                ++failCount;
                std::printf("[FAIL] %s : %s\n", fileName(in).c_str(), r.error.c_str());
            }
        }
        std::printf("Batch complete: %d ok, %d failed, %d total.\n",
                    okCount, failCount, (int)files.size());
        manager->Destroy();
        return failCount == 0 ? 0 : 1;
    }

    // Single-file mode
    if (argc < 3) { printUsage(); return 2; }
    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    for (int i = 3; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Missing value for %s\n", name);
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--tick-rate") opts.tickRate = std::atof(next("--tick-rate").c_str());
        else if (a == "--up") opts.up = next("--up");
        else if (a == "--unit") opts.unit = next("--unit");
        else if (a == "--max-weights") opts.maxWeights = std::atoi(next("--max-weights").c_str());
        else { std::fprintf(stderr, "Unknown option: %s\n", a.c_str()); printUsage(); return 2; }
    }
    if (opts.tickRate <= 0.0) opts.tickRate = 60.0;
    if (opts.maxWeights < 1) opts.maxWeights = 1;
    if (opts.maxWeights > 4) opts.maxWeights = 4;

    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    ConvertResult r = convertFile(manager, inputPath, outputPath, opts);
    if (!r.ok) {
        std::fprintf(stderr, "Error converting %s: %s\n", inputPath.c_str(), r.error.c_str());
        manager->Destroy();
        return 1;
    }

    std::printf("Wrote %s\n", outputPath.c_str());
    std::printf("  bones=%d vertices=%d triangles=%d clips=%d\n",
                r.boneCount, r.vertexCount, r.triangleCount, r.clipCount);
    if (r.clipCount > 0)
        std::printf("  clip[0]: name='%s' duration_ticks=%lld tracks=%d\n",
                    r.firstClipName.c_str(), r.firstClipDuration, r.firstClipTrackCount);

    manager->Destroy();
    return 0;
}
